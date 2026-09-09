// =====================================================================
// libbgt 入门演示 —— 面向初学者的板块式 API 速查
//
// 发给同学后可以直接运行。每个板块介绍一小块 API，按【空格】进入下一
// 板块，随时可以按【Esc】退出。
//
//   板块 1  窗口与主循环
//   板块 2  颜色
//   板块 3  基本图形
//   板块 4  文本
//   板块 5  键盘
//   板块 6  鼠标
//   板块 7  时间
//   板块 8  错误信息
//
// 写作约定（与课程作业一致）：
//   - 不出现类、结构体、指针、std::string；
//   - 常量用 const int 声明（本文件刻意不使用 constexpr）；
//   - 颜色值用 unsigned 存放（颜色常量本身就是无符号整数，见板块 2）。
// =====================================================================

#include "bgt.h"

#include <cmath>
#include <cstdio>

// NOLINTBEGIN(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity)

int main()
{
    // ---------- 常量（const int，本文件不用 constexpr）----------
    const int window_width = 1000;   // 窗口宽度
    const int window_height = 700;   // 窗口高度
    const int target_fps = 60;       // 帧率上限

    // ---------- 板块 1 的一部分：创建窗口 ----------
    // bgt_open_window 失败时返回 false，用 bgt_print_error() 查看原因。
    // 想让用户拖拽放大缩小窗口时，把 bgt_open_window 换成
    // bgt_open_window_resizable 即可（布局仍按窗口尺寸绘制）。
    if (!bgt_open_window(window_width, window_height, "libbgt 入门演示")) {
        bgt_print_error();
        return 1;
    }

    // 限制帧率（板块 7 会详细介绍时间相关的接口）
    bgt_set_fps_limit(target_fps);

    // ---------- 各板块需要的"跨帧状态"（就是普通变量）----------

    int part = 1;                    // 当前板块号：1 到 8

    // 板块 5（键盘）使用
    int ball_x = 400;                // 小球位置
    int ball_y = 320;
    int tap_count = 0;               // 点按方向键的次数
    int release_count = 0;           // 松开 ↑ 的次数

    // 板块 6（鼠标）使用
    int dot_radius = 15;             // 左键画圆的半径（滚轮调整）
    int click_x = 400;               // 最近一次左键点击的位置
    int click_y = 300;
    bool has_click = false;          // 是否点击过
    bool cursor_hidden = false;      // 光标是否已隐藏

    // 右键拖拽画圆：按下时记下起点；松开后保留一个实心圆
    int press_x = 400;               // 右键按下的起点
    int press_y = 300;
    int released_circle_x = 0;       // 松开右键后留下的圆
    int released_circle_y = 0;
    int released_circle_radius = 0;
    bool has_released_circle = false;

    // 板块 7（时间）使用
    double bar_x = 0.0;              // 移动条的位置
    bool bar_moving_right = true;    // 移动条当前的方向

    // 板块 8（错误信息）使用
    bool error_printed = false;      // bgt_print_error 只打印一次

    // ---------- 主循环：每个板块都在这里轮流演示 ----------
    while (bgt_window_is_open()) {

        // 随时退出
        if (bgt_key_just_pressed(BGT_KEY_ESCAPE)) {
            bgt_close_window();
            break;
        }

        if (part == 1) {
            // =====================================================
            // 板块 1：窗口与主循环
            // =====================================================
            bgt_set_background(bgt_rgb(255, 248, 235));
            bgt_set_window_title("libbgt 入门演示 - 板块 1：窗口与主循环");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 1：窗口与主循环", 32);
            bgt_draw_text(40, 94, "整个程序的外壳就是下面这个 while 循环：", 20);
            bgt_draw_text(40, 134,
                          "while (bgt_window_is_open()) { 读输入; 画图; "
                          "bgt_update_window(); }", 20);
            bgt_draw_text(40, 174,
                          "bgt_window_is_open() 在用户点了关闭按钮后变成 false，"
                          "循环结束。", 20);
            bgt_draw_text(40, 214,
                          "bgt_update_window() 把画面显示到窗口，并接收键盘、"
                          "鼠标、关窗等事件。", 20);
            bgt_draw_text(40, 254,
                          "bgt_close_window() 释放资源；也可以不写，程序结束时"
                          "会自动清理。", 20);
            bgt_draw_text(40, 288,
                          "画布由库负责：每帧更新后自动清成背景色"
                          "（bgt_set_background），循环里只管画内容。", 20);

            // 查询窗口尺寸（逻辑坐标）。把数字变成文字用 sprintf ——
            // 用法和 printf 一样，只是把结果写进数组而不是屏幕。
            char size_text[80];
            std::snprintf(size_text, sizeof(size_text),
                          "当前窗口大小：%d x %d（bgt_window_width / height）",
                          bgt_window_width(), bgt_window_height());
            bgt_draw_text(40, 314, size_text, 20);

            // 标题可以随时修改：本演示每个板块进入时，都会把板块名写进标题栏。
            bgt_draw_text(40, 354, "看窗口标题栏：每个板块会把自己的名字写进去。", 20);

        } else if (part == 2) {
            // =====================================================
            // 板块 2：颜色
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 入门演示 - 板块 2：颜色");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 2：颜色", 32);
            bgt_draw_text(40, 94,
                          "预定义颜色常量：BGT_RED、BGT_BLUE……（共 16 种，"
                          "见 docs/api-v0.md）。", 18);
            bgt_draw_text(40, 126,
                          "颜色常量是无符号整数，所以存放颜色的数组/变量要用"
                          " unsigned：", 18);

            // 调色板：颜色放数组要用 unsigned
            const unsigned palette[6] = {BGT_RED, BGT_ORANGE, BGT_YELLOW,
                                         BGT_GREEN, BGT_BLUE, BGT_PURPLE};
            for (int i = 0; i < 6; ++i) {
                bgt_set_color(palette[i]);
                bgt_fill_rect(80 + i * 140, 170, 90, 60);
            }

            // 自定义颜色由分量合成：bgt_rgb(r, g, b)
            bgt_draw_text(40, 270,
                          "自定义颜色：bgt_rgb 不透明 / bgt_rgba 带透明度。", 18);
            bgt_set_color(bgt_rgb(255, 190, 120));
            bgt_fill_rect(80, 300, 300, 60);

            // 半透明图形叠在灰白棋盘格上，透明度与叠加效果一目了然
            const int board_x = 430;           // 棋盘格左上角
            const int board_y = 300;
            const int board_cell = 24;         // 每格边长
            for (int cy = board_y; cy < board_y + 144; cy = cy + board_cell) {
                for (int cx = board_x; cx < board_x + 432; cx = cx + board_cell) {
                    const int cell_pos =
                        ((cx - board_x) + (cy - board_y)) / board_cell;
                    if (cell_pos % 2 == 0) {
                        bgt_set_color(BGT_WHITE);
                    } else {
                        bgt_set_color(BGT_LIGHT_GRAY);
                    }
                    bgt_fill_rect(cx, cy, board_cell, board_cell);
                }
            }

            // 几个半透明图形互相重叠，颜色会叠出新的层次
            bgt_set_color(bgt_rgba(0, 120, 255, 120));    // 蓝色矩形
            bgt_fill_rect(450, 330, 380, 80);
            bgt_set_color(bgt_rgba(255, 40, 40, 140));    // 红色圆形
            bgt_fill_circle(600, 385, 56);
            bgt_set_color(bgt_rgba(40, 200, 90, 120));    // 绿色椭圆
            bgt_fill_ellipse(720, 360, 78, 48);
            bgt_set_color(bgt_rgba(240, 200, 40, 100));   // 黄色三角形
            bgt_fill_triangle(480, 420, 620, 330, 750, 420);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(456, 306, "半透明叠加：先画的在下面", 16);
            bgt_draw_text(40, 470,
                          "bgt_rgba 的第四个参数是透明度：0 全透明，255 不透明。",
                          18);

            // bgt_get_color 取出当前颜色，配合保存/恢复
            const unsigned old_color = bgt_get_color();
            bgt_set_color(BGT_YELLOW);
            bgt_fill_rect(80, 520, 500, 50);             // 先换成黄色
            bgt_set_color(old_color);                    // 再恢复原来的颜色
            bgt_draw_text(80, 590,
                          "bgt_get_color() 取出当前颜色，bgt_set_color() 修改。",
                          18);

        } else if (part == 3) {
            // =====================================================
            // 板块 3：基本图形
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 入门演示 - 板块 3：基本图形");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 3：基本图形", 32);
            bgt_draw_text(40, 94,
                          "所有图形都使用当前颜色（bgt_set_color），"
                          "先设置颜色再画。", 18);
            bgt_draw_text(40, 122,
                          "小提示：半径或尺寸 <= 0 时函数什么都不画；"
                          "超出窗口的部分会自动裁剪。", 18);

            // 点
            bgt_draw_text(50, 140, "点：bgt_draw_point", 18);
            bgt_set_color(BGT_BLUE);
            for (int i = 0; i < 8; ++i) {
                bgt_draw_point(60 + i * 35, 190);
            }

            // 线（线宽可变）
            bgt_draw_text(400, 140, "线：bgt_draw_line，线宽可调", 18);
            bgt_set_color(BGT_RED);
            bgt_set_line_width(1);
            bgt_draw_line(420, 170, 700, 190);
            bgt_set_line_width(4);
            bgt_draw_line(420, 200, 700, 220);
            bgt_set_line_width(8);
            bgt_draw_line(420, 230, 700, 250);
            // bgt_get_line_width() 可以把线宽读回来。

            // 矩形
            bgt_draw_text(50, 280, "矩形：draw_rect 边框 / fill_rect 填充", 18);
            bgt_set_color(BGT_PURPLE);
            bgt_draw_rect(60, 310, 120, 80);
            bgt_fill_rect(200, 310, 120, 80);

            // 圆与椭圆
            bgt_draw_text(400, 280, "圆是圆心 + 半径；椭圆是两个半径", 18);
            bgt_set_color(BGT_GREEN);
            bgt_draw_circle(460, 350, 40);
            bgt_fill_circle(560, 350, 40);
            bgt_set_color(BGT_ORANGE);
            bgt_fill_ellipse(690, 350, 45, 30);

            // 三角形
            bgt_draw_text(50, 420, "三角形：三对坐标", 18);
            bgt_set_color(BGT_CYAN);
            bgt_draw_triangle(60, 500, 130, 460, 200, 500);
            bgt_fill_triangle(240, 500, 310, 460, 380, 500);

            bgt_set_line_width(1);   // 恢复默认线宽

        } else if (part == 4) {
            // =====================================================
            // 板块 4：文本
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 入门演示 - 板块 4：文本");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 4：文本", 32);
            bgt_draw_text(40, 94,
                          "bgt_draw_text(x, y, 文字, 字号)，(x, y) 是文字左上角。",
                          18);

            // 不带字号时使用默认字号
            bgt_set_font_size(24);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(60, 150,
                          "这一行使用默认字号，由 bgt_set_font_size(24) 设置。");
            bgt_set_color(BGT_BLUE);
            bgt_draw_text(60, 205, "这一行指定字号：更大。", 36);

            // 用 bgt_text_width / bgt_text_height 把文字水平居中
            const char center_text[] =
                "这行文字用 bgt_text_width() 计算后水平居中";
            const int text_w = bgt_text_width(center_text, 28);
            const int text_h = bgt_text_height(center_text, 28);
            const int center_x = (window_width - text_w) / 2;
            bgt_set_color(BGT_GRAY);
            bgt_fill_rect(center_x - 16, 290, text_w + 32, text_h + 20);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(center_x, 300, center_text, 28);

            bgt_draw_text(60, 400,
                          "文字可以直接写中文（源文件保存为 UTF-8），"
                          "默认使用系统中文字体。", 20);
            bgt_draw_text(60, 440,
                          "想指定字体文件时用 bgt_set_font(路径, 字号)；"
                          "打不开会返回 false（见板块 8）。", 20);

        } else if (part == 5) {
            // =====================================================
            // 板块 5：键盘
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 入门演示 - 板块 5：键盘");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 5：键盘", 32);
            bgt_draw_text(40, 94,
                          "按住 ←/→：小球持续移动（is_down）。", 20);
            bgt_draw_text(40, 128,
                          "点按 ←/→：次数 +1（just_pressed，一帧只算一次）。", 20);
            bgt_draw_text(40, 162,
                          "松开 ↑：次数 +1（just_released）。", 20);

            // 持续状态：按住就每帧都移动
            if (bgt_key_is_down(BGT_KEY_LEFT)) {
                ball_x = ball_x - 5;
            }
            if (bgt_key_is_down(BGT_KEY_RIGHT)) {
                ball_x = ball_x + 5;
            }
            // 瞬时事件：这一帧刚按下才触发一次
            if (bgt_key_just_pressed(BGT_KEY_LEFT)) {
                tap_count = tap_count + 1;
            }
            if (bgt_key_just_pressed(BGT_KEY_RIGHT)) {
                tap_count = tap_count + 1;
            }
            if (bgt_key_just_released(BGT_KEY_UP)) {
                release_count = release_count + 1;
            }

            // 不越界
            if (ball_x < 40) {
                ball_x = 40;
            }
            if (ball_x > 960) {
                ball_x = 960;
            }

            bgt_set_color(BGT_BLUE);
            bgt_fill_circle(ball_x, ball_y, 30);

            bgt_set_color(BGT_BLACK);
            char tap_text[64];
            std::snprintf(tap_text, sizeof(tap_text),
                          "点按 ←/→ 次数：%d（just_pressed）", tap_count);
            bgt_draw_text(60, 430, tap_text, 20);
            char release_text[64];
            std::snprintf(release_text, sizeof(release_text),
                          "松开 ↑ 次数：%d（just_released）", release_count);
            bgt_draw_text(60, 470, release_text, 20);

        } else if (part == 6) {
            // =====================================================
            // 板块 6：鼠标
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 入门演示 - 板块 6：鼠标");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 6：鼠标", 32);
            bgt_draw_text(40, 94,
                          "左键点击画圆（滚轮调半径）；右键按住出现橡皮筋圆，"
                          "松开后保留；H 键隐藏/显示光标。", 18);

            // 鼠标坐标与滚轮
            char mouse_text[96];
            std::snprintf(mouse_text, sizeof(mouse_text),
                          "鼠标位置：(%d, %d)    滚轮半径：%d",
                          bgt_mouse_x(), bgt_mouse_y(), dot_radius);
            bgt_draw_text(40, 134, mouse_text, 20);

            // 滚轮：bgt_mouse_wheel() 返回本帧的滚动量，可能为负数
            dot_radius = dot_radius + bgt_mouse_wheel() * 3;
            if (dot_radius < 8) {
                dot_radius = 8;
            }
            if (dot_radius > 60) {
                dot_radius = 60;
            }

            // 左键：just_pressed 是"这一帧刚按下"，所以要把位置存下来
            // 每帧重画，否则点只闪现一帧就没了。
            if (bgt_mouse_just_pressed(BGT_MOUSE_LEFT)) {
                click_x = bgt_mouse_x();
                click_y = bgt_mouse_y();
                has_click = true;
            }
            if (has_click) {
                bgt_set_color(BGT_RED);
                bgt_fill_circle(click_x, click_y, dot_radius);
            }

            // 右键：按下时记住起点，同时让上一次留下的圆消失
            if (bgt_mouse_just_pressed(BGT_MOUSE_RIGHT)) {
                press_x = bgt_mouse_x();
                press_y = bgt_mouse_y();
                has_released_circle = false;
            }

            // 按住期间：从起点到当前位置画一条"橡皮筋"线，
            // 再加一个颜色较浅的空心圆做预览（半径 = 起点到鼠标的距离）
            if (bgt_mouse_is_down(BGT_MOUSE_RIGHT)) {
                bgt_set_color(BGT_BLUE);
                bgt_draw_line(press_x, press_y, bgt_mouse_x(), bgt_mouse_y());
                const int dx = bgt_mouse_x() - press_x;
                const int dy = bgt_mouse_y() - press_y;
                const int preview_radius = static_cast<int>(
                    std::sqrt(static_cast<double>(dx * dx + dy * dy)));
                bgt_set_color(bgt_rgb(150, 190, 230));
                bgt_draw_circle(press_x, press_y, preview_radius);
            }

            // 松开：把预览变成实心圆，并持续画到下一次右键按下（见上面）。
            // 只调用一次绘制是不够的——库每帧自动清空画布（见板块 1 说明），
            // 必须每帧重画才看得到，所以要用变量记住"画完了吗 + 圆在哪"，
            // 靠下面的 if 每帧重画。
            if (bgt_mouse_just_released(BGT_MOUSE_RIGHT)) {
                const int dx = bgt_mouse_x() - press_x;
                const int dy = bgt_mouse_y() - press_y;
                released_circle_radius = static_cast<int>(
                    std::sqrt(static_cast<double>(dx * dx + dy * dy)));
                released_circle_x = press_x;
                released_circle_y = press_y;
                has_released_circle = true;
            }
            if (has_released_circle) {
                bgt_set_color(BGT_GREEN);
                bgt_fill_circle(released_circle_x, released_circle_y,
                                released_circle_radius);
            }

            // 隐藏/显示光标
            if (bgt_key_just_pressed(BGT_KEY_H)) {
                if (cursor_hidden) {
                    bgt_show_mouse();
                    cursor_hidden = false;
                } else {
                    bgt_hide_mouse();
                    cursor_hidden = true;
                }
            }

        } else if (part == 7) {
            // =====================================================
            // 板块 7：时间
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 入门演示 - 板块 7：时间");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 7：时间", 32);
            bgt_draw_text(40, 94,
                          "移动条的速度用 bgt_delta_time() 计算，与帧率无关。", 18);
            bgt_draw_text(40, 128,
                          "试一试：按 D 让程序停顿半秒（bgt_delay）。", 20);

            // 每帧移动 300 像素/秒，碰边折返
            if (bar_moving_right) {
                bar_x = bar_x + 300.0 * bgt_delta_time();
                if (bar_x >= 900.0) {
                    bar_x = 900.0;
                    bar_moving_right = false;
                }
            } else {
                bar_x = bar_x - 300.0 * bgt_delta_time();
                if (bar_x <= 0.0) {
                    bar_x = 0.0;
                    bar_moving_right = true;
                }
            }

            bgt_set_color(BGT_GREEN);
            bgt_fill_rect(static_cast<int>(bar_x), 210, 100, 46);

            char time_text[96];
            std::snprintf(time_text, sizeof(time_text),
                          "delta_time = %.2f 秒    total_time = %.1f 秒    "
                          "fps = %.1f",
                          bgt_delta_time(), bgt_total_time(), bgt_fps());
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 320, time_text, 20);
            bgt_draw_text(40, 380,
                          "bgt_delta_time 上一帧到本帧的秒数（动画用）；"
                          "bgt_total_time 打开窗口以来的秒数；", 18);
            bgt_draw_text(40, 415,
                          "bgt_fps 最近一秒钟的实际帧率；主循环开头设置的"
                          " bgt_set_fps_limit(60) 就是它。", 18);

            if (bgt_key_just_pressed(BGT_KEY_D)) {
                bgt_delay(500);   // 暂停约 0.5 秒
            }

        } else {
            // =====================================================
            // 板块 8：错误信息
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 入门演示 - 板块 8：错误信息");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 8：错误信息", 32);
            bgt_draw_text(40, 94,
                          "很多函数失败时返回 false，并把原因记下来，"
                          "程序不会崩溃。", 20);
            bgt_draw_text(40, 128,
                          "下面故意调用一次失败的 bgt_set_font"
                          "（空文件名不合理）：", 20);

            // 故意制造一个错误：空文件名直接被拒绝
            const bool ok = bgt_set_font("", 24);

            if (bgt_has_error()) {
                char code_text[64];
                std::snprintf(code_text, sizeof(code_text),
                              "bgt_has_error() 为 true，错误码：%d"
                              "（BGT_ERROR_FONT）", bgt_error_code());
                bgt_set_color(BGT_RED);
                bgt_draw_text(40, 190, code_text, 20);
                bgt_draw_text(40, 232, "错误内容（bgt_draw_error 画在窗口里）：", 18);
                bgt_draw_error(40, 268, 18);
                bgt_draw_text(40, 310,
                              "同时也可以 bgt_print_error() 把错误打印到控制台。",
                              18);
                if (!error_printed) {
                    error_printed = true;
                    bgt_print_error();
                }
            }

            if (!ok) {
                bgt_clear_error();   // 清除错误
                if (!bgt_has_error()) {
                    bgt_set_color(BGT_DARK_GRAY);
                    bgt_draw_text(40, 400,
                                  "bgt_clear_error() 清除后，"
                                  "bgt_has_error() 变回 false。", 18);
                }
            }

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 640, "按【空格】关闭窗口（Esc 也可以）。", 20);
        }

        // 底部统一提示（最后一个板块自己提示了关闭方式）
        if (part < 8) {
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(40, 648, "按【空格】进入下一板块，按【Esc】退出程序", 18);
        }

        // 每个板块都支持按空格进入下一板块；最后一个板块按空格关闭
        if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
            if (part == 8) {
                break;
            }
            part = part + 1;
        }

        // 一帧结束：把画画好的内容显示出来，并处理键盘、鼠标事件
        bgt_update_window();
    }

    // 关闭窗口并释放资源（不写也行，程序结束时自动清理）
    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity)
