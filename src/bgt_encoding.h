/**
 * @file bgt_encoding.h
 * @brief libbgt 内部编码转换层的声明。
 * @details libbgt 内部编码转换层：不依赖 SDL，也不对外暴露。
 *
 *          公开 API 的窄字符串一律使用“源编码”，由构建选项
 *          `BGT_SOURCE_ENCODING` 决定：
 *
 *          - UTF-8（默认）：源编码就是 UTF-8。所有转换在编译期被
 *            `if constexpr` 消除，不产生任何额外开销，行为与未引入本层时
 *            完全一致。
 *          - GBK：源编码是 GBK（Windows 代码页 936）。交给 SDL3、SDL3_ttf、
 *            SDL3_mixer 的字符串在边界处转成 UTF-8；从 SDL 取回的字符串
 *            （`SDL_GetError`）转回 GBK。库内部一切字符串保持源编码，因此
 *            存档文件、错误历史、`printf` 输出与学生的源码编码一致。
 *
 *          设计说明与接入方式见 `docs/encoding.md`。
 */

#ifndef BGT_ENCODING_H_
#define BGT_ENCODING_H_

#include "bgt.h"

#include <cstddef>
#include <filesystem>
#include <string>

/**
 * @brief 是否具备 GBK <-> UTF-8 编解码后端。
 *
 * Windows 使用 WinAPI 完成 GBK <-> UTF-8 转换。其他平台预留 iconv 后端：
 * 实现后在此定义 `BGT_HAVE_GBK_CODEC` 即可，调用方无需改动。
 */
#if defined(_WIN32)
#define BGT_HAVE_GBK_CODEC 1
#endif

namespace bgt {

/**
 * @brief 支持的编码。
 *
 * GBK 对应 Windows 代码页 936。
 */
enum class Encoding {
    Utf8,  ///< UTF-8（默认）。
    Gbk,   ///< GBK（Windows 代码页 936）。
};

/**
 * @brief 本次构建中公开 API 使用的源编码。
 */
inline constexpr Encoding kSourceEncoding =
    BGT_SOURCE_ENCODING == BGT_ENCODING_GBK ? Encoding::Gbk : Encoding::Utf8;

/**
 * @brief 是否具备 GBK <-> UTF-8 编解码后端。
 */
#ifdef BGT_HAVE_GBK_CODEC
inline constexpr bool kHasGbkCodec = true;
#else
inline constexpr bool kHasGbkCodec = false;
#endif

/**
 * @brief 在两种编码之间转换字符串。
 *
 * 目标编码无法表示的字符会退化成替换字符（宽松，不抛异常）。
 *
 * @param text 待转换的字符串。
 * @param from 输入字符串的编码。
 * @param to 目标编码。
 * @return 转换后的字符串。
 * @note 没有 GBK 编解码后端或输入无法解析时返回原文，避免丢数据。
 */
std::string convert(const std::string &text, Encoding from, Encoding to);

/**
 * @brief 严格转换：出现无法表示的字符或非法输入时失败。
 *
 * 出现无法表示的字符、或输入不是合法的 `from` 编码时返回 false，此时
 * `out` 保持不变。构建期工具用它保证转码不丢信息。
 *
 * @param text 待转换的字符串。
 * @param from 输入字符串的编码。
 * @param to 目标编码。
 * @param[out] out 成功时写入转换结果；失败时保持不变。
 * @return 转换成功返回 true；否则返回 false。
 */
bool convert_strict(const std::string &text, Encoding from, Encoding to,
                    std::string &out);

/**
 * @brief 源编码字符串转 UTF-8。
 *
 * 需要把结果留下来时用这个；只是临时传给 SDL 时用 `Utf8View`，后者在
 * UTF-8 模式下零拷贝。
 *
 * @param text 源编码字符串。
 * @return UTF-8 编码的字符串。
 */
std::string to_utf8(const std::string &text);

/**
 * @brief UTF-8 C 字符串转源编码。
 *
 * 例如 `SDL_GetError()` 的返回值。
 *
 * @param text UTF-8 编码的 C 字符串；可以是 nullptr。
 * @return 源编码的字符串；`text` 为 nullptr 时返回空字符串。
 */
std::string from_utf8(const char *text);

/**
 * @brief UTF-8 字符串转源编码。
 *
 * @param text UTF-8 编码的字符串。
 * @return 源编码的字符串。
 */
std::string from_utf8(const std::string &text);

/**
 * @brief 源编码路径转本机文件系统路径。
 *
 * Windows 下窄路径按 ANSI 解释，因此先转成 UTF-16，保证中文文件名在两种
 * 模式下都能正确打开。
 *
 * @param text 源编码的路径字符串。
 * @return 本机文件系统路径。
 */
std::filesystem::path to_native_path(const std::string &text);

/**
 * @brief 按源编码的字符边界求前缀长度。
 *
 * 截断时不会把一个多字节字符切成两半。
 *
 * @param text 源编码字符串。
 * @param limit 允许的前缀字节数上限。
 * @return 不超过 `limit`、且不切开多字节字符的最大前缀字节数。
 */
std::size_t encoded_prefix_length(const std::string &text, std::size_t limit);

/**
 * @brief 判断某个 ASCII 字符是否“作为一个完整字符”出现在源编码下。
 *
 * GBK 的尾字节可以是 `[` `]` `#` 等 ASCII 字节，直接 `find()` 会误判，
 * 所以校验节名/键名/行尾用它。
 *
 * @param text 源编码字符串。
 * @param ascii 要查找的 ASCII 字符。
 * @return 该字符以完整字符形式出现返回 true；否则返回 false。
 */
bool encoded_contains_ascii(const std::string &text, char ascii);

/**
 * @brief 判断源编码字符串是否以某个 ASCII 字符作为最后一个完整字符结束。
 *
 * @param text 源编码字符串。
 * @param ascii 要检查的 ASCII 字符。
 * @return 以该字符结束返回 true；否则返回 false。
 */
bool encoded_ends_with_ascii(const std::string &text, char ascii);

/**
 * @brief 源编码字符串的 UTF-8 视图：UTF-8 模式下零拷贝，GBK 模式下持有
 *        转换结果。
 *
 * 生命周期与临时对象一样短，只能立即使用（不可拷贝、不可保存 `c_str()`）。
 */
class Utf8View {
public:
    /**
     * @brief 从源编码的 C 字符串构造 UTF-8 视图。
     *
     * @param text 源编码字符串；nullptr 视为空字符串。
     */
    explicit Utf8View(const char *text)
    {
        const char *value = text == nullptr ? "" : text;
        if constexpr (kSourceEncoding == Encoding::Utf8) {
            text_ = value;
        } else {
            storage_ = convert(value, Encoding::Gbk, Encoding::Utf8);
            text_ = storage_.c_str();
        }
    }

    /// 视图只持有短暂有效的 UTF-8 指针，因此禁止拷贝与移动。
    Utf8View(const Utf8View &) = delete;
    Utf8View &operator=(const Utf8View &) = delete;
    Utf8View(Utf8View &&) = delete;
    Utf8View &operator=(Utf8View &&) = delete;
    ~Utf8View() = default;

    /**
     * @brief 返回 UTF-8 编码的 C 字符串。
     *
     * @return 内部持有的 UTF-8 C 字符串，只在视图存活期间有效。
     */
    [[nodiscard]] const char *c_str() const
    {
        return text_;
    }

private:
    std::string storage_;    ///< GBK 模式下的转换结果；UTF-8 模式下为空。
    const char *text_ = "";  ///< 持有的 UTF-8 字符串，可能指向 storage_。
};

} // namespace bgt

#endif
