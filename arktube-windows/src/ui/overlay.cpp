#include "overlay.h"

#include <gdiplus.h>

#include "../config.h"
#include "../platform/resource_paths.h"

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

namespace {

constexpr wchar_t kOverlayClassName[] = L"ArktubeOverlay";
constexpr UINT_PTR kSpinnerTimerId = 1;
constexpr UINT_PTR kFallbackTimerId = 2;

// Started once, on the first overlay ever created; never shut down
// explicitly -- the process exits as a whole, the same lifetime
// tradeoff the rest of this app makes for other one-time COM/Winsock
// setup (see net/connectivity.cpp's WSAStartup()).
ULONG_PTR g_gdiplusToken = 0;

void EnsureGdiplusStarted() {
    if (g_gdiplusToken != 0) {
        return;
    }
    GdiplusStartupInput input;
    GdiplusStartup(&g_gdiplusToken, &input, nullptr);
}

void EnsureWindowClassRegistered(HINSTANCE instance) {
    static bool registered = false;
    if (registered) {
        return;
    }
    WNDCLASSW wc{};
    wc.lpfnWndProc = ArktubeOverlayWindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kOverlayClassName;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    registered = true;
}

/* Recomputes the spinner's position for the current client size, using
   independent horizontal/vertical scale factors -- the same
   non-uniform-stretch math as
   arktube_splash_reposition_spinner()/the ARKTUBE_SPLASH_* constants in
   ../../../arktube_linux/src/{config.h,ui/splash_overlay.c}, since this
   is positioning the spinner over the same 1672x941 artwork. */
Gdiplus::RectF ComputeSpinnerRect(int clientWidth, int clientHeight) {
    double scaleX = static_cast<double>(clientWidth) / ARKTUBE_SPLASH_LOGO_NATIVE_WIDTH;
    double scaleY = static_cast<double>(clientHeight) / ARKTUBE_SPLASH_LOGO_NATIVE_HEIGHT;
    double textCenterNative =
        (ARKTUBE_SPLASH_TEXT_LEFT_NATIVE + ARKTUBE_SPLASH_TEXT_RIGHT_NATIVE) / 2.0;

    float left = static_cast<float>(textCenterNative * scaleX) - (ARKTUBE_SPLASH_SPINNER_SIZE / 2.0f);
    float top = static_cast<float>(clientHeight) -
                static_cast<float>(ARKTUBE_SPLASH_SPINNER_BOTTOM_MARGIN_NATIVE * scaleY) -
                ARKTUBE_SPLASH_SPINNER_SIZE;

    return Gdiplus::RectF(left, top, ARKTUBE_SPLASH_SPINNER_SIZE, ARKTUBE_SPLASH_SPINNER_SIZE);
}

} // namespace

LRESULT CALLBACK ArktubeOverlayWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto *self = reinterpret_cast<ArktubeOverlay *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT client;
            GetClientRect(hwnd, &client);
            int width = client.right - client.left;
            int height = client.bottom - client.top;

            Graphics graphics(hdc);
            graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);

            SolidBrush background(Color(255, 0x0f, 0x0f, 0x0f));
            graphics.FillRectangle(&background, 0, 0, width, height);

            if (self) {
                // Non-uniform stretch to exactly fill the client area
                // regardless of aspect ratio, on every paint -- the
                // GDI+ equivalent of the cairo draw callback in
                // arktube_create_stretch_image_area() on Linux.
                auto *image = reinterpret_cast<Image *>(self->gdiplusImage_);
                if (image) {
                    graphics.DrawImage(image, 0, 0, width, height);
                }

                auto *spinner = reinterpret_cast<Image *>(self->gdiplusSpinnerImage_);
                if (spinner && self->kind_ == ArktubeOverlayKind::Splash) {
                    Gdiplus::RectF spinnerRect = ComputeSpinnerRect(width, height);
                    Gdiplus::PointF center(
                        spinnerRect.X + spinnerRect.Width / 2.0f,
                        spinnerRect.Y + spinnerRect.Height / 2.0f);

                    GraphicsState state = graphics.Save();
                    graphics.TranslateTransform(center.X, center.Y);
                    graphics.RotateTransform(static_cast<float>(self->spinnerAngleDeg_));
                    graphics.DrawImage(
                        spinner, -spinnerRect.Width / 2.0f, -spinnerRect.Height / 2.0f,
                        spinnerRect.Width, spinnerRect.Height);
                    graphics.Restore(state);
                }
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_TIMER: {
            if (!self) {
                break;
            }
            if (wParam == kSpinnerTimerId) {
                // Pre-rendered once (the spinner PNG loaded at
                // construction), rotated per frame -- the same
                // "transform per frame rather than re-rasterize" trade
                // ui/spinner.c's comment on Linux describes GtkSpinner
                // itself making internally.
                self->spinnerAngleDeg_ += ARKTUBE_SPLASH_SPINNER_STEP_DEG;
                if (self->spinnerAngleDeg_ >= 360.0) {
                    self->spinnerAngleDeg_ -= 360.0;
                }
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_ERASEBKGND:
            // WM_PAINT above already paints the full client area every
            // time, so letting the default background erase run first
            // would just be a redundant, flicker-causing extra fill.
            return 1;

        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

std::unique_ptr<ArktubeOverlay> ArktubeOverlay::Create(HWND parentWindow, ArktubeOverlayKind kind) {
    EnsureGdiplusStarted();
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parentWindow, GWLP_HINSTANCE));
    EnsureWindowClassRegistered(instance);

    std::wstring relativePath = (kind == ArktubeOverlayKind::Splash)
                                     ? L"splash screen\\boot-logo.png"
                                     : L"splash screen\\no_internet.png";
    auto imagePath = ArktubeFindResource(relativePath);
    if (!imagePath) {
        OutputDebugStringW(
            (kind == ArktubeOverlayKind::Splash)
                ? L"ARKtube: 'splash screen\\boot-logo.png' not found; skipping the "
                  L"splash screen this run.\n"
                : L"ARKtube: 'splash screen\\no_internet.png' not found; cannot show "
                  L"the no-internet screen this run.\n");
        return nullptr;
    }

    auto *image = new Image(imagePath->c_str());
    if (image->GetLastStatus() != Ok) {
        delete image;
        OutputDebugStringW(L"ARKtube: failed to decode a splash/no-internet PNG via GDI+.\n");
        return nullptr;
    }

    Image *spinnerImage = nullptr;
    if (kind == ArktubeOverlayKind::Splash) {
        // GDI+'s built-in codecs don't include SVG, unlike
        // gdk-pixbuf's librsvg-backed loader on Linux (see
        // ui/spinner.c's comment on that fallback path) -- so the
        // packaging step (see ../../CMakeLists.txt) ships a
        // pre-rasterized PNG of the same spinner artwork for Windows
        // instead of the .svg shipped for Linux. A missing spinner
        // PNG degrades the same way a missing SVG does on Linux: the
        // splash still shows, just without the spinner.
        auto spinnerPath = ArktubeFindResource(L"icons\\arktube-spinner.png");
        if (spinnerPath) {
            spinnerImage = new Image(spinnerPath->c_str());
            if (spinnerImage->GetLastStatus() != Ok) {
                delete spinnerImage;
                spinnerImage = nullptr;
            }
        }
    }

    std::unique_ptr<ArktubeOverlay> overlay(new ArktubeOverlay());
    overlay->kind_ = kind;
    overlay->gdiplusImage_ = image;
    overlay->gdiplusSpinnerImage_ = spinnerImage;

    RECT parentClient;
    GetClientRect(parentWindow, &parentClient);

    overlay->hwnd_ = CreateWindowExW(
        0, kOverlayClassName, L"", WS_CHILD,
        0, 0, parentClient.right - parentClient.left, parentClient.bottom - parentClient.top,
        parentWindow, nullptr, instance, nullptr);

    SetWindowLongPtrW(overlay->hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(overlay.get()));

    return overlay;
}

void ArktubeOverlay::Show(void (*onFallbackTimeout)(void *), void *context) {
    if (!hwnd_) {
        return;
    }
    ShowWindow(hwnd_, SW_SHOW);
    SetWindowPos(hwnd_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    if (kind_ == ArktubeOverlayKind::Splash && gdiplusSpinnerImage_) {
        spinnerTimerId_ = kSpinnerTimerId;
        SetTimer(hwnd_, spinnerTimerId_, ARKTUBE_SPLASH_SPINNER_INTERVAL_MS, nullptr);
    }

    if (kind_ == ArktubeOverlayKind::Splash && onFallbackTimeout) {
        // The ARKTUBE_SPLASH_MAX_WAIT_MS defensive ceiling -- reveals
        // whatever's underneath even if WebView2 never reports
        // NavigationCompleted, the same reasoning as
        // on_splash_fallback_timeout() on Linux. fallbackCallback_/
        // fallbackContext_ are stashed on the instance (see overlay.h)
        // so the non-capturing TIMERPROC below can still reach an
        // arbitrary caller-supplied callback.
        fallbackCallback_ = onFallbackTimeout;
        fallbackContext_ = context;
        fallbackTimerId_ = kFallbackTimerId;
        SetTimer(
            hwnd_, fallbackTimerId_, ARKTUBE_SPLASH_MAX_WAIT_MS,
            [](HWND timerHwnd, UINT, UINT_PTR, DWORD) {
                auto *overlaySelf = reinterpret_cast<ArktubeOverlay *>(
                    GetWindowLongPtrW(timerHwnd, GWLP_USERDATA));
                if (!overlaySelf) {
                    return;
                }
                KillTimer(timerHwnd, kFallbackTimerId);
                overlaySelf->fallbackTimerId_ = 0;
                if (overlaySelf->fallbackCallback_) {
                    overlaySelf->fallbackCallback_(overlaySelf->fallbackContext_);
                }
            });
    }
}

void ArktubeOverlay::Dismiss() {
    if (!hwnd_) {
        return;
    }
    if (spinnerTimerId_) {
        KillTimer(hwnd_, spinnerTimerId_);
        spinnerTimerId_ = 0;
    }
    if (fallbackTimerId_) {
        KillTimer(hwnd_, fallbackTimerId_);
        fallbackTimerId_ = 0;
    }
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
}

void ArktubeOverlay::Resize(const RECT &newBounds) {
    if (!hwnd_) {
        return;
    }
    MoveWindow(
        hwnd_, newBounds.left, newBounds.top,
        newBounds.right - newBounds.left, newBounds.bottom - newBounds.top, TRUE);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

ArktubeOverlay::~ArktubeOverlay() {
    Dismiss();
    delete reinterpret_cast<Image *>(gdiplusImage_);
    delete reinterpret_cast<Image *>(gdiplusSpinnerImage_);
}
