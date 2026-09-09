// =====================================================================
// libbgt 错误历史 API 演示（v0.3）
// 每个板块演示错误诊断的一组能力，按【空格】进入下一板块，随时可以按
// 【Esc】退出。库会把最近的错误记成一份编号列表：bgt_error_count() 数
// 条数，序号 0 最老、条数减一是最新，bgt_error_code(i) 和
// bgt_error_text(i, …) 按序号查询，bgt_clear_error() 清空全部。
//   板块 1  错误列表 —— 触发错误，循环画出整个历史
//   板块 2  按序号查询 —— 取错误码做统计，取文本自己画
//   板块 3  单条呈现 —— 无参版画最新一条，长消息自动换行
//   板块 4  清空历史 —— clear 之后列表消失
// =====================================================================
// NOLINTBEGIN(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity)

#include "bgt.h"

#include <cstdio>

int main()
{
    constexpr int window_width = 800;
    constexpr int window_height = 700;

    if (!bgt_open_window(window_width, window_height, "libbgt 错误演示")) {
        bgt_print_error();
        return 1;
    }
    bgt_set_background(BGT_WHITE);

    int part = 1; // 当前板块号：1 到 4

    while (bgt_window_is_open()) {
        // ---------- 板块 1：错误列表 ----------
        if (part == 1) {
            bgt_set_window_title("libbgt 错误演示 - 板块 1：错误列表");
            if (bgt_key_just_pressed(BGT_KEY_E)) {
                bgt_load_image("不存在的图片.png"); // 触发图片错误
            }
            if (bgt_key_just_pressed(BGT_KEY_F)) {
                bgt_set_font(nullptr, 16); // 触发字体错误
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 1：错误列表 —— 最近的错误都在这里", 32);
            char count_text[64] = {};
            std::snprintf(count_text, sizeof(count_text),
                          "历史里有 %d 条错误（最多保留 10 条）",
                          bgt_error_count());
            bgt_draw_text(40, 84, count_text, 22);
            // 自己写循环把整份历史画出来：这就是一块自搭的调试面板。
            int y = 130;
            for (int i = 0; i < bgt_error_count(); i = i + 1) {
                char label[8] = {};
                std::snprintf(label, sizeof(label), "%d.", i);
                bgt_set_color(BGT_DARK_GRAY);
                bgt_draw_text(40, y, label, 16);
                bgt_set_color(BGT_BLACK);
                // 每条留两行的高度：长消息换行也不会压到下一条，
                // 满容量 10 条时也碰不到底部提示。
                bgt_draw_error(70, y, 16, i);
                y = y + 48;
            }
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(40, 650,
                          "按【E】触发图片错误，按【F】触发字体错误。", 22);
        }

        // ---------- 板块 2：按序号查询 ----------
        if (part == 2) {
            bgt_set_window_title("libbgt 错误演示 - 板块 2：按序号查询");
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 2：按序号查询错误码和文本", 32);
            // 用 bgt_error_code(i) 给历史里的错误分类计数。
            int image_errors = 0;
            int font_errors = 0;
            for (int i = 0; i < bgt_error_count(); i = i + 1) {
                if (bgt_error_code(i) == BGT_ERROR_IMAGE) {
                    image_errors = image_errors + 1;
                }
                if (bgt_error_code(i) == BGT_ERROR_FONT) {
                    font_errors = font_errors + 1;
                }
            }
            char stat_text[64] = {};
            std::snprintf(stat_text, sizeof(stat_text),
                          "图片类错误 %d 条，字体类错误 %d 条", image_errors,
                          font_errors);
            bgt_draw_text(80, 150, stat_text, 26);
            // bgt_error_text 把文本取进自己的数组：拿去统计、拼提示都行。
            // 这里数一数取出来的文本有几个字节（学生刚学的数组循环）。
            char latest[128] = {};
            bgt_error_text(bgt_error_count() - 1, latest, 128);
            int text_bytes = 0;
            while (latest[text_bytes] != '\0') {
                text_bytes = text_bytes + 1;
            }
            char taken[48] = {};
            std::snprintf(taken, sizeof(taken),
                          "文本已取进数组：共 %d 个字节", text_bytes);
            bgt_draw_text(80, 260, taken, 22);
            // 画长文本时让库按序号画：超宽自动换行，不用自己操心宽度。
            bgt_set_color(BGT_BLUE);
            bgt_draw_error(80, 300, 22, bgt_error_count() - 1);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(80, 420,
                          "上面的统计来自板块 1 里按【E】/【F】触发的错误。", 22);
        }

        // ---------- 板块 3：单条呈现与换行 ----------
        if (part == 3) {
            bgt_set_window_title("libbgt 错误演示 - 板块 3：单条呈现");
            if (bgt_key_just_pressed(BGT_KEY_G)) {
                // 超长文件名触发一条超长消息，演示自动换行。
                bgt_load_image(
                    "一个特别特别特别特别特别长的文件名"
                    "用来演示错误消息自动换行.png");
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 3：单条呈现 —— 长消息自动换行", 32);
            bgt_draw_text(80, 130,
                          "按【G】触发一条超长错误，再看下面：", 22);
            // 无参版 bgt_draw_error 画的就是最新一条，超宽自动分行。
            bgt_set_color(BGT_RED);
            bgt_draw_error(80, 190, 24);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(80, 420,
                          "普通消息一行画完；超长消息按窗口宽度分行，", 22);
            bgt_draw_text(80, 460,
                          "不会再冲出屏幕右边界。", 22);
        }

        // ---------- 板块 4：清空历史 ----------
        if (part == 4) {
            bgt_set_window_title("libbgt 错误演示 - 板块 4：清空历史");
            if (bgt_key_just_pressed(BGT_KEY_C)) {
                bgt_clear_error();
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 4：清空历史 —— 从零开始", 32);
            char count_text[48] = {};
            std::snprintf(count_text, sizeof(count_text),
                          "当前错误条数：%d", bgt_error_count());
            bgt_draw_text(80, 200, count_text, 30);
            if (bgt_has_error()) {
                bgt_draw_text(80, 280, "还有错误在历史里。", 26);
            } else {
                bgt_draw_text(80, 280, "历史已经空了。", 26);
            }
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(80, 400,
                          "按【C】清空；清空后 count 归零、has_error 变 false。",
                          22);
        }

        // 底部统一提示
        bgt_set_color(BGT_DARK_GRAY);
        if (part < 4) {
            bgt_draw_text(40, 672, "按【空格】进入下一板块，按【Esc】退出", 22);
        } else {
            bgt_draw_text(40, 672, "按【Esc】退出程序", 22);
        }

        if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
            if (part < 4) {
                part = part + 1;
            } else {
                break;
            }
        }
        if (bgt_key_just_pressed(BGT_KEY_ESCAPE)) {
            break;
        }

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity)
