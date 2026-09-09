// =====================================================================
// libbgt 碰撞检测 API 演示（v0.2）
//
// 每个板块介绍一对形状的命中判断，按【空格】进入下一板块，随时可以按
// 【Esc】退出。bgt_hit_* 是纯几何函数，不开窗口也能用：形状×形状必须
// 实际重叠才算命中（恰好相切不算），点测的命中范围与画出的像素一致。
//
//   板块 1  点与矩形 —— 鼠标"悬停按钮"
//   板块 2  点与圆 —— 鼠标"打靶"
//   板块 3  矩形与矩形 —— 方向键推方块撞挡板
//   板块 4  圆与圆 —— 相切不算碰（重点教学）
//   板块 5  圆与矩形 —— "球拍"原型
// =====================================================================

#include "bgt.h"

#include <cstdio>

// NOLINTBEGIN(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity)

int main()
{
    // ---------- 常量 ----------
    const int window_width = 1000;
    const int window_height = 700;
    const int target_fps = 60;

    if (!bgt_open_window(window_width, window_height, "libbgt 碰撞检测演示")) {
        bgt_print_error();
        return 1;
    }
    bgt_set_fps_limit(target_fps);

    // ---------- 各板块需要的"跨帧状态"（就是普通变量）----------

    int part = 1; // 当前板块号：1 到 5

    // 板块 1：按钮矩形
    const int button_x = 350;
    const int button_y = 300;
    const int button_w = 300;
    const int button_h = 90;

    // 板块 2：靶圆与点击计数
    const int target_x = 500;
    const int target_y = 430;
    const int target_r = 60;
    int click_count = 0;
    int hit_count = 0;
    bool last_click_hit = false;
    int last_click_x = -1;
    int last_click_y = -1;

    // 板块 3：方向键控制的方块 vs 固定挡板
    int player_x = 60;
    int player_y = 320;
    const int player_w = 80;
    const int player_h = 60;
    const int wall_x = 420;
    const int wall_y = 330;
    const int wall_w = 200;
    const int wall_h = 150;

    // 板块 4：方向键控制的圆 vs 固定圆
    // （起始位置保证"恰好相切"的位置 (500, 280) 用方向键可达）
    int ball_x = 500;
    int ball_y = 240;
    const int ball_r = 40;
    const int ring_x = 500;
    const int ring_y = 400;
    const int ring_r = 80;

    // 板块 5：方向键控制的圆 vs 挡板矩形
    int puck_x = 120;
    int puck_y = 400;
    const int puck_r = 40;
    const int paddle_x = 380;
    const int paddle_y = 320;
    const int paddle_w = 240;
    const int paddle_h = 160;

    // ---------- 主循环 ----------
    while (bgt_window_is_open()) {

        // 随时退出
        if (bgt_key_just_pressed(BGT_KEY_ESCAPE)) {
            bgt_close_window();
            break;
        }

        // 方向键移动量（板块 3~5 通用）
        int move_x = 0;
        int move_y = 0;
        if (bgt_key_is_down(BGT_KEY_LEFT)) {
            move_x = move_x - 4;
        }
        if (bgt_key_is_down(BGT_KEY_RIGHT)) {
            move_x = move_x + 4;
        }
        if (bgt_key_is_down(BGT_KEY_UP)) {
            move_y = move_y - 4;
        }
        if (bgt_key_is_down(BGT_KEY_DOWN)) {
            move_y = move_y + 4;
        }

        if (part == 1) {
            // =====================================================
            // 板块 1：点与矩形 —— 鼠标"悬停按钮"
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 碰撞检测演示 - 板块 1：点与矩形");

            const int mouse_x = bgt_mouse_x();
            const int mouse_y = bgt_mouse_y();
            const bool hover = bgt_hit_point_rect(mouse_x, mouse_y,
                                                  button_x, button_y,
                                                  button_w, button_h);

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 1：点与矩形 —— 鼠标“悬停按钮”", 32);
            bgt_draw_text(40, 94,
                          "bgt_hit_point_rect(px, py, x, y, width, height)："
                          "点在矩形上返回 true。",
                          20);
            bgt_draw_text(40, 128,
                          "命中范围与 fill_rect 实际画出的像素完全一致："
                          "边框上的点也算命中。",
                          20);
            bgt_draw_text(40, 162,
                          "bgt_mouse_x()/bgt_mouse_y() 加点测，就是最常见的"
                          "“点按钮”判断。",
                          20);

            // 按钮：指针在按钮里时高亮
            if (hover) {
                bgt_set_color(BGT_ORANGE);
            }
            else {
                bgt_set_color(BGT_LIGHT_GRAY);
            }
            bgt_fill_rect(button_x, button_y, button_w, button_h);
            bgt_set_color(BGT_BLACK);
            bgt_draw_rect(button_x, button_y, button_w, button_h);
            if (hover) {
                bgt_set_color(BGT_WHITE);
            }
            else {
                bgt_set_color(BGT_DARK_GRAY);
            }
            bgt_draw_text(button_x + 78, button_y + 28, "我 是 按 钮", 28);

            // 鼠标十字线
            bgt_set_color(BGT_BLUE);
            bgt_draw_line(mouse_x - 10, mouse_y, mouse_x + 10, mouse_y);
            bgt_draw_line(mouse_x, mouse_y - 10, mouse_x, mouse_y + 10);

            char hover_text[96];
            if (hover) {
                std::snprintf(hover_text, sizeof(hover_text),
                              "鼠标位置：(%d, %d) —— bgt_hit_point_rect 返回 true",
                              mouse_x, mouse_y);
            }
            else {
                std::snprintf(hover_text, sizeof(hover_text),
                              "鼠标位置：(%d, %d) —— bgt_hit_point_rect 返回 false",
                              mouse_x, mouse_y);
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 450, hover_text, 20);
        }
        else if (part == 2) {
            // =====================================================
            // 板块 2：点与圆 —— 鼠标"打靶"
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 碰撞检测演示 - 板块 2：点与圆");

            const int mouse_x = bgt_mouse_x();
            const int mouse_y = bgt_mouse_y();

            // 左键：判断这一次点击是否命中靶圆
            if (bgt_mouse_just_pressed(BGT_MOUSE_LEFT)) {
                click_count = click_count + 1;
                last_click_x = mouse_x;
                last_click_y = mouse_y;
                last_click_hit = bgt_hit_point_circle(mouse_x, mouse_y,
                                                      target_x, target_y,
                                                      target_r);
                if (last_click_hit) {
                    hit_count = hit_count + 1;
                }
            }

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 2：点与圆 —— 鼠标“打靶”", 32);
            bgt_draw_text(40, 94,
                          "bgt_hit_point_circle(px, py, x, y, radius)：点到"
                          "圆心的距离不超过半径就算命中（含圆周）。",
                          20);
            bgt_draw_text(40, 128,
                          "bgt_mouse_just_pressed(BGT_MOUSE_LEFT) 只在点击的"
                          "那一帧返回 true。",
                          20);
            bgt_draw_text(40, 162, "用鼠标左键点击靶子试试。", 20);

            // 靶子：三环靶
            bgt_set_color(BGT_RED);
            bgt_fill_circle(target_x, target_y, target_r);
            bgt_set_color(BGT_WHITE);
            bgt_fill_circle(target_x, target_y, target_r / 2);
            bgt_set_color(BGT_RED);
            bgt_fill_circle(target_x, target_y, target_r / 5);

            // 最近一次点击位置画小叉
            if (last_click_x >= 0) {
                bgt_set_color(BGT_BLACK);
                bgt_draw_line(last_click_x - 8, last_click_y - 8,
                              last_click_x + 8, last_click_y + 8);
                bgt_draw_line(last_click_x - 8, last_click_y + 8,
                              last_click_x + 8, last_click_y - 8);
            }

            // 鼠标十字线
            bgt_set_color(BGT_BLUE);
            bgt_draw_line(mouse_x - 10, mouse_y, mouse_x + 10, mouse_y);
            bgt_draw_line(mouse_x, mouse_y - 10, mouse_x, mouse_y + 10);

            char score_text[96];
            if (click_count == 0) {
                std::snprintf(score_text, sizeof(score_text),
                              "等你的第一击——用左键点靶子。");
            }
            else if (last_click_hit) {
                std::snprintf(score_text, sizeof(score_text),
                              "点击 %d 次，命中 %d 次 —— 最近一次：命中！",
                              click_count, hit_count);
            }
            else {
                std::snprintf(score_text, sizeof(score_text),
                              "点击 %d 次，命中 %d 次 —— 最近一次：没打中",
                              click_count, hit_count);
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 450, score_text, 20);
        }
        else if (part == 3) {
            // =====================================================
            // 板块 3：矩形与矩形 —— 方向键推方块撞挡板
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 碰撞检测演示 - 板块 3：矩形与矩形");

            player_x = player_x + move_x;
            player_y = player_y + move_y;
            if (player_x < 0) {
                player_x = 0;
            }
            if (player_y < 220) {
                player_y = 220;
            }
            if (player_x > window_width - player_w) {
                player_x = window_width - player_w;
            }
            if (player_y > window_height - player_h) {
                player_y = window_height - player_h;
            }

            const bool hit = bgt_hit_rect_rect(player_x, player_y, player_w,
                                               player_h, wall_x, wall_y,
                                               wall_w, wall_h);

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 3：矩形与矩形 —— 推方块撞挡板", 32);
            bgt_draw_text(40, 94,
                          "bgt_hit_rect_rect(x1, y1, w1, h1, x2, y2, w2, h2)："
                          "两个矩形实际重叠返回 true。",
                          20);
            bgt_draw_text(40, 128,
                          "只要共有一像素的面积就算；恰好贴边、贴角不算。",
                          20);
            bgt_draw_text(40, 162, "方向键移动绿色方块，撞上灰色挡板。", 20);

            // 固定挡板
            bgt_set_color(BGT_DARK_GRAY);
            bgt_fill_rect(wall_x, wall_y, wall_w, wall_h);
            bgt_set_color(BGT_BLACK);
            bgt_draw_rect(wall_x, wall_y, wall_w, wall_h);

            // 玩家方块：命中变红
            if (hit) {
                bgt_set_color(BGT_RED);
            }
            else {
                bgt_set_color(BGT_GREEN);
            }
            bgt_fill_rect(player_x, player_y, player_w, player_h);
            bgt_set_color(BGT_BLACK);
            bgt_draw_rect(player_x, player_y, player_w, player_h);

            char hit_text[128];
            if (hit) {
                std::snprintf(hit_text, sizeof(hit_text),
                              "方块位置：(%d, %d) —— 与挡板重叠：命中！",
                              player_x, player_y);
            }
            else {
                std::snprintf(hit_text, sizeof(hit_text),
                              "方块位置：(%d, %d) —— 没有重叠",
                              player_x, player_y);
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 596, hit_text, 20);
        }
        else if (part == 4) {
            // =====================================================
            // 板块 4：圆与圆 —— 相切不算碰（重点教学）
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 碰撞检测演示 - 板块 4：圆与圆");

            ball_x = ball_x + move_x;
            ball_y = ball_y + move_y;
            if (ball_x < ball_r) {
                ball_x = ball_r;
            }
            if (ball_y < 240) {
                ball_y = 240;
            }
            if (ball_x > window_width - ball_r) {
                ball_x = window_width - ball_r;
            }
            if (ball_y > window_height - ball_r) {
                ball_y = window_height - ball_r;
            }

            const bool hit = bgt_hit_circle_circle(ball_x, ball_y, ball_r,
                                                   ring_x, ring_y, ring_r);
            const int dx = ball_x - ring_x;
            const int dy = ball_y - ring_y;
            const int dist_sq = dx * dx + dy * dy;
            const int radius_sum = ball_r + ring_r;

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 4：圆与圆 —— 相切不算碰", 32);
            bgt_draw_text(40, 94,
                          "bgt_hit_circle_circle(x1, y1, r1, x2, y2, r2)："
                          "圆心距小于 r1+r2 才算命中。",
                          20);
            bgt_draw_text(40, 128,
                          "方向键移动绿圆慢慢靠近灰圆——真正压上才会变红。",
                          20);
            bgt_draw_text(40, 162,
                          "试试停在“看起来刚碰上”的位置：d² 恰好等于 (r1+r2)²，"
                          "返回的仍是不碰！",
                          20);

            // 固定大圆
            bgt_set_color(BGT_DARK_GRAY);
            bgt_fill_circle(ring_x, ring_y, ring_r);
            bgt_set_color(BGT_BLACK);
            bgt_draw_circle(ring_x, ring_y, ring_r);

            // 玩家圆：命中变红
            if (hit) {
                bgt_set_color(BGT_RED);
            }
            else {
                bgt_set_color(BGT_GREEN);
            }
            bgt_fill_circle(ball_x, ball_y, ball_r);
            bgt_set_color(BGT_BLACK);
            bgt_draw_circle(ball_x, ball_y, ball_r);

            char dist_text[160];
            if (dist_sq == radius_sum * radius_sum) {
                std::snprintf(dist_text, sizeof(dist_text),
                              "d² = %d，(r1+r2)² = %d —— 现在正好相切："
                              "看起来碰上了，但返回 false！",
                              dist_sq, radius_sum * radius_sum);
                bgt_set_color(BGT_RED);
            }
            else if (hit) {
                std::snprintf(dist_text, sizeof(dist_text),
                              "d² = %d < (r1+r2)² = %d —— 实际重叠：命中！",
                              dist_sq, radius_sum * radius_sum);
                bgt_set_color(BGT_BLACK);
            }
            else {
                std::snprintf(dist_text, sizeof(dist_text),
                              "d² = %d > (r1+r2)² = %d —— 还没碰上",
                              dist_sq, radius_sum * radius_sum);
                bgt_set_color(BGT_BLACK);
            }
            bgt_draw_text(40, 596, dist_text, 20);
        }
        else {
            // =====================================================
            // 板块 5：圆与矩形 —— "球拍"原型
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 碰撞检测演示 - 板块 5：圆与矩形");

            puck_x = puck_x + move_x;
            puck_y = puck_y + move_y;
            if (puck_x < puck_r) {
                puck_x = puck_r;
            }
            if (puck_y < 240) {
                puck_y = 240;
            }
            if (puck_x > window_width - puck_r) {
                puck_x = window_width - puck_r;
            }
            if (puck_y > window_height - puck_r) {
                puck_y = window_height - puck_r;
            }

            const bool hit = bgt_hit_circle_rect(puck_x, puck_y, puck_r,
                                                 paddle_x, paddle_y,
                                                 paddle_w, paddle_h);

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 5：圆与矩形 —— “球拍”原型", 32);
            bgt_draw_text(40, 94,
                          "bgt_hit_circle_rect(cx, cy, r, x, y, width, "
                          "height)：圆与矩形实际重叠返回 true。",
                          20);
            bgt_draw_text(40, 128,
                          "圆心进入矩形（含边上）算命中；圆在矩形外侧恰好"
                          "擦到不算。",
                          20);
            bgt_draw_text(40, 162,
                          "这就是球类游戏里“球撞挡板”的判断原型（Pong）。",
                          20);

            // 挡板
            bgt_set_color(BGT_DARK_GRAY);
            bgt_fill_rect(paddle_x, paddle_y, paddle_w, paddle_h);
            bgt_set_color(BGT_BLACK);
            bgt_draw_rect(paddle_x, paddle_y, paddle_w, paddle_h);

            // 玩家圆：命中变红；圆心画小黑点，帮助学生看见"圆心进入矩形"
            if (hit) {
                bgt_set_color(BGT_RED);
            }
            else {
                bgt_set_color(BGT_GREEN);
            }
            bgt_fill_circle(puck_x, puck_y, puck_r);
            bgt_set_color(BGT_BLACK);
            bgt_draw_circle(puck_x, puck_y, puck_r);
            bgt_fill_circle(puck_x, puck_y, 3);

            char puck_text[128];
            if (hit) {
                std::snprintf(puck_text, sizeof(puck_text),
                              "圆心：(%d, %d) —— 圆与挡板重叠：命中！",
                              puck_x, puck_y);
            }
            else {
                std::snprintf(puck_text, sizeof(puck_text),
                              "圆心：(%d, %d) —— 没有重叠",
                              puck_x, puck_y);
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 596, puck_text, 20);
        }

        // 底部统一提示（最后一个板块不再显示"下一板块"）
        if (part < 5) {
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(40, 648, "按【空格】进入下一板块，按【Esc】退出程序",
                          18);
        }

        // 每个板块都支持按空格进入下一板块；最后一个板块按空格关闭
        if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
            if (part == 5) {
                break;
            }
            part = part + 1;
        }

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity)
