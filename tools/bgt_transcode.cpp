/**
 * @file bgt_transcode.cpp
 * @brief 构建期转码工具：把 UTF-8 文本文件写成目标编码。
 * @details 用途：
 *          - 生成 GBK 版头文件（`BGT_SOURCE_ENCODING=GBK` 时分发给学生，
 *            见 `docs/encoding.md`）；
 *          - 生成 GBK 版的课程分发物（示例源码、文档）。
 *
 *          用法：
 *          @code
 *          bgt_transcode <源文件> <目标文件> <utf-8|gbk>
 *                        [--prepend <一行文本>]
 *          @endcode
 *
 *          逐行严格转换：出现既不是合法 UTF-8、又无法用目标编码表示的字符
 *          时，报出文件名与行号并以非 0 退出，避免悄悄发出一个残缺的头文件。
 *          `--prepend` 在输出最前面插入一行，用来让 GBK 版头文件自描述
 *          （固定 `BGT_SOURCE_ENCODING`）。
 */

#include "bgt_encoding.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace {

/// @brief UTF-8 BOM 的字节序列。
const char kUtf8Bom[] = "\xEF\xBB\xBF";

/**
 * @brief 向 stderr 打印命令行用法。
 */
void print_usage()
{
    std::fprintf(stderr, "usage: bgt_transcode <source> <dest> <utf-8|gbk> "
                         "[--prepend <line>]\n");
}

/**
 * @brief 解析命令行给出的目标编码名。
 *
 * 接受 `utf-8` / `utf8` / `UTF-8` 与 `gbk` / `GBK`。
 *
 * @param name 命令行中的编码名。
 * @param[out] out 解析成功时写入目标编码；失败时不改动。
 * @return 解析成功返回 true；无法识别返回 false。
 */
bool parse_encoding(const std::string &name, bgt::Encoding &out)
{
    if (name == "utf-8" || name == "utf8" || name == "UTF-8") {
        out = bgt::Encoding::Utf8;
        return true;
    }
    if (name == "gbk" || name == "GBK") {
        out = bgt::Encoding::Gbk;
        return true;
    }
    return false;
}

/**
 * @brief 以二进制方式把整个文件读入字符串。
 *
 * @param path 文件路径。
 * @param[out] out 读取成功时写入文件内容。
 * @return 读取成功返回 true；打开失败或读取出错返回 false。
 */
bool read_file(const char *path, std::string &out)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(file),
               std::istreambuf_iterator<char>());
    return !file.bad();
}

/**
 * @brief 以二进制方式把字符串写入文件（覆盖已有内容）。
 *
 * @param path 文件路径。
 * @param text 要写入的内容。
 * @return 写入成功返回 true；打开或写入失败返回 false。
 */
bool write_file(const char *path, const std::string &text)
{
    std::ofstream file(path,
                       std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    file.close();
    return static_cast<bool>(file);
}

/**
 * @brief 按 `'\n'` 切分文本并保留行尾。
 *
 * 保留行尾，这样报错可以精确到行。
 *
 * @param text 输入文本。
 * @return 切分后的行；每行都含行尾换行符（最后一行可能没有）。
 */
std::vector<std::string> split_lines(const std::string &text)
{
    std::vector<std::string> lines;
    std::string current;
    for (const char ch : text) {
        current.push_back(ch);
        if (ch == '\n') {
            lines.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) {
        lines.push_back(current);
    }
    return lines;
}

/**
 * @brief 逐行严格转换文本，并把结果追加到 `out` 末尾。
 *
 * 调用方负责准备初始内容（例如 `--prepend` 生成的那一行）。失败时报出
 * 1 起始的行号。
 *
 * @param text 待转换的 UTF-8 文本。
 * @param to 目标编码。
 * @param source_name 出错时报出的文件名。
 * @param first_line 第一行对应的行号。
 * @param[in,out] out 转换结果追加到这里。
 * @return 全部转换成功返回 true；否则返回 false。
 */
bool convert_lines(const std::string &text, bgt::Encoding to,
                   const char *source_name, std::size_t first_line,
                   std::string &out)
{
    const std::vector<std::string> lines = split_lines(text);
    for (std::size_t index = 0; index < lines.size(); ++index) {
        std::string converted;
        if (!bgt::convert_strict(lines[index], bgt::Encoding::Utf8, to,
                                 converted)) {
            std::fprintf(stderr,
                         "bgt_transcode: %s:%zu: character not representable "
                         "in the target encoding\n",
                         source_name, first_line + index);
            return false;
        }
        out += converted;
    }
    return true;
}

} // namespace

/**
 * @brief 程序入口：解析命令行参数、执行转码并返回退出状态。
 *
 * @param argc 参数个数。
 * @param argv 参数数组。
 * @return 成功返回 0；参数错误、转码失败或读写失败返回非 0。
 */
int main(int argc, char **argv)
{
    if (argc < 4) {
        print_usage();
        return 1;
    }
    const char *source_path = argv[1];
    const char *dest_path = argv[2];
    bgt::Encoding target = bgt::Encoding::Utf8;
    if (!parse_encoding(argv[3], target)) {
        std::fprintf(stderr, "bgt_transcode: unknown target encoding '%s'\n",
                     argv[3]);
        print_usage();
        return 1;
    }
    std::string prepend;
    bool has_prepend = false;
    if (argc > 4) {
        if (std::string(argv[4]) != "--prepend" || argc < 6) {
            std::fprintf(stderr, "bgt_transcode: bad option '%s'\n", argv[4]);
            print_usage();
            return 1;
        }
        prepend = argv[5];
        prepend.push_back('\n');
        has_prepend = true;
    }

    std::string source;
    if (!read_file(source_path, source)) {
        std::fprintf(stderr, "bgt_transcode: cannot read %s\n", source_path);
        return 3;
    }
    /// 输入带 UTF-8 BOM 时跳过：输出编码由参数决定，不再重复写 BOM。
    if (source.rfind(kUtf8Bom, 0) == 0) {
        source.erase(0, 3);
    }

    std::string output;
    if (has_prepend &&
        !convert_lines(prepend, target, "(--prepend)", 1, output)) {
        return 2;
    }
    if (!convert_lines(source, target, source_path, 1, output)) {
        return 2;
    }
    if (!write_file(dest_path, output)) {
        std::fprintf(stderr, "bgt_transcode: cannot write %s\n", dest_path);
        return 3;
    }
    std::printf("bgt_transcode: %s -> %s (%s)\n", source_path, dest_path,
                target == bgt::Encoding::Gbk ? "gbk" : "utf-8");
    return 0;
}
