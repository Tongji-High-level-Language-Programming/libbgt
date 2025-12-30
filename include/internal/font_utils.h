#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

/*
* @brief 从 Windows 注册表中获取已安装字体的文件路径
*
* @param font_name 字体名称，如 "SimSun"
*
* @return 字体文件的完整路径，未找到则返回 std::nullopt
*/
auto GetInstalledFontFile(std::string_view font_name)
-> std::optional<std::filesystem::path>;
