/*
    ARKtube Windows -- shared compile-time configuration.

    Mirrors ../../arktube_linux/src/config.h: every constant more than
    one module needs lives here instead of in main.cpp, so main.cpp can
    stay orchestration-only the same way the Linux port's main.c does.
    Values are kept identical to the Linux build wherever the underlying
    thing they describe is identical (the target URL, the spoofed user
    agent, the connectivity-check host) -- only the Windows-specific
    paths (registry/app-data layout) differ.
*/

#ifndef ARKTUBE_CONFIG_H
#define ARKTUBE_CONFIG_H

#define ARKTUBE_TITLE L"ARKtube"
#define ARKTUBE_URL L"https://www.youtube.com/tv#/"

/* Used as the Win32 window class name and as the AppUserModelID set via
   SetCurrentProcessExplicitAppUserModelID() in main.cpp -- that's what
   lets Windows group this app's window(s) under one taskbar/Start-menu
   identity and match it back to the installed shortcut's icon, the same
   role ARKTUBE_WM_CLASS plays for GNOME Shell's dock on Linux (see that
   constant's comment in ../../arktube_linux/src/config.h). */
#define ARKTUBE_WINDOW_CLASS L"ARKtube.Windows"
#define ARKTUBE_APP_USER_MODEL_ID L"ARKtube.Windows"

/* Full replacement for WebView2's default Edge user agent (set via
   CoreWebView2Settings::put_UserAgent(), see src/ui/webview.cpp), kept
   byte-for-byte identical to ARKTUBE_USER_AGENT in
   ../../arktube_linux/src/config.h and to the ARKTUBE_USER_AGENT
   constant duplicated in resources/js/user-script.js -- all three have
   to agree, or the mismatch between the HTTP header, navigator.userAgent
   and the page's own expectations becomes a detectable signal, exactly
   as that file's header comment explains. */
#define ARKTUBE_USER_AGENT \
    L"Mozilla/5.0 (PS4; Leanback Shell) Cobalt/26.lts.0-qa (compatible)"

#define ARKTUBE_DEFAULT_WIDTH 1280
#define ARKTUBE_DEFAULT_HEIGHT 720

/* Same defensive ceiling as ARKTUBE_SPLASH_MAX_WAIT_MS on Linux: the
   splash is dismissed the moment WebView2 reports NavigationCompleted,
   but no later than this, in case that event is ever missed. */
#define ARKTUBE_SPLASH_MAX_WAIT_MS 20000

/* boot-logo.png and no_internet.png are the same 1672x941 artwork
   shipped for the Linux build (resources/ here is a copy of
   ../../arktube_linux/resources/, see docs/README.md). Both are
   stretched non-uniformly to exactly fill the window's client area, on
   every WM_SIZE, the same reasoning as ui/stretch_image.c there -- see
   src/ui/splash_overlay.cpp. */
#define ARKTUBE_SPLASH_LOGO_NATIVE_WIDTH 1672
#define ARKTUBE_SPLASH_LOGO_NATIVE_HEIGHT 941

/* Where the tagline sits in boot-logo.png at native resolution -- same
   measurements as ARKTUBE_SPLASH_TEXT_LEFT_NATIVE /
   ARKTUBE_SPLASH_TEXT_RIGHT_NATIVE /
   ARKTUBE_SPLASH_SPINNER_BOTTOM_MARGIN_NATIVE in
   ../../arktube_linux/src/config.h, since it's the same PNG; see that
   file's comment for how they were measured. */
#define ARKTUBE_SPLASH_TEXT_LEFT_NATIVE 172
#define ARKTUBE_SPLASH_TEXT_RIGHT_NATIVE 881
#define ARKTUBE_SPLASH_SPINNER_BOTTOM_MARGIN_NATIVE 160

/* Native (unscaled) pixel size of the branded spinner, rasterized once
   from resources/icons/arktube-spinner.svg at this size and rotated per
   frame -- same value and same reasoning as
   ARKTUBE_SPLASH_SPINNER_SIZE on Linux. */
#define ARKTUBE_SPLASH_SPINNER_SIZE 90
#define ARKTUBE_SPLASH_SPINNER_STEP_DEG 12.0
#define ARKTUBE_SPLASH_SPINNER_INTERVAL_MS 30

/* Connectivity check: a raw non-blocking Winsock connect(2) to
   ARKTUBE_CONNECTIVITY_HOST:ARKTUBE_CONNECTIVITY_PORT, same host/port
   and same reasoning (a fixed IP so a DNS failure can't be confused
   with the connectivity it's testing) as
   ../../arktube_linux/src/net/connectivity.c. See
   src/net/connectivity.cpp. */
#define ARKTUBE_CONNECTIVITY_HOST "8.8.8.8"
#define ARKTUBE_CONNECTIVITY_PORT 53
#define ARKTUBE_CONNECTIVITY_TIMEOUT_MS 2000
#define ARKTUBE_CONNECTIVITY_RETRY_INTERVAL_MS 3000
#define ARKTUBE_CONNECTIVITY_MONITOR_INTERVAL_MS 5000

/* Persisted window state ("was the window fullscreen last run"), read
   on startup and written whenever F11/Escape changes it. Stored under
   HKEY_CURRENT_USER\Software\<ARKTUBE_REGISTRY_KEY> rather than an INI
   file -- the native per-user settings store on Windows, the role
   GKeyFile under $XDG_CONFIG_HOME plays in
   ../../arktube_linux/src/platform/settings.c. See
   src/platform/settings.cpp. */
#define ARKTUBE_REGISTRY_KEY L"Software\\ARKtube"
#define ARKTUBE_REGISTRY_VALUE_FULLSCREEN L"Fullscreen"

#endif /* ARKTUBE_CONFIG_H */
