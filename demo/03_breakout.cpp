#include "bgt.h"

#include <cmath>
#include <cstdio>

// NOLINTBEGIN(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity, bugprone-easily-swappable-parameters)

// 打砖块演示：标题 / 游戏 / 过关 / 通关 / 结束五态流程。挡板反弹角随命中
// 偏移变化且速度模长恒定；音效来自 exe 旁的 03_*.wav，加载失败静默降级。

namespace {

constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 600;
constexpr int kTargetFps = 60;
constexpr int kFieldWidth = 600;        // 左墙 x=0，右墙 x=600
constexpr int kPanelX = 600;            // 右侧栏起点
constexpr int kPanelWidth = 200;
constexpr int kBrickCols = 10;
constexpr int kBrickRows = 6;
constexpr int kBrickCount = kBrickCols * kBrickRows;  // 60
constexpr int kBrickW = 56;
constexpr int kBrickH = 24;
constexpr int kBrickStepX = 58;        // 56 砖 + 2 缝
constexpr int kBrickStepY = 26;        // 24 砖 + 2 缝
constexpr int kBrickLeftX = 11;        // 10 列共 578px，左右各余 11
constexpr int kBrickTopY = 60;
constexpr int kBrickMissing = 3;       // 每关随机缺席砖数
constexpr int kLevels = 3;
constexpr int kPaddleW = 100;
constexpr int kPaddleH = 16;
constexpr int kPaddleY = 544;          // 距底 40
constexpr double kPaddleSpeed = 500.0; // px/s
constexpr int kBallR = 8;
constexpr double kBallSpeed = 320.0;   // px/s，全游戏恒定
constexpr double kPaddleBounceMax = 0.8;  // 挡板反弹 vx 上限比例
constexpr int kLives = 3;
constexpr int kBrickScore = 10;        // 每砖 10 分
// 过关奖励 = 50 * 关卡号（第 1 关 +50、第 2 关 +100、第 3 关 +150）

constexpr int kStateStart = 0;
constexpr int kStatePlaying = 1;
constexpr int kStateLevelClear = 2;
constexpr int kStateWin = 3;
constexpr int kStateGameOver = 4;

constexpr unsigned kColorBackground = 0xFF181A26U;  // bgt_rgb(24, 26, 38)
constexpr unsigned kColorPanel = 0xFF262A3AU;       // bgt_rgb(38, 42, 58)
constexpr unsigned kColorPaddle = BGT_CYAN;
constexpr unsigned kColorBall = BGT_WHITE;
constexpr unsigned kColorHint = 0xFF9CA0B0U;       // bgt_rgb(156, 160, 176)
constexpr unsigned kBrickColors[kBrickRows] = {
    BGT_RED, BGT_ORANGE, BGT_YELLOW, BGT_GREEN, BGT_CYAN, BGT_MAGENTA};

const char *kLevelPatterns[kLevels][kBrickRows] = {
    {"XXXXXXXXXX", "X........X", "X........X", "X........X",
     "X........X", "XXXXXXXXXX"},                        // 第 1 关 空心方阵 28 砖
    {"X.X.X.X.X.", ".X.X.X.X.X", "X.X.X.X.X.", ".X.X.X.X.X",
     "X.X.X.X.X.", ".X.X.X.X.X"},                         // 第 2 关 棋盘 30 砖
    {"....XX....", "...XXXX...", "..XXXXXX..", ".XXXXXXXX.",
     "XXXXXXXXXX", "XXXXXXXXXX"},                         // 第 3 关 金字塔 48 砖
};

// ---- 局面数据（不引入类、结构体和指针，全部用全局变量与数组）----

int part;                      // 状态机，取值为 kState* 常量
int score = 0;
int lives = kLives;
int level = 1;
bool brick_alive[kBrickCount];
int bricks_left = 0;
int paddle_x = (kFieldWidth - kPaddleW) / 2;  // 250
double ball_x = 0.0;
double ball_y = 0.0;
double ball_vx = 0.0;
double ball_vy = 0.0;
bool ball_stuck = true;
int snd_bounce = 0;            // bgt_load_sound 结果，0 = 没加载上
int snd_brick = 0;
int snd_lose = 0;
int snd_win = 0;

// ---- 几何与音效辅助 ----

int brick_x(int col)
{
    return kBrickLeftX + col * kBrickStepX;
}

int brick_y(int row)
{
    return kBrickTopY + row * kBrickStepY;
}

void play(int id)
{
    if (id > 0) {
        bgt_play_sound(id);
    }
}

// ---- 流程控制 ----

void reset_game()
{
    score = 0;
    lives = kLives;
    level = 1;
    ball_stuck = true;
    paddle_x = (kFieldWidth - kPaddleW) / 2;
}

void load_level(int l)
{
    level = l;
    bricks_left = 0;
    for (int row = 0; row < kBrickRows; ++row) {
        for (int col = 0; col < kBrickCols; ++col) {
            const int i = row * kBrickCols + col;
            brick_alive[i] = kLevelPatterns[l - 1][row][col] == 'X';
            if (brick_alive[i]) {
                bricks_left += 1;
            }
        }
    }
    int removed = 0;   // 每关砖数 >= 28，随机抽缺席砖不会死循环
    while (removed < kBrickMissing) {
        const int i = bgt_random(0, kBrickCount);
        if (brick_alive[i]) {
            brick_alive[i] = false;
            bricks_left -= 1;
            removed += 1;
        }
    }
    paddle_x = (kFieldWidth - kPaddleW) / 2;
    ball_stuck = true;
}

// ---- 每帧逻辑（仅状态 1）----

void update_playing()
{
    const double dt = bgt_delta_time();

    // 1. 挡板：左右移动并钳制在场地内
    if (bgt_key_is_down(BGT_KEY_LEFT)) {
        paddle_x -= static_cast<int>(kPaddleSpeed * dt);
    }
    if (bgt_key_is_down(BGT_KEY_RIGHT)) {
        paddle_x += static_cast<int>(kPaddleSpeed * dt);
    }
    if (paddle_x < 0) {
        paddle_x = 0;
    }
    if (paddle_x > kFieldWidth - kPaddleW) {
        paddle_x = kFieldWidth - kPaddleW;
    }

    // 2. 球贴挡板：跟随挡板，空格发球，本帧其余步骤跳过
    if (ball_stuck) {
        ball_x = paddle_x + kPaddleW / 2.0;
        ball_y = kPaddleY - kBallR - 2;
        if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
            ball_vx = 0;
            ball_vy = -kBallSpeed;
            ball_stuck = false;
        }
        return;
    }

    // 3. 移动
    ball_x += ball_vx * dt;
    ball_y += ball_vy * dt;

    // 4. 左、右、顶三面墙：各自独立反弹
    if (ball_x < kBallR) {
        ball_x = kBallR;
        ball_vx = -ball_vx;
        play(snd_bounce);
    }
    if (ball_x > kFieldWidth - kBallR) {
        ball_x = kFieldWidth - kBallR;
        ball_vx = -ball_vx;
        play(snd_bounce);
    }
    if (ball_y < kBallR) {
        ball_y = kBallR;
        ball_vy = -ball_vy;
        play(snd_bounce);
    }

    // 5. 落底：扣命；命尽进结束屏，否则球回挡板
    if (ball_y > kWindowHeight + kBallR) {
        lives -= 1;
        play(snd_lose);
        if (lives == 0) {
            part = kStateGameOver;
        } else {
            ball_stuck = true;
        }
        return;
    }

    // 6. 挡板反弹：命中偏移决定 vx，速度模长恒定，向上推出防连击
    if (ball_vy > 0 &&
        bgt_hit_circle_rect(static_cast<int>(ball_x),
                            static_cast<int>(ball_y), kBallR, paddle_x,
                            kPaddleY, kPaddleW, kPaddleH)) {
        double r = (ball_x - (paddle_x + kPaddleW / 2.0)) / (kPaddleW / 2.0);
        if (r < -1.0) {
            r = -1.0;
        }
        if (r > 1.0) {
            r = 1.0;
        }
        ball_vx = r * kBallSpeed * kPaddleBounceMax;
        ball_vy = -std::sqrt(kBallSpeed * kBallSpeed - ball_vx * ball_vx);
        ball_y = kPaddleY - kBallR - 1;
        play(snd_bounce);
    }

    // 7. 砖碰撞：一帧可消多砖，逐砖处理不提前退出
    for (int row = 0; row < kBrickRows; ++row) {
        for (int col = 0; col < kBrickCols; ++col) {
            const int i = row * kBrickCols + col;
            if (!brick_alive[i]) {
                continue;
            }
            const int bx = brick_x(col);
            const int by = brick_y(row);
            if (bgt_hit_rect_rect(static_cast<int>(ball_x - kBallR),
                                  static_cast<int>(ball_y - kBallR),
                                  kBallR * 2, kBallR * 2, bx, by,
                                  kBrickW, kBrickH)) {
                brick_alive[i] = false;
                bricks_left -= 1;
                score += kBrickScore;
                play(snd_brick);
                if (ball_x < bx || ball_x > bx + kBrickW) {
                    ball_vx = -ball_vx;
                } else {
                    ball_vy = -ball_vy;
                }
            }
        }
    }

    // 8. 清空砖阵：计过关奖励；第 3 关通关，否则进过关屏
    if (bricks_left == 0) {
        score += 50 * level;
        play(snd_win);
        if (level == kLevels) {
            part = kStateWin;
        } else {
            part = kStateLevelClear;
        }
    }
}

// ---- 绘制 ----

void draw_bricks()
{
    for (int row = 0; row < kBrickRows; ++row) {
        for (int col = 0; col < kBrickCols; ++col) {
            if (!brick_alive[row * kBrickCols + col]) {
                continue;
            }
            bgt_set_color(kBrickColors[row]);
            bgt_fill_rect(brick_x(col), brick_y(row), kBrickW, kBrickH);
        }
    }
}

void draw_paddle_ball()
{
    bgt_set_color(kColorPaddle);
    bgt_fill_rect(paddle_x, kPaddleY, kPaddleW, kPaddleH);
    bgt_set_color(BGT_WHITE);
    bgt_draw_rect(paddle_x, kPaddleY, kPaddleW, kPaddleH);

    bgt_set_color(kColorBall);
    bgt_fill_circle(static_cast<int>(ball_x), static_cast<int>(ball_y),
                    kBallR);
}

void draw_center_text(const char text[], int y, int size)
{
    const int x = (kWindowWidth - bgt_text_width(text, size)) / 2;
    bgt_draw_text(x, y, text, size);
}

void draw_panel()
{
    const int text_x = kPanelX + 24;

    bgt_set_color(BGT_WHITE);
    bgt_draw_text(text_x, 40, "打砖块", 32);

    char line[64];
    std::snprintf(line, sizeof(line), "得分：%d", score);
    bgt_draw_text(text_x, 120, line, 24);
    std::snprintf(line, sizeof(line), "关卡：%d / 3", level);
    bgt_draw_text(text_x, 160, line, 24);
    bgt_draw_text(text_x, 205, "生命", 20);

    for (int i = 0; i < kLives; ++i) {
        const int cx = kPanelX + 42 + i * 40;
        if (i < lives) {
            bgt_set_color(BGT_ORANGE);
            bgt_fill_circle(cx, 258, 12);
        } else {
            bgt_set_color(BGT_GRAY);
            bgt_draw_circle(cx, 258, 12);
        }
    }

    if (ball_stuck) {
        bgt_set_color(kColorHint);
        bgt_draw_text(text_x, 400, "空格发球", 20);
    }
}

void draw_start_screen()
{
    bgt_set_color(BGT_WHITE);
    draw_center_text("打砖块", 200, 52);
    draw_center_text("←/→ 移动挡板，空格发球", 280, 24);
    bgt_set_color(kColorHint);
    draw_center_text("接住小球，清空砖阵！每砖 10 分", 320, 22);
    bgt_set_color(BGT_YELLOW);
    draw_center_text("按【空格】开始", 400, 26);
}

void draw_level_clear_screen()
{
    char line[64];
    std::snprintf(line, sizeof(line), "第 %d 关完成！", level);
    bgt_set_color(BGT_WHITE);
    draw_center_text(line, 240, 40);
    std::snprintf(line, sizeof(line), "过关奖励 +%d 分", 50 * level);
    bgt_set_color(BGT_YELLOW);
    draw_center_text(line, 300, 24);
    bgt_set_color(kColorHint);
    draw_center_text("按【空格】进入下一关", 350, 24);
}

void draw_win_screen()
{
    char line[64];
    bgt_set_color(BGT_WHITE);
    draw_center_text("恭喜通关！", 220, 48);
    std::snprintf(line, sizeof(line), "最终得分：%d", score);
    draw_center_text(line, 300, 28);
    bgt_set_color(kColorHint);
    draw_center_text("按【R】重新开始", 360, 22);
}

void draw_game_over_screen()
{
    char line[64];
    bgt_set_color(BGT_RED);
    draw_center_text("游戏结束", 220, 48);
    bgt_set_color(BGT_WHITE);
    std::snprintf(line, sizeof(line), "得分：%d", score);
    draw_center_text(line, 300, 28);
    bgt_set_color(kColorHint);
    draw_center_text("按【R】重新开始", 360, 22);
}

void draw_frame()
{
    if (part == kStateStart) {
        draw_start_screen();
    } else if (part == kStatePlaying) {
        // 绘制顺序：侧栏底 -> 砖 -> 挡板 -> 球 -> 侧栏内容
        bgt_set_color(kColorPanel);
        bgt_fill_rect(kPanelX, 0, kPanelWidth, kWindowHeight);
        draw_bricks();
        draw_paddle_ball();
        draw_panel();
    } else if (part == kStateLevelClear) {
        draw_level_clear_screen();
    } else if (part == kStateWin) {
        draw_win_screen();
    } else {
        draw_game_over_screen();
    }
}

} // namespace

int main()
{
    if (!bgt_open_window(kWindowWidth, kWindowHeight, "libbgt 打砖块")) {
        bgt_print_error();
        return 1;
    }

    bgt_set_fps_limit(kTargetFps);
    bgt_set_background(kColorBackground);

    snd_bounce = bgt_load_sound("03_bounce.wav");
    snd_brick = bgt_load_sound("03_brick.wav");
    snd_lose = bgt_load_sound("03_lose.wav");
    snd_win = bgt_load_sound("03_win.wav");

    part = kStateStart;

    while (bgt_window_is_open()) {
        if (bgt_key_just_pressed(BGT_KEY_ESCAPE)) {
            break;
        }

        if (part == kStateStart) {
            if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
                reset_game();
                load_level(1);
                part = kStatePlaying;
            }
        } else if (part == kStatePlaying) {
            update_playing();
        } else if (part == kStateLevelClear) {
            if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
                load_level(level + 1);
                part = kStatePlaying;
            }
        } else if (part == kStateWin) {
            if (bgt_key_just_pressed(BGT_KEY_R)) {
                part = kStateStart;
            }
        } else if (part == kStateGameOver) {
            if (bgt_key_just_pressed(BGT_KEY_R)) {
                part = kStateStart;
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
