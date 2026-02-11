#include <algorithm>
#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <print>

#include <SDL3/SDL_assert.h>
#include <fontconfig/fontconfig.h>
#include <internal/font_utils.h>

#ifdef _WIN32
struct StderrMaskGuard {
  // Windows 下，FontConfig 由于系统内通常没有配置文件会报错 "Cannot load
  // default config file"，但实际上并不影响使用，
  // 作为 workaround，我们需要暂时重定向 stderr 到 NUL 以屏蔽这个错误输出
  StderrMaskGuard() { freopen("NUL", "w", stderr); }
  ~StderrMaskGuard() { freopen("CON", "w", stderr); }
};
#endif

FontManager::FontManager() {
#ifdef _WIN32
  // 在 Windows 上屏蔽 FontConfig 的错误输出
  StderrMaskGuard _{};
#endif
  m_config.reset(FcInitLoadConfigAndFonts());
}

auto FontManager::internal_resolve(const FontQuery &query) const -> FontSetPtr {

  // 1. 复制 Pattern，因为后续的替换操作会修改它
  PatternPtr pat(FcPatternDuplicate(query.get()));

  // 2. 配置替换 (System Config Substitution)
  // 处理 Alias (如 Helvetica -> sans-serif -> Noto Sans)
  FcConfigSubstitute(m_config.get(), pat.get(), FcMatchPattern);

  // 3. 默认值替换 (Default Substitution)
  // 填充未指定的属性 (如 size, weight 默认值)
  {
#ifdef _WIN32
    // 在 Windows 上屏蔽 FontConfig 的错误输出
    StderrMaskGuard _{};
#endif
    FcDefaultSubstitute(pat.get());
  }

  // 4. 执行排序 (Sort)
  // 返回一个 FontSet，里面包含了系统找到的最佳 fallback 链
  FcResult result;
  // trim 参数设为 FcTrue，表示移除那些没有包含"新字符"的字体，优化列表长度
  FontSetPtr fontSet(
      FcFontSort(m_config.get(), pat.get(), FcTrue, nullptr, &result));
  return fontSet;
}

auto FontManager::resolve(const FontQuery &query) const
    -> std::vector<fs::path> {
  auto fontSet = internal_resolve(query);
  if (!fontSet)
    return {};

  // 5. 遍历结果并提取文件路径
  std::vector<fs::path> resultPaths;

  for (int i = 0; i < fontSet->nfont; ++i) {
    FcPattern *font = fontSet->fonts[i];
    FcChar8 *file = nullptr;

    // 从 Pattern 中提取文件名
    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
      // 转换为 fs::path
      fs::path p{reinterpret_cast<char *>(file)};
      using namespace std::string_literals;
      SDL_assert(fs::exists(p) &&
                 ("Font file does not exist: "s + p.string()).c_str());

      resultPaths.push_back(std::move(p));
    }
  }

  return resultPaths;
}

auto FontManager::listChineseFamilies() const -> std::vector<std::string> {
  std::vector<std::string> families;

  // 创建一个模式：查找所有支持 zh-cn 的字体
  PatternPtr pat(FcPatternCreate());
  FcPatternAddString(pat.get(), FC_LANG, (const FcChar8 *)"zh-cn");

  FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, FC_FAMILYLANG, nullptr);

  FontSetPtr fs(FcFontList(m_config.get(), pat.get(), os));

  FcObjectSetDestroy(os);

  if (fs) {
    for (int i = 0; i < fs->nfont; ++i) {
      std::vector<std::string> family_names, langs;
      FcChar8 *family;
      FcChar8 *family_lang;

      std::string family_str;
      bool found_zh_cn = false;
      
      for (int j = 0; FcPatternGetString(fs->fonts[i], FC_FAMILY, j, &family) ==
                      FcResultMatch;
           ++j) {
        FcPatternGetString(fs->fonts[i], FC_FAMILYLANG, j, &family_lang);
        family_names.emplace_back(reinterpret_cast<const char*>(family));
        langs.emplace_back(reinterpret_cast<const char*>(family_lang));
        
        // 优先选择支持中文的字体族名，如果没有任何一个字体族支持中文，则选择第一个字体族名
        if (FcPatternGetString(fs->fonts[i], FC_FAMILYLANG, j, &family_lang) ==
                FcResultMatch &&
            strcmp(reinterpret_cast<char *>(family_lang), "zh-cn") == 0) {
          family_str = reinterpret_cast<char *>(family);
          found_zh_cn = true;
          break;
        } else if (family_str.empty()) {
          family_str = reinterpret_cast<char *>(family);
        }
        std::print("{} {} {}\n",family_str, family_names, langs);
      }
      families.emplace_back(std::move(family_str));
      
    }
  }
  std::ranges::sort(families);
  families.erase(std::ranges::unique(families).begin(), families.end());
  return families;
}
