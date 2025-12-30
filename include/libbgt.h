#pragma once

#ifdef _MSC_VER
    #pragma comment(lib, "winmm.lib")
    #pragma comment(lib, "imm32.lib")
    #pragma comment(lib, "version.lib")
    #pragma comment(lib, "setupapi.lib")
    #pragma comment(lib, "rpcrt4.lib")
    #pragma comment(lib, "usp10.lib")
    #pragma comment(lib, "libbgt.lib")
#endif

#include <iosfwd>
#include <sstream>

/* 定义鼠标键盘操作类型 */
#define BGT_NO_EVENT 0
#define BGT_MOUSE_EVENT 1
#define BGT_KEYBOARD_EVENT 2

/* 定义鼠标的操作 */
#define MOUSE_NO_ACTION 0x0000                 // 无鼠标操作
#define MOUSE_ONLY_MOVED 0x0001                // 鼠标移动
#define MOUSE_LEFT_BUTTON_CLICK 0x0002         // 按下左键
#define MOUSE_LEFT_BUTTON_DOUBLE_CLICK 0x0004  // 双击左键
#define MOUSE_RIGHT_BUTTON_CLICK 0x0008        // 按下右键
#define MOUSE_RIGHT_BUTTON_DOUBLE_CLICK 0x0010 // 双击右键
#define MOUSE_LEFTRIGHT_BUTTON_CLICK 0x0020    // 同时按下左右键
#define MOUSE_WHEEL_CLICK 0x0040               // 滚轮被按下
#define MOUSE_WHEEL_MOVED_UP 0x0080            // 滚轮向上移动
#define MOUSE_WHEEL_MOVED_DOWN 0x0100          // 滚轮向下移动

/* 定义Alpha通道不透明和透明值 */
#define BGT_ALPHA_OPAQUE 255		// 完全不透明
#define BGT_ALPHA_TRANSPARENT 0		// 完全透明

/* 定义混色模式 */
#define BGT_BLENDMODE_NONE                  0x00000000u /**< no blending: dstRGBA = srcRGBA */
#define BGT_BLENDMODE_BLEND                 0x00000001u /**< alpha blending: dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA)), dstA = srcA + (dstA * (1-srcA)) */
#define BGT_BLENDMODE_BLEND_PREMULTIPLIED   0x00000010u /**< pre-multiplied alpha blending: dstRGBA = srcRGBA + (dstRGBA * (1-srcA)) */
#define BGT_BLENDMODE_ADD                   0x00000002u /**< additive blending: dstRGB = (srcRGB * srcA) + dstRGB, dstA = dstA */
#define BGT_BLENDMODE_ADD_PREMULTIPLIED     0x00000020u /**< pre-multiplied additive blending: dstRGB = srcRGB + dstRGB, dstA = dstA */
#define BGT_BLENDMODE_MOD                   0x00000004u /**< color modulate: dstRGB = srcRGB * dstRGB, dstA = dstA */
#define BGT_BLENDMODE_MUL                   0x00000008u /**< color multiply: dstRGB = (srcRGB * dstRGB) + (dstRGB * (1-srcA)), dstA = dstA */
#define BGT_BLENDMODE_INVALID               0x7FFFFFFFu


/**
 * @brief 初始化图形窗口
 *
 * 若 fix_display_scale 为 true, 则实际窗口大小会受系统缩放设置的影响, 但逻辑大小总是与指定的宽高相同。
 *
 * 注意，为了方便使用，强烈建议所有ASCII字符宽度相同、每个汉字均严格占用英文字符的两倍宽度的等宽字体。
 *
 * 使用高分屏时, 可以设置 fix_display_scale 为 true 以避免窗口过小,
 * 但由于我们使用的是软渲染, 若缩放倍数非整数倍, 则性能可能显著变差
 *
 * @param w, h 窗口宽度与高度，单位为像素
 * @param window_title 窗口标题
 * @param font_name 字体英文名称，应当为系统已安装的等宽字体；若指定字体不可用，则使用新宋体
 * @param font_size 字体大小，单位为点数（pt），注意实际所占像素点数会受 DPI 设置影响，应当通过 bgt_get_font_height 获取
 * @param fix_display_scale 是否自动适配系统缩放设置
 */
bool bgt_init(int w, int h, const char* window_title, const char* font_name = "SimSun", int font_size = 20, bool fix_display_scale = false);

/**
 * @brief 关闭图形窗口，释放资源
 *
 * 调用此函数后，所有其他 bgt_* 函数均不可再使用，除非重新调用 bgt_init 初始化
 *
 * 每一次成功的 bgt_init 都必须对应一次 bgt_quit 调用以释放资源
 */
void bgt_quit();

/**
 * @brief bgt 系列函数出错时，获取最后一次错误信息
 */
const char* bgt_get_error();

/**
 * @brief 使用指定颜色清除整个窗口内容
 */
bool bgt_cls(int r = 0, int g = 0, int b = 0);

/**
 * @brief 刷新屏幕显示，将已绘制但未刷新的内容显示到窗口上
 */
bool bgt_flush();

/**
 * @brief 绘制矩形
 *
 * @param x, y 矩形左上角坐标
 * @param w, h 矩形宽高
 * @param r, g, b 矩形颜色 RGB 分量（0-255）
 * @param a 矩形颜色 Alpha 分量（0-255），默认不透明
 * @param flush 是否立即刷新屏幕显示，默认立即刷新
 * @return 成功返回 true，失败返回 false，失败原因可通过 bgt_get_error 获取
 */
bool bgt_rectangle(int x, int y, int w, int h, int r, int g, int b,
	int a = BGT_ALPHA_OPAQUE, bool flush = true);

/**
 * @brief 绘制直线
 *
 * @param x1, y1 直线起点坐标
 * @param x2, y2 直线终点坐标
 * @param r, g, b 直线颜色RGB分量（0-255）
 * @param a 直线颜色Alpha分量（0-255），默认不透明
 * @param flush 是否立即刷新屏幕显示，默认立即刷新
 * @return 成功返回true，失败返回false，失败原因可通过 bgt_get_error 获取
 */
bool bgt_line(int x1, int y1, int x2, int y2, int r, int g, int b,
	int a = BGT_ALPHA_OPAQUE, bool flush = true);

/**
 * @brief 绘制圆形
 * @param center_x, center_y 圆心坐标(center_x, center_y)
 * @param radius 圆半径
 * @param r, g, b 圆颜色RGB分量（0-255)
 * @param a 圆颜色Alpha分量（0-255），默认不透明
 * @param flush 是否立即刷新屏幕显示，默认立即刷新
 * @return 成功返回true，失败返回false，失败原因可通过 bgt_get_error 获取
 */
bool bgt_circle(int center_x, int center_y, int radius, int r, int g, int b,
	int a = BGT_ALPHA_OPAQUE, bool flush = true);

/**
* @brief 非阻塞读取键盘和鼠标输入事件
*
* 返回第一个未处理的键盘或鼠标事件的信息。如果没有事件发生，返回 BGT_NO_EVENT。
*
* @param mouse_x, mouse_y 若有鼠标事件，返回鼠标的坐标位置
* @param mouse_action 若有鼠标事件，返回鼠标操作类型，取值为 MOUSE_* 系列宏定义
* @param keycode 若有键盘事件，返回键盘按键的 SDL_Keycode 值，使用 BGTK_* 系列宏定义判断
* @param key_modifier 返回键盘按键的修饰符（Ctrl/Alt/Shift/Windows/Numlock/Capslock），取值为SDL_KMOD_*系列宏定义的按位或
*
* @return 返回事件类型，取值为 BGT_NO_EVENT、BGT_MOUSE_EVENT 或 BGT_KEYBOARD_EVENT 之一
*/
int bgt_read_keyboard_and_mouse(int& mouse_x, int& mouse_y, int& mouse_action,
	int& keycode, int& key_modifier);

/**
* @brief 从键盘阻塞读取一个按键
*
* 比起 bgt_read_keyboard_and_mouse 更轻量级
*/
int bgt_getch();

/**
* @brief 在指定位置输入一个整数
*
* 调用函数将会在指定位置闪烁光标，读取键盘输入并回显，忽略键盘中非数字的输入。
* 光标可以通过方向键控制，按回车确认输入。
*
* 输入完成后，回显将会被清除。如果希望屏幕上能保留显示效果，应自行重新输出。
*
* @param x, y 输入框左上角坐标
* @param fg_r, fg_g, fg_b, fg_a 前景色 RGBA 分量（0-255）
* @param bg_r, bg_g, bg_b 背景色 RGB 分量（0-255），Alpha 分量默认为不透明
*
* @return 读取到的整数。若输入为空，则读到0。
*/
int bgt_input_number(int x, int y, int fg_r, int fg_g, int fg_b, int fg_a, int bg_r, int bg_g, int bg_b);

/**
* @brief 在指定位置输入一个 ASCII 字符串存入 buf 中，长度不超过 max_len - 1
*
* 调用函数将会在指定位置闪烁光标，读取键盘输入并回显，忽略非可显示 ASCII 字符的输入。
* 光标可以通过方向键控制，按回车确认输入。
*
* 输入完成后，回显将会被清除。如果希望屏幕上能保留显示效果，应自行重新输出。
*
* 读入中文需要和输入法交互，故本函数不支持读取中文。可以阅读 https://wiki.libsdl.org/SDL3/BestKeyboardPractices 学习如何处理输入法事件。
*
* @param x, y 输入框左上角坐标
* @param buf, max_len 一个字符数组，用于存放读到的字符串; 调用者应保证 buf 至少有 max_len 字节大小。
* @param fg_r, fg_g, fg_b, fg_a 前景色 RGBA 分量（0-255）
* @param bg_r, bg_g, bg_b 背景色 RGB 分量（0-255）
*/
int bgt_input_ascii(int x, int y, char* buf, int max_len, int fg_r, int fg_g, int fg_b, int fg_a, int bg_r, int bg_g, int bg_b);

/**
* @brief 绘制字符串
*
* @param x, y 输出位置的横纵坐标
* @param str 待输出的字符串
* @param r, g, b, a 颜色的 RGBA 分量（0-255），Alpha 分量默认为不透明
* @param flush 是否立即刷新
*
* @return 输出的字符串的宽度，单位为像素
*/
int bgt_show_str(int x, int y, const char* str, int r, int g, int b,
	int a = BGT_ALPHA_OPAQUE, bool flush = true);

/**
* @brief 使用类似 cout 的方式格式化输出
*
* 支持 <iomanip> 中的格式控制
*
* 使用例子：
*
* bgt_cout(50, 100, 255, 255, 255) << "PI = " << setiosflags(ios::fixed) << setprecision(2) << 3.1415926;
*
* 将会在 (50, 100) 处使用白色输出 PI = 3.14
*
* @param x, y 输出位置的横纵坐标
* @param r, g, b, a 颜色的 RGBA 分量（0-255），Alpha 分量默认为不透明
* @param flush 是否立即刷新
*
* @return 一个类似 cout 的对象, 请不要存储它，而是立即通过 << 运算符使用
*/
class BGT_Ostream bgt_cout(int x, int y, int r, int g, int b, int a = BGT_ALPHA_OPAQUE, bool flush = true);

/**
* @brief 获取当前字体的宽度，单位为像素
*
* 默认使用的新宋体以及部分等宽字体保证所有英文字符宽度相同、中文宽度严格是英文字符的两倍，
* 对于不保证这一点的其他字体，应当使用 `bgt_measure_text` 函数测量字符串宽度。
*/
int bgt_get_font_width();

/**
* @brief 获取当前字体的高度，单位为像素
*/
int bgt_get_font_height();

/**
* @brief 测量字符串的显示宽度，单位为像素
*
* 默认使用的新宋体以及部分等宽字体保证中文宽度严格是英文字符的两倍，
* 在满足这一条件且使用 GB2312 编码的前提下，本函数等价于 `bgt_get_font_width() * strlen(str)`
*/
int bgt_measure_text(const char* str);

/**
* @brief 设置色彩混合模式
*
* 这一选项描述了在原本颜色为 src 的位置绘制一个颜色为 dst 的像素时，最终得到的颜色是什么
*
* @param mode 一个形如 SDL_BLENDMODE_* 的宏，标识混合模式
*
* 常用取值：
*
* SDL_BLENDMODE_NONE 不进行混合，直接用 dst 覆盖 src：dstRGBA = srcRGBA
*
* SDL_BLENDMODE_BLEND 默认值, alpha 混合，将 dst 理解为带半透明效果的色彩:
* dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA)), dstA = srcA + (dstA * (1-srcA))
*
* SDL_BLENDMODE_ADD 加性混合，将 dst 理解为光照，混合后颜色变亮:
* additive blending: dstRGB = (srcRGB * srcA) + dstRGB, dstA = dstA
*
* SDL_BLENDMODE_MOD 乘性混合，将 dst 理解为阴影，混合有颜色变暗:
* dstRGB = (srcRGB * dstRGB), dstA = dstA
*/
bool bgt_set_blend_mode(unsigned int mode);

/**
* @brief 延迟一段时间, 单位为毫秒
*
* 效果类似于 Windows 的 Sleep，但不会卡死窗口导致无响应
*
* 在初学阶段，可以使用延时来达成动画效果
*/
void bgt_delay(int ms);

/**
* @brief 获取从 bgt_init() 到当前时刻的毫秒数
*
* 其绝对值通常无意义，但可以用来做差获取时间段长度
*/
unsigned long long bgt_get_ticks();

// bgt_cout 所使用的工具类, 暂时不需要理解原理
class BGT_Ostream
{
public:
	BGT_Ostream(int x, int y, int r, int g, int b, int a = BGT_ALPHA_OPAQUE, bool flush = true)
		: x_(x), y_(y), r_(r), g_(g), b_(b), a_(a), flush_(flush) {
	}

	~BGT_Ostream() {
		bgt_show_str(x_, y_, oss_.str().c_str(), r_, g_, b_, a_, flush_);
	}

	template <typename T>
	BGT_Ostream& operator<<(T&& value) {
		oss_ << std::forward<T>(value);
		return *this;
	}

	// for std::endl
	BGT_Ostream& operator<<(std::ostream& (*func)(std::ostream&)) {
		func(oss_);
		return *this;
	}
private:
	int x_;
	int y_;
	int r_;
	int g_;
	int b_;
	int a_;
	bool flush_;
	std::ostringstream oss_;
};

/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
	 claim that you wrote the original software. If you use this software
	 in a product, an acknowledgment in the product documentation would be
	 appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/


#define BGTK_EXTENDED_MASK          (1u << 29)
#define BGTK_SCANCODE_MASK          (1u << 30)
#define BGT_SCANCODE_TO_KEYCODE(X)  ((X) | BGTK_SCANCODE_MASK)
#define BGTK_UNKNOWN                0x00000000u /**< 0 */
#define BGTK_RETURN                 0x0000000du /**< '\r' */
#define BGTK_ESCAPE                 0x0000001bu /**< '\x1B' */
#define BGTK_BACKSPACE              0x00000008u /**< '\b' */
#define BGTK_TAB                    0x00000009u /**< '\t' */
#define BGTK_SPACE                  0x00000020u /**< ' ' */
#define BGTK_EXCLAIM                0x00000021u /**< '!' */
#define BGTK_DBLAPOSTROPHE          0x00000022u /**< '"' */
#define BGTK_HASH                   0x00000023u /**< '#' */
#define BGTK_DOLLAR                 0x00000024u /**< '$' */
#define BGTK_PERCENT                0x00000025u /**< '%' */
#define BGTK_AMPERSAND              0x00000026u /**< '&' */
#define BGTK_APOSTROPHE             0x00000027u /**< '\'' */
#define BGTK_LEFTPAREN              0x00000028u /**< '(' */
#define BGTK_RIGHTPAREN             0x00000029u /**< ')' */
#define BGTK_ASTERISK               0x0000002au /**< '*' */
#define BGTK_PLUS                   0x0000002bu /**< '+' */
#define BGTK_COMMA                  0x0000002cu /**< ',' */
#define BGTK_MINUS                  0x0000002du /**< '-' */
#define BGTK_PERIOD                 0x0000002eu /**< '.' */
#define BGTK_SLASH                  0x0000002fu /**< '/' */
#define BGTK_0                      0x00000030u /**< '0' */
#define BGTK_1                      0x00000031u /**< '1' */
#define BGTK_2                      0x00000032u /**< '2' */
#define BGTK_3                      0x00000033u /**< '3' */
#define BGTK_4                      0x00000034u /**< '4' */
#define BGTK_5                      0x00000035u /**< '5' */
#define BGTK_6                      0x00000036u /**< '6' */
#define BGTK_7                      0x00000037u /**< '7' */
#define BGTK_8                      0x00000038u /**< '8' */
#define BGTK_9                      0x00000039u /**< '9' */
#define BGTK_COLON                  0x0000003au /**< ':' */
#define BGTK_SEMICOLON              0x0000003bu /**< ';' */
#define BGTK_LESS                   0x0000003cu /**< '<' */
#define BGTK_EQUALS                 0x0000003du /**< '=' */
#define BGTK_GREATER                0x0000003eu /**< '>' */
#define BGTK_QUESTION               0x0000003fu /**< '?' */
#define BGTK_AT                     0x00000040u /**< '@' */
#define BGTK_LEFTBRACKET            0x0000005bu /**< '[' */
#define BGTK_BACKSLASH              0x0000005cu /**< '\\' */
#define BGTK_RIGHTBRACKET           0x0000005du /**< ']' */
#define BGTK_CARET                  0x0000005eu /**< '^' */
#define BGTK_UNDERSCORE             0x0000005fu /**< '_' */
#define BGTK_GRAVE                  0x00000060u /**< '`' */
#define BGTK_A                      0x00000061u /**< 'a' */
#define BGTK_B                      0x00000062u /**< 'b' */
#define BGTK_C                      0x00000063u /**< 'c' */
#define BGTK_D                      0x00000064u /**< 'd' */
#define BGTK_E                      0x00000065u /**< 'e' */
#define BGTK_F                      0x00000066u /**< 'f' */
#define BGTK_G                      0x00000067u /**< 'g' */
#define BGTK_H                      0x00000068u /**< 'h' */
#define BGTK_I                      0x00000069u /**< 'i' */
#define BGTK_J                      0x0000006au /**< 'j' */
#define BGTK_K                      0x0000006bu /**< 'k' */
#define BGTK_L                      0x0000006cu /**< 'l' */
#define BGTK_M                      0x0000006du /**< 'm' */
#define BGTK_N                      0x0000006eu /**< 'n' */
#define BGTK_O                      0x0000006fu /**< 'o' */
#define BGTK_P                      0x00000070u /**< 'p' */
#define BGTK_Q                      0x00000071u /**< 'q' */
#define BGTK_R                      0x00000072u /**< 'r' */
#define BGTK_S                      0x00000073u /**< 's' */
#define BGTK_T                      0x00000074u /**< 't' */
#define BGTK_U                      0x00000075u /**< 'u' */
#define BGTK_V                      0x00000076u /**< 'v' */
#define BGTK_W                      0x00000077u /**< 'w' */
#define BGTK_X                      0x00000078u /**< 'x' */
#define BGTK_Y                      0x00000079u /**< 'y' */
#define BGTK_Z                      0x0000007au /**< 'z' */
#define BGTK_LEFTBRACE              0x0000007bu /**< '{' */
#define BGTK_PIPE                   0x0000007cu /**< '|' */
#define BGTK_RIGHTBRACE             0x0000007du /**< '}' */
#define BGTK_TILDE                  0x0000007eu /**< '~' */
#define BGTK_DELETE                 0x0000007fu /**< '\x7F' */
#define BGTK_PLUSMINUS              0x000000b1u /**< '\xB1' */
#define BGTK_CAPSLOCK               0x40000039u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CAPSLOCK) */
#define BGTK_F1                     0x4000003au /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F1) */
#define BGTK_F2                     0x4000003bu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F2) */
#define BGTK_F3                     0x4000003cu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F3) */
#define BGTK_F4                     0x4000003du /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F4) */
#define BGTK_F5                     0x4000003eu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F5) */
#define BGTK_F6                     0x4000003fu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F6) */
#define BGTK_F7                     0x40000040u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F7) */
#define BGTK_F8                     0x40000041u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F8) */
#define BGTK_F9                     0x40000042u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F9) */
#define BGTK_F10                    0x40000043u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F10) */
#define BGTK_F11                    0x40000044u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F11) */
#define BGTK_F12                    0x40000045u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F12) */
#define BGTK_PRINTSCREEN            0x40000046u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PRINTSCREEN) */
#define BGTK_SCROLLLOCK             0x40000047u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SCROLLLOCK) */
#define BGTK_PAUSE                  0x40000048u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAUSE) */
#define BGTK_INSERT                 0x40000049u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_INSERT) */
#define BGTK_HOME                   0x4000004au /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_HOME) */
#define BGTK_PAGEUP                 0x4000004bu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAGEUP) */
#define BGTK_END                    0x4000004du /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_END) */
#define BGTK_PAGEDOWN               0x4000004eu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAGEDOWN) */
#define BGTK_RIGHT                  0x4000004fu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RIGHT) */
#define BGTK_LEFT                   0x40000050u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LEFT) */
#define BGTK_DOWN                   0x40000051u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DOWN) */
#define BGTK_UP                     0x40000052u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_UP) */
#define BGTK_NUMLOCKCLEAR           0x40000053u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_NUMLOCKCLEAR) */
#define BGTK_KP_DIVIDE              0x40000054u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DIVIDE) */
#define BGTK_KP_MULTIPLY            0x40000055u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MULTIPLY) */
#define BGTK_KP_MINUS               0x40000056u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MINUS) */
#define BGTK_KP_PLUS                0x40000057u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PLUS) */
#define BGTK_KP_ENTER               0x40000058u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_ENTER) */
#define BGTK_KP_1                   0x40000059u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_1) */
#define BGTK_KP_2                   0x4000005au /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_2) */
#define BGTK_KP_3                   0x4000005bu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_3) */
#define BGTK_KP_4                   0x4000005cu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_4) */
#define BGTK_KP_5                   0x4000005du /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_5) */
#define BGTK_KP_6                   0x4000005eu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_6) */
#define BGTK_KP_7                   0x4000005fu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_7) */
#define BGTK_KP_8                   0x40000060u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_8) */
#define BGTK_KP_9                   0x40000061u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_9) */
#define BGTK_KP_0                   0x40000062u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_0) */
#define BGTK_KP_PERIOD              0x40000063u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PERIOD) */
#define BGTK_APPLICATION            0x40000065u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_APPLICATION) */
#define BGTK_POWER                  0x40000066u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_POWER) */
#define BGTK_KP_EQUALS              0x40000067u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EQUALS) */
#define BGTK_F13                    0x40000068u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F13) */
#define BGTK_F14                    0x40000069u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F14) */
#define BGTK_F15                    0x4000006au /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F15) */
#define BGTK_F16                    0x4000006bu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F16) */
#define BGTK_F17                    0x4000006cu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F17) */
#define BGTK_F18                    0x4000006du /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F18) */
#define BGTK_F19                    0x4000006eu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F19) */
#define BGTK_F20                    0x4000006fu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F20) */
#define BGTK_F21                    0x40000070u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F21) */
#define BGTK_F22                    0x40000071u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F22) */
#define BGTK_F23                    0x40000072u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F23) */
#define BGTK_F24                    0x40000073u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F24) */
#define BGTK_EXECUTE                0x40000074u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_EXECUTE) */
#define BGTK_HELP                   0x40000075u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_HELP) */
#define BGTK_MENU                   0x40000076u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MENU) */
#define BGTK_SELECT                 0x40000077u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SELECT) */
#define BGTK_STOP                   0x40000078u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_STOP) */
#define BGTK_AGAIN                  0x40000079u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AGAIN) */
#define BGTK_UNDO                   0x4000007au /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_UNDO) */
#define BGTK_CUT                    0x4000007bu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CUT) */
#define BGTK_COPY                   0x4000007cu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_COPY) */
#define BGTK_PASTE                  0x4000007du /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PASTE) */
#define BGTK_FIND                   0x4000007eu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_FIND) */
#define BGTK_MUTE                   0x4000007fu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MUTE) */
#define BGTK_VOLUMEUP               0x40000080u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_VOLUMEUP) */
#define BGTK_VOLUMEDOWN             0x40000081u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_VOLUMEDOWN) */
#define BGTK_KP_COMMA               0x40000085u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_COMMA) */
#define BGTK_KP_EQUALSAS400         0x40000086u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EQUALSAS400) */
#define BGTK_ALTERASE               0x40000099u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_ALTERASE) */
#define BGTK_SYSREQ                 0x4000009au /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SYSREQ) */
#define BGTK_CANCEL                 0x4000009bu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CANCEL) */
#define BGTK_CLEAR                  0x4000009cu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CLEAR) */
#define BGTK_PRIOR                  0x4000009du /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PRIOR) */
#define BGTK_RETURN2                0x4000009eu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RETURN2) */
#define BGTK_SEPARATOR              0x4000009fu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SEPARATOR) */
#define BGTK_OUT                    0x400000a0u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_OUT) */
#define BGTK_OPER                   0x400000a1u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_OPER) */
#define BGTK_CLEARAGAIN             0x400000a2u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CLEARAGAIN) */
#define BGTK_CRSEL                  0x400000a3u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CRSEL) */
#define BGTK_EXSEL                  0x400000a4u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_EXSEL) */
#define BGTK_KP_00                  0x400000b0u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_00) */
#define BGTK_KP_000                 0x400000b1u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_000) */
#define BGTK_THOUSANDSSEPARATOR     0x400000b2u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_THOUSANDSSEPARATOR) */
#define BGTK_DECIMALSEPARATOR       0x400000b3u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DECIMALSEPARATOR) */
#define BGTK_CURRENCYUNIT           0x400000b4u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CURRENCYUNIT) */
#define BGTK_CURRENCYSUBUNIT        0x400000b5u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CURRENCYSUBUNIT) */
#define BGTK_KP_LEFTPAREN           0x400000b6u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LEFTPAREN) */
#define BGTK_KP_RIGHTPAREN          0x400000b7u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_RIGHTPAREN) */
#define BGTK_KP_LEFTBRACE           0x400000b8u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LEFTBRACE) */
#define BGTK_KP_RIGHTBRACE          0x400000b9u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_RIGHTBRACE) */
#define BGTK_KP_TAB                 0x400000bau /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_TAB) */
#define BGTK_KP_BACKSPACE           0x400000bbu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_BACKSPACE) */
#define BGTK_KP_A                   0x400000bcu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_A) */
#define BGTK_KP_B                   0x400000bdu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_B) */
#define BGTK_KP_C                   0x400000beu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_C) */
#define BGTK_KP_D                   0x400000bfu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_D) */
#define BGTK_KP_E                   0x400000c0u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_E) */
#define BGTK_KP_F                   0x400000c1u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_F) */
#define BGTK_KP_XOR                 0x400000c2u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_XOR) */
#define BGTK_KP_POWER               0x400000c3u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_POWER) */
#define BGTK_KP_PERCENT             0x400000c4u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PERCENT) */
#define BGTK_KP_LESS                0x400000c5u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LESS) */
#define BGTK_KP_GREATER             0x400000c6u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_GREATER) */
#define BGTK_KP_AMPERSAND           0x400000c7u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_AMPERSAND) */
#define BGTK_KP_DBLAMPERSAND        0x400000c8u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DBLAMPERSAND) */
#define BGTK_KP_VERTICALBAR         0x400000c9u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_VERTICALBAR) */
#define BGTK_KP_DBLVERTICALBAR      0x400000cau /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DBLVERTICALBAR) */
#define BGTK_KP_COLON               0x400000cbu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_COLON) */
#define BGTK_KP_HASH                0x400000ccu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_HASH) */
#define BGTK_KP_SPACE               0x400000cdu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_SPACE) */
#define BGTK_KP_AT                  0x400000ceu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_AT) */
#define BGTK_KP_EXCLAM              0x400000cfu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EXCLAM) */
#define BGTK_KP_MEMSTORE            0x400000d0u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMSTORE) */
#define BGTK_KP_MEMRECALL           0x400000d1u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMRECALL) */
#define BGTK_KP_MEMCLEAR            0x400000d2u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMCLEAR) */
#define BGTK_KP_MEMADD              0x400000d3u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMADD) */
#define BGTK_KP_MEMSUBTRACT         0x400000d4u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMSUBTRACT) */
#define BGTK_KP_MEMMULTIPLY         0x400000d5u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMMULTIPLY) */
#define BGTK_KP_MEMDIVIDE           0x400000d6u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMDIVIDE) */
#define BGTK_KP_PLUSMINUS           0x400000d7u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PLUSMINUS) */
#define BGTK_KP_CLEAR               0x400000d8u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_CLEAR) */
#define BGTK_KP_CLEARENTRY          0x400000d9u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_CLEARENTRY) */
#define BGTK_KP_BINARY              0x400000dau /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_BINARY) */
#define BGTK_KP_OCTAL               0x400000dbu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_OCTAL) */
#define BGTK_KP_DECIMAL             0x400000dcu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DECIMAL) */
#define BGTK_KP_HEXADECIMAL         0x400000ddu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_HEXADECIMAL) */
#define BGTK_LCTRL                  0x400000e0u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LCTRL) */
#define BGTK_LSHIFT                 0x400000e1u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LSHIFT) */
#define BGTK_LALT                   0x400000e2u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LALT) */
#define BGTK_LGUI                   0x400000e3u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LGUI) */
#define BGTK_RCTRL                  0x400000e4u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RCTRL) */
#define BGTK_RSHIFT                 0x400000e5u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RSHIFT) */
#define BGTK_RALT                   0x400000e6u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RALT) */
#define BGTK_RGUI                   0x400000e7u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RGUI) */
#define BGTK_MODE                   0x40000101u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MODE) */
#define BGTK_SLEEP                  0x40000102u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SLEEP) */
#define BGTK_WAKE                   0x40000103u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_WAKE) */
#define BGTK_CHANNEL_INCREMENT      0x40000104u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CHANNEL_INCREMENT) */
#define BGTK_CHANNEL_DECREMENT      0x40000105u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CHANNEL_DECREMENT) */
#define BGTK_MEDIA_PLAY             0x40000106u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_PLAY) */
#define BGTK_MEDIA_PAUSE            0x40000107u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_PAUSE) */
#define BGTK_MEDIA_RECORD           0x40000108u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_RECORD) */
#define BGTK_MEDIA_FAST_FORWARD     0x40000109u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_FAST_FORWARD) */
#define BGTK_MEDIA_REWIND           0x4000010au /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_REWIND) */
#define BGTK_MEDIA_NEXT_TRACK       0x4000010bu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_NEXT_TRACK) */
#define BGTK_MEDIA_PREVIOUS_TRACK   0x4000010cu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_PREVIOUS_TRACK) */
#define BGTK_MEDIA_STOP             0x4000010du /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_STOP) */
#define BGTK_MEDIA_EJECT            0x4000010eu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_EJECT) */
#define BGTK_MEDIA_PLAY_PAUSE       0x4000010fu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_PLAY_PAUSE) */
#define BGTK_MEDIA_SELECT           0x40000110u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_SELECT) */
#define BGTK_AC_NEW                 0x40000111u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_NEW) */
#define BGTK_AC_OPEN                0x40000112u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_OPEN) */
#define BGTK_AC_CLOSE               0x40000113u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_CLOSE) */
#define BGTK_AC_EXIT                0x40000114u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_EXIT) */
#define BGTK_AC_SAVE                0x40000115u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_SAVE) */
#define BGTK_AC_PRINT               0x40000116u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_PRINT) */
#define BGTK_AC_PROPERTIES          0x40000117u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_PROPERTIES) */
#define BGTK_AC_SEARCH              0x40000118u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_SEARCH) */
#define BGTK_AC_HOME                0x40000119u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_HOME) */
#define BGTK_AC_BACK                0x4000011au /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_BACK) */
#define BGTK_AC_FORWARD             0x4000011bu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_FORWARD) */
#define BGTK_AC_STOP                0x4000011cu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_STOP) */
#define BGTK_AC_REFRESH             0x4000011du /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_REFRESH) */
#define BGTK_AC_BOOKMARKS           0x4000011eu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_BOOKMARKS) */
#define BGTK_SOFTLEFT               0x4000011fu /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SOFTLEFT) */
#define BGTK_SOFTRIGHT              0x40000120u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SOFTRIGHT) */
#define BGTK_CALL                   0x40000121u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CALL) */
#define BGTK_ENDCALL                0x40000122u /**< BGT_SCANCODE_TO_KEYCODE(SDL_SCANCODE_ENDCALL) */
#define BGTK_LEFT_TAB               0x20000001u /**< Extended key Left Tab */
#define BGTK_LEVEL5_SHIFT           0x20000002u /**< Extended key Level 5 Shift */
#define BGTK_MULTI_KEY_COMPOSE      0x20000003u /**< Extended key Multi-key Compose */
#define BGTK_LMETA                  0x20000004u /**< Extended key Left Meta */
#define BGTK_RMETA                  0x20000005u /**< Extended key Right Meta */
#define BGTK_LHYPER                 0x20000006u /**< Extended key Left Hyper */
#define BGTK_RHYPER                 0x20000007u /**< Extended key Right Hyper */
