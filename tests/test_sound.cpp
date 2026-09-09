// libbgt 声音播放 API 的测试。
// 声音“真响不响”由示例人工验收；这里测确定性面：ID 分配、错误路径、
// 音量收敛、音乐起播两态。测试会短暂真实发声（本机有声卡），属正常。
// 用 -DBGT_BUILD_TESTS=ON 配置后由 ctest 运行；13_*.wav 由 CMake
// 复制到测试程序旁。

#include "bgt.h"

#include <cstdio>

// NOLINTBEGIN(readability-magic-numbers)

namespace {

int g_failed_checks = 0;

// 显式检查宏：失败时打印表达式与位置，任何构建配置下都生效。
#define BGT_CHECK(condition)                                            \
    do {                                                                \
        if (!(condition)) {                                             \
            std::fprintf(stderr, "FAILED: %s (%s:%d)\n", #condition,    \
                         __FILE__, __LINE__);                           \
            g_failed_checks = g_failed_checks + 1;                      \
        }                                                                \
    } while (false)

} // namespace

int main()
{
    // 1) 加载仓库资产：ID 从 1 开始、依次递增；同文件重复加载给新 ID。
    bgt_clear_error();
    const int jump = bgt_load_sound("11_jump.wav");
    BGT_CHECK(jump > 0);
    BGT_CHECK(!bgt_has_error());
    const int ding = bgt_load_sound("11_ding.wav");
    BGT_CHECK(ding > jump);
    const int boom = bgt_load_sound("11_boom.wav");
    BGT_CHECK(boom > ding);
    const int jump2 = bgt_load_sound("11_jump.wav");
    BGT_CHECK(jump2 > boom);

    // 2) 失败路径：不存在的文件 / 空名 / null → 0 + BGT_ERROR_AUDIO。
    BGT_CHECK(bgt_load_sound("no_such_file.wav") == 0);
    BGT_CHECK(bgt_error_code() == BGT_ERROR_AUDIO);
    bgt_clear_error();
    BGT_CHECK(bgt_load_sound("") == 0);
    BGT_CHECK(bgt_error_code() == BGT_ERROR_AUDIO);
    bgt_clear_error();
    BGT_CHECK(bgt_load_sound(nullptr) == 0);
    BGT_CHECK(bgt_error_code() == BGT_ERROR_AUDIO);
    bgt_clear_error();

    // 3) 播放：有效 ID 不报错；无效 ID 记错、不崩。
    bgt_play_sound(jump);
    BGT_CHECK(!bgt_has_error());
    bgt_play_sound(0);
    BGT_CHECK(bgt_error_code() == BGT_ERROR_AUDIO);
    bgt_clear_error();
    bgt_play_sound(999);
    BGT_CHECK(bgt_error_code() == BGT_ERROR_AUDIO);
    bgt_clear_error();

    // 4) 音量收敛：越界值收到边界、不报错；无效 id 记错。
    bgt_set_sound_volume(jump, -5);
    bgt_set_sound_volume(jump, 250);
    BGT_CHECK(!bgt_has_error());
    bgt_set_sound_volume(0, 50);
    BGT_CHECK(bgt_error_code() == BGT_ERROR_AUDIO);
    bgt_clear_error();

    // 5) 音乐两态：起播成功；坏文件失败 + 记错；停止是安全空操作。
    BGT_CHECK(bgt_play_music("11_melody.wav"));
    BGT_CHECK(!bgt_has_error());
    bgt_set_music_volume(40);
    BGT_CHECK(!bgt_has_error());
    bgt_stop_music();
    BGT_CHECK(!bgt_has_error());
    BGT_CHECK(!bgt_play_music("no_such_file.wav"));
    BGT_CHECK(bgt_error_code() == BGT_ERROR_AUDIO);
    bgt_clear_error();
    bgt_stop_music();
    BGT_CHECK(!bgt_has_error());
    BGT_CHECK(!bgt_play_music(nullptr));
    BGT_CHECK(bgt_error_code() == BGT_ERROR_AUDIO);
    bgt_clear_error();

    if (g_failed_checks > 0) {
        std::fprintf(stderr, "bgt_test_sound: %d checks failed\n",
                     g_failed_checks);
        return 1;
    }
    std::puts("bgt_test_sound: all tests passed");
    return 0;
}

// NOLINTEND(readability-magic-numbers)
