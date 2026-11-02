#include "webview.h"

#include <fstream>
#include <sstream>

#include "../config.h"
#include "../platform/resource_paths.h"

using namespace Microsoft::WRL;

namespace {

/* Reads resources/js/user-script.js whole, the same file (byte-for-byte)
   as ../../../arktube_linux/resources/js/user-script.js -- see that
   file's header comment for why both platforms ship an identical copy.
   Returns an empty string (and lets the caller warn) if it can't be
   found or read, matching arktube_webview_inject_user_script()'s
   fail-soft behavior on Linux: a missing script means degraded input
   handling, not a crash. */
std::wstring ReadUserScriptOrEmpty() {
    auto path = ArktubeFindResource(L"js\\user-script.js");
    if (!path) {
        OutputDebugStringW(
            L"ARKtube: js\\user-script.js not found; Home-key navigation, "
            L"cursor auto-hide, and gamepad/remote input will be unavailable "
            L"this run.\n");
        return L"";
    }

    std::wifstream file(*path, std::ios::binary);
    if (!file) {
        return L"";
    }
    std::wstringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace

void ArktubeWebView::Initialize(
    HWND parentWindow, const std::wstring &userDataFolder,
    std::function<void()> onReady) {
    // userDataFolder must exist and be writable before
    // CreateCoreWebView2EnvironmentWithOptions() is called -- unlike
    // WebKitGTK, which manages its own profile directory internally
    // with no equivalent setup step on the Linux side.
    CreateDirectoryW(userDataFolder.c_str(), nullptr);

    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, userDataFolder.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this, parentWindow, onReady](HRESULT result, ICoreWebView2Environment *env) -> HRESULT {
                if (FAILED(result) || !env) {
                    OutputDebugStringW(L"ARKtube: WebView2 environment creation failed.\n");
                    return result;
                }
                environment_ = env;

                environment_->CreateCoreWebView2Controller(
                    parentWindow,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this, onReady](HRESULT innerResult, ICoreWebView2Controller *controller) -> HRESULT {
                            if (FAILED(innerResult) || !controller) {
                                OutputDebugStringW(
                                    L"ARKtube: WebView2 controller creation failed.\n");
                                return innerResult;
                            }
                            controller_ = controller;
                            controller_->get_CoreWebView2(&webview_);

                            ComPtr<ICoreWebView2Settings> settings;
                            webview_->get_Settings(&settings);
                            // Full replacement, not an append -- the
                            // same requirement
                            // webkit_settings_set_user_agent() satisfies
                            // on Linux (see config.h's ARKTUBE_USER_AGENT
                            // comment). put_UserAgent() requires the
                            // ICoreWebView2Settings2 interface, not the
                            // base ICoreWebView2Settings.
                            ComPtr<ICoreWebView2Settings2> settings2;
                            if (SUCCEEDED(settings.As(&settings2))) {
                                settings2->put_UserAgent(ARKTUBE_USER_AGENT);
                            }
                            settings->put_AreDevToolsEnabled(TRUE);
                            settings->put_IsStatusBarEnabled(FALSE);

                            // Injected before any navigation happens,
                            // so this is in place for the very first
                            // (and every subsequent) document, the
                            // AddScriptToExecuteOnDocumentCreatedAsync()
                            // equivalent of WebKitGTK's
                            // WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START
                            // -- see resources/js/user-script.js's
                            // header comment.
                            std::wstring script = ReadUserScriptOrEmpty();
                            if (!script.empty()) {
                                webview_->AddScriptToExecuteOnDocumentCreated(
                                    script.c_str(),
                                    Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                                        [](HRESULT, LPCWSTR) -> HRESULT { return S_OK; })
                                        .Get());
                            }

                            if (onReady) {
                                onReady();
                            }
                            return S_OK;
                        })
                        .Get());
                return S_OK;
            })
            .Get());
}

void ArktubeWebView::SetAcceleratorKeyHandler(std::function<bool(WPARAM)> handler) {
    acceleratorKeyHandler_ = std::move(handler);
    if (!controller_) {
        return;
    }

    ComPtr<ICoreWebView2Controller4> controller4;
    if (FAILED(controller_.As(&controller4))) {
        // Older WebView2 Runtime without Controller4 -- F11/Escape
        // simply won't be intercepted ahead of the page on that
        // runtime; everything else in this app still works, the same
        // fail-soft shape as a missing user-script.js degrading input
        // handling rather than crashing.
        return;
    }

    EventRegistrationToken token;
    controller4->add_AcceleratorKeyPressed(
        Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
            [this](ICoreWebView2Controller *, ICoreWebView2AcceleratorKeyPressedEventArgs *args) -> HRESULT {
                COREWEBVIEW2_KEY_EVENT_KIND kind;
                args->get_KeyEventKind(&kind);
                if (kind != COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN &&
                    kind != COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN) {
                    return S_OK;
                }

                UINT virtualKey = 0;
                args->get_VirtualKey(&virtualKey);
                if (acceleratorKeyHandler_ && acceleratorKeyHandler_(virtualKey)) {
                    args->put_Handled(TRUE);
                }
                return S_OK;
            })
            .Get(),
        &token);
}

void ArktubeWebView::Navigate(const std::wstring &url) {
    if (webview_) {
        webview_->Navigate(url.c_str());
    }
}

void ArktubeWebView::Resize(const RECT &newBounds) {
    if (controller_) {
        controller_->put_Bounds(newBounds);
    }
}
