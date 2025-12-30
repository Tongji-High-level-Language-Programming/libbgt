#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <internal/font_utils.h>


auto GetInstalledFontFile(std::string_view face_name)
-> std::optional<std::filesystem::path> {

    LPCSTR font_registry_path =
        R"(Software\Microsoft\Windows NT\CurrentVersion\Fonts)";

    auto get_font_file =
        [&](HKEY base_path_hkey) -> std::optional<std::filesystem::path> {
        HKEY hKey;
        LONG result;
        // Open Windows font registry key
        result = RegOpenKeyExA(base_path_hkey, font_registry_path, 0, KEY_READ, &hKey);
        if (result != ERROR_SUCCESS) {
            return std::nullopt;
        }

        DWORD max_value_name_size;
        DWORD max_value_data_size;
        result = RegQueryInfoKeyA(hKey, 0, 0, 0, 0, 0, 0, 0, &max_value_name_size,
            &max_value_data_size, 0, 0);
        if (result != ERROR_SUCCESS) {
            return std::nullopt;
        }

        DWORD value_index = 0;
        LPSTR value_name = new CHAR[max_value_name_size];
        LPBYTE value_data = new BYTE[max_value_data_size];
        DWORD value_name_size;
        DWORD value_data_size;
        DWORD value_type;
        std::filesystem::path font_file;

        // Look for a matching font name
        do {
            value_data_size = max_value_data_size;
            value_name_size = max_value_name_size;

            result = RegEnumValueA(hKey, value_index, value_name, &value_name_size, 0,
                &value_type, value_data, &value_data_size);

            value_index++;

            if (result != ERROR_SUCCESS || value_type != REG_SZ) {
                continue;
            }

            std::string sValueName(value_name, value_name_size);
            // Found a match
            if (_strnicmp(face_name.data(), sValueName.c_str(), face_name.length()) ==
                0) {
                font_file.assign((LPSTR)value_data, (LPSTR)value_data + value_data_size);
                break;
            }
        } while (result != ERROR_NO_MORE_ITEMS);

        delete[] value_name;
        delete[] value_data;

        RegCloseKey(hKey);

        if (font_file.empty()) {
            return std::nullopt;
        }

        if (font_file.is_absolute()) {
            return font_file;
		}

        // Build full font file path
        CHAR win_dir[MAX_PATH];
        (void)GetWindowsDirectoryA(win_dir, MAX_PATH);

        return std::filesystem::path{ win_dir } / "Fonts" / font_file;
        };

    return get_font_file(HKEY_CURRENT_USER).or_else([&]() {
        return get_font_file(HKEY_LOCAL_MACHINE);
        });
}
