#ifndef BGT_H_
#define BGT_H_

/**
 * @file bgt.h
 * @brief libbgt 的公开 API，面向 C++ 初学者。
 * @details 基础接口统一使用 `bgt_` 前缀与 `snake_case` 命名，只使用普通函数、
 *          整数、布尔值与字符串字面量；窗口、画布、字体、事件循环与资源生命
 *          周期都由库内部管理。
 *
 *          本文件刻意使用整数常量、C 风格字符串参数与 x/y 这样的短坐标名
 *          （面向初学者），NOLINTBEGIN 放行的正是这些检查项。设计说明见
 *          docs/design.md。
 */

// NOLINTBEGIN(readability-magic-numbers, modernize-avoid-c-arrays,
// bugprone-easily-swappable-parameters, readability-identifier-length)

/** @name 版本号
 * 库的语义化版本号，与 CMake 的 project(libbgt VERSION ...) 保持一致。
 * @{
 */
#define BGT_VERSION_MAJOR 0
#define BGT_VERSION_MINOR 1
#define BGT_VERSION_PATCH 0

/** @} */

/** @name 运行期窄字符串编码
 * 窄字符串在运行期使用的编码，由构建选项 `BGT_SOURCE_ENCODING` 决定：
 * - `BGT_ENCODING_UTF8`（默认）：源码与运行期都是 UTF-8，库不做任何转换；
 * - `BGT_ENCODING_GBK`：学生源码与运行期窄字符串都是 GBK（Windows 代码页
 *   936），库在 SDL 边界自动完成 GBK 与 UTF-8 的双向转换，存档文件、printf
 *   输出与错误文本都保持 GBK。
 * 构建系统会传 `-DBGT_SOURCE_ENCODING=BGT_ENCODING_GBK`；分发的 GBK 版头
 * 文件里也直接写死了这个定义，因此学生工程只要按 GBK 编译即可。详见
 * docs/encoding.md。
 * @{
 */
#define BGT_ENCODING_UTF8 0
#define BGT_ENCODING_GBK 1

#ifndef BGT_SOURCE_ENCODING
#define BGT_SOURCE_ENCODING BGT_ENCODING_UTF8
#endif

/** @} */

/// 编译期防呆：GBK 构建下，本编译单元的执行字符集必须是 GBK，否则库收到的
/// 窄字符串与它期望的编码不一致，运行期只会表现为中文乱码。
/// BGT_BUILDING_LIBRARY 表示正在编译 libbgt 自身：库始终按 UTF-8 编译，不受
/// 这项检查约束。
///
/// 这里刻意用通用字符名 U+4E2D（即“中”）而不是直接写字面量：通用字符名在
/// 源码里是纯 ASCII，因此无论本编译单元按哪种源字符集解析它，比较的都是
/// “执行字符集把 U+4E2D 编成几个字节”——GBK 是 2，UTF-8 是 3。
#if BGT_SOURCE_ENCODING == BGT_ENCODING_GBK && !defined(BGT_BUILDING_LIBRARY) && \
    !defined(BGT_DISABLE_ENCODING_CHECK)
static_assert(sizeof("\u4E2D") - 1 == 2,
              "libbgt(GBK 构建)：本编译单元的执行字符集必须是 GBK。MSVC 请去掉 "
              "/utf-8（中文 Windows 的默认字符集就是 936），或显式加 "
              "/source-charset:.936 /execution-charset:.936；GCC/Clang 请加 "
              "-finput-charset=GBK -fexec-charset=GBK。");
#endif

/// 链接期防呆（MSVC）：头文件与 libbgt 库的构建模式必须一致。用错编码版本的
/// 头文件或库时链接器会直接报 LNK2038，而不是等到运行期才显示乱码。
#if defined(_MSC_VER) && !defined(BGT_DISABLE_ENCODING_CHECK)
#if BGT_SOURCE_ENCODING == BGT_ENCODING_GBK
#pragma detect_mismatch("BGT_SOURCE_ENCODING", "GBK")
#else
#pragma detect_mismatch("BGT_SOURCE_ENCODING", "UTF-8")
#endif
#endif

#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "usp10.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "rpcrt4.lib")
#endif

/** @name 颜色常量
 * 可直接传给 bgt_set_color()、bgt_set_background() 的无符号整数颜色值。
 * @{
 */
#define BGT_BLACK 0xFF000000
#define BGT_WHITE 0xFFFFFFFF
#define BGT_RED 0xFFFF0000
#define BGT_GREEN 0xFF00FF00
#define BGT_BLUE 0xFF0000FF
#define BGT_YELLOW 0xFFFFFF00
#define BGT_CYAN 0xFF00FFFF
#define BGT_MAGENTA 0xFFFF00FF
#define BGT_GRAY 0xFF888888
#define BGT_LIGHT_GRAY 0xFFCCCCCC
#define BGT_DARK_GRAY 0xFF444444
#define BGT_ORANGE 0xFFFF8800
#define BGT_PINK 0xFFFF88CC
#define BGT_PURPLE 0xFF8800FF
#define BGT_BROWN 0xFF8B4513
#define BGT_TRANSPARENT 0x00000000

/** @} */

/** @name 按键常量
 * 可直接传给 bgt_key_is_down()、bgt_key_just_pressed()、
 * bgt_key_just_released() 的按键编号。
 * 方向键、回车、空格、修饰键等为 1-12，字母键为 100-125，数字键为
 * 200-209，功能键为 300-311。
 * @{
 */
#define BGT_KEY_LEFT 1
#define BGT_KEY_RIGHT 2
#define BGT_KEY_UP 3
#define BGT_KEY_DOWN 4
#define BGT_KEY_SPACE 5
#define BGT_KEY_ENTER 6
#define BGT_KEY_ESCAPE 7
#define BGT_KEY_TAB 8
#define BGT_KEY_BACKSPACE 9
#define BGT_KEY_SHIFT 10
#define BGT_KEY_CTRL 11
#define BGT_KEY_ALT 12

#define BGT_KEY_A 100
#define BGT_KEY_B 101
#define BGT_KEY_C 102
#define BGT_KEY_D 103
#define BGT_KEY_E 104
#define BGT_KEY_F 105
#define BGT_KEY_G 106
#define BGT_KEY_H 107
#define BGT_KEY_I 108
#define BGT_KEY_J 109
#define BGT_KEY_K 110
#define BGT_KEY_L 111
#define BGT_KEY_M 112
#define BGT_KEY_N 113
#define BGT_KEY_O 114
#define BGT_KEY_P 115
#define BGT_KEY_Q 116
#define BGT_KEY_R 117
#define BGT_KEY_S 118
#define BGT_KEY_T 119
#define BGT_KEY_U 120
#define BGT_KEY_V 121
#define BGT_KEY_W 122
#define BGT_KEY_X 123
#define BGT_KEY_Y 124
#define BGT_KEY_Z 125

#define BGT_KEY_0 200
#define BGT_KEY_1 201
#define BGT_KEY_2 202
#define BGT_KEY_3 203
#define BGT_KEY_4 204
#define BGT_KEY_5 205
#define BGT_KEY_6 206
#define BGT_KEY_7 207
#define BGT_KEY_8 208
#define BGT_KEY_9 209

#define BGT_KEY_F1 300
#define BGT_KEY_F2 301
#define BGT_KEY_F3 302
#define BGT_KEY_F4 303
#define BGT_KEY_F5 304
#define BGT_KEY_F6 305
#define BGT_KEY_F7 306
#define BGT_KEY_F8 307
#define BGT_KEY_F9 308
#define BGT_KEY_F10 309
#define BGT_KEY_F11 310
#define BGT_KEY_F12 311

/** @} */

/** @name 鼠标按键常量
 * 由 bgt_mouse_is_down()、bgt_mouse_just_pressed()、
 * bgt_mouse_just_released() 使用的按键编号。
 * @{
 */
#define BGT_MOUSE_LEFT 1
#define BGT_MOUSE_RIGHT 2
#define BGT_MOUSE_MIDDLE 3

/** @} */

/**
 * @brief 无效的图片编号（0）。
 *
 * bgt_load_image() 加载失败时返回该值；bgt_image_width() 等函数在编号无效时
 * 返回 0。
 */
#define BGT_IMAGE_NONE 0

/** @name 错误码
 * bgt_error_code() 返回的错误码常量；BGT_ERROR_NONE 表示没有错误。
 * @{
 */
#define BGT_ERROR_NONE 0
#define BGT_ERROR_SDL 1
#define BGT_ERROR_TTF 2
#define BGT_ERROR_WINDOW 3
#define BGT_ERROR_RENDERER 4
#define BGT_ERROR_FONT 5
#define BGT_ERROR_NOT_OPEN 6
#define BGT_ERROR_IMAGE 7
#define BGT_ERROR_STORAGE 8
#define BGT_ERROR_AUDIO 9

/** @} */

/** @name 窗口与主循环
 * 创建、刷新与关闭窗口，以及窗口尺寸查询。
 * @{
 */
/** @brief 创建一个固定大小的图形窗口。
 *
 * 同时初始化 libbgt 内部需要的 SDL3 与 SDL3_ttf 资源。width 和 height 是
 * 窗口的逻辑绘图尺寸，title 是窗口标题，可以直接写中文字符串。
 *
 * @param width 窗口的逻辑宽度（像素）。
 * @param height 窗口的逻辑高度（像素）。
 * @param title 窗口标题，可以包含中文。
 * @return 创建成功返回 true；失败返回 false，此时可以调用 bgt_print_error() 查看原因。
 * @note 程序中通常只需要创建一个窗口。
 * @see bgt_close_window
 */
bool bgt_open_window(int width, int height, const char title[]);

/** @brief 创建一个允许用户拖拽改变大小的图形窗口。
 *
 * 绘图仍然使用 width 和 height 指定的逻辑坐标，库内部会把画面缩放到真实
 * 窗口大小。其他行为与 bgt_open_window() 相同。
 *
 * @param width 窗口的逻辑宽度（像素）。
 * @param height 窗口的逻辑高度（像素）。
 * @param title 窗口标题，可以包含中文。
 * @return 创建成功返回 true；失败返回 false，此时可以调用 bgt_print_error() 查看原因。
 * @see bgt_open_window
 */
bool bgt_open_window_resizable(int width, int height, const char title[]);

/** @brief 判断窗口是否仍然处于打开状态。
 *
 * 主循环一般写成 while (bgt_window_is_open()) { ... }。当用户点击关闭
 * 按钮，或库检测到关闭事件时，该函数返回 false。
 *
 * @return 窗口仍然打开返回 true；已关闭或尚未创建返回 false。
 */
bool bgt_window_is_open();

/** @brief 刷新窗口画面，并处理键盘、鼠标、窗口关闭等系统事件。
 *
 * 每一帧绘图结束后都应该调用一次该函数；bgt_key_just_pressed()、
 * bgt_mouse_just_pressed() 这类“刚按下”的状态也会以这个函数为帧边界
 * 进行更新。
 */
void bgt_update_window();

/** @brief 关闭窗口并释放 libbgt 内部持有的 SDL3、SDL3_ttf、字体等资源。
 *
 * 程序正常结束时资源也会被清理，但教学示例中建议显式调用，帮助形成完整结构。
 *
 * @see bgt_open_window
 */
void bgt_close_window();

/** @brief 返回当前窗口的逻辑宽度。
 *
 * 也就是 bgt_open_window() 创建窗口时传入的 width。即使窗口被缩放，这个值
 * 也表示绘图坐标系中的宽度。
 *
 * @return 窗口的逻辑宽度（像素）。
 */
int bgt_window_width();

/** @brief 返回当前窗口的逻辑高度。
 *
 * 也就是 bgt_open_window() 创建窗口时传入的 height。即使窗口被缩放，这个值
 * 也表示绘图坐标系中的高度。
 *
 * @return 窗口的逻辑高度（像素）。
 */
int bgt_window_height();

/** @brief 修改窗口标题。
 *
 * title 可以是 UTF-8 中文字符串；如果窗口尚未创建，该函数不做任何事情。
 *
 * @param title 新的窗口标题，可以包含中文。
 */
void bgt_set_window_title(const char title[]);

/** @} */

/** @name 颜色与背景
 * 创建颜色、设置当前绘图颜色与画布底色。
 * @{
 */
/** @brief 使用红、绿、蓝三个颜色分量创建一个不透明颜色。
 *
 * 每个分量建议取 0 到 255；超出范围的值会被限制到有效范围。
 *
 * @param r 红色分量（0 到 255）。
 * @param g 绿色分量（0 到 255）。
 * @param b 蓝色分量（0 到 255）。
 * @return 不透明的无符号整数颜色值，可以传给 bgt_set_color() 或 bgt_set_background()。
 */
unsigned bgt_rgb(int r, int g, int b);

/** @brief 使用红、绿、蓝、透明度四个分量创建颜色。
 *
 * a 表示不透明度：0 是完全透明，255 是完全不透明。返回值可以作为普通颜色
 * 使用。
 *
 * @param r 红色分量（0 到 255）。
 * @param g 绿色分量（0 到 255）。
 * @param b 蓝色分量（0 到 255）。
 * @param a 不透明度（0 到 255）。
 * @return 无符号整数颜色值。
 */
unsigned bgt_rgba(int r, int g, int b, int a);

/** @brief 设置当前绘图颜色。
 *
 * 后续的点、线、矩形、圆、文字等绘制函数都会使用这个颜色，直到再次调用
 * bgt_set_color() 改变它。color 可以是 BGT_* 颜色常量、bgt_rgb() 或
 * bgt_rgba() 的返回值。
 *
 * @param color 无符号整数颜色值。
 */
void bgt_set_color(unsigned color);

/** @brief 返回当前绘图颜色。
 *
 * 通常用于临时保存颜色，之后再恢复原来的颜色。
 *
 * @return 当前的无符号整数颜色值。
 */
unsigned bgt_get_color();

/** @brief 设置画布底色。
 *
 * libbgt 会在每次 bgt_update_window() 结束时自动把画布清空成这个颜色，
 * 因此不需要（也没有）专门“清屏”的函数：每帧把要画的内容画完整即可，
 * 没画到的部分就是背景色。默认背景色是黑色；通常在进入主循环之前调用一次。
 *
 * @param color 背景色的无符号整数颜色值。
 */
void bgt_set_background(unsigned color);

/** @} */

/** @name 基本图形
 * 点、线、矩形、圆、椭圆、三角形与线宽。
 * @{
 */
/** @brief 在 (x, y) 位置绘制一个点。
 *
 * 坐标系左上角是 (0, 0)，x 向右增加，y 向下增加。如果点在窗口外部，库会
 * 自动忽略它。
 *
 * @param x 点的横坐标。
 * @param y 点的纵坐标。
 */
void bgt_draw_point(int x, int y);

/** @brief 从 (x1, y1) 到 (x2, y2) 绘制一条直线。
 *
 * 颜色使用当前绘图颜色，线宽由 bgt_set_line_width() 控制。
 *
 * @param x1 起点横坐标。
 * @param y1 起点纵坐标。
 * @param x2 终点横坐标。
 * @param y2 终点纵坐标。
 */
void bgt_draw_line(int x1, int y1, int x2, int y2);

/** @brief 绘制矩形边框。
 *
 * (x, y) 是矩形左上角，width 是宽度，height 是高度。矩形内部不会被填充。
 *
 * @param x 矩形左上角横坐标。
 * @param y 矩形左上角纵坐标。
 * @param width 矩形宽度。
 * @param height 矩形高度。
 */
void bgt_draw_rect(int x, int y, int width, int height);

/** @brief 绘制填充矩形。
 *
 * (x, y) 是矩形左上角，width 是宽度，height 是高度。整个矩形区域都会使用
 * 当前颜色填满。
 *
 * @param x 矩形左上角横坐标。
 * @param y 矩形左上角纵坐标。
 * @param width 矩形宽度。
 * @param height 矩形高度。
 */
void bgt_fill_rect(int x, int y, int width, int height);

/** @brief 绘制圆形边框。
 *
 * (x, y) 是圆心，radius 是半径。圆内部不会被填充。
 *
 * @param x 圆心横坐标。
 * @param y 圆心纵坐标。
 * @param radius 圆的半径。
 */
void bgt_draw_circle(int x, int y, int radius);

/** @brief 绘制填充圆。
 *
 * (x, y) 是圆心，radius 是半径。整个圆形区域都会使用当前颜色填满。
 *
 * @param x 圆心横坐标。
 * @param y 圆心纵坐标。
 * @param radius 圆的半径。
 */
void bgt_fill_circle(int x, int y, int radius);

/** @brief 绘制椭圆边框。
 *
 * (x, y) 是椭圆中心，radius_x 是横向半径，radius_y 是纵向半径。椭圆内部
 * 不会被填充。
 *
 * @param x 椭圆中心的横坐标。
 * @param y 椭圆中心的纵坐标。
 * @param radius_x 横向半径。
 * @param radius_y 纵向半径。
 */
void bgt_draw_ellipse(int x, int y, int radius_x, int radius_y);

/** @brief 绘制填充椭圆。
 *
 * (x, y) 是椭圆中心，radius_x 是横向半径，radius_y 是纵向半径。整个椭圆
 * 区域都会使用当前颜色填满。
 *
 * @param x 椭圆中心的横坐标。
 * @param y 椭圆中心的纵坐标。
 * @param radius_x 横向半径。
 * @param radius_y 纵向半径。
 */
void bgt_fill_ellipse(int x, int y, int radius_x, int radius_y);

/** @brief 绘制三角形边框。
 *
 * 三个点分别是 (x1, y1)、(x2, y2)、(x3, y3)。三角形内部不会被填充。
 *
 * @param x1 第一个顶点的横坐标。
 * @param y1 第一个顶点的纵坐标。
 * @param x2 第二个顶点的横坐标。
 * @param y2 第二个顶点的纵坐标。
 * @param x3 第三个顶点的横坐标。
 * @param y3 第三个顶点的纵坐标。
 */
void bgt_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3);

/** @brief 绘制填充三角形。
 *
 * 三个点分别是 (x1, y1)、(x2, y2)、(x3, y3)。三角形内部会使用当前颜色填满。
 *
 * @param x1 第一个顶点的横坐标。
 * @param y1 第一个顶点的纵坐标。
 * @param x2 第二个顶点的横坐标。
 * @param y2 第二个顶点的纵坐标。
 * @param x3 第三个顶点的横坐标。
 * @param y3 第三个顶点的纵坐标。
 */
void bgt_fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3);

/** @brief 设置线条宽度。
 *
 * width 小于 1 时会按 1 处理。该设置主要影响直线和图形边框。
 *
 * @param width 线宽（像素）。
 */
void bgt_set_line_width(int width);

/** @brief 返回当前线条宽度。
 *
 * @return 当前线宽（像素）。
 */
int bgt_get_line_width();

/** @} */

/** @name 图片
 * 加载图片，以及图片的平移（T）、旋转（R）、缩放（S）变换。
 * @{
 */
/** @brief 从文件加载一张图片。
 *
 * 支持 PNG、JPG、BMP 等常见格式。filename 是按 UTF-8 解释的图片文件路径。
 * 请在主循环外加载图片：同一文件多次加载会得到多个互相独立的编号。
 * 图片由库统一管理，程序结束时自动释放；关闭窗口后所有编号失效，需要重新
 * 加载。
 *
 * @param filename 图片文件路径（UTF-8 编码）。
 * @return 加载成功返回一个大于 0 的图片编号；失败返回 BGT_IMAGE_NONE（0），此时可以调用 bgt_print_error() 查看原因。
 * @see bgt_draw_image
 */
int bgt_load_image(const char filename[]);

/** @brief 返回图片的原始宽度，单位是像素。
 *
 * @param image_id bgt_load_image() 返回的图片编号。
 * @return 图片的原始宽度；image_id 无效时返回 0。
 */
int bgt_image_width(int image_id);

/** @brief 返回图片的原始高度，单位是像素。
 *
 * @param image_id bgt_load_image() 返回的图片编号。
 * @return 图片的原始高度；image_id 无效时返回 0。
 */
int bgt_image_height(int image_id);

/** @brief 设置图片相对绘制位置的平移偏移量（T 变换）。
 *
 * bgt_draw_image() 的 (x, y) 加上这里的 (dx, dy) 得到最终的平移位置：
 * 图片的局部原点（左上角）会落在 (x + dx, y + dy)。默认 (0, 0)（无偏移）。
 *
 * @param image_id bgt_load_image() 返回的图片编号。
 * @param dx 横向偏移量（像素）。
 * @param dy 纵向偏移量（像素）。
 * @note image_id 无效时不修改并记录错误。
 */
void bgt_translate_image(int image_id, int dx, int dy);

/** @brief 设置图片的旋转角度（R 变换）。
 *
 * angle 单位是度，正值表示顺时针。旋转围绕图片的局部原点（左上角）进行：
 * 旋转后左上角位置不变，图片整体绕它转动。默认 0（不旋转）。
 *
 * @param image_id bgt_load_image() 返回的图片编号。
 * @param angle 旋转角度（度，顺时针为正）。
 * @note image_id 无效时不修改并记录错误。
 */
void bgt_rotate_image(int image_id, double angle);

/** @brief 设置图片的缩放倍率（S 变换）。
 *
 * sx、sy 是缩放因子：负值表示翻转。缩放围绕图片的局部原点（左上角）进行：
 * 缩放后左上角位置不变。默认 (1.0, 1.0)（原始大小）。
 *
 * @param image_id bgt_load_image() 返回的图片编号。
 * @param sx 横向缩放因子。
 * @param sy 纵向缩放因子。
 * @note image_id 无效时不修改并记录错误。
 */
void bgt_scale_image(int image_id, double sx, double sy);

/** @brief 清除图片的全部变换状态，恢复为恒等变换。
 *
 * 清除后图片回到刚加载时的样子。
 *
 * @param image_id bgt_load_image() 返回的图片编号。
 * @note image_id 无效时不修改并记录错误。
 */
void bgt_clear_image_transform(int image_id);

/** @brief 在基准位置 (x, y) 绘制图片，应用该图当前的全部变换状态。
 *
 * 变换的合成顺序为：先缩放（S），再旋转（R），最后平移（T），即
 * M = T(x + dx, y + dy) * R(angle) * S(sx, sy)。图片的局部原点（左上角）
 * 最终落在 (x + dx, y + dy)。图片自带的透明部分会正确地与背景混合；绘制
 * 图片不受 bgt_set_color() 设置的当前颜色影响。
 *
 * @param image_id bgt_load_image() 返回的图片编号。
 * @param x 基准位置的横坐标。
 * @param y 基准位置的纵坐标。
 * @note image_id 无效时不绘制并记录错误。
 */
void bgt_draw_image(int image_id, int x, int y);

/** @} */

/** @name 文本与字体
 * 字体、字号与文本绘制、文本尺寸测量。
 * @{
 */
/** @brief 设置后续文本绘制使用的字体文件和默认字号。
 *
 * filename 是字体文件路径，size 是默认字号。若不调用该函数，库会尝试使用
 * 系统自带的中文字体（Windows 下通常是微软雅黑）；找不到可用字体时文本
 * 绘制会失败并记录错误，此时应调用该函数指定可用字体。
 *
 * @param filename 字体文件路径（UTF-8 编码）。
 * @param size 默认字号。
 * @return 成功返回 true；失败返回 false。
 */
bool bgt_set_font(const char filename[], int size);

/** @brief 设置默认字号。
 *
 * 后续不带 size 参数的 bgt_draw_text()、bgt_text_width()、
 * bgt_text_height() 会使用这个字号。
 *
 * @param size 默认字号，应为正数。
 */
void bgt_set_font_size(int size);

/** @brief 返回当前默认字号。
 *
 * @return 当前默认字号。
 */
int bgt_get_font_size();

/** @brief 在 (x, y) 位置绘制文本，使用当前绘图颜色和默认字号。
 *
 * text 按 UTF-8 解释，因此可以直接绘制中文字符串。这里的 (x, y) 表示文字
 * 左上角。
 *
 * @param x 文字左上角的横坐标。
 * @param y 文字左上角的纵坐标。
 * @param text 要绘制的文本（UTF-8 编码）。
 */
void bgt_draw_text(int x, int y, const char text[]);

/** @brief 在 (x, y) 位置用指定字号绘制文本。
 *
 * 该函数只影响本次绘制，不改变默认字号。
 *
 * @param x 文字左上角的横坐标。
 * @param y 文字左上角的纵坐标。
 * @param text 要绘制的文本（UTF-8 编码）。
 * @param size 本次绘制使用的字号。
 */
void bgt_draw_text(int x, int y, const char text[], int size);

/** @brief 测量文本使用默认字号绘制时需要的宽度，单位是像素。
 *
 * 常用于文字居中、对齐或判断文字是否超出窗口。
 *
 * @param text 要测量的文本（UTF-8 编码）。
 * @return 文本宽度（像素）。
 */
int bgt_text_width(const char text[]);

/** @brief 测量文本使用指定字号绘制时需要的宽度，单位是像素。
 *
 * @param text 要测量的文本（UTF-8 编码）。
 * @param size 测量时使用的字号。
 * @return 文本宽度（像素）。
 */
int bgt_text_width(const char text[], int size);

/** @brief 测量文本使用默认字号绘制时需要的高度，单位是像素。
 *
 * @param text 要测量的文本（UTF-8 编码）。
 * @return 文本高度（像素）。
 */
int bgt_text_height(const char text[]);

/** @brief 测量文本使用指定字号绘制时需要的高度，单位是像素。
 *
 * @param text 要测量的文本（UTF-8 编码）。
 * @param size 测量时使用的字号。
 * @return 文本高度（像素）。
 */
int bgt_text_height(const char text[], int size);

/** @} */

/** @name 键盘
 * 按键按下、刚按下与刚松开的查询。
 * @{
 */
/** @brief 判断某个键当前是否正被按住。
 *
 * key 使用 BGT_KEY_LEFT、BGT_KEY_A 等按键常量。左右 Shift/Ctrl/Alt 会分别
 * 合并为 BGT_KEY_SHIFT、BGT_KEY_CTRL、BGT_KEY_ALT。小键盘在 Num Lock 开启
 * 时会映射为数字键；关闭时 2/4/6/8 会映射为方向键。适合持续移动这类
 * “按住就一直生效”的操作。
 *
 * @param key 按键常量（BGT_KEY_*）。
 * @return 按住返回 true，否则返回 false。
 */
bool bgt_key_is_down(int key);

/** @brief 判断某个键是否在当前帧刚刚按下。
 *
 * 按住不放时只会在第一次按下的那一帧返回 true，按键合并和小键盘映射规则
 * 与 bgt_key_is_down() 相同。适合发射子弹、切换状态等只触发一次的操作。
 *
 * @param key 按键常量（BGT_KEY_*）。
 * @return 当前帧刚刚按下返回 true，否则返回 false。
 */
bool bgt_key_just_pressed(int key);

/** @brief 判断某个键是否在当前帧刚刚松开。
 *
 * 按键合并和小键盘映射规则与 bgt_key_is_down() 相同，适合处理“松手时确认”
 * 这类操作。
 *
 * @param key 按键常量（BGT_KEY_*）。
 * @return 当前帧刚刚松开返回 true，否则返回 false。
 */
bool bgt_key_just_released(int key);

/** @} */

/** @name 鼠标
 * 鼠标位置、按键状态、滚轮与光标显示。
 * @{
 */
/** @brief 返回鼠标当前 x 坐标。
 *
 * 坐标值使用窗口逻辑坐标。
 *
 * @return 鼠标的横坐标。
 */
int bgt_mouse_x();

/** @brief 返回鼠标当前 y 坐标。
 *
 * 坐标值使用窗口逻辑坐标。
 *
 * @return 鼠标的纵坐标。
 */
int bgt_mouse_y();

/** @brief 判断鼠标某个按键当前是否正被按住。
 *
 * button 使用 BGT_MOUSE_LEFT、BGT_MOUSE_RIGHT 或 BGT_MOUSE_MIDDLE。
 *
 * @param button 鼠标按键常量（BGT_MOUSE_*）。
 * @return 按住返回 true，否则返回 false。
 */
bool bgt_mouse_is_down(int button);

/** @brief 判断鼠标某个按键是否在当前帧刚刚按下。
 *
 * 适合处理点击开始、开始拖拽等操作。
 *
 * @param button 鼠标按键常量（BGT_MOUSE_*）。
 * @return 当前帧刚刚按下返回 true，否则返回 false。
 */
bool bgt_mouse_just_pressed(int button);

/** @brief 判断鼠标某个按键是否在当前帧刚刚松开。
 *
 * 适合处理点击完成、结束拖拽等操作。
 *
 * @param button 鼠标按键常量（BGT_MOUSE_*）。
 * @return 当前帧刚刚松开返回 true，否则返回 false。
 */
bool bgt_mouse_just_released(int button);

/** @brief 返回当前帧鼠标滚轮的滚动量。
 *
 * 正负方向由 SDL3 平台事件决定；没有滚动时返回 0。该值会在每次
 * bgt_update_window() 时更新。
 *
 * @return 鼠标滚轮的滚动量。
 */
int bgt_mouse_wheel();

/** @brief 显示鼠标光标。
 */
void bgt_show_mouse();

/** @brief 隐藏鼠标光标。
 */
void bgt_hide_mouse();

/** @} */

/** @name 时间与帧率
 * 延时、帧率限制与时间查询。
 * @{
 */
/** @brief 暂停程序指定的毫秒数。
 *
 * milliseconds 是毫秒，例如 1000 表示约 1 秒。
 *
 * @param milliseconds 暂停的毫秒数。
 */
void bgt_delay(int milliseconds);

/** @brief 设置帧率上限。
 *
 * fps 大于 0 时，bgt_update_window() 会尝试把帧率限制到该值；fps 小于等于
 * 0 时表示不限制帧率。
 *
 * @param fps 目标帧率上限。
 */
void bgt_set_fps_limit(int fps);

/** @brief 返回上一帧到当前帧经过的时间，单位是秒。
 *
 * 常用于与帧率无关的动画，例如 x = x + speed * bgt_delta_time()。
 *
 * @return 上一帧到当前帧的时间（秒）。
 */
double bgt_delta_time();

/** @brief 返回窗口创建以来经过的总时间，单位是秒。
 *
 * 常用于计时器和周期动画。
 *
 * @return 窗口创建以来经过的总时间（秒）。
 */
double bgt_total_time();

/** @brief 返回最近统计到的实际帧率。
 *
 * 刚开始运行时可能为 0，因为还没有足够帧数用于统计。
 *
 * @return 最近统计到的实际帧率。
 */
double bgt_fps();

/** @} */

/** @name 随机数
 * 随机整数、随机浮点数与随机数种子。
 * @{
 */
/** @brief 设置随机数种子。
 *
 * 设置相同种子后，程序接下来产生的随机数序列每次运行都完全相同，适合调试
 * 和课堂演示“可复现的随机”。不设置种子时，库会自动播种，因此每次运行得到
 * 的随机数都不同。通常不需要手动调用该函数。
 *
 * @param seed 随机数种子。
 */
void bgt_random_seed(unsigned seed);

/** @brief 返回一个 [min, max) 半开区间内的随机整数。
 *
 * 包含 min、不包含 max。例如 bgt_random(0, 6) 可能得到 0~5，正好当长度为 6
 * 的数组下标用；掷六面骰要写 bgt_random(1, 7)。min 大于 max 时会自动交换
 * 两者；min 等于 max 时直接返回该值。
 *
 * @param min 区间下界（包含）。
 * @param max 区间上界（不包含）。
 * @return 区间内的随机整数。
 */
int bgt_random(int min, int max);

/** @brief 返回一个 [min, max) 半开区间内的随机浮点数。
 *
 * 包含 min、不包含 max。想要小数时两个参数都要写小数：bgt_random(0.0, 1.0)
 * 正确，bgt_random(0, 1) 会调用整数版本。min 大于 max 时会自动交换两者；
 * min 等于 max 时直接返回该值。
 *
 * @param min 区间下界（包含）。
 * @param max 区间上界（不包含）。
 * @return 区间内的随机浮点数。
 */
double bgt_random(double min, double max);

/** @} */

/** @name 碰撞检测
 * 点、矩形、圆之间的命中与重叠判断。
 * @{
 */
/** @brief 判断点 (px, py) 是否在矩形上。
 *
 * x、y 是矩形左上角，width、height 是宽高。命中范围与 bgt_fill_rect() 实际
 * 画出的像素完全一致：包含左、上边界，不包含右、下边界（即
 * x <= px < x + width）。width 或 height 小于等于 0 时矩形是“空的”，恒返回
 * false。
 *
 * @param px 点的横坐标。
 * @param py 点的纵坐标。
 * @param x 矩形左上角横坐标。
 * @param y 矩形左上角纵坐标。
 * @param width 矩形宽度。
 * @param height 矩形高度。
 * @return 点在矩形内返回 true，否则返回 false。
 */
bool bgt_hit_point_rect(int px, int py, int x, int y, int width, int height);

/** @brief 判断点 (px, py) 是否在圆上。
 *
 * x、y 是圆心，radius 是半径。点到圆心的距离不超过 radius 就算命中（圆周
 * 上的点算）。radius 小于等于 0 时恒返回 false。
 *
 * @param px 点的横坐标。
 * @param py 点的纵坐标。
 * @param x 圆心横坐标。
 * @param y 圆心纵坐标。
 * @param radius 圆的半径。
 * @return 点在圆内或圆周上返回 true，否则返回 false。
 */
bool bgt_hit_point_circle(int px, int py, int x, int y, int radius);

/** @brief 判断两个矩形是否实际重叠。
 *
 * 只有真实共有一部分面积才返回 true；恰好贴边、贴角不算。宽或高小于等于 0
 * 的矩形是“空的”，恒返回 false。
 *
 * @param x1 第一个矩形左上角横坐标。
 * @param y1 第一个矩形左上角纵坐标。
 * @param width1 第一个矩形宽度。
 * @param height1 第一个矩形高度。
 * @param x2 第二个矩形左上角横坐标。
 * @param y2 第二个矩形左上角纵坐标。
 * @param width2 第二个矩形宽度。
 * @param height2 第二个矩形高度。
 * @return 两个矩形实际重叠返回 true，否则返回 false。
 */
bool bgt_hit_rect_rect(int x1, int y1, int width1, int height1,
                       int x2, int y2, int width2, int height2);

/** @brief 判断两个圆是否实际重叠。
 *
 * 只有真实重叠才返回 true；恰好相切不算。radius1 或 radius2 小于等于 0 时恒返回 false。
 *
 * @param x1 第一个圆的圆心横坐标。
 * @param y1 第一个圆的圆心纵坐标。
 * @param radius1 第一个圆的半径。
 * @param x2 第二个圆的圆心横坐标。
 * @param y2 第二个圆的圆心纵坐标。
 * @param radius2 第二个圆的半径。
 * @return 两个圆实际重叠返回 true，否则返回 false。
 */
bool bgt_hit_circle_circle(int x1, int y1, int radius1, int x2, int y2,
                           int radius2);

/** @brief 判断圆与矩形是否实际重叠。
 *
 * (cx, cy) 是圆心，radius 是半径；(x, y, width, height) 描述矩形。圆心在
 * 矩形内部（含边上）算命中；圆在矩形外侧恰好相切不算。radius 或宽、高
 * 小于等于 0 时恒返回 false。
 *
 * @param cx 圆心的横坐标。
 * @param cy 圆心的纵坐标。
 * @param radius 圆的半径。
 * @param x 矩形左上角横坐标。
 * @param y 矩形左上角纵坐标。
 * @param width 矩形宽度。
 * @param height 矩形高度。
 * @return 圆与矩形实际重叠返回 true，否则返回 false。
 */
bool bgt_hit_circle_rect(int cx, int cy, int radius, int x, int y,
                         int width, int height);

/** @} */

/** @name 存档
 * 内存存档表的读写，以及存档文件与普通文件的存取。
 * @{
 */
/** @brief 把存档文件读入内存存档表。
 *
 * 之前内存表里的内容会被整体替换。存档文件是普通文本文件，每一行是
 * “键=值”，并用 [节名] 行把键分组，可以用记事本直接打开查看或修改。
 * filename 是文件名，通常写 “save.txt” 这样的相对路径。文件不存在时不会
 * 出错：内存表变为空表并返回 true，适合游戏第一次运行的情况。
 *
 * @param filename 存档文件名。
 * @return 成功返回 true；文件存在但读不开、或里面有格式不对的行时返回 false 并记录错误（格式不对的行会被跳过，其他行照常读入）。
 */
bool bgt_load(const char filename[]);

/** @brief 把内存存档表写入存档文件。
 *
 * 之前用 bgt_set_int() 等函数放进表里的数据会一次性全部写入。库会先写一个
 * 临时文件、成功后再替换存档文件，避免写一半把旧存档弄坏。
 *
 * @param filename 存档文件名。
 * @return 成功返回 true；失败返回 false 并记录错误。
 */
bool bgt_save(const char filename[]);

/** @brief 判断文件是否存在。
 *
 * 常用于判断是不是第一次运行，例如 if (!bgt_file_exists(“save.txt”)) { ... }。
 *
 * @param filename 文件名。
 * @return 文件存在返回 true，否则返回 false。
 */
bool bgt_file_exists(const char filename[]);

/** @brief 在内存存档表中写入一个整数。
 *
 * section 是节名，key 是键名，例如 bgt_set_int(“最高分”, “best”, 100)。
 * 同一个 (节, 键) 已有值时会被覆盖。
 *
 * @param section 节名。
 * @param key 键名。
 * @param value 要写入的整数值。
 */
void bgt_set_int(const char section[], const char key[], int value);

/** @brief 在内存存档表中写入一个小数。
 *
 * 用法与 bgt_set_int() 相同。
 *
 * @param section 节名。
 * @param key 键名。
 * @param value 要写入的小数值。
 */
void bgt_set_double(const char section[], const char key[], double value);

/** @brief 在内存存档表中写入一个字符串。
 *
 * value 可以直接写中文字符串字面量，例如 bgt_set_string(“玩家”, “name”,
 * “张三”)。字符串首尾的空白会被去掉。
 *
 * @param section 节名。
 * @param key 键名。
 * @param value 要写入的字符串。
 * @warning 去掉首尾空白后仍含换行的字符串会记录错误、不存入。
 */
void bgt_set_string(const char section[], const char key[], const char value[]);

/** @brief 从内存存档表中读取一个整数。
 *
 * 节或键不存在时返回 default_value，不会出错。键里存的值必须能按整数理解：
 * 如果存的是 45.5 这样的小数文本，就读不出来，返回 default_value 并记录错误。
 *
 * @param section 节名。
 * @param key 键名。
 * @param default_value 节或键不存在时返回的默认值。
 * @return 读到的整数；读不出来时返回 default_value。
 */
int bgt_get_int(const char section[], const char key[], int default_value);

/** @brief 从内存存档表中读取一个小数。
 *
 * 整数文本可以按小数读出（存 100 能读出 100.0），反向不行。节或键不存在时
 * 返回 default_value，不会出错。
 *
 * @param section 节名。
 * @param key 键名。
 * @param default_value 节或键不存在时返回的默认值。
 * @return 读到的小数；读不出来时返回 default_value。
 */
double bgt_get_double(const char section[], const char key[],
                      double default_value);

/** @brief 从内存存档表中读取一个字符串，写进 out 数组。
 *
 * out_size 是 out 的大小，例如 char name[32]; bgt_get_string(“玩家”, “name”,
 * name, 32, “无名”)。节或键不存在时写入 default_value。字符串加结束符放不
 * 进 out_size 时，只写入放得下的部分（保证是完整的中文）并记录错误。
 *
 * @param section 节名。
 * @param key 键名。
 * @param[out] out 接收字符串的字符数组。
 * @param out_size out 数组的大小（字节数）。
 * @param default_value 节或键不存在时写入的默认字符串。
 */
void bgt_get_string(const char section[], const char key[],
                    char out[], int out_size, const char default_value[]);

/** @} */

/** @name 声音
 * 音效与背景音乐的加载和播放。
 * @{
 */
/** @brief 从文件加载一段音效，返回声音编号。
 *
 * WAV/OGG/MP3 都可以。音效会一直留在内存里，关闭窗口或程序退出时统一
 * 清理，不需要（也没有）释放函数。通常在程序开头把所有音效加载完，存进
 * int 变量备用。
 *
 * @param filename 音效文件路径（UTF-8 编码）。
 * @return 编号大于 0 表示成功；加载失败返回 0 并记录错误，此时可以调用 bgt_print_error() 查看原因。
 * @see bgt_play_sound
 */
int bgt_load_sound(const char filename[]);

/** @brief 播放一次编号对应的音效。
 *
 * 连按连响：每次调用都是一次新的发声，多个音效会自动混音重叠。
 *
 * @param id bgt_load_sound() 返回的声音编号。
 * @note id 无效（0 或没加载过）时记录错误，不播放。
 */
void bgt_play_sound(int id);

/** @brief 设置某个音效的音量。
 *
 * volume 取 0 到 100：0 静音，100 最大，超出范围的值会被收到边界。影响这个
 * 音效之后每次播放的音量；正在响的发声不会变。
 *
 * @param id bgt_load_sound() 返回的声音编号。
 * @param volume 音量（0 到 100）。
 */
void bgt_set_sound_volume(int id, int volume);

/** @brief 播放一段背景音乐，默认无限循环。
 *
 * WAV/OGG/MP3 都可以。整个程序同一时刻只有一首音乐：正在播放时再调用本
 * 函数会自动切到新曲子。音乐是流式播放的，内存占用和音乐文件长度无关。
 *
 * @param filename 音乐文件路径（UTF-8 编码）。
 * @return 成功返回 true；文件不存在、格式不支持等失败返回 false 并记录错误。
 * @see bgt_stop_music
 */
bool bgt_play_music(const char filename[]);

/** @brief 停止背景音乐。
 *
 * 没有音乐在播时调用它没有任何效果，也不报错。
 */
void bgt_stop_music();

/** @brief 设置背景音乐的音量，0 到 100。
 *
 * 规则与音效音量相同，立即生效。
 *
 * @param volume 音量（0 到 100）。
 */
void bgt_set_music_volume(int volume);

/** @} */

/** @name 错误诊断
 * 错误历史的查询、打印与绘制。
 * @{
 */
/** @brief 判断库内部是否记录了错误（错误历史里有至少一条）。
 *
 * 通常在某个返回 false 的函数之后调用。
 *
 * @return 错误历史非空返回 true，否则返回 false。
 */
bool bgt_has_error();

/** @brief 返回错误历史中的条数。
 *
 * 历史最多保留 10 条：更早的错误会被最老的挤出。bgt_clear_error() 会清空全部历史。
 *
 * @return 错误历史中的条数。
 */
int bgt_error_count();

/** @brief 按序号返回历史中某条错误的错误码。
 *
 * 0 是最老的一条，bgt_error_count() - 1 是最新的一条。错误码是 BGT_ERROR_*
 * 常量。序号越界（含历史为空）时返回 BGT_ERROR_NONE（0），不会产生新错误。
 *
 * @param index 错误序号（从 0 开始）。
 * @return 对应的错误码；序号越界时返回 BGT_ERROR_NONE。
 */
int bgt_error_code(int index);

/** @brief 返回最新一条错误的错误码。
 *
 * 历史为空时返回 BGT_ERROR_NONE。等价于 bgt_error_code(bgt_error_count() - 1)。
 *
 * @return 最新一条错误的错误码。
 */
int bgt_error_code();

/** @brief 按序号把历史中某条错误的消息文本复制进 out 数组。
 *
 * 最多放 out_size - 1 个字节加结束符，放不下时按 UTF-8 字符边界安全截断。
 * 序号越界时 out 得到空串。查询不产生新错误，截断也是静默的。
 *
 * @param index 错误序号（从 0 开始）。
 * @param[out] out 接收消息文本的字符数组。
 * @param out_size out 数组的大小（字节数）。
 */
void bgt_error_text(int index, char out[], int out_size);

/** @brief 把历史中第 index 条错误打印到标准错误输出。
 *
 * 格式与库的报告一致。序号越界时什么都不打印。
 *
 * @param index 错误序号（从 0 开始）。
 */
void bgt_print_error(int index);

/** @brief 把最新一条错误信息打印到标准错误输出。
 *
 * 主要用于调试和示例程序中的简单错误报告。等价于
 * bgt_print_error(bgt_error_count() - 1)。
 */
void bgt_print_error();

/** @brief 把历史中第 index 条错误绘制到窗口中。
 *
 * (x, y) 是文字左上角，size 是字号，绘制颜色使用当前绘图颜色。消息太长时
 * 会自动按窗口宽度换行，逐行向下画。序号越界时什么都不画。
 *
 * @param x 文字左上角的横坐标。
 * @param y 文字左上角的纵坐标。
 * @param size 字号。
 * @param index 错误序号（从 0 开始）。
 */
void bgt_draw_error(int x, int y, int size, int index);

/** @brief 把最新一条错误信息绘制到窗口中。
 *
 * 等价于 bgt_draw_error(x, y, size, bgt_error_count() - 1)。
 *
 * @param x 文字左上角的横坐标。
 * @param y 文字左上角的纵坐标。
 * @param size 字号。
 */
void bgt_draw_error(int x, int y, int size);

/** @brief 清除全部错误历史。
 *
 * 清除后 bgt_has_error() 返回 false、bgt_error_count() 返回 0。
 */
void bgt_clear_error();

/** @} */

// NOLINTEND(readability-magic-numbers, modernize-avoid-c-arrays,
// bugprone-easily-swappable-parameters, readability-identifier-length)

#endif
