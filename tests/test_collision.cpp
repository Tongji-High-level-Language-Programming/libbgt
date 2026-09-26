// libbgt 碰撞检测 API 的最小测试（纯函数，无需打开窗口）。
// 用显式检查而非 assert：Release（NDEBUG）下 assert 是空操作，
// 显式检查在任何构建配置下都真正生效。
// 用 -DBGT_BUILD_TESTS=ON 配置后由 ctest 运行。

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
            g_failed_checks = g_failed_checks + 1;                     \
        }                                                               \
    } while (false)

} // namespace

int main()
{
    // 1) 点与矩形：命中范围与 fill_rect 画出的像素完全一致
    BGT_CHECK(bgt_hit_point_rect(10, 20, 10, 20, 100, 50));   // 左上角
    BGT_CHECK(bgt_hit_point_rect(109, 69, 10, 20, 100, 50)); // 右下角像素
    BGT_CHECK(bgt_hit_point_rect(55, 45, 10, 20, 100, 50));   // 内部
    BGT_CHECK(!bgt_hit_point_rect(110, 20, 10, 20, 100, 50)); // 右边界外
    BGT_CHECK(!bgt_hit_point_rect(10, 70, 10, 20, 100, 50));  // 下边界外
    BGT_CHECK(!bgt_hit_point_rect(110, 70, 10, 20, 100, 50)); // 右下角外
    BGT_CHECK(!bgt_hit_point_rect(9, 20, 10, 20, 100, 50));   // 左边界外
    BGT_CHECK(!bgt_hit_point_rect(10, 19, 10, 20, 100, 50));  // 上边界外
    BGT_CHECK(!bgt_hit_point_rect(10, 20, 10, 20, 0, 50));    // 零宽
    BGT_CHECK(!bgt_hit_point_rect(10, 20, 10, 20, -5, 50));   // 负宽
    BGT_CHECK(!bgt_hit_point_rect(10, 20, 10, 20, 100, 0));   // 零高

    // 2) 点与圆：距离不超过半径（含圆周）；3-4-5 直角三角形做精确验证
    BGT_CHECK(bgt_hit_point_circle(0, 0, 0, 0, 5));    // 圆心
    BGT_CHECK(bgt_hit_point_circle(3, 4, 0, 0, 5));    // 圆周（3-4-5）
    BGT_CHECK(bgt_hit_point_circle(0, 5, 0, 0, 5));    // 正上圆周
    BGT_CHECK(!bgt_hit_point_circle(0, 6, 0, 0, 5));   // 圆外一步
    BGT_CHECK(!bgt_hit_point_circle(3, 5, 0, 0, 5));   // 圆外
    BGT_CHECK(!bgt_hit_point_circle(1, 0, 0, 0, 0));   // 零半径
    BGT_CHECK(!bgt_hit_point_circle(3, 4, 0, 0, -5));  // 负半径

    // 3) 矩形与矩形：实际重叠才算；仅共享边、角不算
    BGT_CHECK(bgt_hit_rect_rect(0, 0, 10, 10, 5, 5, 10, 10));    // 部分重叠
    BGT_CHECK(bgt_hit_rect_rect(0, 0, 10, 10, 2, 2, 3, 3));      // 完全内含
    BGT_CHECK(bgt_hit_rect_rect(0, 0, 10, 10, 9, 9, 10, 10));   // 1 像素重叠
    BGT_CHECK(!bgt_hit_rect_rect(0, 0, 10, 10, 10, 0, 10, 10));  // 共享左右边
    BGT_CHECK(!bgt_hit_rect_rect(0, 0, 10, 10, 0, 10, 10, 10));  // 共享上下边
    BGT_CHECK(!bgt_hit_rect_rect(0, 0, 10, 10, 10, 10, 10, 10)); // 只贴角
    BGT_CHECK(!bgt_hit_rect_rect(0, 0, 10, 10, 20, 20, 10, 10)); // 完全分离
    BGT_CHECK(!bgt_hit_rect_rect(0, 0, 0, 10, 5, 5, 10, 10));   // 空矩形
    BGT_CHECK(!bgt_hit_rect_rect(0, 0, -3, 10, 5, 5, 10, 10));  // 负宽
    // 跨立反例：空矩形被另一矩形跨住时，严格不等式仍会成立，
    // 必须由实现里的显式守卫排除（守卫的回归防线）。
    BGT_CHECK(!bgt_hit_rect_rect(0, 0, 0, 10, -5, 0, 10, 10));  // 零宽被跨立
    BGT_CHECK(!bgt_hit_rect_rect(0, 0, -3, 10, -50, 0, 100, 10)); // 负宽被跨立
    BGT_CHECK(!bgt_hit_rect_rect(0, 0, 10, 0, 0, -5, 10, 10));  // 零高被跨立

    // 4) 圆与圆：实际重叠才算；恰好相切不算
    BGT_CHECK(bgt_hit_circle_circle(0, 0, 3, 4, 0, 2));   // 重叠
    BGT_CHECK(bgt_hit_circle_circle(0, 0, 5, 0, 0, 3));   // 内含
    BGT_CHECK(!bgt_hit_circle_circle(0, 0, 3, 5, 0, 2));  // d=5=r1+r2 相切
    BGT_CHECK(!bgt_hit_circle_circle(0, 0, 3, 6, 0, 2));  // 分离
    BGT_CHECK(!bgt_hit_circle_circle(0, 0, 0, 0, 0, 5));  // 零半径
    BGT_CHECK(!bgt_hit_circle_circle(5, 0, -3, 0, 0, 2)); // 负半径

    // 5) 圆与矩形：圆心在内/边上算命中；外侧恰好相切不算
    BGT_CHECK(bgt_hit_circle_rect(5, 5, 3, 0, 0, 10, 10));    // 圆心在内
    BGT_CHECK(bgt_hit_circle_rect(0, 5, 3, 0, 0, 10, 10));    // 圆心在左边上
    BGT_CHECK(bgt_hit_circle_rect(10, 5, 3, 0, 0, 10, 10));   // 圆心在右边上
    BGT_CHECK(bgt_hit_circle_rect(0, 0, 3, 0, 0, 10, 10));    // 圆心在角上
    BGT_CHECK(bgt_hit_circle_rect(11, 11, 5, 0, 0, 10, 10));  // 角旁仍重叠
    BGT_CHECK(!bgt_hit_circle_rect(13, 5, 3, 0, 0, 10, 10));  // 外侧相切
    BGT_CHECK(!bgt_hit_circle_rect(16, 16, 5, 0, 0, 10, 10)); // 角外分离
    BGT_CHECK(!bgt_hit_circle_rect(5, 5, 0, 0, 0, 10, 10));   // 零半径
    BGT_CHECK(!bgt_hit_circle_rect(5, 5, -3, 0, 0, 10, 10));  // 负半径
    BGT_CHECK(!bgt_hit_circle_rect(5, 5, 3, 0, 0, 0, 10));    // 零宽矩形

    // 6) 极端坐标：内部用 long long，坐标差 ±3×10^9 内不溢出
    //    （下面的 x+width 与 d^2 若用 int 计算都会溢出）
    BGT_CHECK(bgt_hit_point_rect(2147483647, 5, 2000000000, 0,
                                2000000000, 10));
    BGT_CHECK(bgt_hit_rect_rect(2000000000, 0, 100000000, 10,
                                2050000000, 0, 100, 10));
    BGT_CHECK(bgt_hit_circle_circle(-1000000000, 0, 1500000000,
                                    1000000000, 0, 1500000000));
    BGT_CHECK(!bgt_hit_circle_circle(-1000000000, 0, 5, 1000000000, 0, 5));
    BGT_CHECK(!bgt_hit_point_circle(2000000000, 0, -1000000000, 0, 100000));
    BGT_CHECK(!bgt_hit_circle_rect(1000000000, 500000000, 10, -1000000000, 0,
                                  1000, 1000));

    if (g_failed_checks > 0) {
        std::fprintf(stderr, "bgt_test_collision: %d checks failed\n",
                     g_failed_checks);
        return 1;
    }
    std::puts("bgt_test_collision: all tests passed");
    return 0;
}

// NOLINTEND(readability-magic-numbers)
