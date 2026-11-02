#include "resource_paths.h"

#include <windows.h>

#include <filesystem>

namespace fs = std::filesystem;

std::optional<std::wstring> ArktubeFindResource(const std::wstring &relative) {
    wchar_t exePath[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        // GetModuleFileNameW truncates silently rather than erroring on
        // overflow (pre-Windows 10 1607 behavior; kept as the safe
        // check regardless of the deployment target) -- treat a path
        // that fills the whole buffer as a failure rather than risking
        // a mangled directory.
        return std::nullopt;
    }

    fs::path exeDir = fs::path(exePath).parent_path();
    fs::path candidate = exeDir / L"resources" / relative;

    std::error_code ec;
    if (fs::exists(candidate, ec) && !ec) {
        return candidate.wstring();
    }

    return std::nullopt;
}
