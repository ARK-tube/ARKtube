#include "input.h"

#include "../platform/settings.h"

namespace {

// Classic Win32 "toggle fullscreen" idiom (as documented by Raymond
// Chen's "window mode" blog post and used by most native Win32 apps
// that don't use a dedicated fullscreen API): drop WS_OVERLAPPEDWINDOW
// and size the window to its monitor's full rect, remembering the
// prior WINDOWPLACEMENT to restore exactly. The "is it already
// fullscreen" check below (no WS_OVERLAPPEDWINDOW bits set) is the
// Win32 equivalent of GDK_WINDOW_STATE_FULLSCREEN in
// arktube_window_is_fullscreen() on Linux.
bool IsWindowFullscreen(HWND window) {
    LONG_PTR style = GetWindowLongPtrW(window, GWL_STYLE);
    return (style & WS_OVERLAPPEDWINDOW) == 0;
}

} // namespace

void ArktubeSetFullscreen(HWND window, BOOL fullscreen, WINDOWPLACEMENT *prevPlacement) {
    LONG_PTR style = GetWindowLongPtrW(window, GWL_STYLE);

    if (fullscreen) {
        MONITORINFO monitorInfo{sizeof(MONITORINFO)};
        if (GetWindowPlacement(window, prevPlacement) &&
            GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitorInfo)) {
            SetWindowLongPtrW(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(
                window, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLongPtrW(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, prevPlacement);
        SetWindowPos(
            window, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }

    ArktubeSaveFullscreenPref(fullscreen != FALSE);
}

BOOL ArktubeHandleWindowKeyDown(HWND window, WPARAM virtualKey) {
    static WINDOWPLACEMENT prevPlacement{sizeof(WINDOWPLACEMENT)};

    switch (virtualKey) {
        case VK_F11: {
            bool nowFullscreen = IsWindowFullscreen(window);
            ArktubeSetFullscreen(window, nowFullscreen ? FALSE : TRUE, &prevPlacement);
            return TRUE;
        }

        case VK_ESCAPE:
            if (IsWindowFullscreen(window)) {
                ArktubeSetFullscreen(window, FALSE, &prevPlacement);
                return TRUE;
            }
            return FALSE;

        default:
            return FALSE;
    }
}
