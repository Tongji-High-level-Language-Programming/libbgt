// =====================================================================
// libbgt 随机数 API 演示（v0.2）
//
// 每个板块介绍一小块随机数 API，按【空格】进入下一板块，随时可以按
// 【Esc】退出。bgt_random(min, max) 是统一入口：整数参数得到随机整数，
// 小数参数得到随机小数，区间都是半开 [min, max)。
//
//   板块 1  掷骰子 —— bgt_random 半开区间与数组下标
//   板块 2  满天星 —— 随机位置、大小、颜色
//   板块 3  随机游走 —— 每帧随机挪一步
//   板块 4  随机种子 —— 可复现的随机
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

    if (!bgt_open_window(window_width, window_height, "libbgt 随机数演示")) {
        bgt_print_error();
        return 1;
    }
    bgt_set_fps_limit(target_fps);

    // ---------- 各板块需要的"跨帧状态"（就是普通变量）----------

    int part = 1; // 当前板块号：1 到 4

    // 板块 1：骰子直方图
    int counts[6] = {0, 0, 0, 0, 0, 0}; // counts[i] 记录点数 i+1 出现的次数
    int total_rolls = 0;
    int last_roll = 1;

    // 板块 2：星星调色板（星空画在深色背景上）
    const unsigned star_colors[6] = {BGT_WHITE, BGT_YELLOW, BGT_CYAN,
                                     BGT_MAGENTA, BGT_ORANGE, BGT_PINK};

    // 板块 3：八只随机游走的小虫
    bool walk_started = false;
    int walker_x[8];
    int walker_y[8];
    const unsigned walker_colors[8] = {BGT_RED, BGT_BLUE, BGT_GREEN,
                                       BGT_ORANGE, BGT_PURPLE, BGT_BROWN,
                                       BGT_CYAN, BGT_MAGENTA};

    // 板块 4：种子对比
    bool seed_demo_ready = false;
    int seeded_nums[10]; // 左列：每次都重新 seed(42) 后取的 10 个数
    int plain_nums[10];  // 右列：不重新播种，延续全局序列取的 10 个数
    double double_samples[3];

    // ---------- 主循环 ----------
    while (bgt_window_is_open()) {

        // 随时退出
        if (bgt_key_just_pressed(BGT_KEY_ESCAPE)) {
            bgt_close_window();
            break;
        }

        if (part == 1) {
            // =====================================================
            // 板块 1：掷骰子 —— bgt_random 半开区间与数组下标
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 随机数演示 - 板块 1：掷骰子");

            // 按 R：掷 100 次骰子，加入直方图
            if (bgt_key_just_pressed(BGT_KEY_R)) {
                for (int i = 0; i < 100; i++) {
                    const int v = bgt_random(1, 7);
                    counts[v - 1] = counts[v - 1] + 1;
                    last_roll = v;
                    total_rolls = total_rolls + 1;
                }
            }

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 1：掷骰子 —— bgt_random 半开区间", 32);
            bgt_draw_text(40, 94,
                          "bgt_random(min, max) 返回 [min, max) 半开区间内"
                          "的随机整数：包含 min，不包含 max。",
                          20);
            bgt_draw_text(40, 128,
                          "掷六面骰要写 bgt_random(1, 7)：可能得到 1~6，"
                          "7 永远不会出现。",
                          20);
            bgt_draw_text(40, 162,
                          "参数写反也没关系——min 大于 max 时库会自动交换。",
                          20);

            // 画骰子：白底黑框，按点数画骰面
            const int die_x = 80;
            const int die_y = 220;
            bgt_set_color(BGT_WHITE);
            bgt_fill_rect(die_x, die_y, 120, 120);
            bgt_set_color(BGT_BLACK);
            bgt_draw_rect(die_x, die_y, 120, 120);

            // 骰面上 6 个"点位"的坐标（相对骰子左上角）
            const int pip_tl_x = die_x + 30;
            const int pip_tl_y = die_y + 30;
            const int pip_tr_x = die_x + 90;
            const int pip_tr_y = die_y + 30;
            const int pip_ml_x = die_x + 30;
            const int pip_ml_y = die_y + 60;
            const int pip_mr_x = die_x + 90;
            const int pip_mr_y = die_y + 60;
            const int pip_c_x = die_x + 60;
            const int pip_c_y = die_y + 60;
            const int pip_bl_x = die_x + 30;
            const int pip_bl_y = die_y + 90;
            const int pip_br_x = die_x + 90;
            const int pip_br_y = die_y + 90;

            if (last_roll == 1) {
                bgt_fill_circle(pip_c_x, pip_c_y, 11);
            }
            else if (last_roll == 2) {
                bgt_fill_circle(pip_tl_x, pip_tl_y, 11);
                bgt_fill_circle(pip_br_x, pip_br_y, 11);
            }
            else if (last_roll == 3) {
                bgt_fill_circle(pip_tl_x, pip_tl_y, 11);
                bgt_fill_circle(pip_c_x, pip_c_y, 11);
                bgt_fill_circle(pip_br_x, pip_br_y, 11);
            }
            else if (last_roll == 4) {
                bgt_fill_circle(pip_tl_x, pip_tl_y, 11);
                bgt_fill_circle(pip_tr_x, pip_tr_y, 11);
                bgt_fill_circle(pip_bl_x, pip_bl_y, 11);
                bgt_fill_circle(pip_br_x, pip_br_y, 11);
            }
            else if (last_roll == 5) {
                bgt_fill_circle(pip_tl_x, pip_tl_y, 11);
                bgt_fill_circle(pip_tr_x, pip_tr_y, 11);
                bgt_fill_circle(pip_c_x, pip_c_y, 11);
                bgt_fill_circle(pip_bl_x, pip_bl_y, 11);
                bgt_fill_circle(pip_br_x, pip_br_y, 11);
            }
            else {
                bgt_fill_circle(pip_tl_x, pip_tl_y, 11);
                bgt_fill_circle(pip_tr_x, pip_tr_y, 11);
                bgt_fill_circle(pip_ml_x, pip_ml_y, 11);
                bgt_fill_circle(pip_mr_x, pip_mr_y, 11);
                bgt_fill_circle(pip_bl_x, pip_bl_y, 11);
                bgt_fill_circle(pip_br_x, pip_br_y, 11);
            }

            char roll_text[64];
            std::snprintf(roll_text, sizeof(roll_text), "最近一次点数：%d",
                          last_roll);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(die_x, 380, roll_text, 20);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(die_x, 412, "按【R】掷 100 次", 18);

            // 直方图：六根柱子显示每个点数被掷出的次数
            int max_count = 1;
            for (int v = 0; v < 6; v++) {
                if (counts[v] > max_count) {
                    max_count = counts[v];
                }
            }
            for (int v = 0; v < 6; v++) {
                const int bar_x = 300 + v * 110;
                const int bar_height = counts[v] * 300 / max_count;
                bgt_set_color(BGT_BLUE);
                bgt_fill_rect(bar_x, 560 - bar_height, 80, bar_height);

                char count_text[32];
                std::snprintf(count_text, sizeof(count_text), "%d 次",
                              counts[v]);
                bgt_set_color(BGT_BLACK);
                bgt_draw_text(bar_x, 560 - bar_height - 26, count_text, 16);

                char face_text[32];
                std::snprintf(face_text, sizeof(face_text), "点数 %d", v + 1);
                bgt_draw_text(bar_x + 8, 575, face_text, 18);
            }

            char total_text[128];
            std::snprintf(total_text, sizeof(total_text),
                          "总掷骰次数：%d —— 掷得越多，六根柱子越接近一样高"
                          "（均匀分布）。",
                          total_rolls);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(300, 615, total_text, 18);

            // 半开区间的招牌场景：bgt_random(0, 8) 的结果直接当数组下标
            const int strip_index = bgt_random(0, 8);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(60, 440, "下标 = bgt_random(0, 8)：", 18);
            for (int i = 0; i < 8; i++) {
                const int cell_x = die_x + i * 24;
                if (i == strip_index) {
                    bgt_set_color(BGT_ORANGE);
                    bgt_fill_rect(cell_x, 460, 20, 20);
                }
                else {
                    bgt_set_color(BGT_DARK_GRAY);
                    bgt_draw_rect(cell_x, 460, 20, 20);
                }
            }
            char index_text[48];
            std::snprintf(index_text, sizeof(index_text), "本帧高亮：第 %d 格",
                          strip_index);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(die_x, 496, index_text, 18);
        }
        else if (part == 2) {
            // =====================================================
            // 板块 2：满天星 —— 随机位置、大小、颜色
            // =====================================================
            bgt_set_background(bgt_rgb(8, 8, 32));
            bgt_set_window_title("libbgt 随机数演示 - 板块 2：满天星");

            // 先抽一个样例，稍后展示它的数值
            const int sample_x = bgt_random(0, window_width);
            const int sample_y = bgt_random(260, window_height);
            const double sample_r = bgt_random(1.0, 3.5);

            // 每一帧随机画 200 颗星星：位置、大小、颜色全部随机
            for (int i = 0; i < 200; i++) {
                const int x = bgt_random(0, window_width);
                const int y = bgt_random(260, window_height);
                const int r = static_cast<int>(bgt_random(1.0, 3.5));
                bgt_set_color(star_colors[bgt_random(0, 6)]);
                bgt_fill_circle(x, y, r);
            }

            // 把样例星画成黄色并加白圈，方便和下方数字对照
            bgt_set_color(BGT_YELLOW);
            bgt_fill_circle(sample_x, sample_y,
                            static_cast<int>(sample_r) + 2);
            bgt_set_color(BGT_WHITE);
            bgt_draw_circle(sample_x, sample_y,
                            static_cast<int>(sample_r) + 6);

            // 文字最后画，保证可读
            bgt_set_color(BGT_WHITE);
            bgt_draw_text(40, 36, "板块 2：满天星 —— 随机填充画面", 32);
            bgt_draw_text(40, 94,
                          "每一帧随机画 200 颗星星：位置和颜色用整数版 "
                          "bgt_random，大小用小数版。",
                          20);
            bgt_draw_text(40, 128,
                          "bgt_random(min, max) 返回 [min, max) 的随机数："
                          "包含 min，不包含 max。",
                          20);
            bgt_draw_text(40, 162,
                          "想要小数必须写小数：bgt_random(0, 1) 是整数版，"
                          "bgt_random(0.0, 1.0) 才是小数。",
                          20);

            char sample_xy_text[96];
            std::snprintf(sample_xy_text, sizeof(sample_xy_text),
                          "x = bgt_random(0, %d) = %d    y = bgt_random(260, "
                          "%d) = %d",
                          window_width, sample_x, window_height, sample_y);
            bgt_draw_text(40, 196, sample_xy_text, 18);

            char sample_r_text[96];
            std::snprintf(sample_r_text, sizeof(sample_r_text),
                          "r = bgt_random(1.0, 3.5) = %.2f", sample_r);
            bgt_draw_text(40, 224, sample_r_text, 18);
        }
        else if (part == 3) {
            // =====================================================
            // 板块 3：随机游走 —— 每帧随机挪一步
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 随机数演示 - 板块 3：随机游走");

            // 第一次进入本板块时，把小虫撒在画面中央区域
            if (!walk_started) {
                walk_started = true;
                for (int i = 0; i < 8; i++) {
                    walker_x[i] = bgt_random(100, 900);
                    walker_y[i] = bgt_random(260, 660);
                }
            }

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 3：随机游走 —— 每帧随机挪一步", 32);
            bgt_draw_text(40, 94,
                          "八只小虫每帧向随机方向挪一步：dx、dy 都是 "
                          "bgt_random(-3, 4)。",
                          20);
            bgt_draw_text(40, 128,
                          "半开区间 max 取不到，所以写 4：-3 到 3 共 7 种"
                          "步长。",
                          20);
            bgt_draw_text(40, 162,
                          "碰到边界就停住——很多游戏里的“随机巡逻”就是这个"
                          "思路。",
                          20);

            // 每只小虫走一步，然后画出来
            for (int i = 0; i < 8; i++) {
                const int dx = bgt_random(-3, 4);
                const int dy = bgt_random(-3, 4);
                walker_x[i] = walker_x[i] + dx;
                walker_y[i] = walker_y[i] + dy;

                if (walker_x[i] < 20) {
                    walker_x[i] = 20;
                }
                if (walker_x[i] > 980) {
                    walker_x[i] = 980;
                }
                if (walker_y[i] < 220) {
                    walker_y[i] = 220;
                }
                if (walker_y[i] > 680) {
                    walker_y[i] = 680;
                }

                bgt_set_color(walker_colors[i]);
                bgt_fill_circle(walker_x[i], walker_y[i], 6);
            }
        }
        else {
            // =====================================================
            // 板块 4：随机种子 —— 可复现的随机
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 随机数演示 - 板块 4：随机种子");

            // 第一次进入本板块、或按 R 时：重新生成"不播种"的右列和
            // 三个小数样例——它们延续全局序列，每次都不一样。
            // （必须在下面的播种之前取数：播种会把全局序列固定下来。）
            if (!seed_demo_ready || bgt_key_just_pressed(BGT_KEY_R)) {
                for (int i = 0; i < 10; i++) {
                    plain_nums[i] = bgt_random(1, 100);
                }
                for (int i = 0; i < 3; i++) {
                    double_samples[i] = bgt_random(0.0, 1.0);
                }
            }

            // 左列只在第一次进入时生成一次：固定种子 42 的结果永远
            // 不变，重新生成也会一模一样。
            if (!seed_demo_ready) {
                seed_demo_ready = true;
                bgt_random_seed(42);
                for (int i = 0; i < 10; i++) {
                    seeded_nums[i] = bgt_random(1, 100);
                }
            }

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 4：随机种子 —— 可复现的随机", 32);
            bgt_draw_text(40, 94,
                          "bgt_random_seed(种子) 让随机序列变得可复现：同样"
                          "的种子，同样的序列。",
                          20);
            bgt_draw_text(40, 128,
                          "左列用固定种子 42 生成：重开程序、按 R，都是这 "
                          "10 个数。",
                          20);
            bgt_draw_text(40, 162,
                          "右列不播种，延续全局序列 —— 每次按 R 都不同。",
                          20);

            bgt_set_color(BGT_BLUE);
            bgt_draw_text(40, 205, "先 seed(42)，再取 10 个：", 20);
            for (int i = 0; i < 10; i++) {
                char num_text[32];
                std::snprintf(num_text, sizeof(num_text), "第 %2d 个：%3d",
                              i + 1, seeded_nums[i]);
                bgt_draw_text(60, 240 + i * 30, num_text, 18);
            }

            bgt_set_color(BGT_ORANGE);
            bgt_draw_text(520, 205, "不播种，直接取 10 个：", 20);
            for (int i = 0; i < 10; i++) {
                char num_text[32];
                std::snprintf(num_text, sizeof(num_text), "第 %2d 个：%3d",
                              i + 1, plain_nums[i]);
                bgt_draw_text(540, 240 + i * 30, num_text, 18);
            }

            char double_text[128];
            std::snprintf(double_text, sizeof(double_text),
                          "bgt_random(0.0, 1.0) 样例：%.3f  %.3f  %.3f"
                          "（永远取不到 1.0）",
                          double_samples[0], double_samples[1],
                          double_samples[2]);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 570, double_text, 18);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(40, 605, "按【R】重新生成右列与小数样例。", 18);
        }

        // 底部统一提示（板块 2 是深色背景，用浅色文字）
        if (part < 4) {
            if (part == 2) {
                bgt_set_color(BGT_LIGHT_GRAY);
            }
            else {
                bgt_set_color(BGT_DARK_GRAY);
            }
            bgt_draw_text(40, 648, "按【空格】进入下一板块，按【Esc】退出程序",
                          18);
        }

        // 每个板块都支持按空格进入下一板块；最后一个板块按空格关闭
        if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
            if (part == 4) {
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
