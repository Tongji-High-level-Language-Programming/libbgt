#include "bgt.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// The implementation mirrors the beginner-facing API, where short coordinate
// names, integer colors, and C-style string parameters are deliberate.
// NOLINTBEGIN(readability-magic-numbers, modernize-avoid-c-arrays,
// bugprone-easily-swappable-parameters, readability-identifier-length)

namespace {

constexpr int kMaxPublicKey = 512;
constexpr int kMouseButtonCount = 4;
constexpr int kDefaultFontSize = 24;
constexpr int kSuperSample = 2;           // SSAA 倍率
constexpr double kFpsWindowSeconds = 1.0;
constexpr double kTwoPi = 6.28318530717958647692;

struct FontEntry {
    std::string path;
    int size = kDefaultFontSize;
    TTF_Font *font = nullptr;
};

struct Point {
    int x = 0;
    int y = 0;
};

struct State {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    bool sdl_ready = false;
    bool ttf_ready = false;
    bool open = false;
    bool closing = false;
    int width = 0;
    int height = 0;
    unsigned color = BGT_WHITE;
    unsigned background = BGT_BLACK;
    int line_width = 1;
    int font_size = kDefaultFontSize;
    int fps_limit = 0;
    std::string font_path;
    std::vector<FontEntry> fonts;
    std::array<bool, kMaxPublicKey> keys{};
    std::array<bool, kMaxPublicKey> previous_keys{};
    std::array<bool, kMouseButtonCount> mouse_buttons{};
    std::array<bool, kMouseButtonCount> previous_mouse_buttons{};
    int mouse_x = 0;
    int mouse_y = 0;
    int mouse_wheel = 0;
    SDL_Texture *canvas = nullptr;       // SSAA 离屏纹理
    std::uint64_t start_ticks = 0;
    std::uint64_t previous_ticks = 0;
    std::uint64_t fps_ticks = 0;
    int fps_frames = 0;
    std::uint64_t next_frame_ns = 0;
    double delta_time = 0.0;
    double total_time = 0.0;
    double fps = 0.0;
    int error_code = BGT_ERROR_NONE;
    std::string error_message;

    ~State()
    {
        close();
    }

    void clear_error()
    {
        error_code = BGT_ERROR_NONE;
        error_message.clear();
        SDL_ClearError();
    }

    void set_error(int code, const std::string &message)
    {
        error_code = code;
        error_message = message;
        const char *sdl_error = SDL_GetError();
        if (sdl_error != nullptr && sdl_error[0] != '\0') {
            error_message += ": ";
            error_message += sdl_error;
        }
    }

    void close_fonts()
    {
        for (FontEntry &entry : fonts) {
            if (entry.font != nullptr) {
                TTF_CloseFont(entry.font);
                entry.font = nullptr;
            }
        }
        fonts.clear();
    }

    void close()
    {
        if (canvas != nullptr) {
            SDL_DestroyTexture(canvas);
            canvas = nullptr;
        }
        close_fonts();
        if (renderer != nullptr) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window != nullptr) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        if (ttf_ready) {
            TTF_Quit();
            ttf_ready = false;
        }
        if (sdl_ready) {
            SDL_Quit();
            sdl_ready = false;
        }
        open = false;
        closing = false;
        width = 0;
        height = 0;
        keys.fill(false);
        previous_keys.fill(false);
        mouse_buttons.fill(false);
        previous_mouse_buttons.fill(false);
        mouse_x = 0;
        mouse_y = 0;
        mouse_wheel = 0;
    }
};

State &state()
{
    static State instance;
    return instance;
}

int clamp_byte(int value)
{
    return std::clamp(value, 0, 255);
}

std::uint8_t color_a(unsigned color)
{
    return static_cast<std::uint8_t>((static_cast<std::uint32_t>(color) >> 24U) &
                                     0xFFU);
}

std::uint8_t color_r(unsigned color)
{
    return static_cast<std::uint8_t>(
        (static_cast<std::uint32_t>(color) >> 16U) & 0xFFU);
}

std::uint8_t color_g(unsigned color)
{
    return static_cast<std::uint8_t>((static_cast<std::uint32_t>(color) >> 8U) &
                                     0xFFU);
}

std::uint8_t color_b(unsigned color)
{
    return static_cast<std::uint8_t>(static_cast<std::uint32_t>(color) & 0xFFU);
}

SDL_Color to_sdl_color(unsigned color)
{
    return SDL_Color{color_r(color), color_g(color), color_b(color),
                     color_a(color)};
}

void apply_render_color(State &s, unsigned color)
{
    if (s.renderer == nullptr) {
        return;
    }
    SDL_SetRenderDrawColor(s.renderer, color_r(color), color_g(color),
                           color_b(color), color_a(color));
}

bool ensure_open(State &s)
{
    if (!s.open || s.renderer == nullptr) {
        s.set_error(BGT_ERROR_NOT_OPEN, "libbgt window is not open");
        return false;
    }
    return true;
}

std::string join_path(const std::string &left, const std::string &right)
{
    if (left.empty()) {
        return right;
    }
    const char tail = left[left.size() - 1];
    if (tail == '/' || tail == '\\') {
        return left + right;
    }
    return left + "/" + right;
}

bool file_exists(const std::string &path)
{
    SDL_IOStream *stream = SDL_IOFromFile(path.c_str(), "rb");
    if (stream == nullptr) {
        return false;
    }
    SDL_CloseIO(stream);
    return true;
}

void add_candidate(std::vector<std::string> &candidates,
                   const std::string &path)
{
    if (path.empty()) {
        return;
    }
    if (std::find(candidates.begin(), candidates.end(), path) ==
        candidates.end()) {
        candidates.push_back(path);
    }
}

std::vector<std::string> default_font_candidates()
{
    std::vector<std::string> candidates;
#ifdef _WIN32
    const char *win_dir = SDL_getenv("WINDIR");
    const std::string font_dir =
        win_dir == nullptr ? "C:/Windows/Fonts" : join_path(win_dir, "Fonts");
    add_candidate(candidates, join_path(font_dir, "msyh.ttc"));
    add_candidate(candidates, join_path(font_dir, "simhei.ttf"));
#elif defined(__APPLE__)
    add_candidate(candidates, "/System/Library/Fonts/PingFang.ttc");
    add_candidate(candidates, "/System/Library/Fonts/STHeiti Light.ttc");
#else
    add_candidate(candidates,
                  "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
    add_candidate(candidates, "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc");
    add_candidate(candidates,
                  "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif

    return candidates;
}

std::string resolve_default_font()
{
    for (const std::string &candidate : default_font_candidates()) {
        if (file_exists(candidate)) {
            return candidate;
        }
    }
    return "";
}

TTF_Font *get_font(State &s, int size)
{
    if (size <= 0) {
        size = s.font_size;
    }
    if (size <= 0) {
        size = kDefaultFontSize;
    }

    if (s.font_path.empty()) {
        s.font_path = resolve_default_font();
        if (s.font_path.empty()) {
            s.set_error(BGT_ERROR_FONT,
                        "no usable system font found; install a CJK font or "
                        "use bgt_set_font()");
            return nullptr;
        }
    }

    for (FontEntry &entry : s.fonts) {
        if (entry.path == s.font_path && entry.size == size) {
            return entry.font;
        }
    }

    TTF_Font *font =
        TTF_OpenFont(s.font_path.c_str(), static_cast<float>(size));
    if (font == nullptr) {
        s.set_error(BGT_ERROR_FONT, "failed to open font " + s.font_path);
        return nullptr;
    }

    s.fonts.push_back(FontEntry{s.font_path, size, font});
    return font;
}

int digit_key_from_scancode(SDL_Scancode scancode)
{
    switch (scancode) {
        case SDL_SCANCODE_0:
        case SDL_SCANCODE_KP_0:
            return BGT_KEY_0;
        case SDL_SCANCODE_1:
        case SDL_SCANCODE_KP_1:
            return BGT_KEY_1;
        case SDL_SCANCODE_2:
        case SDL_SCANCODE_KP_2:
            return BGT_KEY_2;
        case SDL_SCANCODE_3:
        case SDL_SCANCODE_KP_3:
            return BGT_KEY_3;
        case SDL_SCANCODE_4:
        case SDL_SCANCODE_KP_4:
            return BGT_KEY_4;
        case SDL_SCANCODE_5:
        case SDL_SCANCODE_KP_5:
            return BGT_KEY_5;
        case SDL_SCANCODE_6:
        case SDL_SCANCODE_KP_6:
            return BGT_KEY_6;
        case SDL_SCANCODE_7:
        case SDL_SCANCODE_KP_7:
            return BGT_KEY_7;
        case SDL_SCANCODE_8:
        case SDL_SCANCODE_KP_8:
            return BGT_KEY_8;
        case SDL_SCANCODE_9:
        case SDL_SCANCODE_KP_9:
            return BGT_KEY_9;
        default:
            return 0;
    }
}

int numlock_off_keypad_key(SDL_Scancode scancode)
{
    switch (scancode) {
        case SDL_SCANCODE_KP_2:
            return BGT_KEY_DOWN;
        case SDL_SCANCODE_KP_4:
            return BGT_KEY_LEFT;
        case SDL_SCANCODE_KP_6:
            return BGT_KEY_RIGHT;
        case SDL_SCANCODE_KP_8:
            return BGT_KEY_UP;
        default:
            return 0;
    }
}

bool is_keypad_digit_scancode(SDL_Scancode scancode)
{
    switch (scancode) {
        case SDL_SCANCODE_KP_0:
        case SDL_SCANCODE_KP_1:
        case SDL_SCANCODE_KP_2:
        case SDL_SCANCODE_KP_3:
        case SDL_SCANCODE_KP_4:
        case SDL_SCANCODE_KP_5:
        case SDL_SCANCODE_KP_6:
        case SDL_SCANCODE_KP_7:
        case SDL_SCANCODE_KP_8:
        case SDL_SCANCODE_KP_9:
            return true;
        default:
            return false;
    }
}

int scancode_to_key(SDL_Scancode scancode, SDL_Keymod modifiers)
{
    switch (scancode) {
        case SDL_SCANCODE_LEFT:
            return BGT_KEY_LEFT;
        case SDL_SCANCODE_RIGHT:
            return BGT_KEY_RIGHT;
        case SDL_SCANCODE_UP:
            return BGT_KEY_UP;
        case SDL_SCANCODE_DOWN:
            return BGT_KEY_DOWN;
        case SDL_SCANCODE_SPACE:
            return BGT_KEY_SPACE;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
            return BGT_KEY_ENTER;
        case SDL_SCANCODE_ESCAPE:
            return BGT_KEY_ESCAPE;
        case SDL_SCANCODE_TAB:
        case SDL_SCANCODE_KP_TAB:
            return BGT_KEY_TAB;
        case SDL_SCANCODE_BACKSPACE:
        case SDL_SCANCODE_KP_BACKSPACE:
            return BGT_KEY_BACKSPACE;
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:
            return BGT_KEY_SHIFT;
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:
            return BGT_KEY_CTRL;
        case SDL_SCANCODE_LALT:
        case SDL_SCANCODE_RALT:
            return BGT_KEY_ALT;
        default:
            break;
    }

    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
        return BGT_KEY_A + (scancode - SDL_SCANCODE_A);
    }
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) {
        return BGT_KEY_F1 + (scancode - SDL_SCANCODE_F1);
    }

    if ((modifiers & SDL_KMOD_NUM) == 0) {
        const int keypad_key = numlock_off_keypad_key(scancode);
        if (keypad_key != 0) {
            return keypad_key;
        }
    }

    const int digit_key = digit_key_from_scancode(scancode);
    if (digit_key != 0 && ((modifiers & SDL_KMOD_NUM) != 0 ||
                           !is_keypad_digit_scancode(scancode))) {
        return digit_key;
    }

    return 0;
}

void sync_keyboard(State &s)
{
    s.keys.fill(false);

    int key_count = 0;
    const bool *keyboard = SDL_GetKeyboardState(&key_count);
    if (keyboard == nullptr) {
        return;
    }

    const SDL_Keymod modifiers = SDL_GetModState();
    for (int scancode_value = 0; scancode_value < key_count; ++scancode_value) {
        if (!keyboard[scancode_value]) {
            continue;
        }
        const int key = scancode_to_key(
            static_cast<SDL_Scancode>(scancode_value), modifiers);
        if (key > 0 && key < kMaxPublicKey) {
            s.keys[static_cast<std::size_t>(key)] = true;
        }
    }
}

int mouse_index(int button)
{
    if (button < BGT_MOUSE_LEFT || button > BGT_MOUSE_MIDDLE) {
        return 0;
    }
    return button;
}

void sync_mouse(State &s)
{
    float window_x = 0.0F;
    float window_y = 0.0F;
    const SDL_MouseButtonFlags flags = SDL_GetMouseState(&window_x, &window_y);

    float render_x = window_x;
    float render_y = window_y;
    if (s.renderer != nullptr) {
        SDL_RenderCoordinatesFromWindow(s.renderer, window_x, window_y,
                                        &render_x, &render_y);
    }

    s.mouse_x = static_cast<int>(std::lround(render_x));
    s.mouse_y = static_cast<int>(std::lround(render_y));
    s.mouse_buttons[BGT_MOUSE_LEFT] = (flags & SDL_BUTTON_LMASK) != 0U;
    s.mouse_buttons[BGT_MOUSE_RIGHT] = (flags & SDL_BUTTON_RMASK) != 0U;
    s.mouse_buttons[BGT_MOUSE_MIDDLE] = (flags & SDL_BUTTON_MMASK) != 0U;
}

void update_time(State &s)
{
    const std::uint64_t now = SDL_GetTicks();
    if (s.previous_ticks == 0) {
        s.previous_ticks = now;
    }
    s.delta_time = static_cast<double>(now - s.previous_ticks) / 1000.0;
    s.total_time = static_cast<double>(now - s.start_ticks) / 1000.0;
    s.previous_ticks = now;
}

void update_fps(State &s)
{
    ++s.fps_frames;
    const std::uint64_t now = SDL_GetTicks();
    const double elapsed = static_cast<double>(now - s.fps_ticks) / 1000.0;
    if (elapsed >= kFpsWindowSeconds) {
        s.fps = static_cast<double>(s.fps_frames) / elapsed;
        s.fps_frames = 0;
        s.fps_ticks = now;
    }
}

void apply_fps_limit(State &s)
{
    if (s.fps_limit <= 0) {
        s.next_frame_ns = 0;
        return;
    }
    const std::uint64_t period_ns = static_cast<std::uint64_t>(SDL_NS_PER_SECOND)
        / static_cast<std::uint64_t>(s.fps_limit);
    const std::uint64_t now = SDL_GetTicksNS();

    if (s.next_frame_ns == 0) {
        s.next_frame_ns = now + period_ns;
        return;
    }
    if (now < s.next_frame_ns) {
        SDL_DelayPrecise(s.next_frame_ns - now);
    }
    else if (now - s.next_frame_ns >= period_ns) {
        // 落后超过一整个帧周期（如卡顿、拖动窗口）时放弃补帧，
        // 从当前时刻重新对齐节拍，避免连续高速追赶。
        s.next_frame_ns = now + period_ns;
        return;
    }
    s.next_frame_ns += period_ns;
}


void draw_text_impl(State &s, int x, int y, const char text[], int size)
{
    if (!ensure_open(s) || text == nullptr || text[0] == '\0') {
        return;
    }

    TTF_Font *font = get_font(s, size);
    if (font == nullptr) {
        return;
    }

    SDL_Surface *surface =
        TTF_RenderText_Blended(font, text, 0, to_sdl_color(s.color));
    if (surface == nullptr) {
        s.set_error(BGT_ERROR_TTF, "failed to render text");
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(s.renderer, surface);
    SDL_DestroySurface(surface);
    if (texture == nullptr) {
        s.set_error(BGT_ERROR_RENDERER, "failed to create text texture");
        return;
    }

    float width = 0.0F;
    float height = 0.0F;
    if (!SDL_GetTextureSize(texture, &width, &height)) {
        SDL_DestroyTexture(texture);
        s.set_error(BGT_ERROR_RENDERER, "failed to query text texture size");
        return;
    }

    SDL_FRect dst{static_cast<float>(x), static_cast<float>(y), width, height};
    SDL_RenderTexture(s.renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

int text_size_impl(State &s, const char text[], int size, bool width)
{
    if (!ensure_open(s) || text == nullptr || text[0] == '\0') {
        return 0;
    }
    TTF_Font *font = get_font(s, size);
    if (font == nullptr) {
        return 0;
    }
    int measured_width = 0;
    int measured_height = 0;
    if (!TTF_GetStringSize(font, text, 0, &measured_width, &measured_height)) {
        s.set_error(BGT_ERROR_TTF, "failed to measure text");
        return 0;
    }
    return width ? measured_width : measured_height;
}

int edge_value(const Point &a, const Point &b, const Point &p)
{
    return ((p.x - a.x) * (b.y - a.y)) - ((p.y - a.y) * (b.x - a.x));
}

struct PointD {
    double x = 0.0;
    double y = 0.0;
};

// 求点 (p1x,p1y) 沿方向 (d1x,d1y) 的直线与点 (p2x,p2y) 沿方向 (d2x,d2y) 的直线的交点。
// 两条直线平行时返回中点作为 fallback。
PointD line_intersection(double p1x, double p1y, double d1x, double d1y,
                         double p2x, double p2y, double d2x, double d2y)
{
    const double denom = d1x * d2y - d2x * d1y;
    if (std::abs(denom) < 1e-10) {
        return {(p1x + p2x) / 2.0, (p1y + p2y) / 2.0};
    }
    const double t = ((p2x - p1x) * d2y - (p2y - p1y) * d2x) / denom;
    return {p1x + t * d1x, p1y + t * d1y};
}

// 用 SDL_RenderGeometry 以 triangle fan 方式填充凸多边形。
void fill_convex_polygon(State &s, const PointD *pts, int count)
{
    if (count < 3) {
        return;
    }
    SDL_Vertex *vertices = static_cast<SDL_Vertex *>(
        SDL_stack_alloc(SDL_Vertex, count));
    const SDL_Color col = to_sdl_color(s.color);
    const float inv255 = 1.0F / 255.0F;
    for (int i = 0; i < count; ++i) {
        vertices[i].position.x = static_cast<float>(pts[i].x);
        vertices[i].position.y = static_cast<float>(pts[i].y);
        vertices[i].color.r = static_cast<float>(col.r) * inv255;
        vertices[i].color.g = static_cast<float>(col.g) * inv255;
        vertices[i].color.b = static_cast<float>(col.b) * inv255;
        vertices[i].color.a = static_cast<float>(col.a) * inv255;
        vertices[i].tex_coord.x = 0.0F;
        vertices[i].tex_coord.y = 0.0F;
    }
    const int index_count = (count - 2) * 3;
    int *indices = static_cast<int *>(SDL_stack_alloc(int, index_count));
    int idx = 0;
    for (int i = 1; i < count - 1; ++i) {
        indices[idx++] = 0;
        indices[idx++] = i;
        indices[idx++] = i + 1;
    }
    SDL_RenderGeometry(s.renderer, nullptr, vertices, count, indices,
                       index_count);
    SDL_stack_free(indices);
    SDL_stack_free(vertices);
}

bool open_window_impl(int width, int height, const char title[],
                      SDL_WindowFlags flags)
{
    State &s = state();
    if (width <= 0 || height <= 0) {
        s.set_error(BGT_ERROR_WINDOW, "window size must be positive");
        return false;
    }

    s.close();
    s.clear_error();

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        s.set_error(BGT_ERROR_SDL, "failed to initialize SDL3");
        return false;
    }
    s.sdl_ready = true;

    if (!TTF_Init()) {
        s.set_error(BGT_ERROR_TTF, "failed to initialize SDL3_ttf");
        s.close();
        return false;
    }
    s.ttf_ready = true;

    s.window = SDL_CreateWindow(title == nullptr ? "libbgt" : title, width,
                                height, flags);
    if (s.window == nullptr) {
        s.set_error(BGT_ERROR_WINDOW, "failed to create window");
        s.close();
        return false;
    }

    s.renderer = SDL_CreateRenderer(s.window, nullptr);
    if (s.renderer == nullptr) {
        s.set_error(BGT_ERROR_RENDERER, "failed to create renderer");
        s.close();
        return false;
    }

    SDL_SetRenderDrawBlendMode(s.renderer, SDL_BLENDMODE_BLEND);

    // SSAA: 创建 2x 分辨率的离屏纹理作为画布，所有绘制画到它上面，
    // bgt_update_window 时线性缩放到窗口，实现全场景抗锯齿。
    s.canvas = SDL_CreateTexture(
        s.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
        width * kSuperSample, height * kSuperSample);
    if (s.canvas == nullptr) {
        s.set_error(BGT_ERROR_RENDERER, "failed to create SSAA canvas");
        s.close();
        return false;
    }
    SDL_SetTextureScaleMode(s.canvas, SDL_SCALEMODE_LINEAR);

    // 先切到画布，再设逻辑坐标：logical presentation 绑定在当前 render target 上，
    // 这样 800×600 的逻辑坐标会映射到画布的 1600×1200 像素
    SDL_SetRenderTarget(s.renderer, s.canvas);
    SDL_SetRenderLogicalPresentation(s.renderer, width, height,
                                     SDL_LOGICAL_PRESENTATION_STRETCH);

    s.open = true;
    s.closing = false;
    s.width = width;
    s.height = height;
    s.color = BGT_WHITE;
    s.background = BGT_BLACK;
    s.line_width = 1;
    s.font_size = kDefaultFontSize;
    s.fps_limit = 0;
    s.start_ticks = SDL_GetTicks();
    s.previous_ticks = s.start_ticks;
    s.fps_ticks = s.start_ticks;
    s.fps_frames = 0;
    s.next_frame_ns = 0;
    s.delta_time = 0.0;
    s.total_time = 0.0;
    s.fps = 0.0;
    s.keys.fill(false);
    s.previous_keys.fill(false);
    s.mouse_buttons.fill(false);
    s.previous_mouse_buttons.fill(false);
    apply_render_color(s, s.background);
    SDL_RenderClear(s.renderer);
    apply_render_color(s, s.color);
    return true;
}

} // namespace

bool bgt_open_window(int width, int height, const char title[])
{
    return open_window_impl(width, height, title, 0);
}

bool bgt_open_window_resizable(int width, int height, const char title[])
{
    return open_window_impl(width, height, title, SDL_WINDOW_RESIZABLE);
}

bool bgt_window_is_open()
{
    const State &s = state();
    return s.open && !s.closing;
}

void bgt_update_window()
{
    State &s = state();
    if (!ensure_open(s)) {
        return;
    }

    s.previous_keys = s.keys;
    s.previous_mouse_buttons = s.mouse_buttons;
    s.mouse_wheel = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT ||
            event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            s.closing = true;
        }
        else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            s.mouse_wheel += static_cast<int>(std::lround(event.wheel.y));
        }
    }

    sync_keyboard(s);
    sync_mouse(s);
    update_time(s);

    // SSAA: 把 2x 离屏纹理线性缩放到窗口显示
    SDL_SetRenderTarget(s.renderer, nullptr);
    apply_render_color(s, s.background);
    SDL_RenderClear(s.renderer);
    SDL_RenderTexture(s.renderer, s.canvas, nullptr, nullptr);
    SDL_RenderPresent(s.renderer);

    // 切回离屏纹理，清成背景色，下一帧从空白画布开始
    SDL_SetRenderTarget(s.renderer, s.canvas);
    apply_render_color(s, s.background);
    SDL_RenderClear(s.renderer);
    apply_render_color(s, s.color);

    update_fps(s);
    apply_fps_limit(s);
}

void bgt_close_window()
{
    state().close();
}

int bgt_window_width()
{
    return state().width;
}

int bgt_window_height()
{
    return state().height;
}

void bgt_set_window_title(const char title[])
{
    State &s = state();
    if (s.window != nullptr) {
        SDL_SetWindowTitle(s.window, title == nullptr ? "" : title);
    }
}

unsigned bgt_rgb(int r, int g, int b)
{
    return bgt_rgba(r, g, b, 255);
}

unsigned bgt_rgba(int r, int g, int b, int a)
{
    const std::uint32_t color =
        (static_cast<std::uint32_t>(clamp_byte(a)) << 24U) |
        (static_cast<std::uint32_t>(clamp_byte(r)) << 16U) |
        (static_cast<std::uint32_t>(clamp_byte(g)) << 8U) |
        static_cast<std::uint32_t>(clamp_byte(b));
    return static_cast<unsigned>(color);
}

void bgt_set_color(unsigned color)
{
    State &s = state();
    s.color = color;
    apply_render_color(s, color);
}

unsigned bgt_get_color()
{
    return state().color;
}

void bgt_set_background(unsigned color)
{
    // 只记录状态：真正的清空发生在 bgt_update_window() 结束时，
    // 下一帧起生效，中途调用不会擦掉本帧已经画好的内容。
    state().background = color;
}

void bgt_draw_point(int x, int y)
{
    State &s = state();
    if (!ensure_open(s)) {
        return;
    }
    apply_render_color(s, s.color);
    SDL_RenderPoint(s.renderer, static_cast<float>(x), static_cast<float>(y));
}

void bgt_draw_line(int x1, int y1, int x2, int y2)
{
    State &s = state();
    if (!ensure_open(s)) {
        return;
    }
    apply_render_color(s, s.color);

    if (s.line_width <= 1) {
        // 1px 线：SSAA 已提供全场景抗锯齿，直接用 SDL_RenderLine
        SDL_RenderLine(s.renderer, static_cast<float>(x1), static_cast<float>(y1),
                       static_cast<float>(x2), static_cast<float>(y2));
    } else {
        // 粗线：沿垂直方向偏移 SDL_RenderLine，避免多条 AA 线叠加导致的颜色不均和端点突出
        const double dx = static_cast<double>(x2 - x1);
        const double dy = static_cast<double>(y2 - y1);
        const double length = std::sqrt(dx * dx + dy * dy);
        if (length < 0.5) {
            const int half = s.line_width / 2;
            bgt_fill_rect(x1 - half, y1 - half, s.line_width, s.line_width);
        } else {
            const double px = -dy / length;
            const double py = dx / length;
            // 浮点偏移：偶数线宽时也精确（如 width=2 → off=-0.5, +0.5）
            const double center = (s.line_width - 1) / 2.0;
            for (int i = 0; i < s.line_width; ++i) {
                const float off = static_cast<float>(i - center);
                SDL_RenderLine(
                    s.renderer,
                    static_cast<float>(x1) + static_cast<float>(px) * off,
                    static_cast<float>(y1) + static_cast<float>(py) * off,
                    static_cast<float>(x2) + static_cast<float>(px) * off,
                    static_cast<float>(y2) + static_cast<float>(py) * off);
            }
        }
    }
}

void bgt_draw_rect(int x, int y, int width, int height)
{
    State &s = state();
    if (!ensure_open(s) || width <= 0 || height <= 0) {
        return;
    }
    const int line_width = std::max(1, s.line_width);

    // 线宽 >= 短边的一半时，直接填充整个矩形
    if (line_width * 2 >= width || line_width * 2 >= height) {
        bgt_fill_rect(x, y, width, height);
        return;
    }

    // 四条边互不重叠：上下画满宽，左右只填中间部分
    bgt_fill_rect(x, y, width, line_width);                          // 上
    bgt_fill_rect(x, y + height - line_width, width, line_width);    // 下
    bgt_fill_rect(x, y + line_width, line_width, height - 2 * line_width);         // 左
    bgt_fill_rect(x + width - line_width, y + line_width, line_width, height - 2 * line_width);  // 右
}

void bgt_fill_rect(int x, int y, int width, int height)
{
    State &s = state();
    if (!ensure_open(s) || width <= 0 || height <= 0) {
        return;
    }
    apply_render_color(s, s.color);
    SDL_FRect rect{static_cast<float>(x), static_cast<float>(y),
                   static_cast<float>(width), static_cast<float>(height)};
    SDL_RenderFillRect(s.renderer, &rect);
}

void bgt_draw_circle(int x, int y, int radius)
{
    State &s = state();
    if (!ensure_open(s) || radius <= 0) {
        return;
    }
    apply_render_color(s, s.color);
    const int line_width = std::max(1, s.line_width);
    const int outer_r = radius + (line_width + 1) / 2;
    const int inner_r = radius - line_width / 2;

    if (line_width <= 1) {
        // 1px：Bresenham 逐点画圆，收集后批量绘制
        std::vector<SDL_FPoint> circle_pts;
        circle_pts.reserve(static_cast<std::size_t>(8 * radius));
        int dx = radius;
        int dy = 0;
        int error = 1 - dx;
        while (dx >= dy) {
            const bool diag = (dx == dy);
            circle_pts.push_back(SDL_FPoint{static_cast<float>(x + dx),
                                             static_cast<float>(y + dy)});
            circle_pts.push_back(SDL_FPoint{static_cast<float>(x - dx),
                                             static_cast<float>(y - dy)});
            if (dy != 0) {
                circle_pts.push_back(SDL_FPoint{static_cast<float>(x - dx),
                                                 static_cast<float>(y + dy)});
                circle_pts.push_back(SDL_FPoint{static_cast<float>(x + dx),
                                                 static_cast<float>(y - dy)});
            }
            if (!diag) {
                if (dy != 0) {
                    circle_pts.push_back(SDL_FPoint{static_cast<float>(x + dy),
                                                     static_cast<float>(y + dx)});
                    circle_pts.push_back(SDL_FPoint{static_cast<float>(x - dy),
                                                     static_cast<float>(y + dx)});
                    circle_pts.push_back(SDL_FPoint{static_cast<float>(x - dy),
                                                     static_cast<float>(y - dx)});
                    circle_pts.push_back(SDL_FPoint{static_cast<float>(x + dy),
                                                     static_cast<float>(y - dx)});
                } else {
                    circle_pts.push_back(SDL_FPoint{static_cast<float>(x),
                                                     static_cast<float>(y + dx)});
                    circle_pts.push_back(SDL_FPoint{static_cast<float>(x),
                                                     static_cast<float>(y - dx)});
                }
            }
            ++dy;
            if (error < 0) {
                error += (2 * dy) + 1;
            }
            else {
                --dx;
                error += 2 * (dy - dx + 1);
            }
        }
        SDL_RenderPoints(s.renderer, circle_pts.data(),
                         static_cast<int>(circle_pts.size()));
    } else if (inner_r <= 0) {
        // 线宽 >= 半径：填充整个外圆
        bgt_fill_circle(x, y, outer_r);
    } else {
        // 粗线：扫描线填充环形区域（外圆 - 内圆），无间隙
        const int row_count = 2 * outer_r + 1;
        SDL_FRect *rects = static_cast<SDL_FRect *>(
            SDL_stack_alloc(SDL_FRect, row_count * 2));
        int idx = 0;
        for (int dy = -outer_r; dy <= outer_r; ++dy) {
            const double oy = static_cast<double>(dy) / outer_r;
            const int outer_span = static_cast<int>(std::lround(
                outer_r * std::sqrt(1.0 - oy * oy)));
            if (std::abs(dy) <= inner_r) {
                const double iy = static_cast<double>(dy) / inner_r;
                const int inner_span = static_cast<int>(std::lround(
                    inner_r * std::sqrt(1.0 - iy * iy)));
                const int left_w = outer_span - inner_span;
                // 左半环
                rects[idx++] = SDL_FRect{
                    static_cast<float>(x - outer_span),
                    static_cast<float>(y + dy),
                    static_cast<float>(left_w), 1.0F};
                // 右半环（+1 跳过内圆占据的中心列）
                rects[idx++] = SDL_FRect{
                    static_cast<float>(x + inner_span + 1),
                    static_cast<float>(y + dy),
                    static_cast<float>(left_w), 1.0F};
            } else {
                // 内圆不覆盖此行：整行都是环
                rects[idx++] = SDL_FRect{
                    static_cast<float>(x - outer_span),
                    static_cast<float>(y + dy),
                    static_cast<float>(2 * outer_span + 1), 1.0F};
            }
        }
        SDL_RenderFillRects(s.renderer, rects, idx);
        SDL_stack_free(rects);
    }
}

void bgt_fill_circle(int x, int y, int radius)
{
    State &s = state();
    if (!ensure_open(s) || radius <= 0) {
        return;
    }
    apply_render_color(s, s.color);
    const int row_count = 2 * radius + 1;
    SDL_FRect *rects = static_cast<SDL_FRect *>(
        SDL_stack_alloc(SDL_FRect, row_count));
    int idx = 0;
    for (int dy = -radius; dy <= radius; ++dy) {
        const double normalized_y =
            static_cast<double>(dy) / static_cast<double>(radius);
        const int span = static_cast<int>(std::lround(
            radius * std::sqrt(1.0 - (normalized_y * normalized_y))));
        rects[idx++] = SDL_FRect{
            static_cast<float>(x - span), static_cast<float>(y + dy),
            static_cast<float>(2 * span + 1), 1.0F};
    }
    SDL_RenderFillRects(s.renderer, rects, row_count);
    SDL_stack_free(rects);
}

void bgt_draw_ellipse(int x, int y, int radius_x, int radius_y)
{
    State &s = state();
    if (!ensure_open(s) || radius_x <= 0 || radius_y <= 0) {
        return;
    }
    apply_render_color(s, s.color);
    const int line_width = std::max(1, s.line_width);

    if (line_width <= 1) {
        // 1px：采样折线，一次 SDL_RenderLines 绘制
        const int steps = std::max(24, (radius_x + radius_y) * 3);
        const int pt_count = steps + 1;
        SDL_FPoint *pts = static_cast<SDL_FPoint *>(
            SDL_stack_alloc(SDL_FPoint, pt_count));
        for (int step = 0; step <= steps; ++step) {
            const double angle =
                (static_cast<double>(step) * kTwoPi) / static_cast<double>(steps);
            pts[step] = SDL_FPoint{
                static_cast<float>(x + std::lround(std::cos(angle) * radius_x)),
                static_cast<float>(y + std::lround(std::sin(angle) * radius_y))};
        }
        SDL_RenderLines(s.renderer, pts, pt_count);
        SDL_stack_free(pts);
    } else {
        // 粗线：扫描线填充环形区域（外椭圆 - 内椭圆），无间隙
        const int outer_rx = radius_x + (line_width + 1) / 2;
        const int outer_ry = radius_y + (line_width + 1) / 2;
        const int inner_rx = radius_x - line_width / 2;
        const int inner_ry = radius_y - line_width / 2;

        if (inner_rx <= 0 || inner_ry <= 0) {
            // 线宽 >= 短轴：填充整个外椭圆
            bgt_fill_ellipse(x, y, outer_rx, outer_ry);
            return;
        }

        const int row_count = 2 * outer_ry + 1;
        // 每行最多 2 个 rect（左半环 + 右半环）
        SDL_FRect *rects = static_cast<SDL_FRect *>(
            SDL_stack_alloc(SDL_FRect, row_count * 2));
        int idx = 0;
        for (int dy = -outer_ry; dy <= outer_ry; ++dy) {
            const double oy = static_cast<double>(dy) / outer_ry;
            const int outer_span = static_cast<int>(std::lround(
                outer_rx * std::sqrt(1.0 - oy * oy)));
            if (std::abs(dy) <= inner_ry) {
                const double iy = static_cast<double>(dy) / inner_ry;
                const int inner_span = static_cast<int>(std::lround(
                    inner_rx * std::sqrt(1.0 - iy * iy)));
                const int left_w = outer_span - inner_span;
                if (left_w > 0) {
                    rects[idx++] = SDL_FRect{
                        static_cast<float>(x - outer_span),
                        static_cast<float>(y + dy),
                        static_cast<float>(left_w), 1.0F};
                    rects[idx++] = SDL_FRect{
                        static_cast<float>(x + inner_span + 1),
                        static_cast<float>(y + dy),
                        static_cast<float>(left_w), 1.0F};
                }
            } else {
                rects[idx++] = SDL_FRect{
                    static_cast<float>(x - outer_span),
                    static_cast<float>(y + dy),
                    static_cast<float>(2 * outer_span + 1), 1.0F};
            }
        }
        SDL_RenderFillRects(s.renderer, rects, idx);
        SDL_stack_free(rects);
    }
}

void bgt_fill_ellipse(int x, int y, int radius_x, int radius_y)
{
    State &s = state();
    if (!ensure_open(s) || radius_x <= 0 || radius_y <= 0) {
        return;
    }
    apply_render_color(s, s.color);
    const int row_count = 2 * radius_y + 1;
    SDL_FRect *rects = static_cast<SDL_FRect *>(
        SDL_stack_alloc(SDL_FRect, row_count));
    int idx = 0;
    for (int dy = -radius_y; dy <= radius_y; ++dy) {
        const double normalized_y =
            static_cast<double>(dy) / static_cast<double>(radius_y);
        const int span = static_cast<int>(std::lround(
            radius_x * std::sqrt(1.0 - (normalized_y * normalized_y))));
        rects[idx++] = SDL_FRect{
            static_cast<float>(x - span), static_cast<float>(y + dy),
            static_cast<float>(2 * span + 1), 1.0F};
    }
    SDL_RenderFillRects(s.renderer, rects, row_count);
    SDL_stack_free(rects);
}

void bgt_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
    State &s = state();
    if (!ensure_open(s)) {
        return;
    }
    apply_render_color(s, s.color);

    if (s.line_width <= 1) {
        // 1px：用 SDL_RenderLines 画闭合折线，避免角点重复绘制
        SDL_FPoint tri_pts[4] = {
            {static_cast<float>(x1), static_cast<float>(y1)},
            {static_cast<float>(x2), static_cast<float>(y2)},
            {static_cast<float>(x3), static_cast<float>(y3)},
            {static_cast<float>(x1), static_cast<float>(y1)},
        };
        SDL_RenderLines(s.renderer, tri_pts, 4);
    } else {
        // 粗线：miter join —— 算出 6 个偏移顶点，填充 3 条边的矩形带
        const double p1x = static_cast<double>(x1);
        const double p1y = static_cast<double>(y1);
        const double p2x = static_cast<double>(x2);
        const double p2y = static_cast<double>(y2);
        const double p3x = static_cast<double>(x3);
        const double p3y = static_cast<double>(y3);

        // 三条边的方向向量
        const double e1x = p2x - p1x;
        const double e1y = p2y - p1y;
        const double e2x = p3x - p2x;
        const double e2y = p3y - p2y;
        const double e3x = p1x - p3x;
        const double e3y = p1y - p3y;

        const double len1 = std::sqrt(e1x * e1x + e1y * e1y);
        const double len2 = std::sqrt(e2x * e2x + e2y * e2y);
        const double len3 = std::sqrt(e3x * e3x + e3y * e3y);

        if (len1 < 0.5 || len2 < 0.5 || len3 < 0.5) {
            // 退化：有边太短， fallback 到独立粗线
            bgt_draw_line(x1, y1, x2, y2);
            bgt_draw_line(x2, y2, x3, y3);
            bgt_draw_line(x3, y3, x1, y1);
            return;
        }

        // 共线检查：三点共线时偏移线全部平行，无法形成 miter
        const Point a{x1, y1};
        const Point b{x2, y2};
        const Point c{x3, y3};
        if (edge_value(a, b, c) == 0) {
            bgt_draw_line(x1, y1, x2, y2);
            bgt_draw_line(x2, y2, x3, y3);
            bgt_draw_line(x3, y3, x1, y1);
            return;
        }

        // 三条边的垂直方向（单位法向量）
        const double n1x = -e1y / len1;
        const double n1y =  e1x / len1;
        const double n2x = -e2y / len2;
        const double n2y =  e2x / len2;
        const double n3x = -e3y / len3;
        const double n3y =  e3x / len3;

        const double half = static_cast<double>(s.line_width) / 2.0;

        // 每条边偏移 +half / -half 得到平行线，相邻边的同侧偏移线求交得到 miter point
        // pts[0..2]: +half 侧（分别在顶点 P1, P2, P3 处）
        // pts[3..5]: -half 侧（分别在顶点 P1, P2, P3 处）

        const double o1px = p1x + n1x * half;
        const double o1py = p1y + n1y * half;
        const double o2px = p2x + n2x * half;
        const double o2py = p2y + n2y * half;
        const double o3px = p3x + n3x * half;
        const double o3py = p3y + n3y * half;

        const double i1px = p1x - n1x * half;
        const double i1py = p1y - n1y * half;
        const double i2px = p2x - n2x * half;
        const double i2py = p2y - n2y * half;
        const double i3px = p3x - n3x * half;
        const double i3py = p3y - n3y * half;

        PointD pts[6];
        pts[0] = line_intersection(o3px, o3py, e3x, e3y,
                                    o1px, o1py, e1x, e1y);
        pts[1] = line_intersection(o1px, o1py, e1x, e1y,
                                    o2px, o2py, e2x, e2y);
        pts[2] = line_intersection(o2px, o2py, e2x, e2y,
                                    o3px, o3py, e3x, e3y);
        pts[3] = line_intersection(i3px, i3py, e3x, e3y,
                                    i1px, i1py, e1x, e1y);
        pts[4] = line_intersection(i1px, i1py, e1x, e1y,
                                    i2px, i2py, e2x, e2y);
        pts[5] = line_intersection(i2px, i2py, e2x, e2y,
                                    i3px, i3py, e3x, e3y);

        // Miter limit：当 miter point 离顶点超过 2*line_width 时，回退到该
        // 顶点本身（bevel join 的效果），防止极尖角产生巨大尖刺。
        const double miter_limit = 2.0 * s.line_width;
        const double vertices_xyz[6][2] = {
            {p1x, p1y}, {p2x, p2y}, {p3x, p3y},
            {p1x, p1y}, {p2x, p2y}, {p3x, p3y},
        };
        for (int i = 0; i < 6; ++i) {
            const double vx = vertices_xyz[i][0];
            const double vy = vertices_xyz[i][1];
            const double dx = pts[i].x - vx;
            const double dy = pts[i].y - vy;
            const double dist = std::sqrt(dx * dx + dy * dy);
            if (dist > miter_limit) {
                pts[i].x = vx + dx * (miter_limit / dist);
                pts[i].y = vy + dy * (miter_limit / dist);
            }
        }

        // 6 个顶点、6 个三角形（3 条 quad 各拆 2 个），一次 SDL_RenderGeometry
        SDL_Vertex verts[6];
        const SDL_Color col = to_sdl_color(s.color);
        const float inv255 = 1.0F / 255.0F;
        for (int i = 0; i < 6; ++i) {
            verts[i].position.x = static_cast<float>(pts[i].x);
            verts[i].position.y = static_cast<float>(pts[i].y);
            verts[i].color.r = static_cast<float>(col.r) * inv255;
            verts[i].color.g = static_cast<float>(col.g) * inv255;
            verts[i].color.b = static_cast<float>(col.b) * inv255;
            verts[i].color.a = static_cast<float>(col.a) * inv255;
            verts[i].tex_coord.x = 0.0F;
            verts[i].tex_coord.y = 0.0F;
        }
        // quad1: 0,1,4,3 → 三角形 (0,1,4) + (0,4,3)
        // quad2: 1,2,5,4 → 三角形 (1,2,5) + (1,5,4)
        // quad3: 2,0,3,5 → 三角形 (2,0,3) + (2,3,5)
        const int indices[] = {
            0, 1, 4,  0, 4, 3,   // quad1
            1, 2, 5,  1, 5, 4,   // quad2
            2, 0, 3,  2, 3, 5,   // quad3
        };
        SDL_RenderGeometry(s.renderer, nullptr, verts, 6, indices, 18);
    }
}

void bgt_fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
    State &s = state();
    if (!ensure_open(s)) {
        return;
    }
    const Point a{x1, y1};
    const Point b{x2, y2};
    const Point c{x3, y3};
    if (edge_value(a, b, c) == 0) {
        bgt_draw_triangle(x1, y1, x2, y2, x3, y3);
        return;
    }
    // 非退化：三个顶点直接交给 GPU 光栅化填充
    PointD pts[3] = {
        {static_cast<double>(x1), static_cast<double>(y1)},
        {static_cast<double>(x2), static_cast<double>(y2)},
        {static_cast<double>(x3), static_cast<double>(y3)},
    };
    fill_convex_polygon(s, pts, 3);
}

void bgt_set_line_width(int width)
{
    state().line_width = std::max(1, width);
}

int bgt_get_line_width()
{
    return state().line_width;
}

bool bgt_set_font(const char filename[], int size)
{
    State &s = state();
    if (filename == nullptr || filename[0] == '\0') {
        s.set_error(BGT_ERROR_FONT, "font filename is empty");
        return false;
    }
    if (size <= 0) {
        s.set_error(BGT_ERROR_FONT, "font size must be positive");
        return false;
    }
    s.close_fonts();
    s.font_path = filename;
    s.font_size = size;
    return get_font(s, size) != nullptr;
}

void bgt_set_font_size(int size)
{
    if (size > 0) {
        state().font_size = size;
    }
}

int bgt_get_font_size()
{
    return state().font_size;
}

void bgt_draw_text(int x, int y, const char text[])
{
    State &s = state();
    draw_text_impl(s, x, y, text, s.font_size);
}

void bgt_draw_text(int x, int y, const char text[], int size)
{
    draw_text_impl(state(), x, y, text, size);
}

int bgt_text_width(const char text[])
{
    State &s = state();
    return text_size_impl(s, text, s.font_size, true);
}

int bgt_text_width(const char text[], int size)
{
    return text_size_impl(state(), text, size, true);
}

int bgt_text_height(const char text[])
{
    State &s = state();
    return text_size_impl(s, text, s.font_size, false);
}

int bgt_text_height(const char text[], int size)
{
    return text_size_impl(state(), text, size, false);
}

bool bgt_key_is_down(int key)
{
    const State &s = state();
    if (key < 0 || key >= kMaxPublicKey) {
        return false;
    }
    return s.keys[static_cast<std::size_t>(key)];
}

bool bgt_key_just_pressed(int key)
{
    const State &s = state();
    if (key < 0 || key >= kMaxPublicKey) {
        return false;
    }
    const auto index = static_cast<std::size_t>(key);
    return s.keys[index] && !s.previous_keys[index];
}

bool bgt_key_just_released(int key)
{
    const State &s = state();
    if (key < 0 || key >= kMaxPublicKey) {
        return false;
    }
    const auto index = static_cast<std::size_t>(key);
    return !s.keys[index] && s.previous_keys[index];
}

int bgt_mouse_x()
{
    return state().mouse_x;
}

int bgt_mouse_y()
{
    return state().mouse_y;
}

bool bgt_mouse_is_down(int button)
{
    const int index = mouse_index(button);
    return index != 0 && state().mouse_buttons[static_cast<std::size_t>(index)];
}

bool bgt_mouse_just_pressed(int button)
{
    const int index = mouse_index(button);
    if (index == 0) {
        return false;
    }
    const State &s = state();
    const auto button_index = static_cast<std::size_t>(index);
    return s.mouse_buttons[button_index] &&
           !s.previous_mouse_buttons[button_index];
}

bool bgt_mouse_just_released(int button)
{
    const int index = mouse_index(button);
    if (index == 0) {
        return false;
    }
    const State &s = state();
    const auto button_index = static_cast<std::size_t>(index);
    return !s.mouse_buttons[button_index] &&
           s.previous_mouse_buttons[button_index];
}

int bgt_mouse_wheel()
{
    return state().mouse_wheel;
}

void bgt_show_mouse()
{
    SDL_ShowCursor();
}

void bgt_hide_mouse()
{
    SDL_HideCursor();
}

void bgt_delay(int milliseconds)
{
    if (milliseconds > 0) {
        SDL_Delay(static_cast<Uint32>(milliseconds));
    }
}

void bgt_set_fps_limit(int fps)
{
    state().fps_limit = std::max(0, fps);
}

double bgt_delta_time()
{
    return state().delta_time;
}

double bgt_total_time()
{
    return state().total_time;
}

double bgt_fps()
{
    return state().fps;
}

bool bgt_has_error()
{
    return state().error_code != BGT_ERROR_NONE;
}

int bgt_error_code()
{
    return state().error_code;
}

void bgt_print_error()
{
    const State &s = state();
    if (s.error_code == BGT_ERROR_NONE) {
        return;
    }
    std::fprintf(stderr, "libbgt error %d: %s\n", s.error_code,
                 s.error_message.c_str());
}

void bgt_draw_error(int x, int y, int size)
{
    State &s = state();
    if (s.error_code == BGT_ERROR_NONE || s.error_message.empty()) {
        return;
    }
    draw_text_impl(s, x, y, s.error_message.c_str(), size);
}

void bgt_clear_error()
{
    state().clear_error();
}

// NOLINTEND(readability-magic-numbers, modernize-avoid-c-arrays,
// bugprone-easily-swappable-parameters, readability-identifier-length)
