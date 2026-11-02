#include "settings.h"

#include <windows.h>

#include "../config.h"

bool ArktubeLoadFullscreenPref() {
    HKEY key;
    LSTATUS openResult =
        RegOpenKeyExW(HKEY_CURRENT_USER, ARKTUBE_REGISTRY_KEY, 0, KEY_READ, &key);
    if (openResult != ERROR_SUCCESS) {
        return false;
    }

    DWORD value = 0;
    DWORD size = sizeof(value);
    DWORD type = 0;
    LSTATUS queryResult = RegQueryValueExW(
        key, ARKTUBE_REGISTRY_VALUE_FULLSCREEN, nullptr, &type,
        reinterpret_cast<LPBYTE>(&value), &size);
    RegCloseKey(key);

    if (queryResult != ERROR_SUCCESS || type != REG_DWORD) {
        return false;
    }
    return value != 0;
}

void ArktubeSaveFullscreenPref(bool fullscreen) {
    HKEY key;
    DWORD disposition = 0;
    LSTATUS createResult = RegCreateKeyExW(
        HKEY_CURRENT_USER, ARKTUBE_REGISTRY_KEY, 0, nullptr, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, nullptr, &key, &disposition);
    if (createResult != ERROR_SUCCESS) {
        // A settings write failing (e.g. a locked-down corporate
        // profile) shouldn't crash the app -- the next launch just
        // falls back to ArktubeLoadFullscreenPref()'s default, same as
        // a missing key.
        return;
    }

    DWORD value = fullscreen ? 1 : 0;
    RegSetValueExW(
        key, ARKTUBE_REGISTRY_VALUE_FULLSCREEN, 0, REG_DWORD,
        reinterpret_cast<const BYTE *>(&value), sizeof(value));
    RegCloseKey(key);
}
