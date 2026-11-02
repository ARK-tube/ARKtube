#ifndef ARKTUBE_UI_WEBVIEW_H
#define ARKTUBE_UI_WEBVIEW_H

#include <WebView2.h>
#include <wrl.h>

#include <functional>
#include <string>
#include <windows.h>

/* Windows counterpart of ../../../arktube_linux/src/ui/webview.h.
   WebView2's environment/controller setup is inherently asynchronous
   (CreateCoreWebView2EnvironmentWithOptions() and
   CreateCoreWebView2Controller() both complete via a callback, not
   inline, unlike webkit_web_view_new_with_user_content_manager() on
   Linux, which is a plain synchronous constructor) -- so this is a
   small class wrapping that callback chain rather than a single
   "create and get a widget back" function. onReady is invoked once the
   controller and its CoreWebView2 are live and the injected script and
   settings below are already applied, which is when main.cpp should
   resize it into place and the connectivity thread's first successful
   check should call Navigate(). */
class ArktubeWebView {
public:
    /* Begins the async environment/controller creation. parentWindow
       is the HWND the WebView2 controller is bound to (this app's main
       window); userDataFolder is where WebView2 keeps its own
       per-install profile data (cookies, cache -- WebView2 requires a
       writable one, unlike WebKitGTK which manages this internally).
       onReady is called on the UI thread once the WebView2 is fully
       initialized, with the injected script (resources/js/user-script.js,
       looked up via ArktubeFindResource()) and the TV/Leanback user
       agent (see config.h's ARKTUBE_USER_AGENT) already applied. */
    void Initialize(
        HWND parentWindow, const std::wstring &userDataFolder,
        std::function<void()> onReady);

    /* Points the WebView2 at ARKTUBE_URL. No-ops before Initialize()'s
       onReady has fired. */
    void Navigate(const std::wstring &url);

    /* Resizes the WebView2's visual bounds to fill newBounds -- called
       from WM_SIZE in main.cpp, the Win32 equivalent of a GtkWidget
       automatically filling its GtkOverlay slot on Linux; WebView2's
       controller has no such automatic layout and must be told
       explicitly on every resize. */
    void Resize(const RECT &newBounds);

    /* Registers handler to run ahead of the page's own keyboard
       handling, via ICoreWebView2Controller4::add_AcceleratorKeyPressed
       -- WebView2's keyboard input normally never reaches this app's
       WndProc at all (unlike WebKitGTK, where "key-press-event" on the
       GtkWindow itself already runs before the webview sees the key;
       see ../../../arktube_linux/src/main.c's
       g_signal_connect(window, "key-press-event", ...)), so this is
       what lets ArktubeHandleWindowKeyDown() (ui/input.h) intercept
       F11/Escape the same way on Windows. handler returning true marks
       the key handled (suppressing it from reaching youtube.com/tv),
       matching ArktubeHandleWindowKeyDown()'s own TRUE/FALSE
       "handled here or not" contract. No-ops before IsReady(). */
    void SetAcceleratorKeyHandler(std::function<bool(WPARAM)> handler);

    /* True once Initialize()'s onReady has fired. */
    bool IsReady() const { return controller_ != nullptr; }

    Microsoft::WRL::ComPtr<ICoreWebView2> Core() const { return webview_; }

private:
    Microsoft::WRL::ComPtr<ICoreWebView2Environment> environment_;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> controller_;
    Microsoft::WRL::ComPtr<ICoreWebView2> webview_;
    std::function<bool(WPARAM)> acceleratorKeyHandler_;
};

#endif // ARKTUBE_UI_WEBVIEW_H
