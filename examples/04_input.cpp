/**
 * @file 04_input.cpp
 * @brief libbgt 输入示例：键盘、鼠标与滚轮驱动一个可移动的圆。
 * @details 方向键或 WASD 连续移动（`bgt_key_is_down()`）；鼠标左键让圆
 *          跳到点击位置并变色，右键换色，滚轮改变半径；空格换色，Esc
 *          退出。窗口用 `bgt_open_window_resizable()` 创建，可以拖拽缩放。
 */

#include "bgt.h"

// NOLINTBEGIN(readability-magic-numbers,
// readability-function-cognitive-complexity)

int main()
{
    constexpr int kWindowWidth = 900;
    constexpr int kWindowHeight = 600;
    constexpr int kTargetFps = 60;
    constexpr double kSpeed = 300.0;

    if (!bgt_open_window_resizable(kWindowWidth, kWindowHeight,
                                   "libbgt input")) {
        bgt_print_error();
        return 1;
    }

    bgt_set_fps_limit(kTargetFps);
    bgt_set_background(bgt_rgb(245, 247, 250));

    double x = 450.0;
    double y = 300.0;
    int radius = 40;
    unsigned color = BGT_BLUE;

    while (bgt_window_is_open()) {
        const double dt = bgt_delta_time();

        if (bgt_key_is_down(BGT_KEY_LEFT) || bgt_key_is_down(BGT_KEY_A)) {
            x -= kSpeed * dt;
        }
        if (bgt_key_is_down(BGT_KEY_RIGHT) || bgt_key_is_down(BGT_KEY_D)) {
            x += kSpeed * dt;
        }
        if (bgt_key_is_down(BGT_KEY_UP) || bgt_key_is_down(BGT_KEY_W)) {
            y -= kSpeed * dt;
        }
        if (bgt_key_is_down(BGT_KEY_DOWN) || bgt_key_is_down(BGT_KEY_S)) {
            y += kSpeed * dt;
        }

        if (bgt_mouse_just_pressed(BGT_MOUSE_LEFT)) {
            x = bgt_mouse_x();
            y = bgt_mouse_y();
            color = BGT_ORANGE;
        }
        if (bgt_mouse_just_pressed(BGT_MOUSE_RIGHT)) {
            color = BGT_PURPLE;
        }

        radius += bgt_mouse_wheel() * 4;
        radius = radius < 15 ? 15 : radius;
        radius = radius > 90 ? 90 : radius;

        if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
            color = BGT_GREEN;
        }
        if (bgt_key_just_pressed(BGT_KEY_ESCAPE)) {
            bgt_close_window();
            break;
        }

        bgt_set_color(BGT_DARK_GRAY);
        bgt_draw_text(40, 32, "方向键或 WASD 移动，鼠标左键跳转，滚轮改变大小",
                      24);
        bgt_draw_text(40, 68,
                      "空格变绿，右键变紫，Esc 退出。窗口可以拖拽缩放。", 22);

        bgt_set_color(bgt_rgb(220, 224, 232));
        bgt_draw_line(0, static_cast<int>(y), bgt_window_width(),
                      static_cast<int>(y));
        bgt_draw_line(static_cast<int>(x), 0, static_cast<int>(x),
                      bgt_window_height());

        bgt_set_color(color);
        bgt_fill_circle(static_cast<int>(x), static_cast<int>(y), radius);

        bgt_set_color(BGT_BLACK);
        bgt_draw_circle(static_cast<int>(x), static_cast<int>(y), radius);

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers,
// readability-function-cognitive-complexity)
