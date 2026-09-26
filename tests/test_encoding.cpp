// libbgt 编码转换层的测试（纯函数，不需要打开窗口）。
// 直接包含内部头文件 src/bgt_encoding.h：这里验证的就是转换层本身——GBK 与
// UTF-8 互转、宽松/严格两种策略、字符边界截断、ASCII 字符搜索、路径转换，以及
// 存档文件与 UTF-8 BOM 的互操作。
// 用 -DBGT_BUILD_TESTS=ON 配置后由 ctest 运行。
//
// 注意：本文件里的窄字符串字面量是“源编码”（UTF-8 模式为 UTF-8，GBK 模式为
// GBK），所以凡是断言具体字节的地方一律用十六进制转义，避免编码假设。

#include "bgt.h"

#include "bgt_encoding.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

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

// 源编码 -> UTF-8，供测试自己拼接期望内容。
std::string to_utf8(const std::string &text)
{
    return bgt::convert(text, bgt::kSourceEncoding, bgt::Encoding::Utf8);
}

// UTF-8 文本的字符数（按首字节判断宽度；输入保证是合法 UTF-8）。
std::size_t utf8_char_count(const std::string &text)
{
    std::size_t count = 0;
    std::size_t index = 0;
    while (index < text.size()) {
        const auto byte = static_cast<unsigned char>(text[index]);
        if ((byte & 0xF8U) == 0xF0U) {
            index += 4;
        } else if ((byte & 0xF0U) == 0xE0U) {
            index += 3;
        } else if ((byte & 0xE0U) == 0xC0U) {
            index += 2;
        } else {
            index += 1;
        }
        count += 1;
    }
    return count;
}

// 1) 转换层本身的正确性。
void check_codec()
{
    // 同编码之间永远是恒等转换，任何平台、任何模式都成立。
    BGT_CHECK(bgt::convert("abc", bgt::Encoding::Utf8,
                           bgt::Encoding::Utf8) == "abc");

    if constexpr (!bgt::kHasGbkCodec) {
        return; // 没有 GBK 后端（非 Windows）时只验证恒等路径
    }

    // “你好”的两种编码字节（写死十六进制，与源文件编码无关）。
    const std::string utf8_hello = "\xE4\xBD\xA0\xE5\xA5\xBD";
    const std::string gbk_hello = "\xC4\xE3\xBA\xC3";
    std::string converted;
    BGT_CHECK(bgt::convert_strict(utf8_hello, bgt::Encoding::Utf8,
                                  bgt::Encoding::Gbk, converted));
    BGT_CHECK(converted == gbk_hello);
    BGT_CHECK(bgt::convert(gbk_hello, bgt::Encoding::Gbk,
                           bgt::Encoding::Utf8) == utf8_hello);

    // 源编码 -> UTF-8 -> 源编码 往返不丢字符。
    const std::string sample = "中文 abc 123 图形";
    const std::string utf8_sample = to_utf8(sample);
    BGT_CHECK(utf8_char_count(utf8_sample) == 13);
    BGT_CHECK(bgt::convert(utf8_sample, bgt::Encoding::Utf8,
                           bgt::kSourceEncoding) == sample);
    if constexpr (bgt::kSourceEncoding == bgt::Encoding::Gbk) {
        BGT_CHECK(utf8_sample != sample); // GBK 与 UTF-8 的字节确实不同
    }

    // 严格转换：GBK 表示不了的字符必须失败，且 out 不被改动。
    std::string strict_out = "unchanged";
    BGT_CHECK(!bgt::convert_strict("\xE2\x9C\x85", bgt::Encoding::Utf8,
                                   bgt::Encoding::Gbk, strict_out));
    BGT_CHECK(strict_out == "unchanged");
    // Windows 的“最佳拟合”会静默改字（U+00B2 -> '2'），严格模式必须拦住。
    BGT_CHECK(!bgt::convert_strict("\xC2\xB2", bgt::Encoding::Utf8,
                                   bgt::Encoding::Gbk, strict_out));
    // 非法（截断的）UTF-8 输入在严格模式下失败，宽松模式下不崩。
    BGT_CHECK(!bgt::convert_strict("\xE4\xBD", bgt::Encoding::Utf8,
                                   bgt::Encoding::Gbk, strict_out));
    BGT_CHECK(bgt::convert("\xE4\xBD", bgt::Encoding::Utf8,
                           bgt::Encoding::Gbk)
                  .size() <= 2);
}

// 2) 字符边界与 ASCII 搜索。
void check_string_helpers()
{
    // 截断不会切汉字：源编码下“张三”用 3 字节只放得下 1 个字。
    const std::string two_hanzi = "张三";
    const std::size_t prefix = bgt::encoded_prefix_length(two_hanzi, 3);
    BGT_CHECK(prefix > 0 && prefix <= 3);
    BGT_CHECK(utf8_char_count(to_utf8(two_hanzi.substr(0, prefix))) == 1);
    BGT_CHECK(bgt::encoded_prefix_length(two_hanzi, two_hanzi.size()) ==
              two_hanzi.size());
    BGT_CHECK(bgt::encoded_prefix_length("abcd", 2) == 2);
    BGT_CHECK(bgt::encoded_prefix_length("abcd", 0) == 0);

    // GBK 尾字节陷阱：0x82 0x5B 在 GBK 下是一个合法汉字（U+8949），尾字节恰好是
    // '['；0x82 0x5D 同理（U+8A2D，尾字节 ']'）。老实现用 find('[') 会误判，
    // 按字符边界搜索才不会。这里的字节写死，避免依赖编译器对字面量的转码方式。
    if constexpr (bgt::kSourceEncoding == bgt::Encoding::Gbk) {
        const std::string trail_lb = "\x82\x5B";
        const std::string trail_rb = "\x82\x5D";
        BGT_CHECK(!bgt::encoded_contains_ascii(trail_lb, '['));
        BGT_CHECK(!bgt::encoded_contains_ascii(trail_rb, ']'));
        BGT_CHECK(!bgt::encoded_ends_with_ascii(trail_rb, ']'));
        // 确认这两个字节确实是合法（可解码的）汉字，而不是被当成了非法序列。
        BGT_CHECK(bgt::convert(trail_lb, bgt::Encoding::Gbk,
                               bgt::Encoding::Utf8)
                      .size() == 3);
        BGT_CHECK(bgt::convert(trail_rb, bgt::Encoding::Gbk,
                               bgt::Encoding::Utf8)
                      .size() == 3);
    } else {
        // UTF-8 的多字节字符不会与 ASCII 混淆（续字节永远 >= 0x80）。
        const std::string hanzi = "\xE8\xA5\x89"; // U+8949 的 UTF-8 字节
        BGT_CHECK(!bgt::encoded_contains_ascii(hanzi, '['));
        BGT_CHECK(!bgt::encoded_ends_with_ascii(hanzi, ']'));
    }

    // 真正的 ASCII 字符仍然要能找到。
    BGT_CHECK(bgt::encoded_contains_ascii("a[b", '['));
    BGT_CHECK(bgt::encoded_contains_ascii("[", '['));
    BGT_CHECK(!bgt::encoded_contains_ascii("ab", '['));
    BGT_CHECK(bgt::encoded_ends_with_ascii("ab]", ']'));
    BGT_CHECK(!bgt::encoded_ends_with_ascii("ab", ']'));
    BGT_CHECK(!bgt::encoded_ends_with_ascii("", ']'));
}

// 3) 中文路径：源编码 -> 本机路径（Windows 下是 UTF-16）。
void check_native_path()
{
    const std::string name = "bgt_编码_路径测试.txt";
    const std::filesystem::path native = bgt::to_native_path(name);
    std::error_code error;
    std::filesystem::remove(native, error);

    {
        std::ofstream file(native,
                           std::ios::out | std::ios::binary | std::ios::trunc);
        file << "ok\n";
    }
    BGT_CHECK(std::filesystem::exists(native));
    BGT_CHECK(std::filesystem::file_size(native) == 3);
    std::filesystem::remove(native, error);
    BGT_CHECK(!std::filesystem::exists(native));
    BGT_CHECK(!std::filesystem::exists(bgt::to_native_path("bgt_不存在的文件.txt")));

    // 端到端：公开 API 用中文文件名写读存档（内部走 to_native_path）。
    bgt_clear_error();
    bgt_set_int("节", "k", 7);
    BGT_CHECK(bgt_save(name.c_str()));
    BGT_CHECK(bgt_file_exists(name.c_str()));
    BGT_CHECK(bgt_load(name.c_str()));
    BGT_CHECK(bgt_get_int("节", "k", 0) == 7);
    BGT_CHECK(!bgt_has_error());
    bgt_clear_error();
    std::filesystem::remove(bgt::to_native_path(name), error);
}

// 4) 存档文件与 UTF-8 BOM 的互操作。
void check_storage_encoding()
{
    // 带 BOM 的文件一律按 UTF-8 解释，读进来后转成源编码。
    // 整个文件（含节名、键名）都必须写成 UTF-8——BOM 是整份文件的声明。
    const std::string bom_name = "bgt_编码_bom.tmp";
    {
        std::ofstream file(bgt::to_native_path(bom_name),
                           std::ios::out | std::ios::binary | std::ios::trunc);
        file << "\xEF\xBB\xBF";
        file << "[" << to_utf8("设置") << "]\n";
        file << to_utf8("名字") << "=" << to_utf8("张三") << "\n";
    }
    bgt_clear_error();
    BGT_CHECK(bgt_load(bom_name.c_str()));
    BGT_CHECK(!bgt_has_error());
    char who[16] = {};
    bgt_get_string("设置", "名字", who, 16, "?");
    BGT_CHECK(std::strcmp(who, "张三") == 0);
    bgt_clear_error();
    std::error_code error;
    std::filesystem::remove(bgt::to_native_path(bom_name), error);

    // 反向：bgt_save() 写出的就是源编码字节，不带 BOM。
    const std::string save_name = "bgt_编码_save.tmp";
    bgt_clear_error();
    bgt_set_string("设置", "名字", "李四");
    BGT_CHECK(bgt_save(save_name.c_str()));
    BGT_CHECK(!bgt_has_error());
    {
        std::ifstream file(bgt::to_native_path(save_name),
                           std::ios::in | std::ios::binary);
        const std::string content((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
        BGT_CHECK(content == "[设置]\n名字=李四\n\n");
        BGT_CHECK(content.rfind("\xEF\xBB\xBF", 0) != 0); // 不带 BOM
    }
    bgt_clear_error();
    std::filesystem::remove(bgt::to_native_path(save_name), error);
}

} // namespace

int main()
{
    check_codec();
    check_string_helpers();
    check_native_path();
    check_storage_encoding();

    if (g_failed_checks > 0) {
        std::fprintf(stderr, "bgt_test_encoding: %d checks failed\n",
                     g_failed_checks);
        return 1;
    }
    std::printf("bgt_test_encoding: all tests passed (%s source encoding)\n",
                bgt::kSourceEncoding == bgt::Encoding::Gbk ? "GBK" : "UTF-8");
    return 0;
}

// NOLINTEND(readability-magic-numbers)
