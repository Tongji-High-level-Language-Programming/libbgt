#include "bgt.h"

#include <cstdio>

// NOLINTBEGIN(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity, bugprone-easily-swappable-parameters)

// 汉诺塔演示：开始/游戏中/完成三态流程；游戏中支持手动游玩与递归自动演示。
// 参考 docs/exercises.md 的作业题库（汉诺塔一节）；本文件是其参考实现。

namespace {

constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 600;
constexpr int kTargetFps = 60;

constexpr int kTowerCount = 3;
constexpr int kDisks = 4;

constexpr int kTowerSpacing = 200;
constexpr int kFirstTowerX = kWindowWidth / 2 - kTowerSpacing;
constexpr int kTowerWidth = 20;
constexpr int kTowerHeight = 240;
constexpr int kBaseY = 540;

constexpr int kDiskHeight = 36;
constexpr int kMinDiskWidth = 48;
constexpr int kDiskWidthStep = 30;

constexpr int kMoveSpeed = 2;     // 每秒完成的移动数
constexpr double kAutoInterval = 0.6;  // 自动演示中两次移动的间隔（秒）
constexpr double kSelectRepeat = 0.18; // 按住方向键时重复选柱的间隔（秒）
constexpr int kMaxAutoSteps = (1 << kDisks) - 1;

constexpr int kStateStart = 0;
constexpr int kStatePlaying = 1;
constexpr int kStateDone = 2;

constexpr unsigned kDiskColors[kDisks + 1] = {BGT_BLACK, BGT_RED, BGT_BLUE,
                                              BGT_GREEN, BGT_ORANGE};

// ---- 局面数据（不引入类、结构体和指针，全部用全局变量与数组）----

int towers[kTowerCount][kDisks];    // 每根柱子从下往上存的盘号
int heights[kTowerCount];           // 每根柱子当前盘数
int state = kStateStart;
int selected = 0;                   // 当前高亮柱子
double select_repeat_timer = 0.0;   // 按住方向键时的重复选柱计时
int move_count = 0;

bool holding = false;               // 隐式状态：是否拿着盘子
int held_disk = 0;
int pickup_from = 0;                // 盘子是从哪根柱子拿起来的

double anim_t = 1.0;                // 动画进度，1.0 表示没有动画
int anim_from = 0;
int anim_to = 0;
double hint_time = -1.0;            // 非法移动提示的开始时间

bool auto_mode = false;             // 隐式状态：是否在自动演示
double auto_timer = 0.0;
int auto_index = 0;
int auto_write = 0;
int auto_moves[kMaxAutoSteps][2];   // 递归生成的移动序列：from -> to

int tower_x(int tower)
{
    return kFirstTowerX + tower * kTowerSpacing;
}

int disk_width(int disk)
{
    return kMinDiskWidth + disk * kDiskWidthStep;
}

void reset_game()
{
    for (int t = 0; t < kTowerCount; ++t) {
        heights[t] = 0;
        for (int i = 0; i < kDisks; ++i) {
            towers[t][i] = 0;
        }
    }
    for (int i = 0; i < kDisks; ++i) {
        towers[0][i] = kDisks - i;   // 大盘在下，小盘在上
        heights[0] += 1;
    }
    selected = 0;
    select_repeat_timer = 0.0;
    move_count = 0;
    holding = false;
    anim_t = 1.0;
    hint_time = -1.0;
    auto_mode = false;
    auto_timer = 0.0;
    auto_index = 0;
}

// ---- 递归求解：课堂终端版 hanoi 的图形版，只改"记录方式"，不改算法 ----

void hanoi_record(int n, int from, int to, int via)
{
    if (n == 1) {
        auto_moves[auto_write][0] = from;
        auto_moves[auto_write][1] = to;
        auto_write += 1;
        return;
    }
    hanoi_record(n - 1, from, via, to);
    auto_moves[auto_write][0] = from;
    auto_moves[auto_write][1] = to;
    auto_write += 1;
    hanoi_record(n - 1, via, to, from);
}

void start_auto_demo()
{
    reset_game();
    auto_write = 0;
    hanoi_record(kDisks, 0, kTowerCount - 1, 1);
    auto_index = 0;
    auto_timer = 0.0;
    auto_mode = true;
}

// ---- 动画：上移 -> 平移 -> 下落，进度用 delta_time 推进 ----

void advance_animation()
{
    if (anim_t >= 1.0) {
        return;
    }
    anim_t += bgt_delta_time() * kMoveSpeed;
    if (anim_t >= 1.0) {
        anim_t = 1.0;
        towers[anim_to][heights[anim_to]] = held_disk;
        heights[anim_to] += 1;
        holding = false;
        move_count += 1;
        if (heights[kTowerCount - 1] == kDisks) {
            state = kStateDone;
        }
    }
}

// ---- 选柱：点按立即移动一格，按住时按 kSelectRepeat 间隔重复 ----

void move_selection()
{
    select_repeat_timer += bgt_delta_time();
    const bool left_held = bgt_key_is_down(BGT_KEY_LEFT);
    const bool right_held = bgt_key_is_down(BGT_KEY_RIGHT);
    if (left_held && !right_held) {
        if (bgt_key_just_pressed(BGT_KEY_LEFT) ||
            select_repeat_timer >= kSelectRepeat) {
            if (selected > 0) {
                selected -= 1;
            }
            select_repeat_timer = 0.0;
        }
    } else if (right_held && !left_held) {
        if (bgt_key_just_pressed(BGT_KEY_RIGHT) ||
            select_repeat_timer >= kSelectRepeat) {
            if (selected < kTowerCount - 1) {
                selected += 1;
            }
            select_repeat_timer = 0.0;
        }
    } else {
        select_repeat_timer = 0.0;
    }
}

void update_playing()
{
    if (bgt_key_just_pressed(BGT_KEY_R)) {
        reset_game();
        state = kStateStart;
        return;
    }

    if (auto_mode) {
        if (bgt_key_just_pressed(BGT_KEY_S)) {
            auto_mode = false;   // 停止演示，当前局面交还手动
        }
        if (auto_mode && anim_t >= 1.0 && auto_index < kMaxAutoSteps) {
            auto_timer += bgt_delta_time();
            if (auto_timer >= kAutoInterval) {
                auto_timer = 0.0;
                const int from = auto_moves[auto_index][0];
                const int to = auto_moves[auto_index][1];
                auto_index += 1;
                if (heights[from] > 0) {
                    held_disk = towers[from][heights[from] - 1];
                    heights[from] -= 1;
                    pickup_from = from;
                    holding = true;
                    anim_from = from;
                    anim_to = to;
                    selected = to;   // 高亮跟随演示步骤
                    anim_t = 0.0;
                }
            }
        }
        advance_animation();
        return;
    }

    if (bgt_key_just_pressed(BGT_KEY_S)) {
        start_auto_demo();
        return;
    }

    if (anim_t >= 1.0) {
        move_selection();

        if (!holding) {
            if (bgt_key_just_pressed(BGT_KEY_SPACE) && heights[selected] > 0) {
                held_disk = towers[selected][heights[selected] - 1];
                heights[selected] -= 1;
                pickup_from = selected;
                holding = true;
            }
        } else if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
            const bool legal = heights[selected] == 0 ||
                towers[selected][heights[selected] - 1] > held_disk;
            if (legal) {
                anim_from = pickup_from;
                anim_to = selected;
                anim_t = 0.0;
            } else {
                hint_time = bgt_total_time();
            }
        }
    }

    advance_animation();   // 动画播放中，忽略其他输入
}

// ---- 绘制 ----

void draw_disks()
{
    for (int t = 0; t < kTowerCount; ++t) {
        for (int i = 0; i < heights[t]; ++i) {
            const int disk = towers[t][i];
            const int w = disk_width(disk);
            const int x = tower_x(t) - w / 2;
            const int y = kBaseY - (i + 1) * kDiskHeight;
            bgt_set_color(kDiskColors[disk]);
            bgt_fill_rect(x, y, w, kDiskHeight);
            bgt_set_color(BGT_BLACK);
            bgt_draw_rect(x, y, w, kDiskHeight);
        }
    }
}

void draw_held_disk()
{
    if (!holding) {
        return;
    }

    int x = 0;
    int y = 0;
    if (anim_t < 1.0) {
        const int x_from = tower_x(anim_from);
        const int x_to = tower_x(anim_to);
        const int y_from = kBaseY - (heights[anim_from] + 2) * kDiskHeight;
        const int y_to = kBaseY - (heights[anim_to] + 1) * kDiskHeight;
        constexpr int kCarryY = kBaseY - kTowerHeight - kDiskHeight - 20;
        constexpr double kLiftEnd = 0.35;
        constexpr double kCarryEnd = 0.65;
        if (anim_t < kLiftEnd) {
            const double k = anim_t / kLiftEnd;
            x = x_from;
            y = y_from + static_cast<int>((kCarryY - y_from) * k);
        } else if (anim_t < kCarryEnd) {
            const double k = (anim_t - kLiftEnd) / (kCarryEnd - kLiftEnd);
            x = x_from + static_cast<int>((x_to - x_from) * k);
            y = kCarryY;
        } else {
            const double k = (anim_t - kCarryEnd) / (1.0 - kCarryEnd);
            x = x_to;
            y = kCarryY + static_cast<int>((y_to - kCarryY) * k);
        }
    } else {
        // 悬在被取出的柱子上方，水平位置不变
        x = tower_x(pickup_from);
        y = kBaseY - (heights[pickup_from] + 2) * kDiskHeight;
    }

    const int w = disk_width(held_disk);
    bgt_set_color(kDiskColors[held_disk]);
    bgt_fill_rect(x - w / 2, y, w, kDiskHeight);
    bgt_set_color(BGT_BLACK);
    bgt_draw_rect(x - w / 2, y, w, kDiskHeight);
}

void draw_marker()
{
    const char marker[] = "▼";
    const int w = bgt_text_width(marker, 30);
    bgt_set_color(BGT_ORANGE);
    bgt_draw_text(tower_x(selected) - w / 2, kBaseY - kTowerHeight - 48,
                  marker, 30);
}

void draw_center_text(int cx, int y, const char text[], int size)
{
    const int w = bgt_text_width(text, size);
    bgt_draw_text(cx - w / 2, y, text, size);
}

void draw_frame()
{
    bgt_set_background(bgt_rgb(248, 246, 240));

    // 底座与柱子
    bgt_set_color(bgt_rgb(150, 118, 78));
    for (int t = 0; t < kTowerCount; ++t) {
        bgt_fill_rect(tower_x(t) - kTowerWidth / 2, kBaseY - kTowerHeight,
                      kTowerWidth, kTowerHeight);
    }
    bgt_set_color(BGT_DARK_GRAY);
    bgt_fill_rect(40, kBaseY, kWindowWidth - 80, 16);

    draw_disks();
    draw_held_disk();
    draw_marker();

    // 标题与状态提示
    bgt_set_color(BGT_BLACK);
    bgt_draw_text(40, 22, "汉诺塔", 34);

    char info[64];
    if (state == kStateStart) {
        bgt_draw_text(40, 70, "按空格开始游戏（Esc 退出）", 20);
    } else if (state == kStatePlaying && auto_mode) {
        bgt_draw_text(40, 70,
                      "自动演示中…（S 停止，R 重新开始，Esc 退出）", 20);
    } else if (state == kStatePlaying) {
        bgt_draw_text(40, 70,
                      "←/→ 选柱，空格 拿起/放下，S 自动演示，R 重新开始", 20);
    } else {
        std::snprintf(info, sizeof(info),
                      "完成！共 %d 步，最少 %d 步。按 R 重新开始", move_count,
                      kMaxAutoSteps);
        bgt_draw_text(40, 70, info, 20);
    }

    std::snprintf(info, sizeof(info), "步数：%d", move_count);
    const int info_width = bgt_text_width(info, 20);
    bgt_draw_text(kWindowWidth - info_width - 40, 30, info, 20);

    if (hint_time >= 0.0 && bgt_total_time() - hint_time < 1.0) {
        bgt_set_color(BGT_RED);
        draw_center_text(kWindowWidth / 2, 150, "不能把大盘压在小盘上！", 24);
    }
}

} // namespace

int main()
{
    if (!bgt_open_window(kWindowWidth, kWindowHeight, "libbgt 汉诺塔")) {
        bgt_print_error();
        return 1;
    }

    bgt_set_fps_limit(kTargetFps);
    reset_game();

    while (bgt_window_is_open()) {
        if (bgt_key_just_pressed(BGT_KEY_ESCAPE)) {
            bgt_close_window();
            break;
        }

        if (state == kStateStart) {
            if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
                state = kStatePlaying;
            }
        } else if (state == kStatePlaying) {
            update_playing();
        } else if (state == kStateDone) {
            if (bgt_key_just_pressed(BGT_KEY_R)) {
                reset_game();
                state = kStateStart;
            }
        }

        draw_frame();
        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity, bugprone-easily-swappable-parameters)
