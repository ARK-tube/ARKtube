/*
    ARKtube Windows -- native port entry point.

    Windows counterpart of ../../arktube_linux/src/main.c: one process,
    one Win32 window, one WebView2 controller pointed at the Microsoft
    Edge/Chromium runtime already installed (or auto-installed) on
    virtually every Windows 10/11 machine -- the WebView2 Runtime is
    the same engine Edge itself uses, the role WebKitGTK/GNOME Web
    plays in that comment on Linux. No bundled Chromium, no second
    process to spawn or reparent.

    Scope of this first port (mirrors the Linux port's own first-cut
    scope, see that file's header comment): window creation, loading
    youtube.com/tv, F11 fullscreen + Escape-to-unfullscreen, and the
    byte-identical injected resources/js/user-script.js. Tray icon and
    Immersive Mode are not ported here either, for the same reason they
    aren't on Linux yet -- tracked as follow-up work on both platforms.

    Connectivity is monitored for the app's whole lifetime via
    net/connectivity.cpp's background thread, posting
    WM_ARKTUBE_CONNECTIVITY_CHANGED back to this window -- the Win32
    analogue of g_idle_add() marshaling the Linux connectivity thread's
    findings onto the GTK main loop.

    Persisted settings (currently just "was the window fullscreen last
    run") live in the registry via platform/settings.cpp, the Windows
    counterpart of the GKeyFile platform/settings.c writes on Linux.

    This file is deliberately just orchestration, the same design goal
    ../../arktube_linux/src/main.c states for itself: it wires Win32 up
    and calls into platform/, net/ and ui/, each owning one piece of the
    app exactly as on Linux.
*/

#include <windows.h>

#include <ShellScalingApi.h>
#include <shlobj.h>

#include <memory>

#include "config.h"
#include "net/connectivity.h"
#include "platform/resource_paths.h"
#include "platform/settings.h"
#include "ui/input.h"
#include "ui/overlay.h"
#include "ui/webview.h"

#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "Shell32.lib")

namespace {

struct AppState {
    HWND window = nullptr;
    ArktubeWebView webview;
    std::unique_ptr<ArktubeOverlay> splashOverlay;
    std::unique_ptr<ArktubeOverlay> noInternetOverlay;
    ArktubeConnectivityState connectivityState;
    WINDOWPLACEMENT prevPlacement{sizeof(WINDOWPLACEMENT)};
    bool webviewReady = false;
};

AppState *StateFor(HWND window) {
    return reinterpret_cast<AppState *>(GetWindowLongPtrW(window, GWLP_USERDATA));
}

RECT ClientRectOf(HWND window) {
    RECT rect;
    GetClientRect(window, &rect);
    return rect;
}

/* Called once WebView2 is fully initialized (ArktubeWebView::Initialize's
   onReady) -- wires up the F11/Escape interception that otherwise can't
   reach this app's WndProc (see webview.h's SetAcceleratorKeyHandler
   comment) and sizes the controller to the window's current client area,
   since nothing has resized it yet. Does NOT navigate: that only
   happens once the connectivity thread reports online, exactly
   mirroring arktube_on_internet_ready() gating webkit_web_view_load_uri()
   on Linux. */
void OnWebViewReady(AppState *state) {
    state->webviewReady = true;
    state->webview.Resize(ClientRectOf(state->window));
    state->webview.SetAcceleratorKeyHandler([state](WPARAM virtualKey) -> bool {
        return ArktubeHandleWindowKeyDown(state->window, virtualKey) != FALSE;
    });
}

/* The one place that actually shows or hides the no-internet overlay,
   driven purely off connectivityState.isOnline -- same idempotent
   shape as arktube_sync_no_internet_overlay() on Linux: only acts when
   the overlay's current on/off state disagrees with the flag. */
void SyncNoInternetOverlay(AppState *state) {
    if (state->connectivityState.isOnline) {
        if (state->noInternetOverlay) {
            state->noInternetOverlay.reset();
        }
        return;
    }

    if (!state->noInternetOverlay) {
        state->noInternetOverlay =
            ArktubeOverlay::Create(state->window, ArktubeOverlayKind::NoInternet);
        if (state->noInternetOverlay) {
            state->noInternetOverlay->Resize(ClientRectOf(state->window));
            state->noInternetOverlay->Show(nullptr, nullptr);
        }
    }
}

void DismissSplash(void *context) {
    auto *state = static_cast<AppState *>(context);
    state->splashOverlay.reset();
}

/* Handles WM_ARKTUBE_CONNECTIVITY_CHANGED, posted from
   net/connectivity.cpp's background thread. wParam is TRUE for "online
   now", FALSE for "offline now" -- the Win32 analogue of
   arktube_on_internet_ready()/arktube_on_internet_unavailable() on
   Linux, folded into one handler here since both are now simple enough
   to share it. */
void OnConnectivityChanged(AppState *state, bool online) {
    state->connectivityState.isOnline = online;
    SyncNoInternetOverlay(state);

    if (online && !state->connectivityState.webviewStarted && state->webviewReady) {
        state->connectivityState.webviewStarted = true;
        state->webview.Navigate(ARKTUBE_URL);

        state->splashOverlay = ArktubeOverlay::Create(state->window, ArktubeOverlayKind::Splash);
        if (state->splashOverlay) {
            state->splashOverlay->Resize(ClientRectOf(state->window));
            state->splashOverlay->Show(DismissSplash, state);
        }
    }
}

LRESULT CALLBACK ArktubeWindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState *state = StateFor(window);

    switch (msg) {
        case WM_SIZE: {
            if (state) {
                RECT bounds = ClientRectOf(window);
                state->webview.Resize(bounds);
                if (state->splashOverlay) {
                    state->splashOverlay->Resize(bounds);
                }
                if (state->noInternetOverlay) {
                    state->noInternetOverlay->Resize(bounds);
                }
            }
            return 0;
        }

        case WM_KEYDOWN:
            // Reached only while a splash/no-internet overlay (or
            // nothing) has focus -- once WebView2 owns input focus,
            // F11/Escape are intercepted via
            // SetAcceleratorKeyHandler() in OnWebViewReady() instead,
            // which runs ahead of the page. Handling it here too means
            // the shortcuts still work before the WebView2 controller
            // has been created, or while an overlay is covering it.
            if (ArktubeHandleWindowKeyDown(window, wParam)) {
                return 0;
            }
            break;

        case WM_ARKTUBE_CONNECTIVITY_CHANGED:
            if (state) {
                OnConnectivityChanged(state, wParam != 0);
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }
    return DefWindowProcW(window, msg, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    (void)showCommand; // windows always launches maximized/fullscreen (see below), not via this hint

    // Must happen before any window is created: without it, Windows'
    // display-scaling compatibility shim stretches this app's own
    // pixels to fill a "virtual" 96-DPI surface, which is exactly the
    // kind of pre-WebView2 blurriness this call exists to opt out of
    // -- the Windows equivalent-in-spirit of why g_set_prgname() on
    // Linux (see ../../arktube_linux/src/main.c) has to run before
    // gtk_init(): both are one-time process-wide identity/behavior
    // settings that later API calls silently assume are already in
    // place.
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    // The taskbar/Start-menu identity match config.h's
    // ARKTUBE_APP_USER_MODEL_ID comment describes -- set once, for the
    // whole process, before the window (or its taskbar button) exists.
    SetCurrentProcessExplicitAppUserModelID(ARKTUBE_APP_USER_MODEL_ID);

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = ArktubeWindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = ARKTUBE_WINDOW_CLASS;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

    auto iconPath = ArktubeFindResource(L"icons\\appIcon.png");
    if (iconPath) {
        // LoadImageW loads a PNG as an icon directly on Windows Vista+
        // via the shell's icon codec, the same "just point it at the
        // PNG" simplicity gtk_window_set_icon_from_file() gives on
        // Linux (see ../../arktube_linux/src/main.c) -- no separate
        // .ico conversion step needed for the window/taskbar icon.
        HICON icon = static_cast<HICON>(LoadImageW(
            nullptr, iconPath->c_str(), IMAGE_ICON, 256, 256, LR_LOADFROMFILE));
        if (icon) {
            windowClass.hIcon = icon;
        } else {
            OutputDebugStringW(L"ARKtube: could not load icons\\appIcon.png as a window icon.\n");
        }
    } else {
        OutputDebugStringW(
            L"ARKtube: icons\\appIcon.png not found next to the executable; "
            L"running without a window icon.\n");
    }
    RegisterClassW(&windowClass);

    auto *state = new AppState();

    // Boots maximized by default, or fullscreen if that was the state
    // when the app last quit -- same reasoning and same persisted
    // source of truth (ArktubeLoadFullscreenPref(), the registry
    // counterpart of arktube_load_fullscreen_pref()'s GKeyFile read)
    // as gtk_window_maximize()/gtk_window_fullscreen() in
    // ../../arktube_linux/src/main.c.
    bool startFullscreen = ArktubeLoadFullscreenPref();

    state->window = CreateWindowExW(
        0, ARKTUBE_WINDOW_CLASS, ARKTUBE_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, ARKTUBE_DEFAULT_WIDTH, ARKTUBE_DEFAULT_HEIGHT,
        nullptr, nullptr, instance, nullptr);
    SetWindowLongPtrW(state->window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

    ShowWindow(state->window, startFullscreen ? SW_SHOW : SW_MAXIMIZE);
    if (startFullscreen) {
        ArktubeSetFullscreen(state->window, TRUE, &state->prevPlacement);
    }
    UpdateWindow(state->window);

    // WebView2's per-install profile data directory -- see
    // webview.h's Initialize() comment for why this is required here
    // and has no Linux-side equivalent step.
    wchar_t appDataPath[MAX_PATH];
    std::wstring userDataFolder;
    if (SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appDataPath) == S_OK) {
        userDataFolder = std::wstring(appDataPath) + L"\\ARKtube\\WebView2";
    }

    // The WebView2 environment/controller are created asynchronously
    // (see webview.h); OnWebViewReady() runs once that chain completes,
    // but the connectivity thread is started immediately, exactly as
    // on Linux, where it's started before gtk_widget_show_all() too --
    // on either platform the first successful check is what actually
    // triggers navigation (OnConnectivityChanged() above /
    // arktube_on_internet_ready() there), so starting it early just
    // means the first check can complete sooner, not that it does
    // anything before the webview is ready (OnConnectivityChanged()
    // checks state->webviewReady before navigating).
    state->connectivityState.mainWindow = state->window;
    state->webview.Initialize(
        state->window, userDataFolder, [state]() { OnWebViewReady(state); });

    CreateThread(
        nullptr, 0, ArktubeConnectivityThreadProc, &state->connectivityState, 0, nullptr);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    delete state;
    return static_cast<int>(msg.wParam);
}
