/**
 * @file 03_text.cpp
 * @brief libbgt 文本示例：中文绘制、默认字号与文字尺寸测量。
 * @details 演示 `bgt_draw_text()` 的两种字号用法：用
 *          `bgt_set_font_size()` 设置默认字号，或在调用时直接传字号。
 *          再用 `bgt_text_width()` / `bgt_text_height()` 量出文字尺寸，
 *          画背景块并做水平居中。
 */

#include "bgt.h"

// NOLINTBEGIN(readability-magic-numbers)

int main()
{
    constexpr int kWindowWidth = 900;
    constexpr int kWindowHeight = 520;
    constexpr int kTargetFps = 60;

    if (!bgt_open_window(kWindowWidth, kWindowHeight, "libbgt text")) {
        bgt_print_error();
        return 1;
    }

    bgt_set_fps_limit(kTargetFps);
    bgt_set_font_size(26);
    bgt_set_background(bgt_rgb(28, 32, 42));

    while (bgt_window_is_open()) {
        const char *title = "中文文本和尺寸测量";
        const int title_width = bgt_text_width(title, 40);
        const int title_x = (bgt_window_width() - title_width) / 2;

        bgt_set_color(BGT_WHITE);
        bgt_draw_text(title_x, 40, title, 40);

        const char *measured = "这行文字下面的色块来自 bgt_text_width()";
        const int measured_x = 90;
        const int measured_y = 145;
        const int measured_width = bgt_text_width(measured);
        const int measured_height = bgt_text_height(measured);

        bgt_set_color(bgt_rgba(255, 255, 255, 48));
        bgt_fill_rect(measured_x - 12, measured_y - 8, measured_width + 24,
                      measured_height + 16);

        bgt_set_color(BGT_YELLOW);
        bgt_draw_text(measured_x, measured_y, measured);

        bgt_set_color(BGT_LIGHT_GRAY);
        bgt_draw_text(90, 230, "默认字号可以用 bgt_set_font_size() 设置。", 24);
        bgt_draw_text(90, 275, "也可以在每次绘制时传入字号。", 30);

        bgt_set_color(BGT_CYAN);
        bgt_draw_text(90, 350, "小字号 18", 18);
        bgt_draw_text(260, 342, "中字号 28", 28);
        bgt_draw_text(470, 330, "大字号 44", 44);

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers)
