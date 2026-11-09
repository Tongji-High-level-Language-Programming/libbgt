/**
 * @file 01_hello.cpp
 * @brief libbgt 的第一个示例：在窗口中央画一个蓝色圆和一行中文。
 * @details 演示一个最小可运行程序的结构：打开窗口、设置帧率上限和
 *          背景色，然后在主循环里画图并调用 `bgt_update_window()` 显示，
 *          循环结束后关闭窗口。所有常量都用 `constexpr int` 声明。
 */

#include "bgt.h"

int main()
{
    constexpr int kWindowWidth = 800;
    constexpr int kWindowHeight = 600;
    constexpr int kTargetFps = 60;
    constexpr int kCircleX = 400;
    constexpr int kCircleY = 300;
    constexpr int kCircleRadius = 60;
    constexpr int kTextX = 260;
    constexpr int kTextY = 210;
    constexpr int kTextSize = 32;

    if (!bgt_open_window(kWindowWidth, kWindowHeight, "libbgt hello")) {
        bgt_print_error();
        return 1;
    }

    bgt_set_fps_limit(kTargetFps);
    bgt_set_background(BGT_WHITE);

    while (bgt_window_is_open()) {
        bgt_set_color(BGT_BLUE);
        bgt_fill_circle(kCircleX, kCircleY, kCircleRadius);

        bgt_set_color(BGT_BLACK);
        bgt_draw_text(kTextX, kTextY, "你好，libbgt！", kTextSize);

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}
