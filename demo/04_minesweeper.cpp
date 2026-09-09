#include "bgt.h"

#include <cstdio>
#include <cstring>

// NOLINTBEGIN(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity, bugprone-easily-swappable-parameters)

// 扫雷演示：续玩提示 / 游戏 / 胜利 / 踩雷四态流程。首踩格的 3×3 从候选雷
// 区排除，第一格必为 0 并连锁展开；退出自动存档（战绩 + 未完成局面双节），
// 启动检测到未完成局面先进入续玩提示。音效来自 exe 旁的 04_*.wav，加载失
// 败静默降级。

namespace {

constexpr int kWindowWidth = 700;
constexpr int kWindowHeight = 600;
constexpr int kTargetFps = 60;

// 棋盘与雷数可在此调整（改大需同时确认窗口放得下）
constexpr int kGridSize = 9;
constexpr int kMineCount = 10;
constexpr int kCellCount = kGridSize * kGridSize;  // 81
constexpr int kCellSize = 44;
constexpr int kBoardX = 32;
constexpr int kBoardY = 96;
constexpr int kBoardPx = kGridSize * kCellSize;  // 396，棋盘底缘 492
constexpr int kPanelX = 460;      // = 32 + 396 + 32
constexpr int kPanelWidth = 216;  // 右缘 676，窗口余 24

constexpr unsigned kColorBackground = 0xFFC0C6C0U; // bgt_rgb(192, 198, 192)
constexpr unsigned kColorCovered = 0xFFD7DBD7U; // bgt_rgb(215, 219, 215)
constexpr unsigned kColorCoveredBorder = 0xFF969C96U; // bgt_rgb(150, 156, 150)
constexpr unsigned kColorRevealed = 0xFFEEEEEEU; // bgt_rgb(238, 238, 238)
constexpr unsigned kColorPanel = 0xFFAAB2AAU; // bgt_rgb(170, 178, 170)

// 数字配色 1..8（经典扫雷）
const unsigned kNumberColors[8] = {BGT_BLUE, BGT_GREEN, BGT_RED, BGT_PURPLE,
                                   BGT_BROWN, BGT_CYAN, BGT_BLACK, BGT_GRAY};

// 存档：节名与键名逐字
// [战绩] wins / best      —— best 存整数秒，0 表示无纪录
// [局面] board / mines / time
const char kSaveFile[] = "minesweeper_save.txt";

constexpr int kStatePrompt = 0;
constexpr int kStatePlaying = 1;
constexpr int kStateWon = 2;
constexpr int kStateLost = 3;

// ---- 局面数据（不引入类、结构体和指针，全部用全局变量与数组）----

int part;                     // 状态机，取值为 kState* 常量
bool cell_revealed[kCellCount];
bool cell_flagged[kCellCount];
bool cell_mine[kCellCount];
int cell_number[kCellCount];  // 邻域雷数，布雷后计算
bool mines_placed = false;
int revealed_safe = 0;        // 已翻开的非雷格数
int flags = 0;                // 当前插旗数
int exploded_index = -1;      // 踩中的雷格下标（-1 无）
double start_time = 0.0;
int elapsed_saved = 0;        // 续玩带入的已用秒数
int wins = 0;
int best_time = 0;            // 0 = 无纪录
int win_seconds = 0;          // 本局胜利用时
bool new_best = false;
bool has_resume = false;      // 启动检测到未完成局面
char saved_board[128] = {};   // 启动读入的局面串（按 L 时解析）
char saved_mines[128] = {};
int snd_mine = 0;             // bgt_load_sound 结果，0 = 没加载上
int snd_win = 0;

// ---- 辅助 ----

int cell_index(int row, int col)
{
    return row * kGridSize + col;
}

bool in_grid(int row, int col)
{
    return row >= 0 && row < kGridSize && col >= 0 && col < kGridSize;
}

void play(int id)
{
    if (id > 0) {
        bgt_play_sound(id);
    }
}

int elapsed_seconds()
{
    return elapsed_saved + static_cast<int>(bgt_total_time() - start_time);
}

// ---- 流程控制 ----

void fresh_game()
{
    // 三个状态数组清零；cell_number 由布雷后的 compute_numbers() 全量重算
    for (int i = 0; i < kCellCount; ++i) {
        cell_revealed[i] = false;
        cell_flagged[i] = false;
        cell_mine[i] = false;
    }
    mines_placed = false;
    revealed_safe = 0;
    flags = 0;
    exploded_index = -1;
    new_best = false;
    elapsed_saved = 0;
    start_time = bgt_total_time();
}

// ---- 布雷与展开 ----

void compute_numbers()
{
    for (int row = 0; row < kGridSize; ++row) {
        for (int col = 0; col < kGridSize; ++col) {
            int count = 0;
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) {
                        continue;  // 不算自己
                    }
                    const int r = row + dr;
                    const int c = col + dc;
                    if (in_grid(r, c) && cell_mine[cell_index(r, c)]) {
                        count += 1;
                    }
                }
            }
            cell_number[cell_index(row, col)] = count;
        }
    }
}

void place_mines(int row, int col)
{
    // 候选 = 全部 81 格去掉首踩格 3×3 邻域（按边界裁剪）：内部首踩 n=72、
    // 边线 75、角落 77。数组取 81 上界：候选最多 77 个，写死 72 会越界。
    int candidates[kCellCount];
    int n = 0;
    for (int r = 0; r < kGridSize; ++r) {
        for (int c = 0; c < kGridSize; ++c) {
            const bool near_first = r >= row - 1 && r <= row + 1 &&
                                    c >= col - 1 && c <= col + 1;
            if (!near_first) {
                candidates[n] = cell_index(r, c);
                n += 1;
            }
        }
    }
    // Fisher-Yates 洗牌前 n 个候选，前 kMineCount 个即雷。区间用实际 n：
    // 保证每个候选格等概率入雷；写死 72 时边线/角落首踩的尾部候选永远
    // 不参与交换（采样有偏）。
    for (int i = 0; i < n - 1; ++i) {
        const int j = bgt_random(i, n);  // 半开 [i, n)
        const int tmp = candidates[i];
        candidates[i] = candidates[j];
        candidates[j] = tmp;
    }
    for (int i = 0; i < kCellCount; ++i) {
        cell_mine[i] = false;
    }
    for (int i = 0; i < kMineCount; ++i) {
        cell_mine[candidates[i]] = true;
    }
    compute_numbers();
    mines_placed = true;
}

void reveal_cell(int row, int col)
{
    // 调用方已保证该格未翻、未插旗、非雷（本函数不重复检查雷）
    const int idx = cell_index(row, col);
    if (cell_revealed[idx] || cell_flagged[idx]) {
        return;
    }
    cell_revealed[idx] = true;
    revealed_safe += 1;
    if (cell_number[idx] == 0) {
        // 0 格连锁：8 邻域逐个递归（边界判定）
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) {
                    continue;
                }
                const int r = row + dr;
                const int c = col + dc;
                if (in_grid(r, c)) {
                    reveal_cell(r, c);
                }
            }
        }
    }
}

// ---- 存档 ----

void build_board_string(char out[])
{
    // board：已翻→('0'+数字)，插旗→'F'，否则→'.'
    for (int i = 0; i < kCellCount; ++i) {
        if (cell_revealed[i]) {
            out[i] = static_cast<char>('0' + cell_number[i]);
        } else if (cell_flagged[i]) {
            out[i] = 'F';
        } else {
            out[i] = '.';
        }
    }
    out[kCellCount] = '\0';
}

void build_mines_string(char out[])
{
    // mines：雷→'X'，否则→'.'
    for (int i = 0; i < kCellCount; ++i) {
        out[i] = cell_mine[i] ? 'X' : '.';
    }
    out[kCellCount] = '\0';
}

void parse_board_string()
{
    // 从启动读入的两条 81 字符串恢复棋盘变量
    for (int i = 0; i < kCellCount; ++i) {
        cell_revealed[i] = false;
        cell_flagged[i] = false;
        cell_mine[i] = false;
        cell_number[i] = 0;
    }
    flags = 0;
    revealed_safe = 0;
    exploded_index = -1;
    for (int i = 0; i < kCellCount; ++i) {
        if (saved_board[i] == 'F') {
            cell_flagged[i] = true;
        } else if (saved_board[i] >= '0' && saved_board[i] <= '8') {
            cell_revealed[i] = true;
            cell_number[i] = saved_board[i] - '0';
        }
        if (saved_mines[i] == 'X') {
            cell_mine[i] = true;
        }
    }
    // 以 mines 为准重算数字，不信任存档里存的数字
    compute_numbers();
    for (int i = 0; i < kCellCount; ++i) {
        if (cell_flagged[i]) {
            flags += 1;
        }
        if (cell_revealed[i] && !cell_mine[i]) {
            revealed_safe += 1;  // 只数非雷已翻格
        }
    }
    mines_placed = true;
    elapsed_saved = bgt_get_int("局面", "time", 0);
    start_time = bgt_total_time();
}

void load_records()
{
    // 首次运行文件不存在：bgt_load 不报错，内存表为空，全部读到默认值
    bgt_load(kSaveFile);
    wins = bgt_get_int("战绩", "wins", 0);
    best_time = bgt_get_int("战绩", "best", 0);
    bgt_get_string("局面", "board", saved_board, 128, "");
    bgt_get_string("局面", "mines", saved_mines, 128, "");
    has_resume = std::strlen(saved_board) == kCellCount &&
                 std::strlen(saved_mines) == kCellCount;
    if (has_resume) {
        // 先解析恢复到棋盘变量：提示屏要能画出存档局面
        parse_board_string();
        part = kStatePrompt;
    } else {
        fresh_game();
        part = kStatePlaying;
    }
}

void save_on_exit()
{
    bgt_set_int("战绩", "wins", wins);
    bgt_set_int("战绩", "best", best_time);
    // 存局条件：已布雷且停在游戏/提示态。提示屏退出必须原样保留存档局面，
    // 否则会误清存档；其余情形（胜利/失败/未布雷）存空串 = 无续玩
    if (mines_placed && (part == kStatePlaying || part == kStatePrompt)) {
        char board_str[kCellCount + 1];
        char mines_str[kCellCount + 1];
        build_board_string(board_str);
        build_mines_string(mines_str);
        bgt_set_string("局面", "board", board_str);
        bgt_set_string("局面", "mines", mines_str);
        bgt_set_int("局面", "time", elapsed_seconds());
    } else {
        // 键保留但值清空，读回得空串自然判无
        bgt_set_string("局面", "board", "");
    }
    bgt_save(kSaveFile);  // 返回值忽略——退出路径不弹错
}

// ---- 每帧输入（仅状态 1）----

void handle_left_click()
{
    if (!bgt_mouse_just_pressed(BGT_MOUSE_LEFT)) {
        return;
    }
    // 命中范围与棋盘矩形一致：含左/上边界，不含右/下边界
    const int mx = bgt_mouse_x();
    const int my = bgt_mouse_y();
    if (mx < kBoardX || mx >= kBoardX + kBoardPx || my < kBoardY ||
        my >= kBoardY + kBoardPx) {
        return;
    }
    const int col = (mx - kBoardX) / kCellSize;
    const int row = (my - kBoardY) / kCellSize;
    const int idx = cell_index(row, col);
    if (cell_revealed[idx] || cell_flagged[idx]) {
        return;  // 已翻或已插旗：左键无操作
    }
    if (!mines_placed) {
        // 首踩布雷：首格 3×3 已从候选排除，首格必 0，必连锁展开一片
        place_mines(row, col);
    }
    if (cell_mine[idx]) {
        exploded_index = idx;
        part = kStateLost;
        play(snd_mine);
        return;
    }
    reveal_cell(row, col);
    if (revealed_safe == kCellCount - kMineCount) {
        win_seconds = elapsed_seconds();
        wins += 1;
        if (best_time == 0 || win_seconds < best_time) {
            best_time = win_seconds;
            new_best = true;
        }
        // 胜利战绩立刻写盘，不等退出
        bgt_set_int("战绩", "wins", wins);
        bgt_set_int("战绩", "best", best_time);
        bgt_save(kSaveFile);
        play(snd_win);
        part = kStateWon;
    }
}

void handle_right_click()
{
    if (!bgt_mouse_just_pressed(BGT_MOUSE_RIGHT)) {
        return;
    }
    const int mx = bgt_mouse_x();
    const int my = bgt_mouse_y();
    if (mx < kBoardX || mx >= kBoardX + kBoardPx || my < kBoardY ||
        my >= kBoardY + kBoardPx) {
        return;
    }
    const int col = (mx - kBoardX) / kCellSize;
    const int row = (my - kBoardY) / kCellSize;
    const int idx = cell_index(row, col);
    if (cell_revealed[idx]) {
        return;  // 已翻格右键无操作
    }
    if (cell_flagged[idx]) {
        cell_flagged[idx] = false;
        flags -= 1;
    } else {
        cell_flagged[idx] = true;
        flags += 1;
    }
}

// ---- 绘制 ----

void draw_center_text(const char text[], int y, int size)
{
    // 以棋盘水平中央为轴居中
    const int cx = kBoardX + kBoardPx / 2;
    const int x = cx - bgt_text_width(text, size) / 2;
    bgt_draw_text(x, y, text, size);
}

void draw_board()
{
    for (int row = 0; row < kGridSize; ++row) {
        for (int col = 0; col < kGridSize; ++col) {
            const int idx = cell_index(row, col);
            const int x = kBoardX + col * kCellSize;
            const int y = kBoardY + row * kCellSize;
            if (cell_revealed[idx]) {
                bgt_set_color(kColorRevealed);
                bgt_fill_rect(x, y, kCellSize, kCellSize);
                if (cell_number[idx] >= 1) {
                    char digit[2];
                    digit[0] = static_cast<char>('0' + cell_number[idx]);
                    digit[1] = '\0';
                    bgt_set_color(kNumberColors[cell_number[idx] - 1]);
                    const int tw = bgt_text_width(digit, 26);
                    const int th = bgt_text_height(digit, 26);
                    bgt_draw_text(x + (kCellSize - tw) / 2,
                                  y + (kCellSize - th) / 2, digit, 26);
                }
            } else {
                if (part == kStateLost && idx == exploded_index) {
                    bgt_set_color(BGT_RED);  // 踩中的雷格：红底衬雷
                    bgt_fill_rect(x, y, kCellSize, kCellSize);
                } else {
                    bgt_set_color(kColorCovered);
                    bgt_fill_rect(x, y, kCellSize, kCellSize);
                    bgt_set_color(kColorCoveredBorder);
                    bgt_draw_rect(x, y, kCellSize, kCellSize);
                }
                if (cell_flagged[idx]) {
                    bgt_set_color(BGT_BLACK);
                    bgt_draw_line(x + 22, y + 10, x + 22, y + 32);  // 旗杆
                    bgt_set_color(BGT_RED);
                    bgt_fill_triangle(x + 22, y + 10, x + 22, y + 22,
                                      x + 10, y + 16);  // 旗面
                }
            }
            if (part == kStateLost && cell_mine[idx]) {
                bgt_set_color(BGT_BLACK);
                bgt_fill_circle(x + 22, y + 22, 9);  // 雷体
            }
        }
    }
}

void draw_panel()
{
    bgt_set_color(kColorPanel);
    bgt_fill_rect(kPanelX, 0, kPanelWidth, kWindowHeight);
    const int text_x = kPanelX + 24;
    char line[64];

    bgt_set_color(BGT_BLACK);
    bgt_draw_text(text_x, 96, "扫雷", 36);
    std::snprintf(line, sizeof(line), "剩余雷数：%d", kMineCount - flags);
    bgt_draw_text(text_x, 170, line, 24);
    std::snprintf(line, sizeof(line), "时间：%d 秒", elapsed_seconds());
    bgt_draw_text(text_x, 210, line, 24);
    std::snprintf(line, sizeof(line), "胜场：%d", wins);
    bgt_draw_text(text_x, 250, line, 24);
    if (best_time == 0) {
        bgt_draw_text(text_x, 290, "最快纪录：无", 24);
    } else {
        std::snprintf(line, sizeof(line), "最快纪录：%d 秒", best_time);
        bgt_draw_text(text_x, 290, line, 24);
    }

    bgt_set_color(BGT_DARK_GRAY);
    bgt_draw_text(text_x, 400, "【R】重开一局", 20);

    if (part == kStatePlaying) {
        bgt_draw_text(text_x, 440, "左键翻格，右键插旗", 22);
    } else if (part == kStateWon) {
        bgt_set_color(BGT_GREEN);
        std::snprintf(line, sizeof(line), "胜利！用时 %d 秒", win_seconds);
        bgt_draw_text(text_x, 440, line, 22);
        if (new_best) {
            bgt_set_color(BGT_RED);
            bgt_draw_text(text_x, 470, "新纪录！", 24);
        }
    } else if (part == kStateLost) {
        bgt_set_color(BGT_RED);
        bgt_draw_text(text_x, 440, "踩雷了！", 22);
    }
}

void draw_frame()
{
    draw_board();
    if (part == kStatePrompt) {
        // 续玩提示：底条白底黑字，压在已恢复的棋盘中央
        bgt_set_color(kColorRevealed);
        bgt_fill_rect(kBoardX, 180, kBoardPx, 150);
        bgt_set_color(BGT_BLACK);
        draw_center_text("检测到未完成的一局！", 210, 32);
        draw_center_text("按【L】继续上局，按【N】开新局", 270, 24);
    } else if (part == kStateWon) {
        bgt_set_color(BGT_GREEN);
        draw_center_text("胜利！", 240, 48);
    } else if (part == kStateLost) {
        bgt_set_color(BGT_RED);
        draw_center_text("踩雷了！", 240, 48);
    }
    draw_panel();
}

} // namespace

int main()
{
    if (!bgt_open_window(kWindowWidth, kWindowHeight, "libbgt 扫雷")) {
        bgt_print_error();
        return 1;
    }

    bgt_set_fps_limit(kTargetFps);
    bgt_set_background(kColorBackground);

    // 音效加载（开窗后）：文件在 exe 旁，加载失败静默降级
    snd_mine = bgt_load_sound("04_mine.wav");
    snd_win = bgt_load_sound("04_win.wav");

    // 启动：读档 → 战绩恢复 + 未完成局面检测
    load_records();

    while (bgt_window_is_open()) {
        if (bgt_key_just_pressed(BGT_KEY_ESCAPE)) {
            break;
        }

        if (part == kStatePrompt) {
            if (bgt_key_just_pressed(BGT_KEY_L)) {
                parse_board_string();  // 按 L：再解析一次并重置计时基准
                part = kStatePlaying;
            } else if (bgt_key_just_pressed(BGT_KEY_N)) {
                fresh_game();  // 丢弃存档局面，开新局
                part = kStatePlaying;
            }
        } else if (part == kStatePlaying) {
            if (bgt_key_just_pressed(BGT_KEY_R)) {
                fresh_game();  // R：重开一局，停在状态 1
            }
            handle_left_click();
            handle_right_click();
        } else if (part == kStateWon || part == kStateLost) {
            if (bgt_key_just_pressed(BGT_KEY_R)) {
                fresh_game();
                part = kStatePlaying;
            }
        }

        draw_frame();
        bgt_update_window();
    }

    // 退出自动存：主循环结束后、关窗前
    save_on_exit();
    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity, bugprone-easily-swappable-parameters)
