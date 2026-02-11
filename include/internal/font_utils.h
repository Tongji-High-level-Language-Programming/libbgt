#include <cassert>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <fontconfig/fontconfig.h>

namespace fs = std::filesystem;

// ==========================================
// 1. RAII Deleters (资源自动回收)
// ==========================================
struct FcPatternDeleter {
  void operator()(FcPattern *p) const {
    if (p)
      FcPatternDestroy(p);
  }
};

struct FcFontSetDeleter {
  void operator()(FcFontSet *fs) const {
    if (fs)
      FcFontSetDestroy(fs);
  }
};

struct FcConfigDeleter {
  void operator()(FcConfig *c) const {
    if (c)
      FcConfigDestroy(c);
  }
};

using PatternPtr = std::unique_ptr<FcPattern, FcPatternDeleter>;
using FontSetPtr = std::unique_ptr<FcFontSet, FcFontSetDeleter>;
using ConfigPtr = std::unique_ptr<FcConfig, FcConfigDeleter>;

// ==========================================
// 2. FontQuery (构建者模式：描述你想要的字体)
// ==========================================
class FontQuery {
public:
  FontQuery() : m_pattern(FcPatternCreate()) {}

  // 允许拷贝 (深拷贝 Pattern)
  FontQuery(const FontQuery &other) {
    m_pattern.reset(FcPatternDuplicate(other.m_pattern.get()));
  }

  // 允许移动
  FontQuery(FontQuery &&) = default;

  // 添加字体族 (支持链式调用) -> "Consolas", "Microsoft YaHei", "sans-serif"
  FontQuery &addFamily(const char *family) {
    FcPatternAddString(m_pattern.get(), FC_FAMILY,
                       reinterpret_cast<const FcChar8 *>(family));
    return *this;
  }

  // 设置粗细 (Weight)
  // 常见值: FC_WEIGHT_REGULAR(80), FC_WEIGHT_BOLD(200)
  FontQuery &setWeight(int weight) {
    FcPatternAddInteger(m_pattern.get(), FC_WEIGHT, weight);
    return *this;
  }

  // 设置斜体
  FontQuery &setSlant(int slant) {
    // FC_SLANT_ROMAN(0), FC_SLANT_ITALIC(100)
    FcPatternAddInteger(m_pattern.get(), FC_SLANT, slant);
    return *this;
  }

  // 设置语言环境，影响字体选择和替换
  // 如 "en", "zh-cn", "ja"
  FontQuery &setLang(const char *lang) {
    FcPatternAddString(m_pattern.get(), FC_LANG,
                       reinterpret_cast<const FcChar8 *>(lang));
    return *this;
  }

  // 获取原始指针(供 Manager 使用)
  FcPattern *get() const { return m_pattern.get(); }

private:
  PatternPtr m_pattern;
};

// ==========================================
// 3. FontManager (核心逻辑：匹配与排序)
// ==========================================
class FontManager {
public:
  FontManager();

  ~FontManager() = default;

  /**
   * @brief 核心方法：解析字体栈，返回按优先级排序的字体文件路径列表
   *
   * @param query 用户构建的查询对象
   * @return std::vector<fs::path> 排序好的字体文件路径
   */
  std::vector<fs::path> resolve(const FontQuery &query) const;

  /**
   * @brief 辅助功能：列出系统中安装的所有中文字体族名
   */
  std::vector<std::string> listChineseFamilies() const;

private:
  FontSetPtr internal_resolve(const FontQuery &query) const;

  ConfigPtr m_config;
};
