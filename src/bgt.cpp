/**
 * @file bgt.cpp
 * @brief libbgt 公开 API 的实现：窗口、绘制、文本、输入、存档、声音与错误诊断。
 * @details 本文件实现 `include/bgt.h` 里声明的全部 `bgt_*` 函数。
 *
 *          The implementation mirrors the beginner-facing API, where short
 *          coordinate names, integer colors, and C-style string parameters are
 *          deliberate.
 *
 *          窗口使用一块 2 倍分辨率的离屏纹理作为画布（SSAA）：所有绘制先画到
 *          画布上，`bgt_update_window()` 时才线性缩放到窗口，实现全场景抗锯齿。
 *          内部辅助函数按职责分在多个匿名命名空间里：窗口与绘制、文本、输入、
 *          存档表、声音、错误历史。
 */

#include "bgt.h"

#include "bgt_encoding.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <random>
#include <string>
#include <system_error>
#include <vector>

// NOLINTBEGIN(readability-magic-numbers, modernize-avoid-c-arrays,
// bugprone-easily-swappable-parameters, readability-identifier-length)

namespace {

/**
 * @brief 内部按键状态数组的长度；键码必须落在 `[0, kMaxPublicKey)` 内。
 */
constexpr int kMaxPublicKey = 512;
/**
 * @brief 鼠标按键状态数组的长度（下标 1-3 对应左/右/中键，0 不用）。
 */
constexpr int kMouseButtonCount = 4;
/**
 * @brief 默认字号（像素）。
 */
constexpr int kDefaultFontSize = 24;
constexpr int kSuperSample = 2; ///< SSAA 倍率（画布是逻辑尺寸的这么多倍）。
/**
 * @brief 统计帧率的时间窗口（秒）。
 */
constexpr double kFpsWindowSeconds = 1.0;
/**
 * @brief 2π，用于圆周/椭圆采样与弧度换算。
 */
constexpr double kTwoPi = 6.28318530717958647692;

/**
 * @brief 字体缓存条目：字体文件路径 + 字号 + 已打开的 TTF 字体句柄。
 */
struct FontEntry {
    std::string path;            ///< 字体文件路径（源编码）。
    int size = kDefaultFontSize; ///< 字号（像素）。
    TTF_Font *font = nullptr;    ///< 已打开的字体句柄；由库统一关闭。
};

/**
 * @brief 整数点坐标，用于整数几何计算（如三角形朝向判断）。
 */
struct Point {
    int x = 0; ///< x 坐标。
    int y = 0; ///< y 坐标。
};

/**
 * @brief 图片条目：纹理 + 严格 RST 变换状态（T 平移 / R 旋转 / S 缩放）。
 *
 * 变换合成顺序固定为 M = T * R * S（先 S 后 R 后 T），全部围绕局部原点。
 */
struct ImageEntry {
    SDL_Texture *texture = nullptr; ///< 解码后的纹理；由库统一销毁。
    double rotation = 0.0;          ///< R：旋转角（度，顺时针）。
    double scale_x = 1.0;           ///< S：横向缩放因子（负值 = 左右翻转）。
    double scale_y = 1.0;           ///< S：纵向缩放因子（负值 = 上下翻转）。
    int offset_x = 0;               ///< T：相对绘制位置的横向偏移。
    int offset_y = 0;               ///< T：相对绘制位置的纵向偏移。
};

/**
 * @brief 一段已加载的音效：解码句柄 + 学生设置的音量（0-100）。
 */
struct SoundEntry {
    MIX_Audio *audio = nullptr; ///< 解码后的音频数据；由库统一销毁。
    int volume = 100;           ///< 学生设置的音量（0-100）。
};

/**
 * @brief 错误历史条目：错误码 + 完整消息（已拼好 SDL 原文后缀）。
 */
struct ErrorEntry {
    int code = BGT_ERROR_NONE; ///< 错误码（`BGT_ERROR_*` 常量）。
    std::string message;       ///< 完整消息（源编码，已拼上 SDL 原文后缀）。
};

/**
 * @brief 错误历史容量：超过后最老的条目被挤出。
 */
constexpr int kMaxErrorHistory = 10;

/**
 * @brief libbgt 的全局运行状态：窗口、渲染器、资源、输入、计时与错误历史。
 *
 * 包含窗口/渲染器、SSAA 画布、字体与图片缓存、输入状态、帧率计时、存档表、
 * 声音句柄与错误历史。整个库只有一个实例，由 `state()` 返回；`close()` 会
 * 释放全部 SDL 资源并把它恢复到未打开状态。
 */
struct State {
    SDL_Window *window = nullptr;     ///< SDL 窗口；未打开时为 nullptr。
    SDL_Renderer *renderer = nullptr; ///< SDL 渲染器；未打开时为 nullptr。
    bool sdl_ready = false;           ///< SDL_Init() 是否成功（关闭时 Quit）。
    bool ttf_ready = false;           ///< TTF_Init() 是否成功（关闭时 Quit）。
    bool open = false;                ///< 窗口是否已成功创建并处于打开状态。
    bool closing = false;             ///< 是否已收到退出/关闭事件。
    int width = 0;                    ///< 逻辑绘图宽度（像素）。
    int height = 0;                   ///< 逻辑绘图高度（像素）。
    unsigned color = BGT_WHITE;       ///< 当前绘图颜色。
    unsigned background = BGT_BLACK;  ///< 画布背景色。
    int line_width = 1;               ///< 当前线宽（像素，至少为 1）。
    int font_size = kDefaultFontSize; ///< 当前默认字号（像素）。
    int fps_limit = 0;                ///< 帧率上限；<= 0 表示不限制。
    std::string font_path;            ///< 当前字体文件路径（源编码）。
    std::vector<FontEntry> fonts;     ///< 已打开字体的缓存。
    std::vector<ImageEntry> images;   ///< 图片条目，编号 = 下标 + 1。
    std::array<bool, kMaxPublicKey> keys{};              ///< 本帧按键状态。
    std::array<bool, kMaxPublicKey> previous_keys{};     ///< 上一帧按键状态。
    std::array<bool, kMouseButtonCount> mouse_buttons{}; ///< 本帧鼠标按键。
    std::array<bool, kMouseButtonCount>
        previous_mouse_buttons{};     ///< 上一帧鼠标键。
    int mouse_x = 0;                  ///< 鼠标 x 坐标（逻辑坐标）。
    int mouse_y = 0;                  ///< 鼠标 y 坐标（逻辑坐标）。
    int mouse_wheel = 0;              ///< 本帧鼠标滚轮滚动量。
    SDL_Texture *canvas = nullptr;    ///< SSAA 离屏纹理（2 倍分辨率）。
    std::uint64_t start_ticks = 0;    ///< 窗口创建时刻（毫秒）。
    std::uint64_t previous_ticks = 0; ///< 上一帧时刻（毫秒）。
    std::uint64_t fps_ticks = 0;      ///< 本次帧率统计窗口起点（毫秒）。
    int fps_frames = 0;               ///< 本次帧率统计窗口内的帧数。
    std::uint64_t next_frame_ns = 0;  ///< 下一帧的目标时刻（纳秒）。
    double delta_time = 0.0;          ///< 上一帧到当前帧的秒数。
    double total_time = 0.0;          ///< 窗口创建以来的总秒数。
    double fps = 0.0;                 ///< 最近统计到的帧率。
    /// v0.3 存档表：节名 → 键名 → 值的文本形式。
    std::map<std::string, std::map<std::string, std::string>> storage;

    /// v0.3 声音：SDL_mixer 句柄、音效表与音乐单实例。
    MIX_Mixer *audio_mixer = nullptr; ///< 懒初始化，见 `ensure_audio()`。
    std::map<int, SoundEntry> sounds; ///< 音效 ID → 数据。
    int next_sound_id = 1;            ///< 下一个音效 ID；0 保留为“无效 ID”。
    std::vector<MIX_Track *> sound_tracks; ///< 音效轨道池（可重叠播放）。
    MIX_Track *music_track = nullptr;      ///< 背景音乐全局单实例。
    int music_volume = 100;                ///< 0-100；无实例时记忆。
    std::deque<ErrorEntry> errors; ///< 错误历史，最多 kMaxErrorHistory 条。

    ~State()
    {
        close();
    }

    void clear_error()
    {
        errors.clear();
        SDL_ClearError();
    }

    void set_error(int code, const std::string &message)
    {
        ErrorEntry entry;
        entry.code = code;
        entry.message = message;
        /// SDL 的错误文本是 UTF-8，先转回源编码再拼接：错误历史里只保留一种
        /// 编码（源编码），`bgt_error_text()` 与 `bgt_print_error()` 才能直接
        /// 使用。
        const std::string sdl_error = bgt::from_utf8(SDL_GetError());
        if (!sdl_error.empty()) {
            entry.message += ": ";
            entry.message += sdl_error;
        }
        /// SDL 错误消费掉就清：错误历史里连续两条错误时，下一条
        /// 不会再拼到这一条留下的陈旧 SDL 文本。
        SDL_ClearError();
        errors.push_back(entry);
        if (static_cast<int>(errors.size()) > kMaxErrorHistory) {
            errors.pop_front();
        }
    }

    void close_sounds()
    {
        if (music_track != nullptr) {
            MIX_DestroyTrack(music_track);
            music_track = nullptr;
        }
        for (MIX_Track *track : sound_tracks) {
            if (track != nullptr) {
                MIX_DestroyTrack(track);
            }
        }
        sound_tracks.clear();
        for (auto &entry : sounds) {
            if (entry.second.audio != nullptr) {
                MIX_DestroyAudio(entry.second.audio);
            }
        }
        sounds.clear();
        next_sound_id = 1;
        music_volume = 100;
        if (audio_mixer != nullptr) {
            MIX_DestroyMixer(audio_mixer);
            audio_mixer = nullptr;
            MIX_Quit();
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

    void close_images()
    {
        for (ImageEntry &entry : images) {
            if (entry.texture != nullptr) {
                SDL_DestroyTexture(entry.texture);
                entry.texture = nullptr;
            }
        }
        images.clear();
    }

    void close()
    {
        if (canvas != nullptr) {
            SDL_DestroyTexture(canvas);
            canvas = nullptr;
        }
        close_images();
        close_fonts();
        close_sounds();
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

/**
 * @brief 返回全局唯一的 `State` 实例（首次调用时构造）。
 */
State &state()
{
    static State instance;
    return instance;
}

/**
 * @brief 返回与窗口生命周期无关的随机数引擎。
 *
 * 首次使用时用 `std::random_device` 自动播种，因此学生不调用
 * `bgt_random_seed()` 也能得到每次运行都不同的随机序列。
 */
std::mt19937 &random_engine()
{
    static std::mt19937 engine{std::random_device{}()};
    return engine;
}

/**
 * @brief 把颜色分量收敛到 0-255。
 */
int clamp_byte(int value)
{
    return std::clamp(value, 0, 255);
}

/**
 * @brief 取出整数颜色值的 alpha（不透明度）分量。
 */
std::uint8_t color_a(unsigned color)
{
    return static_cast<std::uint8_t>(
        (static_cast<std::uint32_t>(color) >> 24U) & 0xFFU);
}

/**
 * @brief 取出整数颜色值的红色分量。
 */
std::uint8_t color_r(unsigned color)
{
    return static_cast<std::uint8_t>(
        (static_cast<std::uint32_t>(color) >> 16U) & 0xFFU);
}

/**
 * @brief 取出整数颜色值的绿色分量。
 */
std::uint8_t color_g(unsigned color)
{
    return static_cast<std::uint8_t>((static_cast<std::uint32_t>(color) >> 8U) &
                                     0xFFU);
}

/**
 * @brief 取出整数颜色值的蓝色分量。
 */
std::uint8_t color_b(unsigned color)
{
    return static_cast<std::uint8_t>(static_cast<std::uint32_t>(color) & 0xFFU);
}

/**
 * @brief 把整数颜色拆成 `SDL_Color` 的 RGBA 四个分量。
 */
SDL_Color to_sdl_color(unsigned color)
{
    return SDL_Color{color_r(color), color_g(color), color_b(color),
                     color_a(color)};
}

/**
 * @brief 把颜色设置到渲染器；渲染器未创建时安全返回。
 */
void apply_render_color(State &s, unsigned color)
{
    if (s.renderer == nullptr) {
        return;
    }
    SDL_SetRenderDrawColor(s.renderer, color_r(color), color_g(color),
                           color_b(color), color_a(color));
}

/**
 * @brief 检查窗口是否已打开。
 *
 * 未打开时记录 `BGT_ERROR_NOT_OPEN` 错误并返回 false。
 *
 * @param s 全局状态。
 * @return 窗口已打开（且渲染器可用）返回 true，否则返回 false。
 */
bool ensure_open(State &s)
{
    if (!s.open || s.renderer == nullptr) {
        s.set_error(BGT_ERROR_NOT_OPEN, "libbgt window is not open");
        return false;
    }
    return true;
}

/**
 * @brief 按图片编号取图片条目。
 *
 * 图片编号从 1 开始；越界时返回 nullptr（bounds 契约，不记错误）。
 *
 * @param s 全局状态。
 * @param image_id 图片编号（从 1 开始）。
 * @return 对应的图片条目；编号越界时返回 nullptr。
 */
ImageEntry *get_image(State &s, int image_id)
{
    if (image_id <= 0) {
        return nullptr;
    }
    const auto index = static_cast<std::size_t>(image_id - 1);
    if (index >= s.images.size()) {
        return nullptr;
    }
    return &s.images[index];
}

/**
 * @brief 拼接两个路径片段，必要时补一个 `/` 分隔符。
 */
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

/**
 * @brief 判断文件是否存在。
 *
 * @param path 文件路径（源编码）。
 * @return 文件存在（且可打开）返回 true，否则返回 false。
 */
bool file_exists(const std::string &path)
{
    /// SDL 的文件接口按 UTF-8 解释路径，path 是源编码。
    const bgt::Utf8View utf8_path(path.c_str());
    SDL_IOStream *stream = SDL_IOFromFile(utf8_path.c_str(), "rb");
    if (stream == nullptr) {
        return false;
    }
    SDL_CloseIO(stream);
    return true;
}

/**
 * @brief 往字体候选列表追加一个路径；空路径与重复项会被跳过。
 *
 * @param candidates 候选路径列表（输出参数）。
 * @param path 待追加的路径。
 */
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

/**
 * @brief 按操作系统返回默认中文字体的候选路径列表。
 *
 * @return 按优先级排列的候选路径。
 */
std::vector<std::string> default_font_candidates()
{
    std::vector<std::string> candidates;
#ifdef _WIN32
    /// `SDL_getenv` 在 Windows 上走 GetEnvironmentVariableA，返回 ANSI
    /// 字符串，正好与源编码一致，因此这里不转换（WINDIR 实际上总是
    /// ASCII 路径）。
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

/**
 * @brief 返回第一个实际存在的默认字体路径；都找不到时返回空串。
 */
std::string resolve_default_font()
{
    for (const std::string &candidate : default_font_candidates()) {
        if (file_exists(candidate)) {
            return candidate;
        }
    }
    return "";
}

/**
 * @brief 按字号取字体，必要时打开并缓存。
 *
 * 字号 <= 0 时依次退回当前默认字号与 `kDefaultFontSize`；字体路径为空时先
 * 解析系统默认字体。失败时记录错误并返回 nullptr。
 *
 * @param s 全局状态。
 * @param size 字号（像素）。
 * @return 字体句柄；失败时返回 nullptr。
 */
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

    /// 字体路径在库内按源编码保存，SDL_ttf 需要 UTF-8。
    const bgt::Utf8View utf8_font_path(s.font_path.c_str());
    TTF_Font *font =
        TTF_OpenFont(utf8_font_path.c_str(), static_cast<float>(size));
    if (font == nullptr) {
        s.set_error(BGT_ERROR_FONT, "failed to open font " + s.font_path);
        return nullptr;
    }

    s.fonts.push_back(FontEntry{s.font_path, size, font});
    return font;
}

/**
 * @brief 把数字键/小键盘数字键的 scancode 映射为 `BGT_KEY_0`..`BGT_KEY_9`。
 *
 * @param scancode SDL 扫描码。
 * @return 对应的数字键码；其他键返回 0。
 */
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

/**
 * @brief Num Lock 关闭时把 4/2/6/8 小键盘键映射为方向键。
 *
 * @param scancode SDL 扫描码。
 * @return 对应的方向键码；其他键返回 0。
 */
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

/**
 * @brief 判断 scancode 是否是小键盘数字键。
 *
 * @param scancode SDL 扫描码。
 * @return 是则返回 true，否则返回 false。
 */
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

/**
 * @brief 把 SDL scancode 与修饰键映射为 `BGT_KEY_*` 键码。
 *
 * 方向键、空格、回车等特殊键按表映射；A-Z 与 F1-F12 按连续区间换算；
 * Num Lock 关闭时小键盘 2/4/6/8 映射为方向键。
 *
 * @param scancode SDL 扫描码。
 * @param modifiers 当前修饰键状态（用于判断 Num Lock）。
 * @return 对应的键码；无法识别时返回 0。
 */
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

/**
 * @brief 用 SDL 的键盘状态刷新本帧按键数组。
 *
 * @param s 全局状态。
 */
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

/**
 * @brief 把鼠标按键常量映射为状态数组下标；无效按键返回 0。
 *
 * @param button 鼠标按键常量（`BGT_MOUSE_*`）。
 * @return 数组下标；无效按键返回 0。
 */
int mouse_index(int button)
{
    if (button < BGT_MOUSE_LEFT || button > BGT_MOUSE_MIDDLE) {
        return 0;
    }
    return button;
}

/**
 * @brief 刷新本帧鼠标位置与按键状态（位置已换算到逻辑坐标）。
 *
 * @param s 全局状态。
 */
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

/**
 * @brief 更新 `delta_time` 与 `total_time`。
 *
 * @param s 全局状态。
 */
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

/**
 * @brief 按 `kFpsWindowSeconds` 窗口统计实际帧率。
 *
 * @param s 全局状态。
 */
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

/**
 * @brief 按 `fps_limit` 精确延时，必要时重新对齐节拍。
 *
 * @param s 全局状态。
 */
void apply_fps_limit(State &s)
{
    if (s.fps_limit <= 0) {
        s.next_frame_ns = 0;
        return;
    }
    const std::uint64_t period_ns =
        static_cast<std::uint64_t>(SDL_NS_PER_SECOND) /
        static_cast<std::uint64_t>(s.fps_limit);
    const std::uint64_t now = SDL_GetTicksNS();

    if (s.next_frame_ns == 0) {
        s.next_frame_ns = now + period_ns;
        return;
    }
    if (now < s.next_frame_ns) {
        SDL_DelayPrecise(s.next_frame_ns - now);
    }
    else if (now - s.next_frame_ns >= period_ns) {
        /// 落后超过一整个帧周期（如卡顿、拖动窗口）时放弃补帧，
        /// 从当前时刻重新对齐节拍，避免连续高速追赶。
        s.next_frame_ns = now + period_ns;
        return;
    }
    s.next_frame_ns += period_ns;
}

/**
 * @brief 用 UTF-8 文本在离屏画布上绘制一行文字。
 *
 * text 必须是 UTF-8：静态文本由 `bgt_draw_text()` 转换，错误消息由
 * `draw_wrapped_error()` 转换，这里不再做编码转换。
 *
 * @param s 全局状态。
 * @param x 文字左上角 x 坐标（逻辑坐标）。
 * @param y 文字左上角 y 坐标（逻辑坐标）。
 * @param text UTF-8 文本；为 nullptr 或空串时直接返回。
 * @param size 字号（像素）；<= 0 时使用当前默认字号。
 */
void draw_text_utf8_impl(State &s, int x, int y, const char text[], int size)
{
    if (!ensure_open(s) || text == nullptr || text[0] == '\0') {
        return;
    }

    if (color_a(s.color) == 0) {
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

/**
 * @brief 测量 UTF-8 文本使用指定字号绘制时的尺寸。
 *
 * text 必须是 UTF-8，理由同 `draw_text_utf8_impl()`。
 *
 * @param s 全局状态。
 * @param text UTF-8 文本；为 nullptr 或空串时返回 0。
 * @param size 字号（像素）；<= 0 时使用当前默认字号。
 * @param width true 返回宽度，false 返回高度。
 * @return 测量得到的像素数；失败时返回 0。
 */
int text_size_utf8_impl(State &s, const char text[], int size, bool width)
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

/**
 * @brief 计算叉积，判断点 p 在有向边 a→b 的哪一侧（0 表示共线）。
 *
 * @param a 有向边起点。
 * @param b 有向边终点。
 * @param p 待判断的点。
 * @return 叉积值（正负号表示方向）。
 */
int edge_value(const Point &a, const Point &b, const Point &p)
{
    return ((p.x - a.x) * (b.y - a.y)) - ((p.y - a.y) * (b.x - a.x));
}

/**
 * @brief 双精度点坐标，用于几何计算与顶点偏移。
 */
struct PointD {
    double x = 0.0; ///< x 坐标。
    double y = 0.0; ///< y 坐标。
};

/**
 * @brief 求两条参数式直线的交点。
 *
 * 求点 (p1x, p1y) 沿方向 (d1x, d1y) 的直线与点 (p2x, p2y) 沿方向
 * (d2x, d2y) 的直线的交点；两条直线平行时返回两点的中点作为 fallback。
 *
 * @return 交点坐标；两条直线平行时返回 (p1 + p2) / 2。
 */
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

/**
 * @brief 用 `SDL_RenderGeometry` 以 triangle fan 方式填充凸多边形。
 *
 * @param s 全局状态。
 * @param pts 多边形顶点数组（至少 count 个）。
 * @param count 顶点数；小于 3 时直接返回。
 */
void fill_convex_polygon(State &s, const PointD *pts, int count)
{
    if (count < 3) {
        return;
    }
    SDL_Vertex *vertices =
        static_cast<SDL_Vertex *>(SDL_stack_alloc(SDL_Vertex, count));
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

/**
 * @brief 实现 `bgt_open_window()` 与 `bgt_open_window_resizable()` 的共同逻辑。
 *
 * 初始化 SDL3/SDL3_ttf，创建窗口、渲染器与 2 倍分辨率的 SSAA 画布，并重置
 * 全部运行状态。任何一步失败都会记录错误并关闭已创建的资源。
 *
 * @param width 逻辑绘图宽度（像素，必须大于 0）。
 * @param height 逻辑绘图高度（像素，必须大于 0）。
 * @param title 窗口标题（源编码；nullptr 时用 "libbgt"）。
 * @param flags 传给 `SDL_CreateWindow()` 的窗口标志。
 * @return 创建成功返回 true；失败返回 false 并记录错误。
 */
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

    /// 窗口标题是 UTF-8 字符串，title 是源编码。
    const bgt::Utf8View utf8_title(title == nullptr ? "libbgt" : title);
    s.window = SDL_CreateWindow(utf8_title.c_str(), width, height, flags);
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

    /// SSAA：创建 2x 分辨率的离屏纹理作为画布，所有绘制画到它上面，
    /// `bgt_update_window()` 时线性缩放到窗口，实现全场景抗锯齿。
    s.canvas = SDL_CreateTexture(s.renderer, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_TARGET, width * kSuperSample,
                                 height * kSuperSample);
    if (s.canvas == nullptr) {
        s.set_error(BGT_ERROR_RENDERER, "failed to create SSAA canvas");
        s.close();
        return false;
    }
    SDL_SetTextureScaleMode(s.canvas, SDL_SCALEMODE_LINEAR);

    /// 先切到画布，再设逻辑坐标：logical presentation 绑定在当前 render
    /// target 上，这样 800×600 的逻辑坐标会映射到画布的 1600×1200 像素。
    SDL_SetRenderTarget(s.renderer, nullptr);
    SDL_SetRenderLogicalPresentation(s.renderer, width, height,
                                     SDL_LOGICAL_PRESENTATION_STRETCH);

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

    /// SSAA：把 2x 离屏纹理线性缩放到窗口显示。
    SDL_SetRenderTarget(s.renderer, nullptr);
    apply_render_color(s, s.background);
    SDL_RenderClear(s.renderer);
    SDL_RenderTexture(s.renderer, s.canvas, nullptr, nullptr);
    SDL_RenderPresent(s.renderer);

    /// 切回离屏纹理，清成背景色，下一帧从空白画布开始。
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
        const bgt::Utf8View utf8_title(title == nullptr ? "" : title);
        SDL_SetWindowTitle(s.window, utf8_title.c_str());
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
    /// 只记录状态：真正的清空发生在 `bgt_update_window()` 结束时，
    /// 下一帧起生效，中途调用不会擦掉本帧已经画好的内容。
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
        /// 1px 线：SSAA 已提供全场景抗锯齿，直接用 `SDL_RenderLine`。
        SDL_RenderLine(s.renderer, static_cast<float>(x1),
                       static_cast<float>(y1), static_cast<float>(x2),
                       static_cast<float>(y2));
    }
    else {
        /// 粗线：沿垂直方向偏移 `SDL_RenderLine`，避免多条 AA 线叠加导致的
        /// 颜色不均和端点突出。
        const double dx = static_cast<double>(x2 - x1);
        const double dy = static_cast<double>(y2 - y1);
        const double length = std::sqrt(dx * dx + dy * dy);
        if (length < 0.5) {
            const int half = s.line_width / 2;
            bgt_fill_rect(x1 - half, y1 - half, s.line_width, s.line_width);
        }
        else {
            const double px = -dy / length;
            const double py = dx / length;
            /// 浮点偏移：偶数线宽时也精确（如 width=2 → off=-0.5, +0.5）。
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

    /// 线宽 >= 短边的一半时，直接填充整个矩形。
    if (line_width * 2 >= width || line_width * 2 >= height) {
        bgt_fill_rect(x, y, width, height);
        return;
    }

    /// 四条边互不重叠：上下画满宽，左右只填中间部分。
    bgt_fill_rect(x, y, width, line_width);                       ///< 上
    bgt_fill_rect(x, y + height - line_width, width, line_width); ///< 下
    bgt_fill_rect(x, y + line_width, line_width,
                  height - 2 * line_width); ///< 左
    bgt_fill_rect(x + width - line_width, y + line_width, line_width,
                  height - 2 * line_width); ///< 右
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
        /// 1px：Bresenham 逐点画圆，收集后批量绘制。
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
                    circle_pts.push_back(
                        SDL_FPoint{static_cast<float>(x + dy),
                                   static_cast<float>(y + dx)});
                    circle_pts.push_back(
                        SDL_FPoint{static_cast<float>(x - dy),
                                   static_cast<float>(y + dx)});
                    circle_pts.push_back(
                        SDL_FPoint{static_cast<float>(x - dy),
                                   static_cast<float>(y - dx)});
                    circle_pts.push_back(
                        SDL_FPoint{static_cast<float>(x + dy),
                                   static_cast<float>(y - dx)});
                }
                else {
                    circle_pts.push_back(SDL_FPoint{
                        static_cast<float>(x), static_cast<float>(y + dx)});
                    circle_pts.push_back(SDL_FPoint{
                        static_cast<float>(x), static_cast<float>(y - dx)});
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
    }
    else if (inner_r <= 0) {
        /// 线宽 >= 半径：填充整个外圆。
        bgt_fill_circle(x, y, outer_r);
    }
    else {
        /// 粗线：扫描线填充环形区域（外圆 - 内圆），无间隙。
        const int row_count = 2 * outer_r + 1;
        SDL_FRect *rects =
            static_cast<SDL_FRect *>(SDL_stack_alloc(SDL_FRect, row_count * 2));
        int idx = 0;
        for (int dy = -outer_r; dy <= outer_r; ++dy) {
            const double oy = static_cast<double>(dy) / outer_r;
            const int outer_span = static_cast<int>(
                std::lround(outer_r * std::sqrt(1.0 - oy * oy)));
            if (std::abs(dy) <= inner_r) {
                const double iy = static_cast<double>(dy) / inner_r;
                const int inner_span = static_cast<int>(
                    std::lround(inner_r * std::sqrt(1.0 - iy * iy)));
                const int left_w = outer_span - inner_span;
                /// 左半环。
                rects[idx++] = SDL_FRect{static_cast<float>(x - outer_span),
                                         static_cast<float>(y + dy),
                                         static_cast<float>(left_w), 1.0F};
                /// 右半环（+1 跳过内圆占据的中心列）。
                rects[idx++] = SDL_FRect{static_cast<float>(x + inner_span + 1),
                                         static_cast<float>(y + dy),
                                         static_cast<float>(left_w), 1.0F};
            }
            else {
                /// 内圆不覆盖此行：整行都是环。
                rects[idx++] =
                    SDL_FRect{static_cast<float>(x - outer_span),
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
    SDL_FRect *rects =
        static_cast<SDL_FRect *>(SDL_stack_alloc(SDL_FRect, row_count));
    int idx = 0;
    for (int dy = -radius; dy <= radius; ++dy) {
        const double normalized_y =
            static_cast<double>(dy) / static_cast<double>(radius);
        const int span = static_cast<int>(std::lround(
            radius * std::sqrt(1.0 - (normalized_y * normalized_y))));
        rects[idx++] =
            SDL_FRect{static_cast<float>(x - span), static_cast<float>(y + dy),
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
        /// 1px：采样折线，一次 `SDL_RenderLines` 绘制。
        const int steps = std::max(24, (radius_x + radius_y) * 3);
        const int pt_count = steps + 1;
        SDL_FPoint *pts =
            static_cast<SDL_FPoint *>(SDL_stack_alloc(SDL_FPoint, pt_count));
        for (int step = 0; step <= steps; ++step) {
            const double angle = (static_cast<double>(step) * kTwoPi) /
                                 static_cast<double>(steps);
            pts[step] = SDL_FPoint{
                static_cast<float>(x + std::lround(std::cos(angle) * radius_x)),
                static_cast<float>(y +
                                   std::lround(std::sin(angle) * radius_y))};
        }
        SDL_RenderLines(s.renderer, pts, pt_count);
        SDL_stack_free(pts);
    }
    else {
        /// 粗线：扫描线填充环形区域（外椭圆 - 内椭圆），无间隙。
        const int outer_rx = radius_x + (line_width + 1) / 2;
        const int outer_ry = radius_y + (line_width + 1) / 2;
        const int inner_rx = radius_x - line_width / 2;
        const int inner_ry = radius_y - line_width / 2;

        if (inner_rx <= 0 || inner_ry <= 0) {
            /// 线宽 >= 短轴：填充整个外椭圆。
            bgt_fill_ellipse(x, y, outer_rx, outer_ry);
            return;
        }

        const int row_count = 2 * outer_ry + 1;
        /// 每行最多 2 个 rect（左半环 + 右半环）。
        SDL_FRect *rects =
            static_cast<SDL_FRect *>(SDL_stack_alloc(SDL_FRect, row_count * 2));
        int idx = 0;
        for (int dy = -outer_ry; dy <= outer_ry; ++dy) {
            const double oy = static_cast<double>(dy) / outer_ry;
            const int outer_span = static_cast<int>(
                std::lround(outer_rx * std::sqrt(1.0 - oy * oy)));
            if (std::abs(dy) <= inner_ry) {
                const double iy = static_cast<double>(dy) / inner_ry;
                const int inner_span = static_cast<int>(
                    std::lround(inner_rx * std::sqrt(1.0 - iy * iy)));
                const int left_w = outer_span - inner_span;
                if (left_w > 0) {
                    rects[idx++] = SDL_FRect{static_cast<float>(x - outer_span),
                                             static_cast<float>(y + dy),
                                             static_cast<float>(left_w), 1.0F};
                    rects[idx++] =
                        SDL_FRect{static_cast<float>(x + inner_span + 1),
                                  static_cast<float>(y + dy),
                                  static_cast<float>(left_w), 1.0F};
                }
            }
            else {
                rects[idx++] =
                    SDL_FRect{static_cast<float>(x - outer_span),
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
    SDL_FRect *rects =
        static_cast<SDL_FRect *>(SDL_stack_alloc(SDL_FRect, row_count));
    int idx = 0;
    for (int dy = -radius_y; dy <= radius_y; ++dy) {
        const double normalized_y =
            static_cast<double>(dy) / static_cast<double>(radius_y);
        const int span = static_cast<int>(std::lround(
            radius_x * std::sqrt(1.0 - (normalized_y * normalized_y))));
        rects[idx++] =
            SDL_FRect{static_cast<float>(x - span), static_cast<float>(y + dy),
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
        /// 1px：用 `SDL_RenderLines` 画闭合折线，避免角点重复绘制。
        SDL_FPoint tri_pts[4] = {
            {static_cast<float>(x1), static_cast<float>(y1)},
            {static_cast<float>(x2), static_cast<float>(y2)},
            {static_cast<float>(x3), static_cast<float>(y3)},
            {static_cast<float>(x1), static_cast<float>(y1)},
        };
        SDL_RenderLines(s.renderer, tri_pts, 4);
    }
    else {
        /// 粗线：miter join —— 算出 6 个偏移顶点，填充 3 条边的矩形带。
        const double p1x = static_cast<double>(x1);
        const double p1y = static_cast<double>(y1);
        const double p2x = static_cast<double>(x2);
        const double p2y = static_cast<double>(y2);
        const double p3x = static_cast<double>(x3);
        const double p3y = static_cast<double>(y3);

        /// 三条边的方向向量。
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
            /// 退化：有边太短，回退到独立粗线。
            bgt_draw_line(x1, y1, x2, y2);
            bgt_draw_line(x2, y2, x3, y3);
            bgt_draw_line(x3, y3, x1, y1);
            return;
        }

        /// 共线检查：三点共线时偏移线全部平行，无法形成 miter。
        const Point a{x1, y1};
        const Point b{x2, y2};
        const Point c{x3, y3};
        if (edge_value(a, b, c) == 0) {
            bgt_draw_line(x1, y1, x2, y2);
            bgt_draw_line(x2, y2, x3, y3);
            bgt_draw_line(x3, y3, x1, y1);
            return;
        }

        /// 三条边的垂直方向（单位法向量）。
        const double n1x = -e1y / len1;
        const double n1y = e1x / len1;
        const double n2x = -e2y / len2;
        const double n2y = e2x / len2;
        const double n3x = -e3y / len3;
        const double n3y = e3x / len3;

        const double half = static_cast<double>(s.line_width) / 2.0;

        /// 每条边偏移 +half / -half 得到平行线，相邻边的同侧偏移线求交得到
        /// miter point。
        /// `pts[0..2]`：+half 侧（分别在顶点 P1, P2, P3 处）。
        /// `pts[3..5]`：-half 侧（分别在顶点 P1, P2, P3 处）。
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
        pts[0] = line_intersection(o3px, o3py, e3x, e3y, o1px, o1py, e1x, e1y);
        pts[1] = line_intersection(o1px, o1py, e1x, e1y, o2px, o2py, e2x, e2y);
        pts[2] = line_intersection(o2px, o2py, e2x, e2y, o3px, o3py, e3x, e3y);
        pts[3] = line_intersection(i3px, i3py, e3x, e3y, i1px, i1py, e1x, e1y);
        pts[4] = line_intersection(i1px, i1py, e1x, e1y, i2px, i2py, e2x, e2y);
        pts[5] = line_intersection(i2px, i2py, e2x, e2y, i3px, i3py, e3x, e3y);

        /// Miter limit：当 miter point 离顶点超过 2*line_width 时，回退到该
        /// 顶点本身（bevel join 的效果），防止极尖角产生巨大尖刺。
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

        /// 6 个顶点、6 个三角形（3 条 quad 各拆 2 个），一次
        /// `SDL_RenderGeometry` 绘制。
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
        /// quad1: 0,1,4,3 → 三角形 (0,1,4) + (0,4,3)
        /// quad2: 1,2,5,4 → 三角形 (1,2,5) + (1,5,4)
        /// quad3: 2,0,3,5 → 三角形 (2,0,3) + (2,3,5)
        const int indices[] = {
            0, 1, 4, 0, 4, 3, ///< quad1
            1, 2, 5, 1, 5, 4, ///< quad2
            2, 0, 3, 2, 3, 5, ///< quad3
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
    /// 非退化：三个顶点直接交给 GPU 光栅化填充。
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

int bgt_load_image(const char filename[])
{
    State &s = state();
    if (!ensure_open(s)) {
        return BGT_IMAGE_NONE;
    }
    if (filename == nullptr || filename[0] == '\0') {
        s.set_error(BGT_ERROR_IMAGE, "image filename is empty");
        return BGT_IMAGE_NONE;
    }

    /// SDL_image 按 UTF-8 解释路径，filename 是源编码；错误消息里仍用
    /// 源编码原文。
    const bgt::Utf8View utf8_filename(filename);
    SDL_Surface *surface = IMG_Load(utf8_filename.c_str());
    if (surface == nullptr) {
        s.set_error(BGT_ERROR_IMAGE,
                    std::string("failed to load image ") + filename);
        return BGT_IMAGE_NONE;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(s.renderer, surface);
    SDL_DestroySurface(surface);
    if (texture == nullptr) {
        s.set_error(BGT_ERROR_IMAGE, "failed to create image texture");
        return BGT_IMAGE_NONE;
    }

    /// 透明 PNG 与缩放采样质量：与 SSAA 画布的设置保持一致。
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);

    /// 新图片的变换状态为恒等（T 0、R 0、S 1）。
    ImageEntry entry;
    entry.texture = texture;
    s.images.push_back(entry);
    return static_cast<int>(s.images.size());
}

int bgt_image_width(int image_id)
{
    State &s = state();
    const ImageEntry *entry = get_image(s, image_id);
    if (entry == nullptr || entry->texture == nullptr) {
        return 0;
    }
    float width = 0.0F;
    float height = 0.0F;
    if (!SDL_GetTextureSize(entry->texture, &width, &height)) {
        return 0;
    }
    return static_cast<int>(std::lround(width));
}

int bgt_image_height(int image_id)
{
    State &s = state();
    const ImageEntry *entry = get_image(s, image_id);
    if (entry == nullptr || entry->texture == nullptr) {
        return 0;
    }
    float width = 0.0F;
    float height = 0.0F;
    if (!SDL_GetTextureSize(entry->texture, &width, &height)) {
        return 0;
    }
    return static_cast<int>(std::lround(height));
}

void bgt_translate_image(int image_id, int dx, int dy)
{
    State &s = state();
    ImageEntry *entry = get_image(s, image_id);
    if (entry == nullptr) {
        s.set_error(BGT_ERROR_IMAGE, "invalid image id");
        return;
    }
    entry->offset_x = dx;
    entry->offset_y = dy;
}

void bgt_rotate_image(int image_id, double angle)
{
    State &s = state();
    ImageEntry *entry = get_image(s, image_id);
    if (entry == nullptr) {
        s.set_error(BGT_ERROR_IMAGE, "invalid image id");
        return;
    }
    entry->rotation = angle;
}

void bgt_scale_image(int image_id, double sx, double sy)
{
    State &s = state();
    ImageEntry *entry = get_image(s, image_id);
    if (entry == nullptr) {
        s.set_error(BGT_ERROR_IMAGE, "invalid image id");
        return;
    }
    entry->scale_x = sx;
    entry->scale_y = sy;
}

void bgt_clear_image_transform(int image_id)
{
    State &s = state();
    ImageEntry *entry = get_image(s, image_id);
    if (entry == nullptr) {
        s.set_error(BGT_ERROR_IMAGE, "invalid image id");
        return;
    }
    /// 恒等变换：T(0,0)、R(0 度)、S(1.0, 1.0)。
    entry->offset_x = 0;
    entry->offset_y = 0;
    entry->rotation = 0.0;
    entry->scale_x = 1.0;
    entry->scale_y = 1.0;
}

void bgt_draw_image(int image_id, int x, int y)
{
    State &s = state();
    if (!ensure_open(s)) {
        return;
    }
    ImageEntry *entry = get_image(s, image_id);
    if (entry == nullptr || entry->texture == nullptr) {
        s.set_error(BGT_ERROR_IMAGE, "invalid image id");
        return;
    }

    float texture_width = 0.0F;
    float texture_height = 0.0F;
    if (!SDL_GetTextureSize(entry->texture, &texture_width, &texture_height)) {
        s.set_error(BGT_ERROR_IMAGE, "failed to query image size");
        return;
    }

    /// T：基准位置 (x, y) + 变换偏移 (dx, dy)。
    const float tx = static_cast<float>(x + entry->offset_x);
    const float ty = static_cast<float>(y + entry->offset_y);

    /// 快路径：恒等变换（无旋转、无缩放）时直接贴图。
    if (entry->rotation == 0.0 && entry->scale_x == 1.0 &&
        entry->scale_y == 1.0) {
        const SDL_FRect dst{tx, ty, texture_width, texture_height};
        SDL_RenderTexture(s.renderer, entry->texture, nullptr, &dst);
        return;
    }

    /// 严格 RST：每个顶点独立走 local → S → R → T，
    /// 不做任何锚点补偿或翻转位置修正。
    /// 刻意不用 `SDL_RenderTextureRotated` 的 flip 参数（它把翻转放在旋转
    /// 之后，顺序是 T × flip × R × S，违反 RST 的 T × R × S(负值) 合成顺序）。
    const double sx = entry->scale_x;
    const double sy = entry->scale_y;
    const double radians = entry->rotation * 3.14159265358979323846 / 180.0;
    const double cos_a = std::cos(radians);
    const double sin_a = std::sin(radians);

    const auto rst = [sx, sy, cos_a, sin_a, tx, ty](double lx, double ly) {
        const double vx = lx * sx; ///< S（缩放，负值翻转）
        const double vy = ly * sy;
        const double rx = vx * cos_a - vy * sin_a; ///< R（顺时针，屏幕坐标系）
        const double ry = vx * sin_a + vy * cos_a;
        return SDL_FPoint{static_cast<float>(rx + tx), ///< T（平移）
                          static_cast<float>(ry + ty)};
    };

    const double w = static_cast<double>(texture_width);
    const double h = static_cast<double>(texture_height);

    SDL_Vertex vertices[4] = {
        {rst(0, 0), SDL_FColor{1.0F, 1.0F, 1.0F, 1.0F}, SDL_FPoint{0.0F, 0.0F}},
        {rst(w, 0), SDL_FColor{1.0F, 1.0F, 1.0F, 1.0F}, SDL_FPoint{1.0F, 0.0F}},
        {rst(w, h), SDL_FColor{1.0F, 1.0F, 1.0F, 1.0F}, SDL_FPoint{1.0F, 1.0F}},
        {rst(0, h), SDL_FColor{1.0F, 1.0F, 1.0F, 1.0F}, SDL_FPoint{0.0F, 1.0F}},
    };
    const int indices[6] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(s.renderer, entry->texture, vertices, 4, indices, 6);
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
    const bgt::Utf8View utf8_text(text);
    draw_text_utf8_impl(s, x, y, utf8_text.c_str(), s.font_size);
}

void bgt_draw_text(int x, int y, const char text[], int size)
{
    const bgt::Utf8View utf8_text(text);
    draw_text_utf8_impl(state(), x, y, utf8_text.c_str(), size);
}

int bgt_text_width(const char text[])
{
    State &s = state();
    const bgt::Utf8View utf8_text(text);
    return text_size_utf8_impl(s, utf8_text.c_str(), s.font_size, true);
}

int bgt_text_width(const char text[], int size)
{
    const bgt::Utf8View utf8_text(text);
    return text_size_utf8_impl(state(), utf8_text.c_str(), size, true);
}

int bgt_text_height(const char text[])
{
    State &s = state();
    const bgt::Utf8View utf8_text(text);
    return text_size_utf8_impl(s, utf8_text.c_str(), s.font_size, false);
}

int bgt_text_height(const char text[], int size)
{
    const bgt::Utf8View utf8_text(text);
    return text_size_utf8_impl(state(), utf8_text.c_str(), size, false);
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

void bgt_random_seed(unsigned seed)
{
    random_engine().seed(seed);
}

int bgt_random(int min, int max)
{
    if (min > max) {
        std::swap(min, max);
    }
    if (min >= max) {
        return min;
    }
    return std::uniform_int_distribution<int>(min, max - 1)(random_engine());
}

double bgt_random(double min, double max)
{
    if (min > max) {
        std::swap(min, max);
    }
    if (min >= max) {
        return min;
    }
    return std::uniform_real_distribution<double>(min, max)(random_engine());
}

bool bgt_hit_point_rect(int px, int py, int x, int y, int width, int height)
{
    /// 全部升为 long long：x + width 在坐标接近 INT_MAX 时不会溢出。
    const long long right = static_cast<long long>(x) + width;
    const long long bottom = static_cast<long long>(y) + height;
    return px >= x && px < right && py >= y && py < bottom;
}

bool bgt_hit_point_circle(int px, int py, int x, int y, int radius)
{
    /// 负半径平方后会变正，必须显式守卫。
    if (radius <= 0) {
        return false;
    }
    const long long dx = static_cast<long long>(px) - x;
    const long long dy = static_cast<long long>(py) - y;
    const long long r = radius;
    return dx * dx + dy * dy <= r * r;
}

bool bgt_hit_rect_rect(int x1, int y1, int width1, int height1, int x2, int y2,
                       int width2, int height2)
{
    /// 显式守卫：空矩形被另一矩形"跨立"时严格不等式仍会成立，
    /// 必须先排除（与圆形函数的半径守卫同理）。
    if (width1 <= 0 || height1 <= 0 || width2 <= 0 || height2 <= 0) {
        return false;
    }
    /// 严格不等式：恰好共享边/角时不成立，贴边自然不算命中。
    const long long a_right = static_cast<long long>(x1) + width1;
    const long long a_bottom = static_cast<long long>(y1) + height1;
    const long long b_right = static_cast<long long>(x2) + width2;
    const long long b_bottom = static_cast<long long>(y2) + height2;
    return x1 < b_right && x2 < a_right && y1 < b_bottom && y2 < a_bottom;
}

bool bgt_hit_circle_circle(int x1, int y1, int radius1, int x2, int y2,
                           int radius2)
{
    /// 逐个守卫，不能用 radius1 + radius2 <= 0（5 与 -3 的和为 2，会漏过）。
    if (radius1 <= 0 || radius2 <= 0) {
        return false;
    }
    const long long dx = static_cast<long long>(x1) - x2;
    const long long dy = static_cast<long long>(y1) - y2;
    const long long sum = static_cast<long long>(radius1) + radius2;
    return dx * dx + dy * dy < sum * sum;
}

bool bgt_hit_circle_rect(int cx, int cy, int radius, int x, int y, int width,
                         int height)
{
    if (radius <= 0 || width <= 0 || height <= 0) {
        return false;
    }
    /// 把圆心"夹"进矩形，得到矩形上离圆心最近的点，再比较距离与半径。
    const long long right = static_cast<long long>(x) + width;
    const long long bottom = static_cast<long long>(y) + height;
    long long nearest_x = cx;
    long long nearest_y = cy;
    if (nearest_x < x) {
        nearest_x = x;
    }
    if (nearest_x > right) {
        nearest_x = right;
    }
    if (nearest_y < y) {
        nearest_y = y;
    }
    if (nearest_y > bottom) {
        nearest_y = bottom;
    }
    const long long dx = cx - nearest_x;
    const long long dy = cy - nearest_y;
    const long long r = radius;
    return dx * dx + dy * dy < r * r;
}

/// ---------------------------------------------------------------------
/// 文件存档（v0.3）：内存中的节-键-值表 + 文本存档文件。
/// “文本即真值”：内存表与存档文件都存值的文本形式，set 把值转成文本，
/// get 按类型解析文本，一条代码路径保证两边永远一致。
/// ---------------------------------------------------------------------
namespace {

/**
 * @brief 去掉字符串首尾的空白字符。
 *
 * 字符串值、节名、键名都用这个规则整理，这样学生手写 `key = 100` 也能
 * 正确读入。
 *
 * @param text 原始文本。
 * @return 去掉首尾空白后的副本。
 */
std::string trim_copy(const std::string &text)
{
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

/**
 * @brief 校验节名：非空，且不含 `[`、`]` 和换行。
 *
 * 否则写进文件后会解析不回来。必须按字符边界搜索：GBK 双字节字符的尾
 * 字节可以是 `[` 或 `]`，直接 `find()` 会误判。
 *
 * @param name 已去掉首尾空白的节名。
 * @return 合法返回 true，否则返回 false。
 */
bool valid_section_name(const std::string &name)
{
    return !name.empty() && !bgt::encoded_contains_ascii(name, '[') &&
           !bgt::encoded_contains_ascii(name, ']') &&
           !bgt::encoded_contains_ascii(name, '\n');
}

/**
 * @brief 校验键名：非空，不含 `=` 和换行，且首字符不能是 `#` 或 `[`。
 *
 * 否则写盘后会被读回逻辑误判成注释行或节头，存档读不回来。同样按字符
 * 边界搜索；front() 可以直接比较，因为双字节字符的首字节 >= 0x81，
 * 不可能是 ASCII。
 *
 * @param name 已去掉首尾空白的键名。
 * @return 合法返回 true，否则返回 false。
 */
bool valid_key_name(const std::string &name)
{
    return !name.empty() && !bgt::encoded_contains_ascii(name, '=') &&
           !bgt::encoded_contains_ascii(name, '\n') && name.front() != '#' &&
           name.front() != '[';
}

/**
 * @brief 按 '\n' 切分文本。
 *
 * 行内不含行尾的 '\n'（与 `std::getline` 的行划分一致）。
 *
 * @param text 原始文本。
 * @return 每一行的列表。
 */
std::vector<std::string> split_lines(const std::string &text)
{
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start < text.size()) {
        const std::size_t end = text.find('\n', start);
        if (end == std::string::npos) {
            lines.push_back(text.substr(start));
            break;
        }
        lines.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    return lines;
}

/**
 * @brief 把存档文件内容统一成源编码文本。
 *
 * 存档文件的内容可能是记事本写出的 UTF-8（带 BOM）：先按 UTF-8 解释并
 * 转回源编码，之后的整条解析路径就只面对一种编码。没有 BOM 时按源编码
 * 解释——libbgt 自己写出的存档就是源编码，学生用记事本按 ANSI 保存的也
 * 是源编码。
 *
 * @param content 读入的原始文件内容。
 * @return 源编码的文本。
 */
std::string storage_file_text(const std::string &content)
{
    if (content.rfind("\xEF\xBB\xBF", 0) == 0) {
        return bgt::from_utf8(content.substr(3));
    }
    return content;
}

/**
 * @brief 整理并校验节名与键名。
 *
 * 不合法时记录错误并返回 false。
 *
 * @param section 节名（可为 nullptr）。
 * @param key 键名（可为 nullptr）。
 * @param[out] section_out 整理后的节名。
 * @param[out] key_out 整理后的键名。
 * @return 校验通过返回 true；否则记录错误并返回 false。
 */
bool storage_names(const char section[], const char key[],
                   std::string &section_out, std::string &key_out)
{
    State &s = state();
    section_out = section == nullptr ? std::string() : trim_copy(section);
    key_out = key == nullptr ? std::string() : trim_copy(key);
    if (!valid_section_name(section_out)) {
        s.set_error(BGT_ERROR_STORAGE,
                    "invalid section name: '" + section_out + "'");
        return false;
    }
    if (!valid_key_name(key_out)) {
        s.set_error(BGT_ERROR_STORAGE, "invalid key name: '" + key_out + "'");
        return false;
    }
    return true;
}

/**
 * @brief 在内存存档表中查找 (节, 键) 对应的值文本。
 *
 * @param section 节名。
 * @param key 键名。
 * @return 值文本的指针；找不到返回 nullptr。
 */
const std::string *find_storage_value(const std::string &section,
                                      const std::string &key)
{
    State &s = state();
    const auto section_it = s.storage.find(section);
    if (section_it == s.storage.end()) {
        return nullptr;
    }
    const auto key_it = section_it->second.find(key);
    if (key_it == section_it->second.end()) {
        return nullptr;
    }
    return &key_it->second;
}

/**
 * @brief 把整数转成存档用的文本。
 *
 * @param value 整数值。
 * @return 十进制文本。
 */
std::string int_to_text(int value)
{
    return std::to_string(value);
}

/**
 * @brief 把小数转成存档用的文本。
 *
 * 小数用最短往返表示：45.5 存 45.5，读回来还是精确的 45.5。
 *
 * @param value 小数值。
 * @return 文本形式。
 */
std::string double_to_text(double value)
{
    char buffer[64];
    const std::to_chars_result result =
        std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::string(buffer, result.ptr);
}

/**
 * @brief 全串严格解析整数。
 *
 * 文本必须整体是整数，且落在 int 范围内。
 *
 * @param text 待解析文本。
 * @param[out] value_out 解析结果。
 * @return 解析成功返回 true，否则返回 false。
 */
bool parse_int_text(const std::string &text, int &value_out)
{
    int value = 0;
    const char *begin = text.data();
    const char *end = begin + text.size();
    const std::from_chars_result result = std::from_chars(begin, end, value);
    if (result.ec != std::errc() || result.ptr != end) {
        return false;
    }
    value_out = value;
    return true;
}

/**
 * @brief 全串严格解析小数。
 *
 * "120" 也能读成 120.0（整数可以当小数用）。
 *
 * @param text 待解析文本。
 * @param[out] value_out 解析结果。
 * @return 解析成功返回 true，否则返回 false。
 */
bool parse_double_text(const std::string &text, double &value_out)
{
    double value = 0.0;
    const char *begin = text.data();
    const char *end = begin + text.size();
    const std::from_chars_result result = std::from_chars(begin, end, value);
    if (result.ec != std::errc() || result.ptr != end) {
        return false;
    }
    value_out = value;
    return true;
}

} // namespace

/// ---------------------------------------------------------------------
/// 声音播放（v0.3）：SDL_mixer 封装。音效与背景音乐两类：
/// 音效预加载成 ID（内存驻留、可重叠并发），音乐全局单实例、流式、
/// 默认无限循环。第一次用声音时才打开音频设备（懒初始化）。
/// ---------------------------------------------------------------------
namespace {

/**
 * @brief 同一时刻最多 32 个音效重叠（raylib 同款上限，远超教学场景需要）。
 */
constexpr int kMaxSoundTracks = 32;

/**
 * @brief 把音量收敛到 0-100。
 *
 * 与 `bgt_rgb()` 的 0-255 收敛同风格。
 *
 * @param volume 原始音量值。
 * @return 0-100 之间的音量。
 */
int clamp_volume(int volume)
{
    return std::clamp(volume, 0, 100);
}

/**
 * @brief 懒初始化音频设备：第一次调用任何声音函数时打开。
 *
 * 失败（比如没有声卡）时记录错误；程序不崩，之后的声音调用都做安全
 * 空操作。
 *
 * @return 音频设备可用返回 true，否则返回 false。
 */
bool ensure_audio()
{
    State &s = state();
    if (s.audio_mixer != nullptr) {
        return true;
    }
    if (!MIX_Init()) {
        s.set_error(BGT_ERROR_AUDIO, "failed to init SDL_mixer");
        return false;
    }
    s.audio_mixer =
        MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (s.audio_mixer == nullptr) {
        s.set_error(BGT_ERROR_AUDIO, "failed to open audio device");
        MIX_Quit();
        return false;
    }
    /// 音效轨道池：一次性建好、反复复用（官方推荐的 track 用法）。
    for (int i = 0; i < kMaxSoundTracks; ++i) {
        MIX_Track *track = MIX_CreateTrack(s.audio_mixer);
        if (track == nullptr) {
            s.set_error(BGT_ERROR_AUDIO, "failed to create sound track");
            for (MIX_Track *created : s.sound_tracks) {
                MIX_DestroyTrack(created);
            }
            s.sound_tracks.clear();
            MIX_DestroyMixer(s.audio_mixer);
            s.audio_mixer = nullptr;
            MIX_Quit();
            return false;
        }
        s.sound_tracks.push_back(track);
    }
    return true;
}

} // namespace

void bgt_set_int(const char section[], const char key[], int value)
{
    std::string section_name;
    std::string key_name;
    if (!storage_names(section, key, section_name, key_name)) {
        return;
    }
    state().storage[section_name][key_name] = int_to_text(value);
}

void bgt_set_double(const char section[], const char key[], double value)
{
    std::string section_name;
    std::string key_name;
    if (!storage_names(section, key, section_name, key_name)) {
        return;
    }
    state().storage[section_name][key_name] = double_to_text(value);
}

void bgt_set_string(const char section[], const char key[], const char value[])
{
    std::string section_name;
    std::string key_name;
    if (!storage_names(section, key, section_name, key_name)) {
        return;
    }
    const std::string text =
        value == nullptr ? std::string() : trim_copy(value);
    if (text.find('\n') != std::string::npos) {
        state().set_error(BGT_ERROR_STORAGE,
                          "string value must not contain a newline");
        return;
    }
    state().storage[section_name][key_name] = text;
}

int bgt_get_int(const char section[], const char key[], int default_value)
{
    std::string section_name;
    std::string key_name;
    if (!storage_names(section, key, section_name, key_name)) {
        return default_value;
    }
    State &s = state();
    const std::string *text = find_storage_value(section_name, key_name);
    if (text == nullptr) {
        return default_value;
    }
    int value = 0;
    if (!parse_int_text(*text, value)) {
        s.set_error(BGT_ERROR_STORAGE, "storage entry '" + section_name + "." +
                                           key_name + "' is not an integer: '" +
                                           *text + "'");
        return default_value;
    }
    return value;
}

double bgt_get_double(const char section[], const char key[],
                      double default_value)
{
    std::string section_name;
    std::string key_name;
    if (!storage_names(section, key, section_name, key_name)) {
        return default_value;
    }
    State &s = state();
    const std::string *text = find_storage_value(section_name, key_name);
    if (text == nullptr) {
        return default_value;
    }
    double value = 0.0;
    if (!parse_double_text(*text, value)) {
        s.set_error(BGT_ERROR_STORAGE, "storage entry '" + section_name + "." +
                                           key_name + "' is not a number: '" +
                                           *text + "'");
        return default_value;
    }
    return value;
}

void bgt_get_string(const char section[], const char key[], char out[],
                    int out_size, const char default_value[])
{
    State &s = state();
    if (out == nullptr || out_size <= 0) {
        s.set_error(BGT_ERROR_STORAGE,
                    "string output array is missing or empty");
        return;
    }
    std::string section_name;
    std::string key_name;
    bool found = false;
    std::string text;
    if (storage_names(section, key, section_name, key_name)) {
        const std::string *stored = find_storage_value(section_name, key_name);
        if (stored != nullptr) {
            text = *stored;
            found = true;
        }
    }
    if (!found) {
        text = default_value == nullptr ? std::string() : default_value;
    }
    const auto limit = static_cast<std::size_t>(out_size - 1);
    if (text.size() <= limit) {
        std::memcpy(out, text.c_str(), text.size() + 1);
        return;
    }
    const std::size_t prefix = bgt::encoded_prefix_length(text, limit);
    std::memcpy(out, text.c_str(), prefix);
    out[prefix] = '\0';
    s.set_error(BGT_ERROR_STORAGE,
                "string value is too long for the output array");
}

bool bgt_load(const char filename[])
{
    State &s = state();
    if (filename == nullptr || filename[0] == '\0') {
        s.set_error(BGT_ERROR_STORAGE, "storage filename is empty");
        return false;
    }
    /// 窄路径在 Windows 上按 ANSI 解释，先转成本机文件系统路径（UTF-16）。
    const std::filesystem::path native_path = bgt::to_native_path(filename);
    std::ifstream file(native_path, std::ios::in | std::ios::binary);
    if (!file) {
        std::error_code probe_error;
        if (std::filesystem::exists(native_path, probe_error)) {
            s.set_error(BGT_ERROR_STORAGE,
                        "failed to open storage file " + std::string(filename));
            return false;
        }
        /// 文件不存在是正常情况（比如游戏第一次运行）：空表、无错误。
        s.storage.clear();
        return true;
    }
    const std::string content((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    if (file.bad()) {
        s.set_error(BGT_ERROR_STORAGE, "failed while reading storage file " +
                                           std::string(filename));
        return false;
    }
    const std::string text = storage_file_text(content);
    s.storage.clear();
    std::string section;
    bool in_section = false; ///< 节头之前（或坏节头之后）的键值行判为坏行。
    bool all_lines_ok = true;
    for (const std::string &raw_line : split_lines(text)) {
        std::string line = raw_line;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::string trimmed = trim_copy(line);
        if (trimmed.empty() || trimmed.front() == '#') {
            continue; ///< 空行与 # 整行注释。
        }
        if (trimmed.front() == '[') {
            std::string name;
            bool ok = false;
            if (bgt::encoded_ends_with_ascii(trimmed, ']')) {
                name = trim_copy(trimmed.substr(1, trimmed.size() - 2));
                ok = valid_section_name(name);
            }
            if (!ok) {
                s.set_error(BGT_ERROR_STORAGE,
                            "bad line in storage file: '" + trimmed + "'");
                all_lines_ok = false;
                in_section = false;
                continue;
            }
            section = name;
            in_section = true;
            s.storage[section]; ///< 空节也登记，往返时保留。
            continue;
        }
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos || !in_section) {
            s.set_error(BGT_ERROR_STORAGE,
                        "bad line in storage file: '" + trimmed + "'");
            all_lines_ok = false;
            continue;
        }
        const std::string key = trim_copy(line.substr(0, eq));
        if (!valid_key_name(key)) {
            s.set_error(BGT_ERROR_STORAGE,
                        "bad line in storage file: '" + trimmed + "'");
            all_lines_ok = false;
            continue;
        }
        const std::string value = trim_copy(line.substr(eq + 1));
        s.storage[section][key] = value;
    }
    return all_lines_ok;
}

bool bgt_save(const char filename[])
{
    State &s = state();
    if (filename == nullptr || filename[0] == '\0') {
        s.set_error(BGT_ERROR_STORAGE, "storage filename is empty");
        return false;
    }
    /// 先写临时文件，成功后再整体替换，避免写一半把旧存档弄坏。
    /// 存档内容按源编码写出（GBK 模式下就是 GBK），与学生的源码编码保持一致。
    const std::string temp_name = std::string(filename) + ".tmp";
    {
        std::ofstream file(bgt::to_native_path(temp_name),
                           std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file) {
            s.set_error(BGT_ERROR_STORAGE,
                        "failed to create storage file " + temp_name);
            return false;
        }
        for (const auto &section : s.storage) {
            file << '[' << section.first << "]\n";
            for (const auto &entry : section.second) {
                file << entry.first << '=' << entry.second << '\n';
            }
            file << '\n';
        }
        file.flush();
        /// 显式 close 并检查：缓冲数据在 close 时才真正落盘，漏看会静默丢档。
        file.close();
        if (!file) {
            s.set_error(BGT_ERROR_STORAGE,
                        "failed while writing storage file " + temp_name);
            /// 写坏的临时文件是残渣：删掉，不留 “*.tmp” 尾巴。
            std::error_code cleanup_error;
            std::filesystem::remove(bgt::to_native_path(temp_name),
                                    cleanup_error);
            return false;
        }
    }
    std::error_code rename_error;
    /// 目标存在时先移除；两步都用本机路径（Windows 下是 UTF-16）。
    std::filesystem::remove(bgt::to_native_path(filename), rename_error);
    rename_error.clear();
    std::filesystem::rename(bgt::to_native_path(temp_name),
                            bgt::to_native_path(filename), rename_error);
    if (rename_error) {
        s.set_error(BGT_ERROR_STORAGE, "failed to replace storage file " +
                                           std::string(filename) + ": " +
                                           rename_error.message());
        return false;
    }
    return true;
}

int bgt_load_sound(const char filename[])
{
    State &s = state();
    if (filename == nullptr || filename[0] == '\0') {
        s.set_error(BGT_ERROR_AUDIO, "sound filename is empty");
        return 0;
    }
    if (!ensure_audio()) {
        return 0;
    }
    /// SDL_mixer 按 UTF-8 解释路径，filename 是源编码；错误消息里仍用
    /// 源编码原文。
    const bgt::Utf8View utf8_filename(filename);
    MIX_Audio *audio =
        MIX_LoadAudio(s.audio_mixer, utf8_filename.c_str(), false);
    if (audio == nullptr) {
        s.set_error(BGT_ERROR_AUDIO,
                    "failed to load sound " + std::string(filename));
        return 0;
    }
    const int id = s.next_sound_id;
    s.next_sound_id = s.next_sound_id + 1;
    s.sounds[id].audio = audio;
    s.sounds[id].volume = 100;
    return id;
}

void bgt_play_sound(int id)
{
    State &s = state();
    const auto entry_it = s.sounds.find(id);
    if (entry_it == s.sounds.end()) {
        s.set_error(BGT_ERROR_AUDIO, "invalid sound id");
        return;
    }
    if (!ensure_audio()) {
        return;
    }
    /// 在轨道池里找一条没在响的；全忙时复用池里最靠前的一条（重新开始）。
    MIX_Track *track = nullptr;
    for (MIX_Track *candidate : s.sound_tracks) {
        if (!MIX_TrackPlaying(candidate)) {
            track = candidate;
            break;
        }
    }
    if (track == nullptr) {
        track = s.sound_tracks.front();
    }
    if (!MIX_SetTrackAudio(track, entry_it->second.audio) ||
        !MIX_SetTrackGain(track, static_cast<float>(entry_it->second.volume) /
                                     100.0f) ||
        !MIX_PlayTrack(track, 0)) {
        s.set_error(BGT_ERROR_AUDIO, "failed to play sound");
    }
}

void bgt_set_sound_volume(int id, int volume)
{
    State &s = state();
    const auto entry_it = s.sounds.find(id);
    if (entry_it == s.sounds.end()) {
        s.set_error(BGT_ERROR_AUDIO, "invalid sound id");
        return;
    }
    entry_it->second.volume = clamp_volume(volume);
}

bool bgt_play_music(const char filename[])
{
    State &s = state();
    if (filename == nullptr || filename[0] == '\0') {
        s.set_error(BGT_ERROR_AUDIO, "music filename is empty");
        return false;
    }
    if (!ensure_audio()) {
        return false;
    }
    if (s.music_track == nullptr) {
        s.music_track = MIX_CreateTrack(s.audio_mixer);
        if (s.music_track == nullptr) {
            s.set_error(BGT_ERROR_AUDIO, "failed to create music track");
            return false;
        }
    }
    /// 流式：不把整个文件读进内存，边读边解码（内存占用与时长无关）。
    /// closeio=true：换曲或销毁轨道时由 mixer 自动关闭旧文件。
    /// SDL 按 UTF-8 解释路径，filename 是源编码。
    const bgt::Utf8View utf8_filename(filename);
    SDL_IOStream *io = SDL_IOFromFile(utf8_filename.c_str(), "rb");
    if (io == nullptr) {
        s.set_error(BGT_ERROR_AUDIO,
                    "failed to open music " + std::string(filename));
        return false;
    }
    if (!MIX_SetTrackIOStream(s.music_track, io, true)) {
        s.set_error(BGT_ERROR_AUDIO,
                    "failed to start music " + std::string(filename));
        return false;
    }
    MIX_SetTrackGain(s.music_track,
                     static_cast<float>(s.music_volume) / 100.0f);
    /// 无限循环（-1 的语义已在 Step 4 对照头文件注释确认）。
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
    const bool ok = MIX_PlayTrack(s.music_track, props);
    SDL_DestroyProperties(props);
    if (!ok) {
        s.set_error(BGT_ERROR_AUDIO,
                    "failed to play music " + std::string(filename));
        return false;
    }
    return true;
}

bool bgt_file_exists(const char filename[])
{
    if (filename == nullptr || filename[0] == '\0') {
        return false;
    }
    std::error_code error;
    return std::filesystem::exists(bgt::to_native_path(filename), error);
}

void bgt_stop_music()
{
    State &s = state();
    if (s.music_track == nullptr) {
        return; ///< 从没播过音乐：什么都不做。
    }
    MIX_StopTrack(s.music_track, 0);
}

void bgt_set_music_volume(int volume)
{
    State &s = state();
    s.music_volume = clamp_volume(volume);
    if (s.music_track != nullptr) {
        MIX_SetTrackGain(s.music_track,
                         static_cast<float>(s.music_volume) / 100.0f);
    }
}

namespace {

/**
 * @brief 把文本按源编码的字符边界截断后复制进输出数组。
 *
 * 把 text 的前 out_size - 1 个字节按源编码的字符边界截断后复制进 out，
 * 保证不会切在多字节字符中间，并补上结束符。查询辅助，绝不记错误。
 * 命名避开平行分支的同名助手（合体时统一）。
 *
 * @param text 源文本。
 * @param[out] out 输出数组。
 * @param out_size 输出数组的大小（字节）。
 */
void prefix_copy(const std::string &text, char out[], int out_size)
{
    if (out == nullptr || out_size <= 0) {
        return;
    }
    int limit = out_size - 1;
    if (static_cast<int>(text.size()) < limit) {
        limit = static_cast<int>(text.size());
    }
    const std::size_t length =
        bgt::encoded_prefix_length(text, static_cast<std::size_t>(limit));
    for (std::size_t i = 0; i < length; i = i + 1) {
        out[i] = text[i];
    }
    out[length] = '\0';
}

/**
 * @brief 把一条错误消息按最大宽度逐行绘制。
 *
 * 先用 `TTF_MeasureString` 量出本行能放下的字节数，再回退到本行范围内的
 * 最后一个空格（优先在空格断行），行内没有空格才在量出的边界硬断。测量与
 * 绘制都基于 UTF-8，所以先把源编码的消息整体转成 UTF-8，再按 UTF-8 字节
 * 偏移切行，多字节字符不会被切开。
 *
 * 消息按值传入：循环内的绘制失败会记新错误，环形队列的 push/pop 可能使按
 * 引用传入的历史条目失效（index 0 且历史满时的悬空引用）。
 *
 * @param s 全局状态。
 * @param x 文字左上角 x 坐标（逻辑坐标）。
 * @param y 文字左上角 y 坐标（逻辑坐标）。
 * @param size 字号（像素）。
 * @param message 错误消息（源编码）；按值传入的副本。
 */
void draw_wrapped_error(State &s, int x, int y, int size, std::string message)
{
    /// message 已经是本次调用的副本，直接原地换成 UTF-8。
    message = bgt::to_utf8(message);
    const int max_width = s.width - x - 16;
    if (max_width <= 0) {
        draw_text_utf8_impl(s, x, y, message.c_str(), size);
        return;
    }
    TTF_Font *font = get_font(s, size);
    if (font == nullptr) {
        /// 字体不可用：`get_font` 已经记录过错误，这里直接放弃绘制，
        /// 避免再走一遍绘制路径把同一条错误记第二次。
        return;
    }
    const int line_height = size + size / 3;
    std::size_t start = 0;
    int line_y = y;
    while (start < message.size()) {
        const std::string rest = message.substr(start);
        int measured_width = 0;
        std::size_t measured_length = 0;
        if (!TTF_MeasureString(font, rest.c_str(), rest.size(), max_width,
                               &measured_width, &measured_length) ||
            measured_length == 0) {
            /// 测量失败或一个字符都放不下：剩余部分当一行画，避免死循环。
            draw_text_utf8_impl(s, x, line_y, rest.c_str(), size);
            return;
        }
        std::size_t break_at = start + measured_length;
        if (break_at < message.size()) {
            /// 本行放不下整条消息：优先在行内最后一个空格处断行。
            const std::size_t space = message.find_last_of(' ', break_at - 1);
            if (space != std::string::npos && space > start) {
                break_at = space;
            }
        }
        const std::string line = message.substr(start, break_at - start);
        draw_text_utf8_impl(s, x, line_y, line.c_str(), size);
        line_y = line_y + line_height;
        start = break_at;
        while (start < message.size() && message[start] == ' ') {
            start = start + 1; ///< 断行点后的空格不进下一行。
        }
    }
}

} // namespace

bool bgt_has_error()
{
    return !state().errors.empty();
}

int bgt_error_count()
{
    return static_cast<int>(state().errors.size());
}

int bgt_error_code(int index)
{
    const State &s = state();
    if (index < 0 || index >= static_cast<int>(s.errors.size())) {
        return BGT_ERROR_NONE;
    }
    return s.errors[static_cast<std::size_t>(index)].code;
}

int bgt_error_code()
{
    return bgt_error_code(bgt_error_count() - 1);
}

void bgt_error_text(int index, char out[], int out_size)
{
    if (out == nullptr || out_size <= 0) {
        return;
    }
    const State &s = state();
    if (index < 0 || index >= static_cast<int>(s.errors.size())) {
        out[0] = '\0';
        return;
    }
    prefix_copy(s.errors[static_cast<std::size_t>(index)].message, out,
                out_size);
}

void bgt_print_error(int index)
{
    const State &s = state();
    if (index < 0 || index >= static_cast<int>(s.errors.size())) {
        return;
    }
    std::fprintf(stderr, "libbgt error %d: %s\n",
                 s.errors[static_cast<std::size_t>(index)].code,
                 s.errors[static_cast<std::size_t>(index)].message.c_str());
}

void bgt_print_error()
{
    bgt_print_error(bgt_error_count() - 1);
}

void bgt_draw_error(int x, int y, int size, int index)
{
    State &s = state();
    if (index < 0 || index >= static_cast<int>(s.errors.size())) {
        return;
    }
    draw_wrapped_error(s, x, y, size,
                       s.errors[static_cast<std::size_t>(index)].message);
}

void bgt_draw_error(int x, int y, int size)
{
    bgt_draw_error(x, y, size, bgt_error_count() - 1);
}

void bgt_clear_error()
{
    state().clear_error();
}

// NOLINTEND(readability-magic-numbers, modernize-avoid-c-arrays,
// bugprone-easily-swappable-parameters, readability-identifier-length)
