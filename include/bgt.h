#ifndef BGT_H_
#define BGT_H_

// The beginner-facing API intentionally uses integer constants, C-style string
// parameters, and short coordinate names such as x/y.
// NOLINTBEGIN(readability-magic-numbers, modernize-avoid-c-arrays,
// bugprone-easily-swappable-parameters, readability-identifier-length)

#define BGT_VERSION_MAJOR 0
#define BGT_VERSION_MINOR 1
#define BGT_VERSION_PATCH 0

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

#define BGT_MOUSE_LEFT 1
#define BGT_MOUSE_RIGHT 2
#define BGT_MOUSE_MIDDLE 3

#define BGT_IMAGE_NONE 0

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

// 创建一个固定大小的图形窗口，并初始化 libbgt 内部需要的 SDL3 与
// SDL3_ttf 资源。width 和 height 是窗口的逻辑绘图尺寸，title 是窗口标题，
// 可以直接写中文字符串。创建成功返回 true；失败返回 false，此时可以调用
// bgt_print_error() 查看原因。程序中通常只需要创建一个窗口。
bool bgt_open_window(int width, int height, const char title[]);

// 创建一个允许用户拖拽改变大小的图形窗口。绘图仍然使用 width 和 height
// 指定的逻辑坐标，库内部会把画面缩放到真实窗口大小。其他行为与
// bgt_open_window() 相同。
bool bgt_open_window_resizable(int width, int height, const char title[]);

// 判断窗口是否仍然处于打开状态。主循环一般写成
// while (bgt_window_is_open()) { ... }。当用户点击关闭按钮，或库检测到关闭
// 事件时，该函数返回 false。
bool bgt_window_is_open();

// 刷新窗口画面，并处理键盘、鼠标、窗口关闭等系统事件。每一帧绘图结束后
// 都应该调用一次该函数；bgt_key_just_pressed()、bgt_mouse_just_pressed()
// 这类“刚按下” 的状态也会以这个函数为帧边界进行更新。
void bgt_update_window();

// 关闭窗口并释放 libbgt 内部持有的 SDL3、SDL3_ttf、字体等资源。程序正常
// 结束时资源也会被清理，但教学示例中建议显式调用，帮助形成完整结构。
void bgt_close_window();

// 返回当前窗口的逻辑宽度，也就是 bgt_open_window() 创建窗口时传入的 width。
// 即使窗口被缩放，这个值也表示绘图坐标系中的宽度。
int bgt_window_width();

// 返回当前窗口的逻辑高度，也就是 bgt_open_window() 创建窗口时传入的 height。
// 即使窗口被缩放，这个值也表示绘图坐标系中的高度。
int bgt_window_height();

// 修改窗口标题。title 可以是 UTF-8 中文字符串；如果窗口尚未创建，该函数不做
// 任何事情。
void bgt_set_window_title(const char title[]);

// 使用红、绿、蓝三个颜色分量创建一个不透明颜色。每个分量建议取 0 到 255；
// 超出范围的值会被限制到有效范围。返回值是一个无符号整数颜色值，可以传给
// bgt_set_color() 或 bgt_set_background()。
unsigned bgt_rgb(int r, int g, int b);

// 使用红、绿、蓝、透明度四个分量创建颜色。a 表示不透明度：0 是完全透明，
// 255 是完全不透明。返回值是一个无符号整数颜色值，可以作为普通颜色使用。
unsigned bgt_rgba(int r, int g, int b, int a);

// 设置当前绘图颜色。后续的点、线、矩形、圆、文字等绘制函数都会使用这个颜色，
// 直到再次调用 bgt_set_color() 改变它。color 可以是 BGT_* 颜色常量、bgt_rgb()
// 或 bgt_rgba() 的返回值。
void bgt_set_color(unsigned color);

// 返回当前绘图颜色。这个函数通常用于临时保存颜色，之后再恢复原来的颜色。
unsigned bgt_get_color();

// 设置画布底色。libbgt 会在每次 bgt_update_window() 结束时自动把画布清空成
// 这个颜色，因此不需要（也没有）专门"清屏"的函数：每帧把要画的内容画完整即可，
// 没画到的部分就是背景色。默认背景色是黑色；通常在进入主循环之前调用一次。
void bgt_set_background(unsigned color);

// 在 (x, y) 位置绘制一个点。坐标系左上角是 (0, 0)，x 向右增加，y 向下增加。
// 如果点在窗口外部，库会自动忽略它。
void bgt_draw_point(int x, int y);

// 从 (x1, y1) 到 (x2, y2) 绘制一条直线，颜色使用当前绘图颜色。线宽由
// bgt_set_line_width() 控制。
void bgt_draw_line(int x1, int y1, int x2, int y2);

// 绘制矩形边框。(x, y) 是矩形左上角，width 是宽度，height 是高度。矩形内部
// 不会被填充。
void bgt_draw_rect(int x, int y, int width, int height);

// 绘制填充矩形。(x, y) 是矩形左上角，width 是宽度，height 是高度。整个矩形
// 区域都会使用当前颜色填满。
void bgt_fill_rect(int x, int y, int width, int height);

// 绘制圆形边框。(x, y) 是圆心，radius 是半径。圆内部不会被填充。
void bgt_draw_circle(int x, int y, int radius);

// 绘制填充圆。(x, y) 是圆心，radius 是半径。整个圆形区域都会使用当前颜色填满。
void bgt_fill_circle(int x, int y, int radius);

// 绘制椭圆边框。(x, y) 是椭圆中心，radius_x 是横向半径，radius_y 是纵向半径。
// 椭圆内部不会被填充。
void bgt_draw_ellipse(int x, int y, int radius_x, int radius_y);

// 绘制填充椭圆。(x, y) 是椭圆中心，radius_x 是横向半径，radius_y 是纵向半径。
// 整个椭圆区域都会使用当前颜色填满。
void bgt_fill_ellipse(int x, int y, int radius_x, int radius_y);

// 绘制三角形边框。三个点分别是 (x1, y1)、(x2, y2)、(x3, y3)。三角形内部
// 不会被填充。
void bgt_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3);

// 绘制填充三角形。三个点分别是 (x1, y1)、(x2, y2)、(x3, y3)。三角形内部
// 会使用当前颜色填满。
void bgt_fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3);

// 设置线条宽度。width 小于 1 时会按 1 处理。该设置主要影响直线和图形边框。
void bgt_set_line_width(int width);

// 返回当前线条宽度。
int bgt_get_line_width();

// 从文件加载一张图片，支持 PNG、JPG、BMP 等常见格式。filename 是按 UTF-8 解释的图片文件路径。
// 加载成功返回一个大于 0 的图片编号，失败返回 BGT_IMAGE_NONE（0），此时可调用 bgt_print_error() 查看原因。
// 请在主循环外加载图片：同一文件多次加载会得到多个互相独立的编号。
// 图片由库统一管理，程序结束时自动释放；关闭窗口后所有编号失效，需要重新加载。
int bgt_load_image(const char filename[]);

// 返回图片的原始宽度，单位是像素。image_id 无效时返回 0。
int bgt_image_width(int image_id);

// 返回图片的原始高度，单位是像素。image_id 无效时返回 0。
int bgt_image_height(int image_id);

// T（平移）：设置图片相对绘制位置的偏移量。bgt_draw_image 的 (x, y) 加上
// 这里的 (dx, dy) 得到最终的平移位置：图片的局部原点（左上角）会落在
// (x + dx, y + dy)。默认 (0, 0)（无偏移）。image_id 无效时不修改并记录错误。
void bgt_translate_image(int image_id, int dx, int dy);

// R（旋转）：设置图片的旋转角度。angle 单位是度，正值表示顺时针。
// 旋转围绕图片的局部原点（左上角）进行——旋转后左上角位置不变，
// 图片整体绕它转动。默认 0（不旋转）。image_id 无效时不修改并记录错误。
void bgt_rotate_image(int image_id, double angle);

// S（缩放）：设置图片的缩放倍率。sx、sy 是缩放因子：
// 缩放围绕图片的局部原点（左上角）进行——缩放后左上角位置不变。
// 默认 (1.0, 1.0)（原始大小）。image_id 无效时不修改并记录错误。
void bgt_scale_image(int image_id, double sx, double sy);

// 清除图片的全部变换状态，恢复为恒等变换。
// 清除后图片回到刚加载时的样子。image_id 无效时不修改并记录错误。
void bgt_clear_image_transform(int image_id);

// 在基准位置 (x, y) 绘制图片，应用该图当前的全部变换状态。
// 变换的合成顺序为：先缩放（S），再旋转（R），最后平移（T），
// 即 M = T(x + dx, y + dy) * R(angle) * S(sx, sy)。
// 图片的局部原点（左上角）最终落在 (x + dx, y + dy)。
// 图片自带的透明部分会正确地与背景混合；绘制图片不受 bgt_set_color()
// 设置的当前颜色影响。image_id 无效时不绘制并记录错误。
void bgt_draw_image(int image_id, int x, int y);

// 设置后续文本绘制使用的字体文件和默认字号。filename 是字体文件路径，size 是
// 默认字号。成功返回 true；失败返回 false。若不调用该函数，库会尝试使用系统
// 自带的中文字体（Windows 下通常是微软雅黑）；找不到可用字体时文本绘制会失败
// 并记录错误，此时应调用该函数指定可用字体。
bool bgt_set_font(const char filename[], int size);

// 设置默认字号。后续不带 size 参数的 bgt_draw_text()、bgt_text_width()、
// bgt_text_height() 会使用这个字号。size 应为正数。
void bgt_set_font_size(int size);

// 返回当前默认字号。
int bgt_get_font_size();

// 在 (x, y) 位置绘制文本，使用当前绘图颜色和默认字号。text 按 UTF-8 解释，
// 因此可以直接绘制中文字符串。这里的 (x, y) 表示文字左上角。
void bgt_draw_text(int x, int y, const char text[]);

// 在 (x, y) 位置用指定字号绘制文本。该函数只影响本次绘制，不改变默认字号。
void bgt_draw_text(int x, int y, const char text[], int size);

// 测量文本使用默认字号绘制时需要的宽度，单位是像素。常用于文字居中、对齐或
// 判断文字是否超出窗口。
int bgt_text_width(const char text[]);

// 测量文本使用指定字号绘制时需要的宽度，单位是像素。
int bgt_text_width(const char text[], int size);

// 测量文本使用默认字号绘制时需要的高度，单位是像素。
int bgt_text_height(const char text[]);

// 测量文本使用指定字号绘制时需要的高度，单位是像素。
int bgt_text_height(const char text[], int size);

// 判断某个键当前是否正被按住。key 使用 BGT_KEY_LEFT、BGT_KEY_A 等按键常量。
// 左右 Shift/Ctrl/Alt 会分别合并为 BGT_KEY_SHIFT、BGT_KEY_CTRL、BGT_KEY_ALT。
// 小键盘在 Num Lock 开启时会映射为数字键；关闭时 2/4/6/8 会映射为方向键。
// 适合持续移动这类“按住就一直生效”的操作。
bool bgt_key_is_down(int key);

// 判断某个键是否在当前帧刚刚按下。按住不放时只会在第一次按下的那一帧返回
// true，按键合并和小键盘映射规则与 bgt_key_is_down() 相同。适合发射子弹、
// 切换状态等只触发一次的操作。
bool bgt_key_just_pressed(int key);

// 判断某个键是否在当前帧刚刚松开。按键合并和小键盘映射规则与
// bgt_key_is_down() 相同，适合处理“松手时确认”这类操作。
bool bgt_key_just_released(int key);

// 返回鼠标当前 x 坐标，坐标值使用窗口逻辑坐标。
int bgt_mouse_x();

// 返回鼠标当前 y 坐标，坐标值使用窗口逻辑坐标。
int bgt_mouse_y();

// 判断鼠标某个按键当前是否正被按住。button 使用 BGT_MOUSE_LEFT、
// BGT_MOUSE_RIGHT 或 BGT_MOUSE_MIDDLE。
bool bgt_mouse_is_down(int button);

// 判断鼠标某个按键是否在当前帧刚刚按下。适合处理点击开始、开始拖拽等操作。
bool bgt_mouse_just_pressed(int button);

// 判断鼠标某个按键是否在当前帧刚刚松开。适合处理点击完成、结束拖拽等操作。
bool bgt_mouse_just_released(int button);

// 返回当前帧鼠标滚轮的滚动量。正负方向由 SDL3 平台事件决定；没有滚动时返回 0。
// 该值会在每次 bgt_update_window() 时更新。
int bgt_mouse_wheel();

// 显示鼠标光标。
void bgt_show_mouse();

// 隐藏鼠标光标。
void bgt_hide_mouse();

// 暂停程序指定的毫秒数。milliseconds 是毫秒，例如 1000 表示约 1 秒。
void bgt_delay(int milliseconds);

// 设置帧率上限。fps 大于 0 时，bgt_update_window() 会尝试把帧率限制到该值；
// fps 小于等于 0 时表示不限制帧率。
void bgt_set_fps_limit(int fps);

// 返回上一帧到当前帧经过的时间，单位是秒。常用于与帧率无关的动画，例如
// x = x + speed * bgt_delta_time()。
double bgt_delta_time();

// 返回窗口创建以来经过的总时间，单位是秒。常用于计时器和周期动画。
double bgt_total_time();

// 返回最近统计到的实际帧率。刚开始运行时可能为 0，因为还没有足够帧数用于统计。
double bgt_fps();

// 设置随机数种子。设置相同种子后，程序接下来产生的随机数序列每次运行都完全
// 相同，适合调试和课堂演示"可复现的随机"。不设置种子时，库会自动播种，因此
// 每次运行得到的随机数都不同。通常不需要手动调用该函数。
void bgt_random_seed(unsigned seed);

// 返回一个 [min, max) 半开区间内的随机整数：包含 min、不包含 max。
// 例如 bgt_random(0, 6) 可能得到 0~5，正好当长度为 6 的数组下标用；
// 掷六面骰要写 bgt_random(1, 7)。min 大于 max 时会自动交换两者；
// min 等于 max 时直接返回该值。
int bgt_random(int min, int max);

// 返回一个 [min, max) 半开区间内的随机浮点数：包含 min、不包含 max。
// 想要小数时两个参数都要写小数：bgt_random(0.0, 1.0) 正确，
// bgt_random(0, 1) 会调用整数版本。min 大于 max 时会自动交换两者；
// min 等于 max 时直接返回该值。
double bgt_random(double min, double max);

// 判断点 (px, py) 是否在矩形上：x、y 是矩形左上角，width、height 是宽高。
// 命中范围与 fill_rect 实际画出的像素完全一致——包含左、上边界，不包含
// 右、下边界（即 x <= px < x + width）。width 或 height 小于等于 0 时矩形
// 是"空的"，恒返回 false。
bool bgt_hit_point_rect(int px, int py, int x, int y, int width, int height);

// 判断点 (px, py) 是否在圆上：x、y 是圆心，radius 是半径。点到圆心的
// 距离不超过 radius 就算命中（圆周上的点算）。radius 小于等于 0 时恒返回
// false。
bool bgt_hit_point_circle(int px, int py, int x, int y, int radius);

// 判断两个矩形是否实际重叠。只有真实共有一部分面积才返回 true；
// 恰好贴边、贴角不算。宽或高小于等于 0 的矩形是"空的"，恒返回 false。
bool bgt_hit_rect_rect(int x1, int y1, int width1, int height1,
                       int x2, int y2, int width2, int height2);

// 判断两个圆是否实际重叠。只有真实重叠才返回 true；恰好相切不算。
// radius1 或 radius2 小于等于 0 时恒返回 false。
bool bgt_hit_circle_circle(int x1, int y1, int radius1, int x2, int y2,
                           int radius2);

// 判断圆与矩形是否实际重叠。(cx, cy) 是圆心，radius 是半径；
// (x, y, width, height) 描述矩形。圆心在矩形内部（含边上）算命中；
// 圆在矩形外侧恰好相切不算。radius 或宽、高小于等于 0 时恒返回 false。
bool bgt_hit_circle_rect(int cx, int cy, int radius, int x, int y,
                         int width, int height);

// 把存档文件读入内存存档表，之前内存表里的内容会被整体替换。存档文件是
// 普通文本文件，每一行是“键=值”，并用 [节名] 行把键分组，可以用记事本
// 直接打开查看或修改。filename 是文件名，通常写 "save.txt" 这样的相对
// 路径。文件不存在时不会出错：内存表变为空表并返回 true，适合游戏第一
// 次运行的情况。成功返回 true；文件存在但读不开、或里面有格式不对的行
// 时返回 false 并记录错误（格式不对的行会被跳过，其他行照常读入）。
bool bgt_load(const char filename[]);

// 把内存存档表写入存档文件。之前用 bgt_set_int() 等函数放进表里的数据
// 会一次性全部写入。库会先写一个临时文件、成功后再替换存档文件，避免
// 写一半把旧存档弄坏。成功返回 true；失败返回 false 并记录错误。
bool bgt_save(const char filename[]);

// 判断文件是否存在。常用于判断是不是第一次运行，例如
// if (!bgt_file_exists("save.txt")) { ... }。
bool bgt_file_exists(const char filename[]);

// 在内存存档表中写入一个整数。section 是节名，key 是键名，例如
// bgt_set_int("最高分", "best", 100)。同一个 (节, 键) 已有值时会被覆盖。
void bgt_set_int(const char section[], const char key[], int value);

// 在内存存档表中写入一个小数，用法与 bgt_set_int() 相同。
void bgt_set_double(const char section[], const char key[], double value);

// 在内存存档表中写入一个字符串，value 可以直接写中文字符串字面量，例如
// bgt_set_string("玩家", "name", "张三")。字符串首尾的空白会被去掉；
// 去掉首尾空白后仍含换行的字符串会记录错误、不存入。
void bgt_set_string(const char section[], const char key[], const char value[]);

// 从内存存档表中读取一个整数。节或键不存在时返回 default_value，不会
// 出错。键里存的值必须能按整数理解：如果存的是 45.5 这样的小数文本，
// 就读不出来，返回 default_value 并记录错误。
int bgt_get_int(const char section[], const char key[], int default_value);

// 从内存存档表中读取一个小数。整数文本可以按小数读出（存 100 能读出
// 100.0），反向不行。节或键不存在时返回 default_value，不会出错。
double bgt_get_double(const char section[], const char key[],
                      double default_value);

// 从内存存档表中读取一个字符串，写进 out 数组。out_size 是 out 的大小，
// 例如 char name[32]; bgt_get_string("玩家", "name", name, 32, "无名");
// 节或键不存在时写入 default_value。字符串加结束符放不进 out_size 时，
// 只写入放得下的部分（保证是完整的中文）并记录错误。
void bgt_get_string(const char section[], const char key[],
                    char out[], int out_size, const char default_value[]);

// 从文件加载一段音效（WAV/OGG/MP3 都可以），返回声音编号。编号大于 0 表示
// 成功；加载失败返回 0 并记录错误，此时可以调用 bgt_print_error() 查看原因。
// 音效会一直留在内存里，关闭窗口或程序退出时统一清理，
// 不需要（也没有）释放函数。
// 通常在程序开头把所有音效加载完，存进 int 变量备用。
int bgt_load_sound(const char filename[]);

// 播放一次编号对应的音效。连按连响：每次调用都是一次新的发声，多个音效
// 会自动混音重叠。id 无效（0 或没加载过）时记录错误，不播放。
void bgt_play_sound(int id);

// 设置某个音效的音量，volume 取 0 到 100：0 静音，100 最大，超出范围的值
// 会被收到边界。影响这个音效之后每次播放的音量；正在响的发声不会变。
void bgt_set_sound_volume(int id, int volume);

// 播放一段背景音乐（WAV/OGG/MP3 都可以），默认无限循环。整个程序同一时刻
// 只有一首音乐：正在播放时再调用本函数会自动切到新曲子。音乐是流式播放的，
// 内存占用和音乐文件长度无关。成功返回 true；文件不存在、格式不支持等
// 失败返回 false 并记录错误。
bool bgt_play_music(const char filename[]);

// 停止背景音乐。没有音乐在播时调用它没有任何效果，也不报错。
void bgt_stop_music();

// 设置背景音乐的音量，0 到 100，规则与音效音量相同。立即生效。
void bgt_set_music_volume(int volume);

// 判断库内部是否记录了错误（错误历史里有至少一条）。通常在某个返回
// false 的函数之后调用。
bool bgt_has_error();

// 返回错误历史中的条数。历史最多保留 10 条：更早的错误会被最老的挤出。
// bgt_clear_error() 会清空全部历史。
int bgt_error_count();

// 按序号返回历史中某条错误的错误码：0 是最老的一条，
// bgt_error_count() - 1 是最新的一条。错误码是 BGT_ERROR_* 常量。
// 序号越界（含历史为空）时返回 BGT_ERROR_NONE（0），不会产生新错误。
int bgt_error_code(int index);

// 返回最新一条错误的错误码；历史为空时返回 BGT_ERROR_NONE。
// 等价于 bgt_error_code(bgt_error_count() - 1)。
int bgt_error_code();

// 按序号把历史中某条错误的消息文本复制进 out 数组：最多放 out_size - 1
// 个字节加结束符，放不下时按 UTF-8 字符边界安全截断。序号越界时 out 得到
// 空串。查询不产生新错误，截断也是静默的。
void bgt_error_text(int index, char out[], int out_size);

// 把历史中第 index 条错误打印到标准错误输出，格式与库的报告一致。
// 序号越界时什么都不打印。
void bgt_print_error(int index);

// 把最新一条错误信息打印到标准错误输出。这个函数主要用于调试和示例
// 程序中的简单错误报告。等价于 bgt_print_error(bgt_error_count() - 1)。
void bgt_print_error();

// 把历史中第 index 条错误绘制到窗口中。(x, y) 是文字左上角，size 是
// 字号，绘制颜色使用当前绘图颜色。消息太长时会自动按窗口宽度换行，
// 逐行向下画。序号越界时什么都不画。
void bgt_draw_error(int x, int y, int size, int index);

// 把最新一条错误信息绘制到窗口中。等价于
// bgt_draw_error(x, y, size, bgt_error_count() - 1)。
void bgt_draw_error(int x, int y, int size);

// 清除全部错误历史。清除后 bgt_has_error() 返回 false、
// bgt_error_count() 返回 0。
void bgt_clear_error();

// NOLINTEND(readability-magic-numbers, modernize-avoid-c-arrays,
// bugprone-easily-swappable-parameters, readability-identifier-length)

#endif
