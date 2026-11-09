/**
 * @file 05_transparency.cpp
 * @brief libbgt 半透明示例：`bgt_rgba()` 的 alpha 叠加效果。
 * @details 先用匿名命名空间里的 `draw_checkerboard()` 画灰白棋盘格做底，
 *          再叠加多个不同透明度的圆、矩形、三角形，直观看清 a=0 到
 *          a=255 的差别。
 */

#include "bgt.h"

// NOLINTBEGIN(readability-magic-numbers)

namespace {

void draw_checkerboard(int cell_size)
{
    for (int cell_y = 0; cell_y < bgt_window_height(); cell_y += cell_size) {
        for (int cell_x = 0; cell_x < bgt_window_width(); cell_x += cell_size) {
            const bool light_cell =
                ((cell_x / cell_size) + (cell_y / cell_size)) % 2 == 0;
            bgt_set_color(light_cell ? bgt_rgb(238, 238, 238)
                                     : bgt_rgb(205, 205, 205));
            bgt_fill_rect(cell_x, cell_y, cell_size, cell_size);
        }
    }
}

} // namespace

int main()
{
    constexpr int kWindowWidth = 900;
    constexpr int kWindowHeight = 600;
    constexpr int kTargetFps = 60;
    constexpr int kCellSize = 32;

    if (!bgt_open_window(kWindowWidth, kWindowHeight, "libbgt transparency")) {
        bgt_print_error();
        return 1;
    }

    bgt_set_fps_limit(kTargetFps);

    while (bgt_window_is_open()) {
        const double time = bgt_total_time();
        const int moving_x = 450 + static_cast<int>(180.0 * time);
        const int wrapped_x = (moving_x % (bgt_window_width() + 240)) - 120;

        draw_checkerboard(kCellSize);

        bgt_set_color(BGT_DARK_GRAY);
        bgt_draw_text(40, 32, "透明度：bgt_rgba(r, g, b, a)", 30);
        bgt_draw_text(40, 74, "灰白网格能更清楚地看到半透明颜色叠加。", 22);

        bgt_set_color(bgt_rgba(255, 0, 0, 120));
        bgt_fill_circle(330, 320, 140);

        bgt_set_color(bgt_rgba(0, 120, 255, 120));
        bgt_fill_circle(450, 320, 140);

        bgt_set_color(bgt_rgba(0, 220, 100, 120));
        bgt_fill_circle(390, 215, 140);

        bgt_set_color(bgt_rgba(255, 180, 0, 150));
        bgt_fill_rect(570, 220, 220, 180);

        bgt_set_color(bgt_rgba(110, 0, 190, 100));
        bgt_fill_triangle(600, 470, 730, 170, 850, 470);

        bgt_set_color(bgt_rgba(0, 0, 0, 90));
        bgt_fill_circle(wrapped_x, 500, 70);

        bgt_set_color(BGT_BLACK);
        bgt_draw_text(40, 535,
                      "a=0 完全透明，a=255 完全不透明；这里使用 90~150。", 22);

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers)
