// libbgt 错误历史 API 的测试（纯函数，无需打开窗口）。
// 用显式检查而非 assert：Release（NDEBUG）下 assert 是空操作，
// 显式检查在任何构建配置下都真正生效。
// 用 -DBGT_BUILD_TESTS=ON 配置后由 ctest 运行。

#include "bgt.h"

#include <cstdio>
#include <cstring>

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
        }                                                               \
    } while (false)

bool text_equals(const char *a, const char *b)
{
    return std::strcmp(a, b) == 0;
}

bool text_starts_with(const char *text, const char *prefix)
{
    return std::strncmp(text, prefix, std::strlen(prefix)) == 0;
}

} // namespace

int main()
{
    // 1) 空历史：所有查询静默，且不产生新错误（铁律）。
    bgt_clear_error();
    BGT_CHECK(bgt_error_count() == 0);
    BGT_CHECK(!bgt_has_error());
    BGT_CHECK(bgt_error_code() == BGT_ERROR_NONE);
    BGT_CHECK(bgt_error_code(0) == BGT_ERROR_NONE);
    BGT_CHECK(bgt_error_code(-1) == BGT_ERROR_NONE);
    BGT_CHECK(bgt_error_code(99) == BGT_ERROR_NONE);
    char text[64] = {};
    bgt_error_text(0, text, sizeof(text));
    BGT_CHECK(text_equals(text, ""));
    bgt_error_text(3, nullptr, 10); // 无处可写：静默不崩
    bgt_error_text(3, text, 0);
    bgt_error_text(3, text, -5);
    BGT_CHECK(bgt_error_count() == 0); // 查询没有往历史里塞条目

    // 2) 触发与增长：未开窗时各函数留下不同错误；序号 0 最老。
    bgt_clear_error();
    BGT_CHECK(bgt_load_image("no.png") == 0); // 未开窗 → NOT_OPEN
    BGT_CHECK(bgt_error_count() == 1);
    BGT_CHECK(bgt_error_code(0) == BGT_ERROR_NOT_OPEN);
    BGT_CHECK(!bgt_set_font(nullptr, 16)); // → FONT：文件名为空
    BGT_CHECK(bgt_error_count() == 2);
    BGT_CHECK(bgt_error_code(1) == BGT_ERROR_FONT);
    BGT_CHECK(!bgt_set_font("a.ttf", 0)); // → FONT：字号必须为正
    BGT_CHECK(bgt_error_count() == 3);
    char first[64] = {};
    char second[64] = {};
    bgt_error_text(0, first, sizeof(first));
    bgt_error_text(1, second, sizeof(second));
    BGT_CHECK(text_equals(first, "libbgt window is not open"));
    BGT_CHECK(text_equals(second, "font filename is empty"));
    BGT_CHECK(bgt_error_code() == BGT_ERROR_FONT); // 无参糖 = 最新一条

    // 3) 铁律：查询越界、取文本都不改历史条数。
    const int before = bgt_error_count();
    BGT_CHECK(bgt_error_code(100) == BGT_ERROR_NONE);
    BGT_CHECK(bgt_error_code(-3) == BGT_ERROR_NONE);
    bgt_error_text(-2, text, sizeof(text));
    BGT_CHECK(text_equals(text, ""));
    BGT_CHECK(bgt_error_count() == before);

    // 4) 容量与挤出：历史封顶 10 条，最老的先被挤出去。
    bgt_clear_error();
    for (int i = 0; i < 10; i = i + 1) {
        bgt_load_image("no.png"); // 返回值在第 2) 组验过
    }
    BGT_CHECK(bgt_error_count() == 10);
    BGT_CHECK(bgt_error_code(0) == BGT_ERROR_NOT_OPEN);
    // 第 11~20 条：连续 FONT 错误把 10 条 NOT_OPEN 全部挤出，
    // 最老的一条从 NOT_OPEN 换成 FONT（每条只挤出一条最老的）。
    for (int i = 0; i < 10; i = i + 1) {
        bgt_set_font(nullptr, 16);
    }
    BGT_CHECK(bgt_error_count() == 10);
    BGT_CHECK(bgt_error_code(0) == BGT_ERROR_FONT); // 最老的换了
    BGT_CHECK(bgt_error_code(9) == BGT_ERROR_FONT); // 最新=刚触发
    char oldest[64] = {};
    bgt_error_text(0, oldest, sizeof(oldest));
    BGT_CHECK(text_equals(oldest, "font filename is empty"));

    // 5) 文本截断：放不下时按 UTF-8 字符边界截断（静默）。
    //    消息前缀 "failed to open font " 是 20 字节（含尾随空格）；
    //    中文文件名每个字符 3 字节。out_size 24 放得下 23 字节；
    //    out_size 22 的截断点切进“不”的中间字节，退回字符边界后
    //    得到 20 字节——注意结果末尾带前缀的尾随空格。
    bgt_clear_error();
    bgt_set_font("不存在的字体.ttf", 16); // 失败消息含中文文件名
    BGT_CHECK(bgt_error_count() == 1);
    BGT_CHECK(bgt_error_code(0) == BGT_ERROR_FONT);
    char big[128] = {};
    bgt_error_text(0, big, sizeof(big));
    BGT_CHECK(text_starts_with(big, "failed to open font 不存在的字体.ttf"));
    char cut[24] = {};
    bgt_error_text(0, cut, 24);
    BGT_CHECK(text_equals(cut, "failed to open font 不"));
    char small[22] = {};
    bgt_error_text(0, small, 22);
    BGT_CHECK(text_equals(small, "failed to open font "));
    BGT_CHECK(bgt_error_count() == 1); // 截断没有记新错误（铁律）

    // 6) 清空：整份历史归零。
    bgt_clear_error();
    BGT_CHECK(bgt_error_count() == 0);
    BGT_CHECK(!bgt_has_error());
    BGT_CHECK(bgt_error_code() == BGT_ERROR_NONE);

    // 7) 打印不产生错误（输出到控制台，内容不作断言）。
    bgt_set_font(nullptr, 16);
    BGT_CHECK(bgt_error_count() == 1);
    bgt_print_error();   // 最新一条
    bgt_print_error(0);  // 按序号
    bgt_print_error(99); // 越界：静默
    BGT_CHECK(bgt_error_count() == 1);
    bgt_clear_error();

    if (g_failed_checks > 0) {
        std::fprintf(stderr, "bgt_test_errors: %d checks failed\n",
                     g_failed_checks);
        return 1;
    }
    std::puts("bgt_test_errors: all tests passed");
    return 0;
}

// NOLINTEND(readability-magic-numbers)
