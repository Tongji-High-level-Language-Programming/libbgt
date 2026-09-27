#include "bgt_encoding.h"

// GBK 源编码目前只在 Windows 上实现（WinAPI）。在其他平台上启用 GBK 模式属于
// 配置错误：CMake 会先报错，这里再兜一道，避免静默产生乱码。
#if !defined(_WIN32) && BGT_SOURCE_ENCODING == BGT_ENCODING_GBK
#error "GBK source encoding is implemented on Windows only; an iconv backend is reserved in src/bgt_encoding.cpp"
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <cstdint>
#include <utility>

namespace bgt {
namespace {

// 源编码下单个字符占用的字节数。UTF-8 按首字节判断宽度；GBK 的首字节在
// 0x81-0xFE 之间表示双字节字符。非法字节按单字节处理，不会越界。
constexpr std::size_t encoded_char_bytes(unsigned char byte)
{
    if constexpr (kSourceEncoding == Encoding::Utf8) {
        if ((byte & 0xF8U) == 0xF0U) {
            return 4;
        }
        if ((byte & 0xF0U) == 0xE0U) {
            return 3;
        }
        if ((byte & 0xE0U) == 0xC0U) {
            return 2;
        }
        return 1;
    } else {
        // GBK 尾字节范围 0x40-0xFE，因此 ASCII（< 0x40）永远不会被当作尾字节，
        // 但尾字节可能是 [ ] # 等符号——这正是不能用 find() 直接搜索的原因。
        return (byte >= 0x81U && byte <= 0xFEU) ? 2 : 1;
    }
}

#if defined(_WIN32)

constexpr UINT code_page_of(Encoding encoding)
{
    // 936 = GBK（Windows 简体中文 ANSI 代码页）。
    return encoding == Encoding::Utf8 ? CP_UTF8 : 936U;
}

// 多字节 -> UTF-16。strict 时拒绝非法字节序列。失败时 out 保持不变。
bool widen(const std::string &text, Encoding from, bool strict,
           std::wstring &out)
{
    if (text.empty()) {
        out.clear();
        return true;
    }
    const UINT code_page = code_page_of(from);
    const int length = static_cast<int>(text.size());
    const DWORD flags =
        (strict && from == Encoding::Utf8) ? MB_ERR_INVALID_CHARS : 0;
    const int wide_length = MultiByteToWideChar(code_page, flags, text.data(),
                                                length, nullptr, 0);
    if (wide_length <= 0) {
        return false;
    }
    std::wstring buffer(static_cast<std::size_t>(wide_length), L'\0');
    const int written = MultiByteToWideChar(code_page, flags, text.data(),
                                            length, buffer.data(), wide_length);
    if (written <= 0) {
        return false;
    }
    buffer.resize(static_cast<std::size_t>(written));
    out = std::move(buffer);
    return true;
}

// UTF-16 -> 多字节。strict 时要求每个字符都能精确表示（不出现默认替换字符、
// 也不接受“最佳拟合”）。失败时 out 保持不变。
bool narrow(const std::wstring &text, Encoding to, bool strict,
            std::string &out)
{
    if (text.empty()) {
        out.clear();
        return true;
    }
    const UINT code_page = code_page_of(to);
    const int length = static_cast<int>(text.size());
    const bool check_default = strict && to != Encoding::Utf8;
    DWORD flags = 0;
    if (strict && to == Encoding::Utf8) {
        flags = WC_ERR_INVALID_CHARS;
    } else if (check_default) {
        // 关掉 Windows 的“最佳拟合”：否则 U+00B2 会被静默换成 '2'（used_default
        // 仍为 FALSE），严格转换就漏掉了这种悄悄改字的情况。
        flags = WC_NO_BEST_FIT_CHARS;
    }
    BOOL used_default = FALSE;
    BOOL *used_default_ptr = check_default ? &used_default : nullptr;

    const int byte_length = WideCharToMultiByte(code_page, flags, text.data(),
                                                length, nullptr, 0, nullptr,
                                                used_default_ptr);
    if (byte_length <= 0) {
        return false;
    }
    std::string buffer(static_cast<std::size_t>(byte_length), '\0');
    used_default = FALSE;
    const int written = WideCharToMultiByte(code_page, flags, text.data(),
                                            length, buffer.data(), byte_length,
                                            nullptr, used_default_ptr);
    if (written <= 0 || (check_default && used_default != FALSE)) {
        return false;
    }
    buffer.resize(static_cast<std::size_t>(written));
    out = std::move(buffer);
    return true;
}

#endif // defined(_WIN32)

bool convert_into(const std::string &text, Encoding from, Encoding to,
                  bool strict, std::string &out)
{
    if (from == to) {
        out = text;
        return true;
    }
#if defined(BGT_HAVE_GBK_CODEC)
    std::wstring wide;
    if (!widen(text, from, strict, wide)) {
        return false;
    }
    return narrow(wide, to, strict, out);
#else
    (void)strict;
    (void)out;
    return false;
#endif
}

} // namespace

std::string convert(const std::string &text, Encoding from, Encoding to)
{
    std::string out;
    if (!convert_into(text, from, to, false, out)) {
        // 没有后端或输入无法解析时保持原样：宁可显示为乱码，也不要丢数据。
        return text;
    }
    return out;
}

bool convert_strict(const std::string &text, Encoding from, Encoding to,
                    std::string &out)
{
    return convert_into(text, from, to, true, out);
}

std::string to_utf8(const std::string &text)
{
    if constexpr (kSourceEncoding == Encoding::Utf8) {
        return text;
    } else {
        return convert(text, Encoding::Gbk, Encoding::Utf8);
    }
}

std::string from_utf8(const char *text)
{
    if (text == nullptr) {
        return {};
    }
    if constexpr (kSourceEncoding == Encoding::Utf8) {
        return std::string(text);
    } else {
        return convert(text, Encoding::Utf8, Encoding::Gbk);
    }
}

std::string from_utf8(const std::string &text)
{
    if constexpr (kSourceEncoding == Encoding::Utf8) {
        return text;
    } else {
        return convert(text, Encoding::Utf8, Encoding::Gbk);
    }
}

std::filesystem::path to_native_path(const std::string &text)
{
#if defined(_WIN32)
    std::wstring wide;
    if (!widen(text, kSourceEncoding, false, wide)) {
        return std::filesystem::path(text);
    }
    return std::filesystem::path(wide);
#else
    // 非 Windows 的文件系统接口直接用本机编码（UTF-8）解释窄路径。
    return std::filesystem::path(text);
#endif
}

std::size_t encoded_prefix_length(const std::string &text, std::size_t limit)
{
    std::size_t length = 0;
    while (length < text.size() && length < limit) {
        const std::size_t char_bytes =
            encoded_char_bytes(static_cast<unsigned char>(text[length]));
        if (length + char_bytes > limit || length + char_bytes > text.size()) {
            break;
        }
        length += char_bytes;
    }
    return length;
}

bool encoded_contains_ascii(const std::string &text, char ascii)
{
    const auto target = static_cast<unsigned char>(ascii);
    std::size_t index = 0;
    while (index < text.size()) {
        const auto byte = static_cast<unsigned char>(text[index]);
        const std::size_t char_bytes = encoded_char_bytes(byte);
        if (char_bytes == 1 && byte == target) {
            return true;
        }
        index += char_bytes;
    }
    return false;
}

bool encoded_ends_with_ascii(const std::string &text, char ascii)
{
    std::size_t last_start = 0;
    bool has_last = false;
    std::size_t index = 0;
    while (index < text.size()) {
        const std::size_t char_bytes =
            encoded_char_bytes(static_cast<unsigned char>(text[index]));
        if (index + char_bytes > text.size()) {
            // 末尾是残缺的多字节字符：它不是任何一个完整字符。
            return false;
        }
        last_start = index;
        has_last = true;
        index += char_bytes;
    }
    return has_last && text[last_start] == ascii &&
           encoded_char_bytes(static_cast<unsigned char>(text[last_start])) == 1;
}

} // namespace bgt
