// libbgt 文件存档 API 的测试（纯函数，无需打开窗口）。
// 用显式检查而非 assert：Release（NDEBUG）下 assert 是空操作，
// 显式检查在任何构建配置下都真正生效。
// 用 -DBGT_BUILD_TESTS=ON 配置后由 ctest 运行。

#include "bgt.h"

#include <climits>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>

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

} // namespace

int main()
{
    // 1) 默认值：节或键不存在时返回默认值，且不产生错误。
    bgt_clear_error();
    BGT_CHECK(bgt_get_int("不存在的节", "x", 7) == 7);
    BGT_CHECK(bgt_get_double("不存在的节", "x", 0.5) == 0.5);
    char buffer[16] = {};
    bgt_get_string("不存在的节", "x", buffer, 16, "默认名");
    BGT_CHECK(text_equals(buffer, "默认名"));
    BGT_CHECK(!bgt_has_error());

    // 2) 三类型往返（纯内存）。
    bgt_set_int("玩家", "hp", -3);
    bgt_set_double("进度", "time", 0.1);
    bgt_set_string("玩家", "name", "  张三  "); // 值首尾空白会被去掉
    bgt_set_string("玩家", "note", "a=b 与 空格");
    bgt_set_string("玩家", "empty", "");
    BGT_CHECK(bgt_get_int("玩家", "hp", 0) == -3);
    BGT_CHECK(bgt_get_double("进度", "time", 0.0) == 0.1);
    char name[16] = {};
    bgt_get_string("玩家", "name", name, 16, "?");
    BGT_CHECK(text_equals(name, "张三"));
    char note[24] = {};
    bgt_get_string("玩家", "note", note, 24, "?");
    BGT_CHECK(text_equals(note, "a=b 与 空格"));
    char empty[4] = {};
    bgt_get_string("玩家", "empty", empty, 4, "?");
    BGT_CHECK(text_equals(empty, ""));

    // 3) 覆盖：同一个 (节, 键) 再次 set 会替换旧值（跨类型也替换）。
    bgt_set_int("玩家", "hp", 100);
    BGT_CHECK(bgt_get_int("玩家", "hp", 0) == 100);
    bgt_set_string("玩家", "hp", "甲");
    char swapped[8] = {};
    bgt_get_string("玩家", "hp", swapped, 8, "?");
    BGT_CHECK(text_equals(swapped, "甲"));
    bgt_clear_error();
    BGT_CHECK(bgt_get_int("玩家", "hp", 9) == 9); // “甲”按整数读 → 默认值
    BGT_CHECK(bgt_error_code() == BGT_ERROR_STORAGE);

    // 4) 宽窄转换矩阵：整数可以按小数读，反向不行；字符串拿原文。
    bgt_clear_error();
    bgt_set_int("进度", "count", 120);
    BGT_CHECK(bgt_get_double("进度", "count", 0.0) == 120.0); // 宽转 OK
    BGT_CHECK(!bgt_has_error());
    bgt_set_double("进度", "time2", 45.5);
    BGT_CHECK(bgt_get_int("进度", "time2", 0) == 0); // 窄转不行
    BGT_CHECK(bgt_error_code() == BGT_ERROR_STORAGE);
    bgt_clear_error();
    char raw[8] = {};
    bgt_get_string("进度", "count", raw, 8, "?");
    BGT_CHECK(text_equals(raw, "120"));

    // 5) 跨节同名键互不干扰。
    bgt_set_int("a", "x", 1);
    bgt_set_int("b", "x", 2);
    BGT_CHECK(bgt_get_int("a", "x", 0) == 1);
    BGT_CHECK(bgt_get_int("b", "x", 0) == 2);

    // 6) 名字校验：set 与 get 都执行；空名、含 = 的键、含 [ ] 的节名
    //    都会记错，set 不存入、get 返回默认值。
    bgt_clear_error();
    bgt_set_int("", "x", 1);
    BGT_CHECK(bgt_has_error());
    bgt_clear_error();
    bgt_set_int("节", "bad=key", 1);
    BGT_CHECK(bgt_has_error());
    BGT_CHECK(bgt_get_int("节", "bad=key", 5) == 5); // 没存进去
    bgt_clear_error();
    bgt_set_int("坏[节]", "x", 1);
    BGT_CHECK(bgt_has_error());
    bgt_clear_error();
    bgt_set_int("节", "#k", 1); // # 开头的键存盘后会丢，set 时就拒绝
    BGT_CHECK(bgt_has_error());
    BGT_CHECK(bgt_get_int("节", "#k", 5) == 5); // 没存进去
    bgt_clear_error();
    bgt_set_int("节", "[k", 1); // [ 开头的键同理
    BGT_CHECK(bgt_has_error());
    bgt_clear_error();
    bgt_set_string("节", "k", "a\nb"); // 值含换行会写坏存档
    BGT_CHECK(bgt_has_error());
    char multi[8] = {};
    bgt_get_string("节", "k", multi, 8, "?");
    BGT_CHECK(text_equals(multi, "?")); // 没存进去
    bgt_clear_error();
    BGT_CHECK(bgt_get_int("", "x", 4) == 4); // get 同样校验并记错
    BGT_CHECK(bgt_has_error());
    bgt_clear_error();

    // 7) 截断：放不下时按 UTF-8 边界截断并记错；放得下则完整拷贝。
    bgt_set_string("截断", "long", "张三");
    char small[5] = {}; // 放得下 1 个中文字符 + NUL
    bgt_get_string("截断", "long", small, 5, "?");
    BGT_CHECK(text_equals(small, "张"));
    BGT_CHECK(bgt_has_error());
    bgt_clear_error();
    char fits[8] = {}; // “张三”共 6 字节，加 NUL 放得进 8
    bgt_get_string("截断", "long", fits, 8, "?");
    BGT_CHECK(text_equals(fits, "张三"));
    BGT_CHECK(!bgt_has_error());
    bgt_get_string("截断", "long", nullptr, 0, "?"); // 无处可写 → 仅记错
    BGT_CHECK(bgt_has_error());
    bgt_clear_error();

    // 8) 缺文件 load：返回 true、空表、无错误（第一次运行的常态）。
    //    顺带验证：空表 save 产出 0 字节文件，0 字节文件再 load 同样
    //    空表、无错误。
    constexpr const char *k_test_file = "bgt_test_storage.tmp";
    std::remove(k_test_file);
    std::remove((std::string(k_test_file) + ".tmp").c_str());
    bgt_clear_error();
    BGT_CHECK(bgt_load(k_test_file));
    BGT_CHECK(!bgt_has_error());
    BGT_CHECK(bgt_get_int("进度", "count", 0) == 0); // 表是空的
    BGT_CHECK(bgt_save(k_test_file));               // 空表 → 0 字节文件
    {
        std::ifstream file(k_test_file, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        BGT_CHECK(content.empty());
    }
    bgt_clear_error();
    BGT_CHECK(bgt_load(k_test_file)); // 0 字节文件 → 空表、无错
    BGT_CHECK(!bgt_has_error());
    BGT_CHECK(bgt_get_int("进度", "count", 0) == 0);

    // 9) 整表往返：set → save → 改内存 → load → 逐键读回（含 int 极值
    //    与含 = 的值）。
    //    先用一次“缺文件 load”把内存表清空，保证写盘内容可精确断言。
    std::remove(k_test_file);
    bgt_clear_error();
    BGT_CHECK(bgt_load(k_test_file)); // 文件不存在 → 清空内存表
    bgt_set_int("最高分", "best", 321);
    bgt_set_double("进度", "time", 45.5);
    bgt_set_string("玩家", "name", "李四");
    bgt_set_int("极限", "min", INT_MIN);
    bgt_set_int("极限", "max", INT_MAX);
    bgt_set_string("玩家", "等式", "1+1=2"); // 值里的 = 不会写坏存档
    BGT_CHECK(bgt_save(k_test_file));
    BGT_CHECK(bgt_file_exists(k_test_file));
    bgt_set_int("最高分", "best", -77); // 只改内存
    BGT_CHECK(bgt_load(k_test_file));   // load 整表替换内存
    BGT_CHECK(bgt_get_int("最高分", "best", 0) == 321);
    BGT_CHECK(bgt_get_double("进度", "time", 0.0) == 45.5);
    char who[16] = {};
    bgt_get_string("玩家", "name", who, 16, "?");
    BGT_CHECK(text_equals(who, "李四"));
    BGT_CHECK(bgt_get_int("极限", "min", 0) == INT_MIN);
    BGT_CHECK(bgt_get_int("极限", "max", 0) == INT_MAX);
    char formula[16] = {};
    bgt_get_string("玩家", "等式", formula, 16, "?");
    BGT_CHECK(text_equals(formula, "1+1=2"));

    // 10) 写盘格式：节与节内键都按字典序（按 UTF-8 字节序），行尾 \n。
    {
        std::ifstream file(k_test_file, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        // 字节序：最高分(E6 9C..) < 极限(E6 9E..) < 玩家(E7..) < 进度(E8..)
        BGT_CHECK(content ==
                  "[最高分]\nbest=321\n\n"
                  "[极限]\nmax=2147483647\nmin=-2147483648\n\n"
                  "[玩家]\nname=李四\n等式=1+1=2\n\n"
                  "[进度]\ntime=45.5\n\n");
    }

    // 11) load 的整表替换：读入后，之前只在内存里的键消失。
    bgt_clear_error();
    bgt_set_int("临时", "x", 1);
    BGT_CHECK(bgt_load(k_test_file));
    BGT_CHECK(bgt_get_int("临时", "x", 0) == 0);

    // 12) 手写文件：# 注释、\r\n 行尾、UTF-8 BOM、键值两侧空格都能读。
    {
        std::ofstream file(k_test_file, std::ios::binary | std::ios::trunc);
        file << "\xEF\xBB\xBF";
        file << "# 用记事本写的存档\r\n";
        file << "\r\n";
        file << "[设置]\r\n";
        file << " volume = 80 \r\n"; // 键值两侧的空格会被去掉
        file << "muted=true\r\n";
    }
    bgt_clear_error();
    BGT_CHECK(bgt_load(k_test_file));
    BGT_CHECK(!bgt_has_error());
    BGT_CHECK(bgt_get_int("设置", "volume", 0) == 80);
    char muted[8] = {};
    bgt_get_string("设置", "muted", muted, 8, "?");
    BGT_CHECK(text_equals(muted, "true"));

    // 13) 坏行容错：节头前的键值行、没有 = 的行、坏节头都跳过并记错，
    //     好行照常生效；load 返回 false。坏节头之后的键值行也判为坏行
    //     （不会悄悄归进前一个节）。
    {
        std::ofstream file(k_test_file, std::ios::binary | std::ios::trunc);
        file << "孤儿=1\n";   // 节头之前 → 坏行
        file << "[好节]\n";
        file << "ok=42\n";
        file << "没有等号的行\n"; // → 坏行
        file << "[坏节\n";        // 缺 ] → 坏行
        file << "also=7\n";       // 坏节头之后 → 也是坏行
    }
    bgt_clear_error();
    BGT_CHECK(!bgt_load(k_test_file)); // 有坏行 → false
    BGT_CHECK(bgt_has_error());
    BGT_CHECK(bgt_error_code() == BGT_ERROR_STORAGE);
    BGT_CHECK(bgt_get_int("好节", "ok", 0) == 42);  // 好行仍生效
    BGT_CHECK(bgt_get_int("好节", "also", 0) == 0); // 坏节头后的行不收编
    bgt_clear_error();

    // 14) 重复节头合并、重复键后者覆盖、空节往返保留。
    {
        std::ofstream file(k_test_file, std::ios::binary | std::ios::trunc);
        file << "[节一]\n";
        file << "a=1\n";
        file << "[节二]\n";
        file << "b=2\n";
        file << "[节一]\n"; // 重复节头 = 同一节继续
        file << "a=9\n";   // 重复键后者覆盖
        file << "[空节]\n";
    }
    bgt_clear_error();
    BGT_CHECK(bgt_load(k_test_file));
    BGT_CHECK(bgt_get_int("节一", "a", 0) == 9);
    BGT_CHECK(bgt_get_int("节二", "b", 0) == 2);
    BGT_CHECK(bgt_save(k_test_file));
    {
        std::ifstream file(k_test_file, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        // 字节序：空节(E7..) < 节一/节二(E8..)，一(B8..) < 二(BA..)
        BGT_CHECK(content == "[空节]\n\n[节一]\na=9\n\n[节二]\nb=2\n\n");
    }

    // 15) 超范围整数文本：按 int 读 → 默认值 + 记错；按 double 读得动。
    bgt_clear_error();
    bgt_set_string("大数", "x", "5000000000");
    BGT_CHECK(bgt_get_int("大数", "x", 3) == 3);
    BGT_CHECK(bgt_error_code() == BGT_ERROR_STORAGE);
    bgt_clear_error();
    BGT_CHECK(bgt_get_double("大数", "x", 0.0) == 5000000000.0);
    bgt_clear_error();

    // 16) file_exists 两态；save 到不存在的目录会失败并记错；
    //     null / 空文件名直接失败并记错（file_exists 只返回 false）。
    std::remove(k_test_file);
    BGT_CHECK(!bgt_file_exists(k_test_file));
    bgt_set_int("临时", "x", 1);
    BGT_CHECK(bgt_save(k_test_file));
    BGT_CHECK(bgt_file_exists(k_test_file));
    bgt_clear_error();
    BGT_CHECK(!bgt_save("不存在的目录/x.txt")); // 建不了文件 → 记错
    BGT_CHECK(bgt_has_error());
    bgt_clear_error();
    BGT_CHECK(bgt_load("不存在的目录/x.txt")); // 不存在 → 空表、无错
    BGT_CHECK(!bgt_has_error());
    bgt_clear_error();
    BGT_CHECK(!bgt_load(nullptr));
    BGT_CHECK(bgt_error_code() == BGT_ERROR_STORAGE);
    bgt_clear_error();
    BGT_CHECK(!bgt_load(""));
    BGT_CHECK(bgt_error_code() == BGT_ERROR_STORAGE);
    bgt_clear_error();
    BGT_CHECK(!bgt_save(nullptr));
    BGT_CHECK(bgt_error_code() == BGT_ERROR_STORAGE);
    bgt_clear_error();
    BGT_CHECK(!bgt_save(""));
    BGT_CHECK(bgt_error_code() == BGT_ERROR_STORAGE);
    bgt_clear_error();
    BGT_CHECK(!bgt_file_exists(nullptr));
    BGT_CHECK(!bgt_file_exists(""));
    BGT_CHECK(!bgt_has_error());

    // 17) 清理测试文件。
    std::remove(k_test_file);
    std::remove((std::string(k_test_file) + ".tmp").c_str());

    if (g_failed_checks > 0) {
        std::fprintf(stderr, "bgt_test_storage: %d checks failed\n",
                     g_failed_checks);
        return 1;
    }
    std::puts("bgt_test_storage: all tests passed");
    return 0;
}

// NOLINTEND(readability-magic-numbers)
