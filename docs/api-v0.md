# libbgt v0.1 API 文档

本文档描述 `libbgt` 最初版本的基础函数式接口。该版本面向 C++ 初学阶段，接口统一使用 `snake_case` 和 `bgt_` 前缀。

首版 API 不包含类、对象、结构体、图片、音频、网格、UI、场景和高级资源系统。

## 1. 基本约定

头文件：

```cpp
#include "bgt.h"
```

命名：

- 函数使用 `bgt_` 前缀，例如 `bgt_open_window`。
- 函数使用 `snake_case`。
- 常量使用 `BGT_` 前缀，例如 `BGT_WHITE`。

坐标：

- 左上角是 `(0, 0)`。
- x 向右增加。
- y 向下增加。
- 坐标和尺寸使用 `int`。

文本：

- 字符串按 UTF-8 处理。
- 默认使用系统中文字体，也可用 `bgt_set_font()` 指定字体文件。
- 示例源码应保存为 UTF-8。
- Windows MSVC 构建应启用 `/utf-8`。

## 2. 最小程序

```cpp
#include "bgt.h"

int main()
{
    bgt_open_window(800, 600, "你好 libbgt");
    bgt_set_background(BGT_WHITE);

    while (bgt_window_is_open()) {
        bgt_set_color(BGT_BLUE);
        bgt_fill_circle(400, 300, 50);

        bgt_set_color(BGT_BLACK);
        bgt_draw_text(300, 220, "你好，世界！", 32);

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}
```

## 3. 窗口

```cpp
bool bgt_open_window(int width, int height, const char title[]);
bool bgt_open_window_resizable(int width, int height, const char title[]);
bool bgt_window_is_open();
void bgt_update_window();
void bgt_close_window();

int bgt_window_width();
int bgt_window_height();
void bgt_set_window_title(const char title[]);
```

### `bgt_open_window`

创建窗口并初始化库。

```cpp
bool bgt_open_window(int width, int height, const char title[]);
```

参数：

- `width`：窗口宽度。
- `height`：窗口高度。
- `title`：窗口标题，使用 UTF-8 字符串。

返回：

- 成功返回 `true`。
- 失败返回 `false`，可用 `bgt_print_error()` 输出错误信息。

### `bgt_open_window_resizable`

创建可调整大小的窗口。

```cpp
bool bgt_open_window_resizable(int width, int height, const char title[]);
```

说明：

- 逻辑绘图尺寸仍以创建窗口时的宽高为准。
- 窗口缩放的具体映射策略由库内部处理。

### `bgt_window_is_open`

判断窗口是否仍然打开。

```cpp
bool bgt_window_is_open();
```

常用于主循环：

```cpp
while (bgt_window_is_open()) {
    bgt_update_window();
}
```

### `bgt_update_window`

刷新窗口画面，并处理键盘、鼠标和关闭窗口等事件。

```cpp
void bgt_update_window();
```

说明：

- 每一帧通常调用一次。
- 每次调用结束时，库会先把画布清空成背景色（见第 5 节），因此下一帧直接从
  空白画布开始绘制。
- `bgt_key_just_pressed` 和 `bgt_mouse_just_pressed` 等“刚按下”状态以帧为单位更新。

### `bgt_close_window`

关闭窗口并释放库内部资源。

```cpp
void bgt_close_window();
```

说明：

- 程序正常结束时也会自动清理。
- 仍建议在示例中显式调用，帮助学生形成完整程序结构。

## 4. 颜色

### 4.1 颜色常量

```cpp
BGT_BLACK
BGT_WHITE
BGT_RED
BGT_GREEN
BGT_BLUE
BGT_YELLOW
BGT_CYAN
BGT_MAGENTA
BGT_GRAY
BGT_LIGHT_GRAY
BGT_DARK_GRAY
BGT_ORANGE
BGT_PINK
BGT_PURPLE
BGT_BROWN
BGT_TRANSPARENT
```

### 4.2 颜色函数

```cpp
unsigned bgt_rgb(int r, int g, int b);
unsigned bgt_rgba(int r, int g, int b, int a);
void bgt_set_color(unsigned color);
unsigned bgt_get_color();
```

颜色值统一使用无符号整数 `unsigned`，与颜色常量类型一致，可以直接放进数组：

```cpp
unsigned palette[3] = {BGT_RED, BGT_BLUE, BGT_GREEN};
```

### `bgt_rgb`

创建不透明颜色。

```cpp
int orange = bgt_rgb(255, 128, 0);
```

参数范围：

- `r`：红色分量，建议 `0` 到 `255`。
- `g`：绿色分量，建议 `0` 到 `255`。
- `b`：蓝色分量，建议 `0` 到 `255`。

### `bgt_rgba`

创建带透明度的颜色。

```cpp
int half_red = bgt_rgba(255, 0, 0, 128);
```

参数范围：

- `a`：透明度分量，`0` 表示完全透明，`255` 表示完全不透明。

### `bgt_set_color`

设置当前绘图颜色。

```cpp
bgt_set_color(BGT_RED);
bgt_fill_circle(100, 100, 30);
```

## 5. 背景色

```cpp
void bgt_set_background(unsigned color);
```

### `bgt_set_background`

设置画布底色。

```cpp
bgt_set_background(BGT_WHITE);
```

说明：

- libbgt 在每次 `bgt_update_window()` 结束后自动把画布清空成背景色，因此
  不需要（也没有）专门"清屏"的函数。
- 每帧把要画的内容画完整即可；没画到的部分就是背景色。
- 默认背景色是黑色；通常在进入主循环之前设置一次。

## 6. 基本绘图

所有基础绘图函数使用当前颜色。

```cpp
void bgt_draw_point(int x, int y);
void bgt_draw_line(int x1, int y1, int x2, int y2);

void bgt_draw_rect(int x, int y, int width, int height);
void bgt_fill_rect(int x, int y, int width, int height);

void bgt_draw_circle(int x, int y, int radius);
void bgt_fill_circle(int x, int y, int radius);

void bgt_draw_ellipse(int x, int y, int radius_x, int radius_y);
void bgt_fill_ellipse(int x, int y, int radius_x, int radius_y);

void bgt_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3);
void bgt_fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3);

void bgt_set_line_width(int width);
int bgt_get_line_width();
```

### 示例

```cpp
bgt_set_color(BGT_RED);
bgt_draw_line(100, 100, 300, 200);

bgt_set_color(BGT_BLUE);
bgt_fill_rect(350, 100, 120, 80);

bgt_set_color(BGT_GREEN);
bgt_fill_circle(200, 350, 50);
```

### 说明

- 矩形的 `x`、`y` 表示左上角。
- 圆形和椭圆的 `x`、`y` 表示中心点。
- `width`、`height`、`radius` 小于等于 `0` 时，函数不绘制任何内容。
- 超出窗口范围的部分自动裁剪。

## 7. 文本

```cpp
bool bgt_set_font(const char filename[], int size);
void bgt_set_font_size(int size);
int bgt_get_font_size();

void bgt_draw_text(int x, int y, const char text[]);
void bgt_draw_text(int x, int y, const char text[], int size);

int bgt_text_width(const char text[]);
int bgt_text_width(const char text[], int size);
int bgt_text_height(const char text[]);
int bgt_text_height(const char text[], int size);
```

### `bgt_draw_text`

绘制 UTF-8 文本。

```cpp
bgt_set_color(BGT_BLACK);
bgt_draw_text(100, 100, "你好，libbgt！", 32);
```

说明：

- 默认字体支持中文。
- `x`、`y` 表示文本左上角。
- 文本颜色使用当前颜色。
- `size` 表示字号，单位为像素。

### `bgt_set_font`

设置字体文件和默认字号。

```cpp
bool bgt_set_font(const char filename[], int size);
```

说明：

- 如果不调用该函数，库使用系统自带的中文字体（Windows 下通常是微软雅黑）。
- 系统找不到可用字体时，文本绘制会失败并记录错误，此时应调用该函数指定可用字体。
- 字体文件路径按 UTF-8 处理。
- 加载失败返回 `false`。

## 8. 键盘

```cpp
bool bgt_key_is_down(int key);
bool bgt_key_just_pressed(int key);
bool bgt_key_just_released(int key);
```

### 8.1 按键常量

```cpp
BGT_KEY_LEFT
BGT_KEY_RIGHT
BGT_KEY_UP
BGT_KEY_DOWN
BGT_KEY_SPACE
BGT_KEY_ENTER
BGT_KEY_ESCAPE
BGT_KEY_TAB
BGT_KEY_BACKSPACE
BGT_KEY_SHIFT
BGT_KEY_CTRL
BGT_KEY_ALT

BGT_KEY_A ... BGT_KEY_Z
BGT_KEY_0 ... BGT_KEY_9
BGT_KEY_F1 ... BGT_KEY_F12
```

左右 Shift、Ctrl、Alt 会分别合并为 `BGT_KEY_SHIFT`、`BGT_KEY_CTRL`、
`BGT_KEY_ALT`。小键盘在 Num Lock 开启时映射为数字键；关闭时 2/4/6/8
映射为方向键。

### 8.2 函数语义

`bgt_key_is_down` 表示按键当前是否正在按住。

```cpp
if (bgt_key_is_down(BGT_KEY_LEFT)) {
    x = x - 5;
}
```

`bgt_key_just_pressed` 表示按键是否在当前帧刚刚按下。

```cpp
if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
    score = score + 1;
}
```

`bgt_key_just_released` 表示按键是否在当前帧刚刚松开。

## 9. 鼠标

```cpp
int bgt_mouse_x();
int bgt_mouse_y();

bool bgt_mouse_is_down(int button);
bool bgt_mouse_just_pressed(int button);
bool bgt_mouse_just_released(int button);

int bgt_mouse_wheel();
void bgt_show_mouse();
void bgt_hide_mouse();
```

### 9.1 鼠标按键常量

```cpp
BGT_MOUSE_LEFT
BGT_MOUSE_RIGHT
BGT_MOUSE_MIDDLE
```

### 9.2 示例

```cpp
if (bgt_mouse_is_down(BGT_MOUSE_LEFT)) {
    bgt_fill_circle(bgt_mouse_x(), bgt_mouse_y(), 5);
}
```

说明：

- 鼠标坐标使用窗口逻辑坐标。
- `bgt_mouse_wheel()` 返回上一帧到当前帧的滚轮变化量。

## 10. 时间

```cpp
void bgt_delay(int milliseconds);
void bgt_set_fps_limit(int fps);
double bgt_delta_time();
double bgt_total_time();
double bgt_fps();
```

### `bgt_delay`

暂停指定毫秒数。

```cpp
bgt_delay(16);
```

### `bgt_set_fps_limit`

设置帧率上限。

```cpp
bgt_set_fps_limit(60);
```

说明：

- `fps <= 0` 表示不限制帧率。
- 帧率限制可由 `bgt_update_window()` 内部执行。

### `bgt_delta_time`

返回上一帧到当前帧经过的秒数。

```cpp
x = x + int(200 * bgt_delta_time());
```

## 11. 错误信息

```cpp
bool bgt_has_error();
int bgt_error_code();
void bgt_print_error();
void bgt_draw_error(int x, int y, int size);
void bgt_clear_error();
```

### 示例

```cpp
if (!bgt_open_window(800, 600, "窗口")) {
    bgt_print_error();
    return 1;
}
```

进阶示例：

```cpp
if (bgt_has_error()) {
    bgt_set_color(BGT_RED);
    bgt_draw_error(10, 10, 20);
}
```

说明：

- 首版不要求学生理解异常。
- 错误文本主要给教师、调试和文档使用。
- 基础 API 不返回错误字符串指针。
- v0.3 起错误保留最近 10 条历史，可按序号逐条查询，详见 [v0.3 API 文档](api-v0.3.md)。

## 12. 首版完整 API 清单

```cpp
bool bgt_open_window(int width, int height, const char title[]);
bool bgt_open_window_resizable(int width, int height, const char title[]);
bool bgt_window_is_open();
void bgt_update_window();
void bgt_close_window();

int bgt_window_width();
int bgt_window_height();
void bgt_set_window_title(const char title[]);

unsigned bgt_rgb(int r, int g, int b);
unsigned bgt_rgba(int r, int g, int b, int a);
void bgt_set_color(unsigned color);
unsigned bgt_get_color();

void bgt_set_background(unsigned color);

void bgt_draw_point(int x, int y);
void bgt_draw_line(int x1, int y1, int x2, int y2);
void bgt_draw_rect(int x, int y, int width, int height);
void bgt_fill_rect(int x, int y, int width, int height);
void bgt_draw_circle(int x, int y, int radius);
void bgt_fill_circle(int x, int y, int radius);
void bgt_draw_ellipse(int x, int y, int radius_x, int radius_y);
void bgt_fill_ellipse(int x, int y, int radius_x, int radius_y);
void bgt_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3);
void bgt_fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3);
void bgt_set_line_width(int width);
int bgt_get_line_width();

bool bgt_set_font(const char filename[], int size);
void bgt_set_font_size(int size);
int bgt_get_font_size();
void bgt_draw_text(int x, int y, const char text[]);
void bgt_draw_text(int x, int y, const char text[], int size);
int bgt_text_width(const char text[]);
int bgt_text_width(const char text[], int size);
int bgt_text_height(const char text[]);
int bgt_text_height(const char text[], int size);

bool bgt_key_is_down(int key);
bool bgt_key_just_pressed(int key);
bool bgt_key_just_released(int key);

int bgt_mouse_x();
int bgt_mouse_y();
bool bgt_mouse_is_down(int button);
bool bgt_mouse_just_pressed(int button);
bool bgt_mouse_just_released(int button);
int bgt_mouse_wheel();
void bgt_show_mouse();
void bgt_hide_mouse();

void bgt_delay(int milliseconds);
void bgt_set_fps_limit(int fps);
double bgt_delta_time();
double bgt_total_time();
double bgt_fps();

bool bgt_has_error();
int bgt_error_code();
void bgt_print_error();
void bgt_draw_error(int x, int y, int size);
void bgt_clear_error();
```

## 13. 首版常量清单

```cpp
BGT_VERSION_MAJOR
BGT_VERSION_MINOR
BGT_VERSION_PATCH

BGT_BLACK
BGT_WHITE
BGT_RED
BGT_GREEN
BGT_BLUE
BGT_YELLOW
BGT_CYAN
BGT_MAGENTA
BGT_GRAY
BGT_LIGHT_GRAY
BGT_DARK_GRAY
BGT_ORANGE
BGT_PINK
BGT_PURPLE
BGT_BROWN
BGT_TRANSPARENT

BGT_KEY_LEFT
BGT_KEY_RIGHT
BGT_KEY_UP
BGT_KEY_DOWN
BGT_KEY_SPACE
BGT_KEY_ENTER
BGT_KEY_ESCAPE
BGT_KEY_TAB
BGT_KEY_BACKSPACE
BGT_KEY_SHIFT
BGT_KEY_CTRL
BGT_KEY_ALT
BGT_KEY_A ... BGT_KEY_Z
BGT_KEY_0 ... BGT_KEY_9
BGT_KEY_F1 ... BGT_KEY_F12

BGT_MOUSE_LEFT
BGT_MOUSE_RIGHT
BGT_MOUSE_MIDDLE

BGT_ERROR_NONE
BGT_ERROR_SDL
BGT_ERROR_TTF
BGT_ERROR_WINDOW
BGT_ERROR_RENDERER
BGT_ERROR_FONT
BGT_ERROR_NOT_OPEN
```

错误码可通过 `bgt_error_code()` 读取。面向学生的示例通常只需要使用 `bgt_print_error()`。

版本号宏 `BGT_VERSION_MAJOR`、`BGT_VERSION_MINOR`、`BGT_VERSION_PATCH`
表示库的当前版本，当前为 `0.1.0`。

## 14. 不属于 v0.1 的能力

以下能力刻意不在首版提供：

网格绘制、单元格填充、图片加载、图片绘制、声音播放、按钮控件、场景管理和 tilemap 绘制。

这些功能要么属于后续版本，要么适合作为学生练习实现。
