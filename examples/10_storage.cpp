// =====================================================================
// libbgt 文件存档 API 演示（v0.3）
// 每个板块演示存档的一组能力，按【空格】进入下一板块，随时可以按
// 【Esc】退出。存档文件 save.txt 就在程序旁边，是普通文本文件，可以
// 用记事本打开查看；改完存档再回到程序里按【R】，改动立刻生效。
//   板块 1  最高分 —— 关掉程序再重新运行，最高分还在
//   板块 2  存档内容 —— 屏幕上看存档文本，手改后重读生效
//   板块 3  玩家名字 —— 字符串存取
//   板块 4  数独盘面 —— 81 字符盘面一键存/读
// =====================================================================
// NOLINTBEGIN(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity)

#include "bgt.h"

#include <cstdio>

int main()
{
    constexpr int window_width = 800;
    constexpr int window_height = 700;

    if (!bgt_open_window(window_width, window_height, "libbgt 文件存档演示")) {
        bgt_print_error();
        return 1;
    }
    bgt_set_background(BGT_WHITE);

    // ---------- 读档：程序一启动就把存档读进内存 ----------
    // 第一次运行时 save.txt 还不存在：bgt_load 不报错，内存表为空，
    // 所有 bgt_get_* 都拿到默认值。
    bgt_load("save.txt");

    // 各板块共用的跨帧状态（就是普通变量）
    int part = 1; // 当前板块号：1 到 4

    // 板块 1：最高分
    int best = bgt_get_int("最高分", "best", 0);
    int score = 0;

    // 板块 3：玩家名字（从存档读，没有就先用默认名）
    char name[32] = {};
    bgt_get_string("玩家", "name", name, 32, "无名");
    char names[4][8] = {"张三", "李四", "王五", "小明"};
    // “无名”不在预设名单里：起始序号停在名单外，
    // 第一次按【N】先轮到“张三”，不会跳过它。
    int name_index = -1;

    // 板块 4：数独盘面（81 个字符，'.' 表示空格）
    // 没存过盘面时，get 的默认值就是当前屏幕上的盘面（原地保持）。
    char board[82] =
        "53..7....6..195....98....6.8...6...34..8.3..1"
        "7...2...6.6....28....419..5....8..79";
    bgt_get_string("盘面", "board", board, 82, board);
    // 手改存档可能把盘面改短：结束符后面的格子补成“空”（.），
    // 免得上一次盘面的残尾数字混进画面。
    int board_end = 0;
    while (board_end < 81 && board[board_end] != '\0') {
        board_end = board_end + 1;
    }
    while (board_end < 81) {
        board[board_end] = '.';
        board_end = board_end + 1;
    }
    int cursor_row = 0;
    int cursor_col = 0;
    char action_text[64] = "按方向键选格子，回车改数字";

    while (bgt_window_is_open()) {
        // ---------- 板块 1：最高分 ----------
        if (part == 1) {
            bgt_set_window_title("libbgt 文件存档演示 - 板块 1：最高分");
            if (bgt_key_just_pressed(BGT_KEY_ENTER)) {
                score = score + 10;
                if (score > best) {
                    best = score;
                    bgt_set_int("最高分", "best", best);
                    bgt_save("save.txt"); // 破纪录立刻写盘
                }
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 1：最高分 —— 关掉再开，最高分还在", 32);
            char score_text[32] = {};
            std::snprintf(score_text, sizeof(score_text), "本次得分：%d",
                          score);
            bgt_draw_text(80, 150, score_text, 28);
            char best_text[32] = {};
            std::snprintf(best_text, sizeof(best_text), "最高分：%d", best);
            bgt_draw_text(80, 200, best_text, 28);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(80, 300,
                          "每按一次【回车】得分 +10；破纪录时立刻写进 "
                          "save.txt。", 22);
            bgt_draw_text(80, 340,
                          "关掉这个窗口，再重新运行程序——最高分还在！", 22);
        }

        // ---------- 板块 2：记事本透明度 ----------
        if (part == 2) {
            bgt_set_window_title("libbgt 文件存档演示 - 板块 2：存档内容");
            if (bgt_key_just_pressed(BGT_KEY_R)) {
                bgt_clear_error();
                bgt_load("save.txt"); // 把文件重新读进内存
                best = bgt_get_int("最高分", "best", 0);
                bgt_get_string("玩家", "name", name, 32, "无名");
                bgt_get_string("盘面", "board", board, 82, board);
                // 盘面同样补尾（见程序开头）
                int board_end = 0;
                while (board_end < 81 && board[board_end] != '\0') {
                    board_end = board_end + 1;
                }
                while (board_end < 81) {
                    board[board_end] = '.';
                    board_end = board_end + 1;
                }
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 2：save.txt 的内容 —— 文本存档看得见", 32);
            bgt_draw_text(80, 130, "存档文件 save.txt 现在长这样（按字典序）：", 22);
            // 存档没有“整表打印”函数：每个键都是自己存进去的，
            // 直接逐个取出来，照文件里的样子画。
            bgt_set_color(BGT_BLUE);
            char line_text[96] = {};
            int y = 170;
            std::snprintf(line_text, sizeof(line_text), "[最高分]  best=%d",
                          bgt_get_int("最高分", "best", 0));
            bgt_draw_text(80, y, line_text, 22);
            y = y + 34;
            std::snprintf(line_text, sizeof(line_text), "[玩家]  name=%s",
                          name);
            bgt_draw_text(80, y, line_text, 22);
            y = y + 34;
            char board_head[48] = {};
            std::snprintf(board_head, sizeof(board_head), "%.24s...", board);
            std::snprintf(line_text, sizeof(line_text), "[盘面]  board=%s",
                          board_head);
            bgt_draw_text(80, y, line_text, 22);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(80, 330,
                          "用记事本打开 save.txt 改一改（比如把 best 改成 "
                          "999），", 22);
            bgt_draw_text(80, 370,
                          "保存后回到这里按【R】——改动立刻生效。存档就是普通"
                          "文本！", 22);
        }

        // ---------- 板块 3：玩家名字 ----------
        if (part == 3) {
            bgt_set_window_title("libbgt 文件存档演示 - 板块 3：玩家名字");
            if (bgt_key_just_pressed(BGT_KEY_N)) {
                name_index = (name_index + 1) % 4;
                for (int i = 0; i < 8; i = i + 1) {
                    name[i] = names[name_index][i];
                }
                bgt_set_string("玩家", "name", name);
                bgt_save("save.txt");
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 3：玩家名字 —— 字符串存进存档", 32);
            char name_text[64] = {};
            std::snprintf(name_text, sizeof(name_text), "当前玩家：%s", name);
            bgt_draw_text(80, 200, name_text, 32);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(80, 320,
                          "按【N】换一个名字，换完立刻写进 save.txt。", 22);
            bgt_draw_text(80, 360,
                          "读字符串要有一个数组接住它：bgt_get_string。", 22);
        }

        // ---------- 板块 4：数独盘面 ----------
        if (part == 4) {
            bgt_set_window_title("libbgt 文件存档演示 - 板块 4：数独盘面");
            if (bgt_key_just_pressed(BGT_KEY_LEFT)) {
                cursor_col = (cursor_col + 8) % 9;
            }
            if (bgt_key_just_pressed(BGT_KEY_RIGHT)) {
                cursor_col = (cursor_col + 1) % 9;
            }
            if (bgt_key_just_pressed(BGT_KEY_UP)) {
                cursor_row = (cursor_row + 8) % 9;
            }
            if (bgt_key_just_pressed(BGT_KEY_DOWN)) {
                cursor_row = (cursor_row + 1) % 9;
            }
            if (bgt_key_just_pressed(BGT_KEY_ENTER)) {
                const int index = cursor_row * 9 + cursor_col;
                if (board[index] == '.') {
                    board[index] = '1';
                } else if (board[index] == '9') {
                    board[index] = '.';
                } else {
                    board[index] = board[index] + 1;
                }
            }
            if (bgt_key_just_pressed(BGT_KEY_S)) {
                bgt_set_string("盘面", "board", board);
                if (bgt_save("save.txt")) {
                    std::snprintf(action_text, sizeof(action_text),
                                  "已存档：盘面写进了 save.txt");
                } else {
                    std::snprintf(action_text, sizeof(action_text),
                                  "存档失败：save.txt 写不进去");
                }
            }
            if (bgt_key_just_pressed(BGT_KEY_L)) {
                if (bgt_load("save.txt")) {
                    bgt_get_string("盘面", "board", board, 82, board);
                    // 盘面同样补尾（见程序开头）
                    int board_end = 0;
                    while (board_end < 81 && board[board_end] != '\0') {
                        board_end = board_end + 1;
                    }
                    while (board_end < 81) {
                        board[board_end] = '.';
                        board_end = board_end + 1;
                    }
                    std::snprintf(action_text, sizeof(action_text),
                                  "已读档：从 save.txt 恢复盘面");
                } else {
                    std::snprintf(action_text, sizeof(action_text),
                                  "读档失败：save.txt 读不出来");
                }
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 4：数独盘面 —— 81 个字符一键存/读", 32);
            // 9x9 网格：每个字符画一个格子，光标格子填黄色
            constexpr int cell = 36;
            constexpr int grid_x = 80;
            constexpr int grid_y = 110;
            for (int row = 0; row < 9; row = row + 1) {
                for (int col = 0; col < 9; col = col + 1) {
                    const int index = row * 9 + col;
                    const int cell_x = grid_x + col * cell;
                    const int cell_y = grid_y + row * cell;
                    if (row == cursor_row && col == cursor_col) {
                        bgt_set_color(BGT_YELLOW);
                        bgt_fill_rect(cell_x, cell_y, cell, cell);
                    }
                    char cell_text[2] = {};
                    cell_text[0] = board[index];
                    if (board[index] == '.') {
                        bgt_set_color(BGT_LIGHT_GRAY);
                    } else {
                        bgt_set_color(BGT_BLACK);
                    }
                    bgt_draw_text(cell_x + 8, cell_y + 4, cell_text, 24);
                }
            }
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(80, 470, action_text, 22);
            bgt_draw_text(80, 510,
                          "方向键移动光标，回车改数字；【S】存盘面，【L】读盘"
                          "面。", 22);
            bgt_draw_text(80, 550,
                          "改几个格子，按【S】存档，关掉程序再运行按【L】——"
                          "盘面还在。", 22);
        }

        // 底部统一提示
        bgt_set_color(BGT_DARK_GRAY);
        if (part < 4) {
            bgt_draw_text(40, 648,
                          "按【空格】进入下一板块，按【Esc】退出程序", 24);
        } else {
            bgt_draw_text(40, 648,
                          "按【Esc】退出程序 —— save.txt 留在程序目录里", 24);
        }

        // 每个板块都支持按空格进入下一板块；最后一个板块按空格关闭
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
