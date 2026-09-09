#include "bgt.h"

#include <cstdio>

// NOLINTBEGIN(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity, bugprone-easily-swappable-parameters)

// 太空射击演示：开始 / 游戏 / 结束 / 资产缺失四态流程。波次先播报后生成，
// 连击窗口内连续击落有 2/3 倍分数加成；快速敌机用同一张图片的第二个编号
// 展示缩放 + 旋转变换（变换状态按编号独立生效）。图片与音效来自 exe 旁的
// 05_*.png / 05_*.wav：音效缺失静默降级，图片缺失进入错误诊断面板。

namespace {

constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 700;
constexpr int kTargetFps = 60;
constexpr int kShipY = 640;            // 飞船中心 y（图 32x32，上缘 624）
constexpr double kShipSpeed = 340.0;   // px/s
constexpr int kShipHitBox = 24;        // 船判定盒（中心对称，公平留边）
constexpr int kBulletMax = 20;
constexpr double kBulletSpeed = 600.0;
constexpr int kBulletR = 4;            // 子弹碰撞圆半径（视觉 2x10 矩形）
constexpr int kCooldownFrames = 12;    // 帧计数冷却（约 0.2 秒）
constexpr int kEnemyMax = 20;
constexpr double kEnemyNormalSpeed = 90.0;
constexpr double kEnemyFastSpeed = 160.0;
constexpr double kWaveSpeedBonus = 10.0;  // 每波 +10 px/s
constexpr double kEnemyFastScale = 0.7;   // 快速敌缩放（22px 级）
constexpr double kEnemyFastSpin = 180.0;  // 翻滚角速度（度/秒）
constexpr int kFastChance = 4;            // bgt_random(0, 4) == 0 → 快速（25%）
constexpr double kSpawnInterval = 0.8;    // 秒
constexpr double kAnnounceTime = 2.0;      // 波前播报秒数
constexpr int kStarCount = 60;            // 前 40 慢层 + 后 20 快层
constexpr double kStarSlowSpeed = 20.0;
constexpr double kStarFastSpeed = 60.0;
constexpr int kExplosionMax = 10;
constexpr int kExplosionFrames = 12;      // 扩散圆环 12 帧
constexpr double kExplosionStep = 0.04;   // 每帧秒数
constexpr int kLives = 3;
constexpr double kInvulnTime = 1.5;       // 被撞后无敌秒数（闪烁）
constexpr double kComboWindow = 2.0;      // 连击窗口秒数
constexpr int kScoreNormal = 10;
constexpr int kScoreFast = 20;

// 存档 [纪录]：score（最高分）/ wave（最高波次）；文件名 "shooter_save.txt"
constexpr unsigned kColorBackground = 0xFF060812U;  // bgt_rgb(6, 8, 18)
constexpr unsigned kColorHint = 0xFFA0A0AAU;       // bgt_rgb(160, 160, 170)
const unsigned kExplosionColors[kExplosionFrames] = {
    BGT_WHITE, BGT_WHITE, BGT_WHITE, BGT_YELLOW, BGT_YELLOW, BGT_YELLOW,
    BGT_ORANGE, BGT_ORANGE, BGT_ORANGE, BGT_RED, BGT_RED, BGT_RED};

constexpr int kStateStart = 0;
constexpr int kStatePlaying = 1;
constexpr int kStateGameOver = 2;
constexpr int kStateAssets = 5;

// ---- 局面数据（不引入类、结构体和指针，全部用全局变量与数组）----

int part;                          // 0 start / 1 playing / 2 gameover / 5 assets
int score = 0;
int lives = kLives;
int wave = 1;
int wave_phase = 0;                // 0 播报 / 1 生成+战斗
double phase_timer = 0.0;
int spawn_pending = 0;
double spawn_timer = 0.0;
int combo = 0;
double combo_time = -10.0;
double invuln_until = 0.0;
double ship_x = 400.0;             // 中心 x
bool bullet_alive[kBulletMax];
double bullet_x[kBulletMax];
double bullet_y[kBulletMax];
int bullet_cooldown = 0;
bool enemy_alive[kEnemyMax];
double enemy_x[kEnemyMax];         // 中心
double enemy_y[kEnemyMax];
int enemy_type[kEnemyMax];         // 0 普通 / 1 快速
double enemy_spin[kEnemyMax];
bool star_fast[kStarCount];
double star_x[kStarCount];
double star_y[kStarCount];
bool boom_alive[kExplosionMax];
double boom_x[kExplosionMax];
double boom_y[kExplosionMax];
double boom_start[kExplosionMax];
int best_score = 0;
int best_wave = 0;
bool new_record = false;
int snd_shoot = 0, snd_boom = 0, snd_lose = 0;

// ---- 辅助 ----

void play(int id)
{
    // 音效缺失只降级：id 无效（0）时不播放，不产生新错误
    if (id > 0) {
        bgt_play_sound(id);
    }
}

int effective_combo()
{
    // 只影响显示：连击窗口过期后按 0 展示（不改 combo 状态本身）
    return (bgt_total_time() - combo_time <= kComboWindow) ? combo : 0;
}

int combo_multiplier(int c)
{
    return c >= 10 ? 3 : (c >= 5 ? 2 : 1);
}

// ---- 流程控制 ----

void start_game()
{
    score = 0;
    lives = kLives;
    wave = 1;
    combo = 0;
    combo_time = -10.0;
    invuln_until = 0.0;
    ship_x = 400.0;
    for (int j = 0; j < kBulletMax; ++j) {
        bullet_alive[j] = false;
    }
    for (int k = 0; k < kEnemyMax; ++k) {
        enemy_alive[k] = false;
    }
    for (int i = 0; i < kExplosionMax; ++i) {
        boom_alive[i] = false;
    }
    bullet_cooldown = 0;
    wave_phase = 0;
    phase_timer = 0.0;
    new_record = false;
}

void spawn_bullet()
{
    for (int j = 0; j < kBulletMax; ++j) {
        if (!bullet_alive[j]) {
            bullet_x[j] = ship_x;
            bullet_y[j] = kShipY - 24;
            bullet_alive[j] = true;
            bullet_cooldown = kCooldownFrames;
            play(snd_shoot);
            return;
        }
    }
    // 无空槽：本帧不发射，冷却保持 0，下一帧再试
}

void spawn_enemy()
{
    for (int k = 0; k < kEnemyMax; ++k) {
        if (!enemy_alive[k]) {
            enemy_x[k] = bgt_random(16, kWindowWidth - 16);
            enemy_y[k] = -32;
            enemy_type[k] = (bgt_random(0, kFastChance) == 0) ? 1 : 0;
            enemy_spin[k] = 0.0;
            enemy_alive[k] = true;
            spawn_pending -= 1;
            spawn_timer = 0.0;
            return;
        }
    }
    spawn_timer = 0.0;  // 无空槽：本次跳过、计时清零重试
}

void spawn_explosion(double x, double y)
{
    for (int i = 0; i < kExplosionMax; ++i) {
        if (!boom_alive[i]) {
            boom_alive[i] = true;
            boom_x[i] = x;
            boom_y[i] = y;
            boom_start[i] = bgt_total_time();
            return;
        }
    }
    // 无空槽：放弃本次爆炸（0.48 秒后很快有槽位释放）
}

// ---- 每帧逻辑 ----

void update_stars()
{
    const double dt = bgt_delta_time();
    for (int i = 0; i < kStarCount; ++i) {
        star_y[i] += (star_fast[i] ? kStarFastSpeed : kStarSlowSpeed) * dt;
        if (star_y[i] >= kWindowHeight) {
            star_y[i] = 0.0;
            star_x[i] = bgt_random(0, kWindowWidth);
        }
    }
}

void update_playing()
{
    const double dt = bgt_delta_time();
    const double now = bgt_total_time();

    // 1. 飞船：左右移动，钳制在 [16, 窗宽-16]
    if (bgt_key_is_down(BGT_KEY_LEFT)) {
        ship_x -= kShipSpeed * dt;
    }
    if (bgt_key_is_down(BGT_KEY_RIGHT)) {
        ship_x += kShipSpeed * dt;
    }
    if (ship_x < 16.0) {
        ship_x = 16.0;
    }
    if (ship_x > kWindowWidth - 16) {
        ship_x = kWindowWidth - 16;
    }

    // 2. 射击：冷却每帧递减到 0 为止；空格且冷却归零时找空槽发射
    if (bullet_cooldown > 0) {
        bullet_cooldown -= 1;
    }
    if (bgt_key_just_pressed(BGT_KEY_SPACE) && bullet_cooldown == 0) {
        spawn_bullet();
    }

    // 3. 波次：0 播报 → 1 生成+战斗；清空后回播报、波数 +1
    if (wave_phase == 0) {
        phase_timer += dt;
        if (phase_timer >= kAnnounceTime) {
            spawn_pending = 4 + wave;
            if (spawn_pending > kEnemyMax) {
                spawn_pending = kEnemyMax;
            }
            spawn_timer = 0.0;
            wave_phase = 1;
        }
    } else {
        if (spawn_pending > 0) {
            spawn_timer += dt;
            if (spawn_timer >= kSpawnInterval) {
                spawn_enemy();
            }
        }
        for (int k = 0; k < kEnemyMax; ++k) {
            if (!enemy_alive[k]) {
                continue;
            }
            const double base =
                enemy_type[k] == 1 ? kEnemyFastSpeed : kEnemyNormalSpeed;
            enemy_y[k] += (base + kWaveSpeedBonus * (wave - 1)) * dt;
            if (enemy_type[k] == 1) {
                enemy_spin[k] += kEnemyFastSpin * dt;
            }
            if (enemy_y[k] > kWindowHeight + 32) {
                enemy_alive[k] = false;  // 漏过底部：宽松规则，不扣命
            }
        }
        bool any_alive = false;
        for (int k = 0; k < kEnemyMax; ++k) {
            if (enemy_alive[k]) {
                any_alive = true;
            }
        }
        if (spawn_pending == 0 && !any_alive) {
            wave += 1;
            wave_phase = 0;
            phase_timer = 0.0;
        }
    }

    // 4. 子弹上移，飞出屏幕即回收
    for (int j = 0; j < kBulletMax; ++j) {
        if (!bullet_alive[j]) {
            continue;
        }
        bullet_y[j] -= kBulletSpeed * dt;
        if (bullet_y[j] < -12.0) {
            bullet_alive[j] = false;
        }
    }

    // 5. 子弹-敌：双循环；命中双双消亡，连击计分
    for (int j = 0; j < kBulletMax; ++j) {
        if (!bullet_alive[j]) {
            continue;
        }
        for (int k = 0; k < kEnemyMax; ++k) {
            if (!enemy_alive[k]) {
                continue;
            }
            const int bx = static_cast<int>(bullet_x[j]);
            const int by = static_cast<int>(bullet_y[j]);
            // 判定盒：普通 32x32；快速 22x22（32 × 0.7 缩放后的大小）
            const int box = enemy_type[k] == 1 ? 11 : 16;
            const int size = enemy_type[k] == 1 ? 22 : 32;
            if (bgt_hit_circle_rect(bx, by, kBulletR,
                                    static_cast<int>(enemy_x[k]) - box,
                                    static_cast<int>(enemy_y[k]) - box, size,
                                    size)) {
                bullet_alive[j] = false;
                enemy_alive[k] = false;
                spawn_explosion(enemy_x[k], enemy_y[k]);
                play(snd_boom);
                if (now - combo_time <= kComboWindow) {
                    combo += 1;
                } else {
                    combo = 1;
                }
                combo_time = now;
                score += (enemy_type[k] == 1 ? kScoreFast : kScoreNormal) *
                         combo_multiplier(combo);
                break;  // 这颗子弹已消亡，换下一颗
            }
        }
    }

    // 6. 敌-船：无敌期内整段跳过
    if (now >= invuln_until) {
        for (int k = 0; k < kEnemyMax; ++k) {
            if (!enemy_alive[k]) {
                continue;
            }
            const int ex = static_cast<int>(enemy_x[k]);
            const int ey = static_cast<int>(enemy_y[k]);
            const int box = enemy_type[k] == 1 ? 11 : 16;
            const int size = enemy_type[k] == 1 ? 22 : 32;
            if (bgt_hit_rect_rect(ex - box, ey - box, size, size,
                                  static_cast<int>(ship_x) - kShipHitBox / 2,
                                  kShipY - kShipHitBox / 2, kShipHitBox,
                                  kShipHitBox)) {
                enemy_alive[k] = false;
                spawn_explosion(enemy_x[k], enemy_y[k]);
                lives -= 1;
                play(snd_lose);
                combo = 0;
                invuln_until = now + kInvulnTime;
                break;  // 一帧只结算一次碰撞
            }
        }
    }

    if (lives == 0) {
        // 命尽：更新纪录并落盘
        if (score > best_score) {
            best_score = score;
            new_record = true;
        }
        if (wave > best_wave) {
            best_wave = wave;
        }
        bgt_set_int("纪录", "score", best_score);
        bgt_set_int("纪录", "wave", best_wave);
        bgt_save("shooter_save.txt");
        part = kStateGameOver;
        return;
    }

    // 7. 爆炸推进：满 12 帧回收
    for (int i = 0; i < kExplosionMax; ++i) {
        const double age = (now - boom_start[i]) / kExplosionStep;
        if (boom_alive[i] && age >= kExplosionFrames) {
            boom_alive[i] = false;
        }
    }
}

// ---- 绘制 ----

void draw_center_text(const char text[], int y, int size)
{
    const int x = (kWindowWidth - bgt_text_width(text, size)) / 2;
    bgt_draw_text(x, y, text, size);
}

void draw_stars()
{
    const unsigned slow_color = bgt_rgb(110, 110, 125);
    const unsigned fast_color = bgt_rgb(210, 210, 220);
    for (int i = 0; i < kStarCount; ++i) {
        bgt_set_color(star_fast[i] ? fast_color : slow_color);
        bgt_fill_rect(static_cast<int>(star_x[i]),
                      static_cast<int>(star_y[i]), 2, 2);
    }
}

void draw_bullets()
{
    bgt_set_color(BGT_YELLOW);
    for (int j = 0; j < kBulletMax; ++j) {
        if (!bullet_alive[j]) {
            continue;
        }
        bgt_fill_rect(static_cast<int>(bullet_x[j]) - 1,
                      static_cast<int>(bullet_y[j]) - 5, 2, 10);
    }
}

void draw_enemies(int img_enemy, int img_enemy_fast)
{
    for (int k = 0; k < kEnemyMax; ++k) {
        if (!enemy_alive[k]) {
            continue;
        }
        const int ex = static_cast<int>(enemy_x[k]);
        const int ey = static_cast<int>(enemy_y[k]);
        if (enemy_type[k] == 1) {
            // 快速敌：R+S 展示，每帧画前设变换。缩放绕左上角，
            // 32 × 0.7 ≈ 22，画位偏 11 保持中心对齐 22x22 判定盒
            bgt_scale_image(img_enemy_fast, kEnemyFastScale, kEnemyFastScale);
            bgt_rotate_image(img_enemy_fast, enemy_spin[k]);
            bgt_draw_image(img_enemy_fast, ex - 11, ey - 11);
        } else {
            bgt_draw_image(img_enemy, ex - 16, ey - 16);
        }
    }
}

void draw_ship(int img_ship)
{
    // 无敌期内闪烁：每 0.1 秒切换一次可见性
    const double now = bgt_total_time();
    if (now < invuln_until && static_cast<int>(now * 10) % 2 == 0) {
        return;
    }
    bgt_draw_image(img_ship, static_cast<int>(ship_x) - 16, kShipY - 16);
}

void draw_explosions()
{
    const double now = bgt_total_time();
    bgt_set_line_width(3);
    for (int i = 0; i < kExplosionMax; ++i) {
        if (!boom_alive[i]) {
            continue;
        }
        const int frame =
            static_cast<int>((now - boom_start[i]) / kExplosionStep);
        if (frame < kExplosionFrames) {
            bgt_set_color(kExplosionColors[frame]);
            bgt_draw_circle(static_cast<int>(boom_x[i]),
                            static_cast<int>(boom_y[i]), 4 + frame * 3);
        }
    }
    bgt_set_line_width(1);  // 画完全部环后复位
}

void draw_best_score()
{
    char line[64];
    std::snprintf(line, sizeof(line), "最高：%d", best_score);
    bgt_set_color(kColorHint);
    const int w = bgt_text_width(line, 20);
    bgt_draw_text(kWindowWidth - w - 24, 20, line, 20);
}

void draw_hud()
{
    char line[64];
    bgt_set_color(BGT_WHITE);
    std::snprintf(line, sizeof(line), "得分：%d", score);
    bgt_draw_text(24, 20, line, 24);
    std::snprintf(line, sizeof(line), "第 %d 波", wave);
    bgt_draw_text(24, 56, line, 20);

    draw_best_score();

    for (int i = 0; i < kLives; ++i) {
        const int cx = kWindowWidth - 36 - i * 36;
        if (i < lives) {
            bgt_set_color(BGT_ORANGE);
            bgt_fill_circle(cx, 64, 10);
        } else {
            bgt_set_color(BGT_GRAY);
            bgt_draw_circle(cx, 64, 10);
        }
    }
}

void draw_combo_and_announce()
{
    // 连击：显示用有效连击（2 秒未续上按 0 处理），倍数跟着有效值走
    const int effective = effective_combo();
    if (effective >= 2) {
        char line[32];
        bgt_set_color(BGT_YELLOW);
        std::snprintf(line, sizeof(line), "连击 ×%d", effective);
        draw_center_text(line, 96, 28);
        const int mult = combo_multiplier(effective);
        if (mult >= 2) {
            bgt_set_color(BGT_ORANGE);
            std::snprintf(line, sizeof(line), "分数 ×%d", mult);
            draw_center_text(line, 132, 22);
        }
    }

    // 播报：生成阶段开始前提示本波来袭
    if (wave_phase == 0) {
        char line[64];
        std::snprintf(line, sizeof(line), "第 %d 波来袭！", wave);
        bgt_set_color(BGT_WHITE);
        draw_center_text(line, 320, 40);
    }
}

void draw_start_screen()
{
    bgt_set_color(BGT_WHITE);
    draw_center_text("太空射击", 220, 52);
    draw_center_text("←/→ 移动，空格 射击", 300, 24);
    bgt_set_color(kColorHint);
    draw_center_text("击落敌人得分，连击有倍数加成", 340, 22);
    bgt_set_color(BGT_YELLOW);
    draw_center_text("按【空格】开始", 420, 26);
    draw_best_score();
}

void draw_game_over_screen()
{
    char line[64];
    bgt_set_color(BGT_RED);
    draw_center_text("游戏结束", 240, 48);
    bgt_set_color(BGT_WHITE);
    std::snprintf(line, sizeof(line), "得分：%d", score);
    draw_center_text(line, 320, 28);
    std::snprintf(line, sizeof(line), "坚持到第 %d 波", wave);
    draw_center_text(line, 365, 24);
    if (new_record) {
        bgt_set_color(BGT_YELLOW);
        draw_center_text("新纪录！", 410, 32);
    }
    bgt_set_color(kColorHint);
    draw_center_text("按【R】重新开始", 470, 22);
}

void draw_assets_panel()
{
    // 专属状态 5：不进游戏逻辑，每帧只画错误面板（12_errors 先例）
    bgt_set_color(BGT_RED);
    bgt_draw_text(60, 40, "资源加载失败", 36);
    int y = 130;
    for (int i = 0; i < bgt_error_count(); ++i) {
        bgt_set_color(BGT_WHITE);
        bgt_draw_error(60, y, 18, i);
        y += 48;
    }
    bgt_set_color(BGT_DARK_GRAY);
    bgt_draw_text(60, 560, "请从可执行文件所在目录运行本程序，", 18);
    bgt_draw_text(60, 588, "图片与音频资源需要复制到 exe 旁边。", 18);
}

void draw_playing(int img_ship, int img_enemy, int img_enemy_fast)
{
    // 状态 1 绘制顺序：星空(外层已画) → 子弹 → 敌 → 飞船 → 爆炸 → HUD
    draw_bullets();
    draw_enemies(img_enemy, img_enemy_fast);
    draw_ship(img_ship);
    draw_explosions();
    draw_hud();
    draw_combo_and_announce();
}

} // namespace

int main()
{
    if (!bgt_open_window(kWindowWidth, kWindowHeight, "libbgt 太空射击")) {
        bgt_print_error();
        return 1;
    }

    bgt_set_fps_limit(kTargetFps);
    bgt_set_background(kColorBackground);

    // 图片加载：同一文件加载两次得到独立编号——img_enemy 保持恒等变换
    // 画普通敌，img_enemy_fast 专门承载快速敌的缩放 + 旋转（07_images 先例）
    const int img_ship = bgt_load_image("05_ship.png");
    const int img_enemy = bgt_load_image("05_enemy.png");
    const int img_enemy_fast = bgt_load_image("05_enemy.png");

    // 音效加载：失败静默降级（play 内 id > 0 判定）
    snd_shoot = bgt_load_sound("05_shoot.wav");
    snd_boom = bgt_load_sound("05_boom.wav");
    snd_lose = bgt_load_sound("05_lose.wav");

    // 星空初始化：前 40 慢层 + 后 20 快层
    for (int i = 0; i < kStarCount; ++i) {
        star_fast[i] = i >= 40;
        star_x[i] = bgt_random(0, kWindowWidth);
        star_y[i] = bgt_random(0, kWindowHeight);
    }

    // 启动读档（开窗后一次）：[纪录] score=最高分 / wave=最高波次
    bgt_load("shooter_save.txt");
    best_score = bgt_get_int("纪录", "score", 0);
    best_wave = bgt_get_int("纪录", "wave", 0);

    // 图片缺失 → 错误面板（不放 BGM）；齐全 → 播 BGM 进开始屏
    if (img_ship == 0 || img_enemy == 0 || img_enemy_fast == 0) {
        part = kStateAssets;
    } else {
        bgt_play_music("05_bgm.wav");  // 返回 false 静默忽略
        bgt_set_music_volume(45);
        part = kStateStart;
    }

    while (bgt_window_is_open()) {
        if (bgt_key_just_pressed(BGT_KEY_ESCAPE)) {
            break;
        }

        // ---- 更新 ----
        if (part == kStateAssets) {
            // 资产缺失：不进游戏循环逻辑
        } else {
            update_stars();  // 星空在开始/游戏/结束三态都滚动
            if (part == kStateStart) {
                if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
                    start_game();
                    part = kStatePlaying;
                }
            } else if (part == kStatePlaying) {
                update_playing();
            } else {
                if (bgt_key_just_pressed(BGT_KEY_R)) {
                    start_game();
                    part = kStatePlaying;
                }
            }
        }

        // ---- 绘制 ----
        if (part == kStateAssets) {
            draw_assets_panel();
        } else {
            draw_stars();
            if (part == kStateStart) {
                draw_start_screen();
            } else if (part == kStatePlaying) {
                draw_playing(img_ship, img_enemy, img_enemy_fast);
            } else {
                draw_game_over_screen();
            }
        }

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity, bugprone-easily-swappable-parameters)
