/**
 * @file test_random.cpp
 * @brief libbgt 随机数 API 的最小测试（纯函数，无需打开窗口）。
 * @details 用显式检查而非 `assert`：Release（NDEBUG）下 `assert` 是空操作，
 *          显式检查在任何构建配置下都真正生效。
 *          用 `-DBGT_BUILD_TESTS=ON` 配置后由 ctest 运行。
 */

#include "bgt.h"

#include <array>
#include <cstddef>
#include <cstdio>

// NOLINTBEGIN(readability-magic-numbers)

namespace {

constexpr unsigned kSeed = 42;

int g_failed_checks = 0;

/**
 * @brief 显式检查宏：失败时打印表达式与位置，任何构建配置下都生效。
 */
#define BGT_CHECK(condition)                                                   \
    do {                                                                       \
        if (!(condition)) {                                                    \
            std::fprintf(stderr, "FAILED: %s (%s:%d)\n", #condition, __FILE__, \
                         __LINE__);                                            \
            g_failed_checks = g_failed_checks + 1;                             \
        }                                                                      \
    } while (false)

/**
 * @brief 从 [min, max) 半开区间反复抽样。
 *
 * 检查每个值都落在区间内，且每个值都出现过（均匀性粗检）。
 *
 * @param min 区间下界（含）。
 * @param max 区间上界（不含）。
 */
void check_all_values_appear(int min, int max)
{
    std::array<bool, 64> seen{};
    for (int i = 0; i < 20000; i++) {
        const int value = bgt_random(min, max);
        BGT_CHECK(value >= min);
        BGT_CHECK(value < max);
        seen[static_cast<std::size_t>(value - min)] = true;
    }
    for (int value = min; value < max; value++) {
        BGT_CHECK(seen[static_cast<std::size_t>(value - min)]);
    }
}

} // namespace

int main()
{
    /// 1) 相同种子 → 完全相同的随机序列（可复现）
    std::array<int, 16> first_ints{};
    std::array<double, 16> first_doubles{};
    bgt_random_seed(kSeed);
    for (int i = 0; i < 16; i++) {
        first_ints[i] = bgt_random(0, 1000);
        first_doubles[i] = bgt_random(0.0, 1.0);
    }

    std::array<int, 16> second_ints{};
    std::array<double, 16> second_doubles{};
    bgt_random_seed(kSeed);
    for (int i = 0; i < 16; i++) {
        second_ints[i] = bgt_random(0, 1000);
        second_doubles[i] = bgt_random(0.0, 1.0);
    }
    for (int i = 0; i < 16; i++) {
        BGT_CHECK(first_ints[i] == second_ints[i]);
        BGT_CHECK(first_doubles[i] == second_doubles[i]);
    }

    /// 2) 半开区间：min 能取到，max 取不到
    check_all_values_appear(0, 6);  ///< 0~5，正好当长度 6 的数组下标
    check_all_values_appear(-3, 4); ///< -3~3

    /// 3) min > max 时自动交换
    for (int i = 0; i < 1000; i++) {
        const int value = bgt_random(50, 2);
        BGT_CHECK(value >= 2 && value < 50);
    }

    /// 4) min == max 时直接返回该值（空区间）
    BGT_CHECK(bgt_random(7, 7) == 7);
    BGT_CHECK(bgt_random(3.5, 3.5) == 3.5);

    /// 5) 小数版同样是半开：永远取不到 max
    for (int i = 0; i < 10000; i++) {
        const double value = bgt_random(0.0, 1.0);
        BGT_CHECK(value >= 0.0);
        BGT_CHECK(value < 1.0);
    }
    for (int i = 0; i < 1000; i++) {
        const double value = bgt_random(5.0, 2.0); ///< 交换后 [2, 5)
        BGT_CHECK(value >= 2.0 && value < 5.0);
    }

    /// 6) 不重新播种时，连续两段序列应当不同（引擎在持续前进）
    std::array<int, 64> window_a{};
    std::array<int, 64> window_b{};
    for (int i = 0; i < 64; i++) {
        window_a[i] = bgt_random(0, 1000000);
    }
    for (int i = 0; i < 64; i++) {
        window_b[i] = bgt_random(0, 1000000);
    }
    bool sequences_differ = false;
    for (int i = 0; i < 64; i++) {
        if (window_a[i] != window_b[i]) {
            sequences_differ = true;
            break;
        }
    }
    BGT_CHECK(sequences_differ);

    if (g_failed_checks > 0) {
        std::fprintf(stderr, "bgt_test_random: %d checks failed\n",
                     g_failed_checks);
        return 1;
    }
    std::puts("bgt_test_random: all tests passed");
    return 0;
}

// NOLINTEND(readability-magic-numbers)
