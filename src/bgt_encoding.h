#ifndef BGT_ENCODING_H_
#define BGT_ENCODING_H_

// libbgt 内部编码转换层：不依赖 SDL，也不对外暴露。
//
// 公开 API 的窄字符串一律使用“源编码”，由构建选项 BGT_SOURCE_ENCODING 决定：
//
//   UTF-8（默认）：源编码就是 UTF-8。所有转换在编译期被 if constexpr 消除，
//                  不产生任何额外开销，行为与未引入本层时完全一致。
//   GBK          ：源编码是 GBK（Windows 代码页 936）。交给 SDL3、SDL3_ttf、
//                  SDL3_mixer 的字符串在边界处转成 UTF-8；从 SDL 取回的字符串
//                  （SDL_GetError）转回 GBK。库内部一切字符串保持源编码，因此
//                  存档文件、错误历史、printf 输出与学生的源码编码一致。
//
// 设计说明与接入方式见 docs/encoding.md。

#include "bgt.h"

#include <cstddef>
#include <filesystem>
#include <string>

// Windows 使用 WinAPI 完成 GBK <-> UTF-8 转换。其他平台预留 iconv 后端：
// 实现后在此定义 BGT_HAVE_GBK_CODEC 即可，调用方无需改动。
#if defined(_WIN32)
#define BGT_HAVE_GBK_CODEC 1
#endif

namespace bgt {

// 支持的编码。GBK 对应 Windows 代码页 936。
enum class Encoding {
    Utf8,
    Gbk,
};

// 本次构建中公开 API 使用的源编码。
inline constexpr Encoding kSourceEncoding =
    BGT_SOURCE_ENCODING == BGT_ENCODING_GBK ? Encoding::Gbk : Encoding::Utf8;

// 是否具备 GBK <-> UTF-8 编解码后端。
#ifdef BGT_HAVE_GBK_CODEC
inline constexpr bool kHasGbkCodec = true;
#else
inline constexpr bool kHasGbkCodec = false;
#endif

// 编码转换。目标编码无法表示的字符会退化成替换字符（宽松，不抛异常）。
std::string convert(const std::string &text, Encoding from, Encoding to);

// 严格转换：出现无法表示的字符、或输入不是合法的 from 编码时返回 false，
// 此时 out 保持不变。构建期工具用它保证转码不丢信息。
bool convert_strict(const std::string &text, Encoding from, Encoding to,
                    std::string &out);

// 源编码 -> UTF-8。需要把结果留下来时用这个；只是临时传给 SDL 时用 Utf8View，
// 后者在 UTF-8 模式下零拷贝。
std::string to_utf8(const std::string &text);

// UTF-8 -> 源编码（例如 SDL_GetError() 的返回值）。
std::string from_utf8(const char *text);
std::string from_utf8(const std::string &text);

// 源编码路径 -> 本机文件系统路径。Windows 下窄路径按 ANSI 解释，因此先转成
// UTF-16，保证中文文件名在两种模式下都能正确打开。
std::filesystem::path to_native_path(const std::string &text);

// 按源编码的字符边界求前缀长度：截断时不会把一个多字节字符切成两半。
std::size_t encoded_prefix_length(const std::string &text, std::size_t limit);

// 源编码下，某个 ASCII 字符是否“作为一个完整字符”出现。GBK 的尾字节可以是
// [ ] # 等 ASCII 字节，直接 find() 会误判，所以校验节名/键名/行尾用它。
bool encoded_contains_ascii(const std::string &text, char ascii);

// 同上：text 是否以某个 ASCII 字符作为最后一个完整字符结束。
bool encoded_ends_with_ascii(const std::string &text, char ascii);

// 源编码字符串的 UTF-8 视图：UTF-8 模式下零拷贝，GBK 模式下持有转换结果。
// 生命周期与临时对象一样短，只能立即使用（不可拷贝、不可保存 c_str()）。
class Utf8View {
public:
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

    Utf8View(const Utf8View &) = delete;
    Utf8View &operator=(const Utf8View &) = delete;
    Utf8View(Utf8View &&) = delete;
    Utf8View &operator=(Utf8View &&) = delete;
    ~Utf8View() = default;

    [[nodiscard]] const char *c_str() const
    {
        return text_;
    }

private:
    std::string storage_;
    const char *text_ = "";
};

} // namespace bgt

#endif
