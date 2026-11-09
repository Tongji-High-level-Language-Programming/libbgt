/**
 * @file 02_shapes.cpp
 * @brief libbgt 基本图形示例：点、线、矩形、圆、椭圆、三角形。
 * @details 在一个固定窗口里分区演示每类图形：点阵、不同线宽的线条、
 *          填充与描边的矩形、圆与椭圆、三角形。图形一律先
 *          `bgt_set_color()` 设颜色再绘制，背景色用 `bgt_rgb()` 自定义。
 */

#include "bgt.h"

// NOLINTBEGIN(readability-magic-numbers)

int main()
{
    constexpr int kWindowWidth = 900;
    constexpr int kWindowHeight = 650;
    constexpr int kTargetFps = 60;

    if (!bgt_open_window(kWindowWidth, kWindowHeight, "libbgt shapes")) {
        bgt_print_error();
        return 1;
    }

    bgt_set_fps_limit(kTargetFps);
    bgt_set_background(bgt_rgb(248, 249, 252));

    while (bgt_window_is_open()) {
        bgt_set_color(BGT_BLACK);
        bgt_draw_text(40, 30, "基本图形：点、线、矩形、圆、椭圆、三角形", 28);

        bgt_set_color(BGT_DARK_GRAY);
        bgt_draw_text(70, 96, "点", 22);
        bgt_set_color(BGT_BLUE);
        for (int point_x = 70; point_x <= 190; point_x += 20) {
            for (int point_y = 140; point_y <= 260; point_y += 20) {
                bgt_draw_point(point_x, point_y);
            }
        }

        bgt_set_color(BGT_DARK_GRAY);
        bgt_draw_text(260, 96, "线条宽度", 22);
        bgt_set_line_width(1);
        bgt_set_color(BGT_RED);
        bgt_draw_line(260, 150, 430, 120);
        bgt_set_line_width(4);
        bgt_set_color(BGT_GREEN);
        bgt_draw_line(260, 190, 430, 160);
        bgt_set_line_width(8);
        bgt_set_color(BGT_BLUE);
        bgt_draw_line(260, 240, 430, 210);

        bgt_set_line_width(3);
        bgt_set_color(BGT_DARK_GRAY);
        bgt_draw_text(540, 96, "矩形", 22);
        bgt_set_color(BGT_ORANGE);
        bgt_fill_rect(540, 140, 120, 80);
        bgt_set_color(BGT_BLACK);
        bgt_draw_rect(690, 140, 120, 80);

        bgt_set_color(BGT_DARK_GRAY);
        bgt_draw_text(70, 340, "圆和椭圆", 22);
        bgt_set_color(BGT_PURPLE);
        bgt_fill_circle(130, 460, 55);
        bgt_set_color(BGT_BLUE);
        bgt_draw_circle(280, 460, 55);
        bgt_set_color(BGT_CYAN);
        bgt_fill_ellipse(430, 460, 75, 45);
        bgt_set_color(BGT_DARK_GRAY);
        bgt_draw_ellipse(600, 460, 75, 45);

        bgt_set_color(BGT_DARK_GRAY);
        bgt_draw_text(690, 340, "三角形", 22);
        bgt_set_color(BGT_PINK);
        bgt_fill_triangle(720, 520, 780, 400, 840, 520);
        bgt_set_color(BGT_BLACK);
        bgt_draw_triangle(705, 535, 780, 375, 855, 535);

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers)
