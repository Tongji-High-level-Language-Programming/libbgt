// =====================================================================
// libbgt 声音播放 API 演示（v0.3）
// 每个板块演示声音的一组能力，按【空格】进入下一板块，随时可以按
// 【Esc】退出。示例用到的 11_*.wav 都是用代码合成的（生成脚本
// make_sound_assets.py），程序开始时把音效加载好，存进 int 变量。
//   板块 1  音效 —— 按 1/2/3 触发，连按可以听到多声重叠
//   板块 2  音量台 —— 方向键分别调音效和音乐的音量
//   板块 3  背景音乐 —— 起播、停止、播放中自动切歌
//   板块 4  错误处理 —— 播放不存在的文件，把错误画出来
// =====================================================================
// NOLINTBEGIN(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity)

#include "bgt.h"

#include <cstdio>

int main()
{
    constexpr int window_width = 800;
    constexpr int window_height = 700;

    if (!bgt_open_window(window_width, window_height, "libbgt 声音演示")) {
        bgt_print_error();
        return 1;
    }
    bgt_set_background(BGT_WHITE);

    // ---------- 程序开头把音效都加载好，存进 int 变量 ----------
    const int jump = bgt_load_sound("11_jump.wav");
    const int ding = bgt_load_sound("11_ding.wav");
    const int boom = bgt_load_sound("11_boom.wav");
    if (jump == 0 || ding == 0 || boom == 0) {
        bgt_print_error();
    }

    int part = 1; // 当前板块号：1 到 4

    // 板块 1：音效触发计数
    int jump_count = 0;
    int ding_count = 0;
    int boom_count = 0;

    // 板块 2：音量（0-100）
    int sound_volume = 100;
    int music_volume = 100;

    // 板块 3：音乐状态提示
    bool music_started = false;

    while (bgt_window_is_open()) {
        // ---------- 板块 1：音效 ----------
        if (part == 1) {
            bgt_set_window_title("libbgt 声音演示 - 板块 1：音效");
            if (bgt_key_just_pressed(BGT_KEY_1)) {
                bgt_play_sound(jump);
                jump_count = jump_count + 1;
            }
            if (bgt_key_just_pressed(BGT_KEY_2)) {
                bgt_play_sound(ding);
                ding_count = ding_count + 1;
            }
            if (bgt_key_just_pressed(BGT_KEY_3)) {
                bgt_play_sound(boom);
                boom_count = boom_count + 1;
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36,
                          "板块 1：音效 —— 按【1】【2】【3】触发", 32);
            char count_text[96] = {};
            std::snprintf(count_text, sizeof(count_text),
                          "跳跃 %d 次，叮咚 %d 次，爆炸 %d 次", jump_count,
                          ding_count, boom_count);
            bgt_draw_text(80, 160, count_text, 26);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(80, 300,
                          "连按同一个键：每次按键都是一次新的发声，", 22);
            bgt_draw_text(80, 340,
                          "多个声音会自动混在一起"
                          "——这就是音效的“重叠”。", 22);
        }

        // ---------- 板块 2：音量台 ----------
        if (part == 2) {
            bgt_set_window_title("libbgt 声音演示 - 板块 2：音量台");
            if (bgt_key_just_pressed(BGT_KEY_UP) && sound_volume < 100) {
                sound_volume = sound_volume + 5;
                bgt_set_sound_volume(jump, sound_volume);
                bgt_set_sound_volume(ding, sound_volume);
                bgt_set_sound_volume(boom, sound_volume);
            }
            if (bgt_key_just_pressed(BGT_KEY_DOWN) && sound_volume > 0) {
                sound_volume = sound_volume - 5;
                bgt_set_sound_volume(jump, sound_volume);
                bgt_set_sound_volume(ding, sound_volume);
                bgt_set_sound_volume(boom, sound_volume);
            }
            if (bgt_key_just_pressed(BGT_KEY_RIGHT) && music_volume < 100) {
                music_volume = music_volume + 5;
                bgt_set_music_volume(music_volume);
            }
            if (bgt_key_just_pressed(BGT_KEY_LEFT) && music_volume > 0) {
                music_volume = music_volume - 5;
                bgt_set_music_volume(music_volume);
            }
            if (bgt_key_just_pressed(BGT_KEY_1)) {
                bgt_play_sound(jump); // 试听当前音效音量
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 2：音量台 —— 方向键调节 0-100", 32);
            // 两条音量条：宽度和数值成比例
            bgt_draw_text(80, 160, "音效音量（↑↓ 调节，按 1 试听）", 22);
            bgt_set_color(BGT_BLUE);
            bgt_fill_rect(80, 200, sound_volume * 6, 30);
            char sv_text[16] = {};
            std::snprintf(sv_text, sizeof(sv_text), "%d", sound_volume);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(700, 200, sv_text, 22);
            bgt_draw_text(80, 280, "音乐音量（←→ 调节）", 22);
            bgt_set_color(BGT_PURPLE);
            bgt_fill_rect(80, 320, music_volume * 6, 30);
            char mv_text[16] = {};
            std::snprintf(mv_text, sizeof(mv_text), "%d", music_volume);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(700, 320, mv_text, 22);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(80, 420,
                          "音量范围 0-100：0 静音、100 最响"
                          "（库还会把越界值", 22);
            bgt_draw_text(80, 460,
                          "自动收到边界，所以怎么按都不会出错）。", 22);
        }

        // ---------- 板块 3：背景音乐 ----------
        if (part == 3) {
            bgt_set_window_title("libbgt 声音演示 - 板块 3：背景音乐");
            if (bgt_key_just_pressed(BGT_KEY_P)) {
                if (bgt_play_music("11_melody.wav")) {
                    music_started = true;
                } else {
                    bgt_print_error();
                }
            }
            if (bgt_key_just_pressed(BGT_KEY_S)) {
                bgt_stop_music();
                music_started = false;
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36,
                          "板块 3：背景音乐 —— 流式播放、默认循环", 32);
            bgt_draw_text(80, 160, "【P】播放 11_melody.wav    【S】停止", 26);
            bgt_set_color(BGT_DARK_GRAY);
            if (music_started) {
                bgt_draw_text(80, 260,
                              "正在播放（循环中）。播放中再按【P】"
                              "会自动切歌：", 22);
                bgt_draw_text(80, 300,
                              "这首曲子会从头开始"
                              "——因为同一时刻只有一首音乐。", 22);
            } else {
                bgt_draw_text(80, 260, "现在没有音乐在播。按【P】试试。", 22);
            }
            bgt_draw_text(80, 420,
                          "音乐是流式播放的：内存占用和音乐文件长度无关，", 22);
            bgt_draw_text(80, 460, "放一整首很长的曲子也不会占很多内存。", 22);
        }

        // ---------- 板块 4：错误处理 ----------
        if (part == 4) {
            bgt_set_window_title("libbgt 声音演示 - 板块 4：错误处理");
            if (bgt_key_just_pressed(BGT_KEY_E)) {
                bgt_clear_error();
                bgt_play_music("missing_file.wav");
            }
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 4：错误处理 —— 播不存在的文件", 32);
            bgt_draw_text(80, 160,
                          "按【E】播放 missing_file.wav"
                          "（一个不存在的文件）", 26);
            bgt_draw_text(80, 220, "最近一次错误：", 22);
            bgt_set_color(BGT_RED);
            bgt_draw_error(80, 260, 22);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(80, 400,
                          "bgt_play_music 返回 false，并把错误记进库里；", 22);
            bgt_draw_text(80, 440,
                          "bgt_draw_error 能把它画在窗口里，方便调试。", 22);
        }

        // 底部统一提示
        bgt_set_color(BGT_DARK_GRAY);
        if (part < 4) {
            bgt_draw_text(40, 648,
                          "按【空格】进入下一板块，按【Esc】退出程序", 24);
        } else {
            bgt_draw_text(40, 648,
                          "按【空格】或【Esc】退出程序"
                          " —— 声音随窗口一起清理", 24);
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
