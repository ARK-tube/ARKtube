#ifndef ARKTUBE_UI_OVERLAY_H
#define ARKTUBE_UI_OVERLAY_H

#include <windows.h>

#include <objidl.h>

#include <memory>

/* Windows counterpart of
   ../../../arktube_linux/src/ui/{stretch_image,splash_overlay,no_internet_overlay}.{c,h}
   and ui/spinner.h, folded into one translation unit: Linux splits
   "stretch a PNG to fill its container", "the splash screen" and "the
   no-internet screen" into three files because each is its own
   GtkWidget subtree composed with GtkOverlay/GtkContainer calls; here
   all three are one borderless child HWND that paints itself with GDI+
   (there is no GTK-style widget tree to compose on plain Win32), so
   splitting them into separate files would just be splitting one
   WM_PAINT handler's logic across translation units for no benefit.

   Both screens share this one class because they share everything
   except which PNG they stretch and whether a spinner is drawn on top
   -- exactly the parts ArktubeOverlayKind below selects. */
enum class ArktubeOverlayKind {
    Splash,     // boot-logo.png + spinner, see ../../../arktube_linux/src/ui/splash_overlay.c
    NoInternet, // no_internet.png, no spinner, see .../ui/no_internet_overlay.c
};

// Forward-declared so ArktubeOverlay can friend it below; defined in
// overlay.cpp.
LRESULT CALLBACK ArktubeOverlayWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* A full-bleed child window, stacked above the WebView2 controller's
   HWND in z-order (see main.cpp), that GDI+-stretches its PNG
   (non-uniformly if need be) to exactly fill whatever size main.cpp
   resizes it to -- the Win32 analogue of
   arktube_create_stretch_image_area()'s hexpand/vexpand GtkDrawingArea
   on Linux. Returns nullptr from Create() (mirroring that function's
   own NULL return) if the backing PNG can't be found or loaded via GDI+. */
class ArktubeOverlay {
public:
    ~ArktubeOverlay();

    static std::unique_ptr<ArktubeOverlay> Create(
        HWND parentWindow, ArktubeOverlayKind kind);

    HWND Handle() const { return hwnd_; }

    /* Shows the overlay and, for Splash, arms the
       ARKTUBE_SPLASH_MAX_WAIT_MS fallback timer that calls
       onFallbackTimeout if the splash hasn't already been dismissed by
       then -- the same defensive ceiling
       arktube_splash_show()/on_splash_fallback_timeout() implement on
       Linux. No-op for NoInternet, which (like its Linux counterpart)
       has no timer of its own: it's dismissed purely by the
       connectivity thread reporting online again. */
    void Show(void (*onFallbackTimeout)(void *), void *context);

    /* Hides and destroys the overlay window. Safe to call more than
       once; the second call is a no-op -- matching
       arktube_dismiss_splash()'s own idempotence on Linux. */
    void Dismiss();

    void Resize(const RECT &newBounds);

private:
    ArktubeOverlay() = default;

    // Implemented in overlay.cpp, where Gdiplus::Image is a complete
    // type; declared a friend so it can reach the opaque
    // gdiplusImage_/gdiplusSpinnerImage_ pointers below without this
    // header having to #include <gdiplus.h> itself (and pull GDI+ into
    // every translation unit that just wants to create/show/resize an
    // overlay, e.g. main.cpp).
    friend LRESULT CALLBACK ArktubeOverlayWindowProc(HWND, UINT, WPARAM, LPARAM);

    HWND hwnd_ = nullptr;
    IStream *imageStream_ = nullptr; // keeps the GDI+ Image's backing bytes alive
    void *gdiplusImage_ = nullptr;   // Gdiplus::Image*, opaque here to keep this header GDI+-free
    void *gdiplusSpinnerImage_ = nullptr; // Gdiplus::Image*, Splash only
    ArktubeOverlayKind kind_ = ArktubeOverlayKind::Splash;
    double spinnerAngleDeg_ = 0.0;
    UINT_PTR spinnerTimerId_ = 0;
    UINT_PTR fallbackTimerId_ = 0;

    // Stashed here (rather than only as SetTimer()'s TIMERPROC
    // argument) so the non-capturing TIMERPROC in overlay.cpp can
    // still reach an arbitrary caller-supplied callback + context --
    // the Win32 TIMERPROC signature has no user-data parameter of its
    // own, unlike GLib's g_timeout_add(), which is what lets
    // on_splash_fallback_timeout() on Linux close over
    // ArktubeSplashState* directly.
    void (*fallbackCallback_)(void *) = nullptr;
    void *fallbackContext_ = nullptr;
};

#endif // ARKTUBE_UI_OVERLAY_H
