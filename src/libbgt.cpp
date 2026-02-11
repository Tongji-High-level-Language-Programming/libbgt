#include <iostream>
#include <utility> // for std::move
#include <string>
#include <algorithm> // for std::ranges::all_of

#include <libbgt.h>
#include <internal/font_utils.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3_ttf/SDL_ttf.h>
#ifdef USE_ANSI
#include <windows.h> // For Windows code page conversion
#endif

/*
* 静态全局变量与工具函数
* 使用匿名命名空间简化，不必每个都加 static 关键词
* https://en.cppreference.com/w/cpp/language/namespace.html#Unnamed_namespaces
*/
namespace {
	
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* render_target = nullptr;
	TTF_Font* font = nullptr;
	TTF_TextEngine* text_engine = nullptr;
#ifdef USE_ANSI
	std::string localized_error_msg;

	// 将 ANSI 编码转换为 UTF-8 编码
	std::string ansi_to_utf8(std::string_view str) {
		std::string result;
		// 首先使用 UTF-16, 即 Windows API 中的 WideChar 进行中转
		// len_required 为缓冲区所需要的字符数, 注意, 由于输入为 string_view, 传入的长度不包含 '\0', 故 len_required 也未计算 '\0'
		int len_required = MultiByteToWideChar(CP_ACP, 0, str.data(), static_cast<int>(str.length()), nullptr, 0);
		static std::wstring wide_buf;
		wide_buf.resize_and_overwrite(len_required, [&](wchar_t* wide_buf,
			size_t) -> size_t {
				MultiByteToWideChar(CP_ACP, 0, str.data(), static_cast<int>(str.length()), wide_buf, len_required);
				return len_required;
			});

		// 再转换回 UTF-8
		// len_required 为缓冲区所需要的字符数
		// 注意, 我们希望返回一个以 '\0' 结尾的字符串, 因此向 WideCharToMultiByte 传入的长度为 -1, 它会一直处理到 '\0' 为止
		len_required = WideCharToMultiByte(CP_UTF8, 0, wide_buf.data(),
			-1, nullptr, 0, nullptr, nullptr);
		// 此时我们实际有效字符数为 len_reqired - 1
		result.resize_and_overwrite(len_required - 1, [&](char* buf,
			size_t) -> size_t {
				WideCharToMultiByte(CP_UTF8, 0, wide_buf.data(), -1,
					buf, len_required, nullptr, nullptr);
				return len_required - 1;
			});
		return result;
	}

	std::string utf8_to_ansi(std::string_view str) {
		const char* error_msg = SDL_GetError();
		int len_required = MultiByteToWideChar(CP_UTF8, 0, error_msg, -1,
			nullptr, 0);
		static std::wstring wide_buf;
		wide_buf.resize_and_overwrite(len_required, [&](wchar_t* wide_buf,
			size_t) -> size_t {
				MultiByteToWideChar(CP_UTF8, 0, error_msg, -1,
					wide_buf, len_required);
				return len_required;
			});
		std::string result;
		len_required = WideCharToMultiByte(CP_ACP, 0, wide_buf.c_str(), -1, nullptr, 0, nullptr, nullptr);
		result.resize_and_overwrite(len_required - 1, [&](char* buf,
			size_t) -> size_t {
				WideCharToMultiByte(CP_ACP, 0, wide_buf.c_str(), -1, buf, len_required, nullptr, nullptr);
				return len_required - 1;
			});
		return result;
	}
#endif

	// 对象创建时储存 DrawColor，生命期结束时恢复。用于避免 SetRenderDrawColor 的影响外溢
	struct RenderDrawColorGuard {
		RenderDrawColorGuard() {
			SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
		}
		~RenderDrawColorGuard() {
			SDL_SetRenderDrawColor(renderer, r, g, b, a);
		}
		Uint8 r, g, b, a;
	};


	/*
	* @brief 将小键盘键码转换为对应的常规键码
	*
	* @details Port from SDL_keyboard.c
	*/
	SDL_Keycode SDL_ConvertNumpadKeycode(SDL_Keycode keycode, bool numlock)
	{
		switch (keycode) {
		case SDLK_KP_DIVIDE:
			return SDLK_SLASH;
		case SDLK_KP_MULTIPLY:
			return SDLK_ASTERISK;
		case SDLK_KP_MINUS:
			return SDLK_MINUS;
		case SDLK_KP_PLUS:
			return SDLK_PLUS;
		case SDLK_KP_ENTER:
			return SDLK_RETURN;
		case SDLK_KP_1:
			return numlock ? SDLK_1 : SDLK_END;
		case SDLK_KP_2:
			return numlock ? SDLK_2 : SDLK_DOWN;
		case SDLK_KP_3:
			return numlock ? SDLK_3 : SDLK_PAGEDOWN;
		case SDLK_KP_4:
			return numlock ? SDLK_4 : SDLK_LEFT;
		case SDLK_KP_5:
			return numlock ? SDLK_5 : SDLK_CLEAR;
		case SDLK_KP_6:
			return numlock ? SDLK_6 : SDLK_RIGHT;
		case SDLK_KP_7:
			return numlock ? SDLK_7 : SDLK_HOME;
		case SDLK_KP_8:
			return numlock ? SDLK_8 : SDLK_UP;
		case SDLK_KP_9:
			return numlock ? SDLK_9 : SDLK_PAGEUP;
		case SDLK_KP_0:
			return numlock ? SDLK_0 : SDLK_INSERT;
		case SDLK_KP_PERIOD:
			return numlock ? SDLK_PERIOD : SDLK_DELETE;
		case SDLK_KP_EQUALS:
			return SDLK_EQUALS;
		case SDLK_KP_COMMA:
			return SDLK_COMMA;
		case SDLK_KP_EQUALSAS400:
			return SDLK_EQUALS;
		case SDLK_KP_LEFTPAREN:
			return SDLK_LEFTPAREN;
		case SDLK_KP_RIGHTPAREN:
			return SDLK_RIGHTPAREN;
		case SDLK_KP_LEFTBRACE:
			return SDLK_LEFTBRACE;
		case SDLK_KP_RIGHTBRACE:
			return SDLK_RIGHTBRACE;
		case SDLK_KP_TAB:
			return SDLK_TAB;
		case SDLK_KP_BACKSPACE:
			return SDLK_BACKSPACE;
		case SDLK_KP_A:
			return SDLK_A;
		case SDLK_KP_B:
			return SDLK_B;
		case SDLK_KP_C:
			return SDLK_C;
		case SDLK_KP_D:
			return SDLK_D;
		case SDLK_KP_E:
			return SDLK_E;
		case SDLK_KP_F:
			return SDLK_F;
		case SDLK_KP_PERCENT:
			return SDLK_PERCENT;
		case SDLK_KP_LESS:
			return SDLK_LESS;
		case SDLK_KP_GREATER:
			return SDLK_GREATER;
		case SDLK_KP_AMPERSAND:
			return SDLK_AMPERSAND;
		case SDLK_KP_COLON:
			return SDLK_COLON;
		case SDLK_KP_HASH:
			return SDLK_HASH;
		case SDLK_KP_SPACE:
			return SDLK_SPACE;
		case SDLK_KP_AT:
			return SDLK_AT;
		case SDLK_KP_EXCLAM:
			return SDLK_EXCLAIM;
		case SDLK_KP_PLUSMINUS:
			return SDLK_PLUSMINUS;
		default:
			return keycode;
		}
	}

	/*
	* 用于提供类终端输入体验的辅助函数
	* 在屏幕指定位置绘制输入光标，响应键盘事件，支持基本的文本编辑功能（光标移动/退格）
	* 向用户提供的 bgt_input_* 系列函数均通过调用 bgt_input，传入自定义的验证器与解析器实现
	* 
	* 这里使用 C++20 的 concept 来表达模板约束：
	* https://en.cppreference.com/w/cpp/language/constraints
	*/
	
	// InputValidator 概念：验证器应当是一个可调用对象，接受 std::string_view 参数，返回 bool
	// 用于验证当前输入是否合法，或者是否为一个合法输入的前缀（可能只输入了一部分）
	template<typename T>
	concept InputValidator = requires(T validator, std::string_view input) {
		{ validator(input) } -> std::same_as<bool>;
	};

	bool is_printable_ascii(char ch) {
		return ch >= 32 && ch <= 126;
	}

	// 默认只接受可打印 ASCII 字符
	auto default_validator = [](std::string_view s) {
		return std::ranges::all_of(s, is_printable_ascii);
		};

	// InputParser 概念：解析器应当是一个可调用对象，接受 std::string_view 参数，将其解析为返回结果
	template<typename T>
	concept InputParser = std::invocable<T, std::string_view>;

	// 默认以 std::string 形式返回
	auto default_parser = [](std::string_view s) {
		return std::string{ s };
		};

	template<InputValidator Validator = decltype(default_validator), InputParser Parser = decltype(default_parser)>
	auto bgt_input(int x, int y, SDL_Color bg_color, SDL_Color fg_color, int max_len,
		Validator validator = default_validator, Parser parser = default_parser)
		-> std::invoke_result_t<Parser, std::string_view> {
		bool input_complete = false;

		// TODO: 支持自定义 cursor_height
		int cursor_x = x, cursor_y = y;
		const int cursor_width = bgt_get_font_width(), cursor_height = 4;

		auto start_tick = SDL_GetTicks();

		std::string input_buf;
		const int line_height = bgt_get_font_height();
		int cursor_pos = 0;

		int text_width = 0;

		while (true) {
			// 渲染已输入部分文本与光标
			auto current_tick = SDL_GetTicks();

			int width_before_cursor = bgt_measure_text(input_buf.substr(0, cursor_pos).c_str());
			int blank_rect_width = cursor_width + text_width;

			// 用背景色擦除输入区域
			bgt_rectangle(x, y, blank_rect_width, line_height, bg_color.r, bg_color.g, bg_color.b, bg_color.a, false);
			// 渲染已输入文本
			bgt_show_str(x, y, input_buf.c_str(), fg_color.r, fg_color.g, fg_color.b, fg_color.a, false);
			// 光标每秒闪烁一次
			auto& cursor_color = (current_tick - start_tick) / 500 % 2 ? bg_color : fg_color;
			bgt_rectangle(x + width_before_cursor, y + line_height - cursor_height, cursor_width, cursor_height,
				cursor_color.r, cursor_color.g, cursor_color.b, cursor_color.a);

			SDL_Event e;
			while (SDL_PollEvent(&e)) {
				switch (e.type) {
				case SDL_EVENT_KEY_UP:
					auto keycode =
						SDL_ConvertNumpadKeycode(
							SDL_GetKeyFromScancode(e.key.scancode, e.key.mod, false),
							e.key.mod & SDL_KMOD_NUM
						);
					if (keycode == SDLK_RETURN) {
						// 用背景色擦除输入区域
						bgt_rectangle(x, y, blank_rect_width, line_height, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
						return parser(input_buf);
					}
					else if (keycode == SDLK_BACKSPACE) {
						// 按了退格，删除一个字符
						if (cursor_pos > 0) {
							SDL_assert(cursor_pos <= static_cast<int>(input_buf.length()));
							input_buf.erase(cursor_pos - 1, 1);
							cursor_pos--;
						}
					}
					// 处理箭头按键
					else if (keycode == SDLK_LEFT) {
						if (cursor_pos > 0) {
							cursor_pos--;
						}
					}
					else if (keycode == SDLK_RIGHT) {
						if (cursor_pos < input_buf.length()) {
							cursor_pos++;
						}
					}
					// 长度超过 max_len - 1 的输入部分会被丢弃
					else if (input_buf.length() < max_len - 1) {
						char ch = static_cast<char>(keycode);

						SDL_assert(cursor_pos <= static_cast<int>(input_buf.length()));

						auto new_str_with_ch = input_buf;
						new_str_with_ch.insert(cursor_pos, 1, ch);

						if (validator(new_str_with_ch)) {
							input_buf = std::move(new_str_with_ch);
							cursor_pos++;
							text_width = bgt_measure_text(input_buf.c_str());
						}
					}

					break;
				}
			}
		}

		std::unreachable();
	}

} // namespace // namespace

bool bgt_flush() {
	// 如果一直不处理事件或者睡太久，窗口会假死
	// 为了向新手使用者隔离事件机制，每次刷新的时候装模作样处理一下事件
	// 实际上什么都没有处理，从队列里拿一个出来再放回去
	SDL_Event e;
	if (SDL_PollEvent(&e)) {
		SDL_PushEvent(&e);
	}
	return SDL_SetRenderTarget(renderer, nullptr) && SDL_RenderClear(renderer) &&
		SDL_RenderTexture(renderer, render_target, nullptr, nullptr) &&
		SDL_RenderPresent(renderer);
}

const char* bgt_get_error() {
#ifdef USE_ANSI
	localized_error_msg = utf8_to_ansi(SDL_GetError());
	return localized_error_msg.c_str();
#else 
	return SDL_GetError();
#endif
}

bool bgt_init(int w, int h, const char* title, const char* font_name, int font_size, bool fix_display_scale) {
	if (!SDL_Init(SDL_INIT_VIDEO) || !TTF_Init()) {
		return false;
	}
#ifdef USE_ANSI
	auto utf8_title = ansi_to_utf8(title);
	const char* title_cstr = utf8_title.c_str();
#else
	const char* title_cstr = title;
	#endif
	window =
		SDL_CreateWindow(title_cstr, w, h,
			SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_INPUT_FOCUS |
			SDL_WINDOW_MOUSE_FOCUS | SDL_WINDOW_RESIZABLE);

	// 创建窗口时根据系统缩放设置调整大小
	// 如果缩放倍数不是整数倍，则性能会显著变差
	if (fix_display_scale) {
		float display_scale = SDL_GetWindowDisplayScale(window);
		if (display_scale != 1.0F) {
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
				reinterpret_cast<const char*>(u8"检测到显示缩放比例: %.2f，调整窗口大小以适应缩放"),
				display_scale);
			SDL_SetWindowSize(window, int(w * display_scale), int(h * display_scale));

			int real_width, real_height;
			if (!SDL_GetWindowSizeInPixels(window, &real_width, &real_height)) {
				return false;
			}

			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, reinterpret_cast<const char*>(u8"实际窗口大小为：%d x %d"), real_width, real_height);
		}
	}

	/* 由于 bgt 系列工具面向初学者，期望达到的效果是每次调用就在屏幕上对应画图，
	   因此必须使用软件渲染器并关闭垂直同步，屏蔽掉缓冲区/硬件加速的复杂性 */
	renderer = SDL_CreateRenderer(window, "software");
	SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_DISABLED);

	// 默认色彩混合模式为常规混合，半透明效果被标准处理
	// 如果希望实现光影叠加等效果，可以调用 bgt_set_blend_mode 改变混合模式
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// 加载字体及文本引擎
	if (auto font_path = GetInstalledFontFile(font_name)) {
		font = TTF_OpenFont(font_path->string().c_str(), static_cast<float>(font_size));
	}

	if (!font) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, reinterpret_cast<const char*>(u8"未找到 %s 字体"), font_name);
		font = TTF_OpenFont(GetInstalledFontFile("SimSun")->string().c_str(), static_cast<float>(font_size));
		SDL_assert(font && u8"未找到新宋体。请检查字体安装情况。");
	}

	// 对于非等宽字体做出警告
	// 新宋体实际上是等宽的，但没有设置等宽字体属性，故此处特判
	if (!TTF_FontIsFixedWidth(font) && strcmp(font_name, "SimSun") != 0) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
			reinterpret_cast<const char*>(u8"%s 不是等宽字体。使用时请注意不同字符宽度不同的细节。"), font_name);
	}

	// 设置字体在亚像素级别渲染，能有效解决缩放后模糊的问题
	TTF_SetFontHinting(font, TTF_HINTING_LIGHT_SUBPIXEL);
	text_engine = TTF_CreateRendererTextEngine(renderer);

	if (!(render_target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET, w, h))) {
		return false;
	}

	return SDL_AddEventWatch(
		+[](void* userdata, SDL_Event* e) -> bool {
			switch (e->type) {
			case SDL_EVENT_QUIT:
				// 关闭窗口
				bgt_quit();
				exit(0);
			case SDL_EVENT_WINDOW_RESIZED:
			case SDL_EVENT_WINDOW_MAXIMIZED:
			case SDL_EVENT_WINDOW_RESTORED:
				// 窗口大小改变时，刷新一下以自动适配
				// 理论上这里其实是不够安全的，因为事件可能来自其他线程，而 bgt_flush 必须在主线程调用
				// 但测试表明这三个事件都会在主线程被处理，考虑到这是给新手的教学用框架，就暂时这样用着吧
				bgt_flush();
				break;
			default:
				break;
			}
			return true;
		},
		nullptr) &&
		// 无论实际窗口多大, renderer 在面对它时的逻辑大小都始终维持 w * h, 由框架自动处理放缩
		SDL_SetRenderLogicalPresentation(renderer, w, h,
			SDL_LOGICAL_PRESENTATION_LETTERBOX) &&
		SDL_SetRenderTarget(renderer, render_target) &&
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, BGT_ALPHA_OPAQUE) &&
		SDL_RenderClear(renderer);
}

void bgt_quit() {
	if (font) {
		TTF_CloseFont(font);
		font = nullptr;
	}
	if (render_target) {
		SDL_DestroyTexture(render_target);
		render_target = nullptr;
	}
	if (renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}
	if (window) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}
	TTF_Quit();
	SDL_Quit();
}

bool bgt_cls(int r, int g, int b, bool flush) {
	if (!renderer || !render_target) {
		return false;
	}
	return SDL_SetRenderTarget(renderer, render_target) &&
		SDL_SetRenderDrawColor(renderer, r, g, b, BGT_ALPHA_OPAQUE) &&
		SDL_RenderClear(renderer) && (!flush || bgt_flush());
}

bool bgt_rectangle(int x, int y, int w, int h, int r, int g, int b, int a,
	bool flush) {
	if (!renderer || !render_target) {
		return false;
	}
	const SDL_FRect rect = { float(x), float(y), float(w), float(h) };

	{
		RenderDrawColorGuard _;
		SDL_SetRenderTarget(renderer, render_target);
		SDL_SetRenderDrawColor(renderer, r, g, b, a);
		SDL_RenderFillRect(renderer, &rect);
	}
	if (flush)
		return bgt_flush();
	return true;
}

bool bgt_set_blend_mode(unsigned int mode) {
	if (!renderer || !render_target) {
		return false;
	}
	return SDL_SetRenderDrawBlendMode(renderer, mode);
}

bool bgt_line(int x1, int y1, int x2, int y2, int r, int g, int b, int a,
	bool flush) {
	if (!renderer || !render_target) {
		return false;
	}
	{
		RenderDrawColorGuard _;
		SDL_SetRenderTarget(renderer, render_target);
		SDL_SetRenderDrawColor(renderer, r, g, b, a);
		SDL_RenderLine(renderer, (float)x1, (float)y1, (float)x2, (float)y2);
	}
	if (flush)
		return bgt_flush();
	return true;
}

bool bgt_circle(int center_x, int center_y, int radius, int r, int g, int b,
	int a, bool flush) {
	if (!renderer || !render_target) {
		return false;
	}
	{
		RenderDrawColorGuard _;
		SDL_SetRenderTarget(renderer, render_target);
		SDL_SetRenderDrawColor(renderer, r, g, b, a);
		for (int y = -radius; y <= radius; y++) {
			int x = (int)sqrt(radius * radius - y * y); // Calculate x for given y
			SDL_RenderLine(renderer, float(center_x - x), float(center_y + y), float(center_x + x),
				float(center_y + y));
		}
	}
	if (flush)
		return bgt_flush();
	return true;
}

int bgt_get_font_width() {
	if (!font) {
		return 0;
	}
	return bgt_measure_text(" ");
}

int bgt_get_font_height() {
	if (!font) {
		return 0;
	}
	return TTF_GetFontHeight(font);
}

int bgt_measure_text(const char* str)
{
#ifdef USE_ANSI
	auto converted_str = ansi_to_utf8(str);
	const char* str_to_measure = converted_str.c_str();
	std::size_t len = converted_str.length();
#else
	const char* str_to_measure = str;
	std::size_t len = std::strlen(str);
#endif

	int w;
	TTF_MeasureString(font, str_to_measure, len, 0, &w, nullptr);
	return w;
}

int bgt_show_str(int x, int y, const char* str, int r, int g, int b, int a, bool flush) {

#ifdef USE_ANSI
	auto converted_str = ansi_to_utf8(str);
	const char* utf8_str = converted_str.c_str();

#else
	const char* utf8_str = str;
#endif
	int text_width_in_pixel;
	TTF_MeasureString(font, utf8_str, 0, 0, &text_width_in_pixel, nullptr);

	auto* text = TTF_CreateText(text_engine, font, utf8_str, 0);

	if (!text) {
		return false;
	}
	{
		RenderDrawColorGuard _;
		TTF_SetTextColor(text, r, g, b, a);
		SDL_SetRenderTarget(renderer, render_target);
		TTF_DrawRendererText(text, (float)x, (float)y);
	}
	if (flush)
		bgt_flush();
	TTF_DestroyText(text);
	return text_width_in_pixel;
}

void bgt_delay(int ms) {
	// 如果一直不处理事件或者睡太久，窗口会假死
	// 为了向新手使用者隔离事件机制，每睡 50ms 就醒过来装模作样处理一下事件
	constexpr int SLEEP_SLICE_MS = 50;
	while (ms > 0) {
		SDL_Delay(ms > SLEEP_SLICE_MS ? SLEEP_SLICE_MS : ms);
		ms -= SLEEP_SLICE_MS;

		SDL_Event e;
		// 实际上什么都没有处理，从队列里拿一个出来再放回去
		if (SDL_PollEvent(&e)) {
			SDL_PushEvent(&e);
		}
	}
}

unsigned long long bgt_get_ticks()
{
	return SDL_GetTicks();
}

int bgt_getch() {
	SDL_Event e;

	while (SDL_WaitEvent(&e)) {
		switch (e.type) {
		case SDL_EVENT_KEY_UP:
			// 通过 GetKeyFromScancode 获取按键码
			// key_event 参数为 false 时，该函数会正确处理 Shift/Capslock 等修饰符，给出一个对用户友好的结果
			auto keycode = SDL_GetKeyFromScancode(e.key.scancode, e.key.mod, false);
			// 额外手动处理一下小键盘的按键
			return SDL_ConvertNumpadKeycode(keycode, e.key.mod & SDL_KMOD_NUM);
		}
	}
	std::unreachable();
}

BGT_Ostream bgt_cout(int x, int y, int r, int g, int b, int a, bool flush)
{
	return BGT_Ostream{x, y, r, g, b, a, flush};
}

int bgt_read_keyboard_and_mouse(int& mouse_x, int& mouse_y, int& mouse_action,
	int& keycode, int& key_modifier) {
	SDL_Event e;
	keycode = 0;
	key_modifier = 0;

	while (SDL_PollEvent(&e)) {
		SDL_ConvertEventToRenderCoordinates(renderer, &e);
		switch (e.type) {
		case SDL_EVENT_KEY_UP:
			keycode = e.key.key;
			key_modifier = e.key.mod;
			mouse_action = MOUSE_NO_ACTION;
			return BGT_KEYBOARD_EVENT;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			mouse_x = static_cast<int>(e.button.x);
			mouse_y = static_cast<int>(e.button.y);
			// 鼠标左键
			if (e.button.button == SDL_BUTTON_LEFT) {
				if (e.button.clicks == 1) {
					mouse_action = MOUSE_LEFT_BUTTON_CLICK;
				}
				else {
					// 暂时把多次点击视为双击
					mouse_action = MOUSE_LEFT_BUTTON_DOUBLE_CLICK;
				}
			}
			else if (e.button.button == SDL_BUTTON_MIDDLE) {
				mouse_action = MOUSE_WHEEL_CLICK;
			}
			else {
				// 暂时把鼠标侧键当成右键
				if (e.button.clicks == 1) {
					mouse_action = MOUSE_RIGHT_BUTTON_CLICK;
				}
				else {
					mouse_action = MOUSE_RIGHT_BUTTON_DOUBLE_CLICK;
				}
			}
			return BGT_MOUSE_EVENT;
		case SDL_EVENT_MOUSE_WHEEL:
			mouse_x = static_cast<int>(e.wheel.mouse_x);
			mouse_y = static_cast<int>(e.wheel.mouse_y);
			if (e.wheel.y > 0) {
				mouse_action = MOUSE_WHEEL_MOVED_UP;
			}
			else {
				mouse_action = MOUSE_WHEEL_MOVED_DOWN;
			}
			return BGT_MOUSE_EVENT;
		case SDL_EVENT_MOUSE_MOTION:
			mouse_x = static_cast<int>(e.motion.x);
			mouse_y = static_cast<int>(e.motion.y);
			mouse_action = MOUSE_ONLY_MOVED;
			return BGT_MOUSE_EVENT;
		default:
			break;
		}
	}
	return BGT_NO_EVENT;
}


int bgt_input_number(int x, int y, int fg_r, int fg_g, int fg_b, int fg_a, int bg_r, int bg_g, int bg_b) {
	auto parser = [](std::string_view sv) -> int {
		int result;
		if (std::from_chars(sv.data(), sv.data() + sv.size(), result).ec == std::errc{}) {
			return result;
		}
		return 0;
		};
	auto validator = [](std::string_view sv) -> bool {
		if (sv.empty()) {
			return true;
		}
		return (sv[0] == '-' || sv[0] == '+' || (sv[0] >= '0' && sv[0] <= '9')) &&
			std::ranges::all_of(sv.substr(1), [](char ch) {
			return ch >= '0' && ch <= '9';
				});
		};
	return bgt_input(x, y,
		SDL_Color{ static_cast<Uint8>(bg_r), static_cast<Uint8>(bg_g), static_cast<Uint8>(bg_b), BGT_ALPHA_OPAQUE },
		SDL_Color{ static_cast<Uint8>(fg_r), static_cast<Uint8>(fg_g), static_cast<Uint8>(fg_b), static_cast<Uint8>(fg_a) },
		11, // -2147483648 共 11 个字符
		validator, parser);
}

int bgt_input_ascii(int x, int y, char* buf, int max_len, int fg_r, int fg_g, int fg_b, int fg_a, int bg_r, int bg_g, int bg_b) {
	auto result = bgt_input(x, y,
		SDL_Color{ static_cast<Uint8>(bg_r), static_cast<Uint8>(bg_g), static_cast<Uint8>(bg_b), BGT_ALPHA_OPAQUE },
		SDL_Color{ static_cast<Uint8>(fg_r), static_cast<Uint8>(fg_g), static_cast<Uint8>(fg_b), static_cast<Uint8>(fg_a) },
		max_len);
	return static_cast<int>(SDL_strlcpy(buf, result.c_str(), max_len));
}
