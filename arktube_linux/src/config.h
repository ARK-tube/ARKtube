/*
    ARKtube Linux -- shared compile-time configuration.

    Every #define that more than one module needs lives here instead of
    in main.c, so splitting the old single-file port into modules (see
    PORTING-NOTES in main.c's header comment for the file-by-file
    mapping) didn't mean duplicating or re-guessing these constants in
    each new .c file.
*/

#ifndef ARKTUBE_CONFIG_H
#define ARKTUBE_CONFIG_H

#define ARKTUBE_APP_ID "com.arktube.linux"
#define ARKTUBE_TITLE "ARKtube"
#define ARKTUBE_URL "https://www.youtube.com/tv#/"

/* Must match packaging/arktube-linux.desktop's filename (its desktop-file
   id, "arktube-linux.desktop" minus the extension) and that file's Icon=
   key. GNOME Shell's dock/dash matches a *running window* back to an
   installed .desktop launcher (to know what to show/pin) by comparing
   WM_CLASS on X11, or the xdg_toplevel app-id on Wayland, against that
   id -- and GTK derives both from g_get_prgname() for a plain GtkWindow
   app like this one, not from the binary name. Left at the g_get_prgname()
   default, that comparison is "arktube_linux" (this binary's argv[0])
   vs. the desktop file's "arktube-linux" -- close enough to read as the
   same app to a person, but not an exact string match, so the window
   goes unmatched and GNOME falls back to a generic icon in the dock.
   Setting the prgname explicitly, here, is what keeps that comparison
   consistent. */
#define ARKTUBE_WM_CLASS "arktube-linux"

/* Full replacement, matching the old shell's "chrome mode" user agent
   (see ../ARKtube/neutralino.config.json's "chrome".args and
   README.md's Chrome-Mode-vs-Window-Mode section) so YouTube's
   server-side device detection serves the same TV/Leanback interface
   this app is built around. */
#define ARKTUBE_USER_AGENT \
    "Mozilla/5.0 (PS4; Leanback Shell) Cobalt/26.lts.0-qa (compatible)"

#define ARKTUBE_DEFAULT_WIDTH 1280
#define ARKTUBE_DEFAULT_HEIGHT 720

/* Splash overlay shown on startup while the main window's WebView loads
   youtube.com/tv underneath it. Drawn as a full-bleed layer *inside* the
   real app window and dismissed the moment the WebView actually reports
   WEBKIT_LOAD_FINISHED, not after a fixed guess at how long that should
   take -- see ui/splash_overlay.c. ARKTUBE_SPLASH_MAX_WAIT_MS is only a
   defensive ceiling: if "load-changed" never reports FINISHED for some
   reason (a stalled resource, a WebKit quirk), the splash is dismissed
   anyway after this long rather than sitting on screen forever. */
#define ARKTUBE_SPLASH_MAX_WAIT_MS 20000

/* boot-logo.png and no_internet.png are both a fixed 1672x941 piece of
   artwork. Rather than scaling either down to some fixed width and
   letter/pillar-boxing it in the middle of whatever the real window's
   client area is, both are stretched -- non-uniformly if need be -- to
   exactly fill it, on every resize, regardless of the window's actual
   resolution or aspect ratio. See ui/stretch_image.c. */
#define ARKTUBE_SPLASH_LOGO_NATIVE_WIDTH 1672
#define ARKTUBE_SPLASH_LOGO_NATIVE_HEIGHT 941

/* Where the "Youtube tv app for desktop devices" tagline actually sits
   in boot-logo.png, measured directly off the artwork's pixels (left
   edge and right edge of the text) at native 1672x941 -- so the spinner
   can be pinned centered under its horizontal midpoint instead of
   guessed. Since the artwork is now stretched non-uniformly to fill the
   window, the spinner's position is recomputed from these on every
   resize (see ui/splash_overlay.c) using independent horizontal and
   vertical scale factors, rather than a single shared scale the way a
   uniformly-scaled image would only need.

   The spinner is deliberately *not* pinned just under the tagline's
   bottom edge any more: that crowded the text instead of reading as its
   own element. Vertically it's anchored to the artwork's bottom edge
   instead, by ARKTUBE_SPLASH_SPINNER_BOTTOM_MARGIN_NATIVE -- chosen from
   the actual pixel content of boot-logo.png (see the color-sampling
   analysis that produced these numbers) so that, at this size, it sits
   in the empty dark space below the wordmark and clear of the diagonal
   red glow band, rather than overlapping it. That band's top edge
   doesn't reach this x-range at all (it only intrudes past roughly
   x=680 native, well right of the tagline's midpoint used below), so
   this margin has real clearance on every side, not just a close miss. */
#define ARKTUBE_SPLASH_TEXT_LEFT_NATIVE 172
#define ARKTUBE_SPLASH_TEXT_RIGHT_NATIVE 881
#define ARKTUBE_SPLASH_SPINNER_BOTTOM_MARGIN_NATIVE 160

/* Native pixel size (unscaled -- kept fixed regardless of window size,
   same as the spinner it replaces) of the branded spinner artwork.
   Sized well above the old 30px default GtkSpinner so it reads as a
   deliberate piece of the composition instead of a stray dot. */
#define ARKTUBE_SPLASH_SPINNER_SIZE 90

/* How many degrees the spinner advances per animation tick, and how
   often -- together giving one full revolution every
   (360 / ARKTUBE_SPLASH_SPINNER_STEP_DEG) * ARKTUBE_SPLASH_SPINNER_INTERVAL_MS
   milliseconds. 12 degrees / 30ms is 30 steps per revolution at roughly
   33fps, about a 1-second-per-turn pace -- close to GtkSpinner's own
   default cadence. */
#define ARKTUBE_SPLASH_SPINNER_STEP_DEG 12.0
#define ARKTUBE_SPLASH_SPINNER_INTERVAL_MS 30

/* Connectivity check, done with plain BSD-socket syscalls (socket(2) /
   connect(2) / poll(2), no libcurl or libsoup dependency) rather than
   trying to load youtube.com/tv itself and hoping WebKit's own failure
   mode is easy to distinguish from a slow page. A fixed IP:port (Google
   Public DNS's TCP/53) is used instead of a hostname so a DNS
   resolution failure -- itself just another symptom of no internet --
   can't be confused with the connect() this is actually testing. */
#define ARKTUBE_CONNECTIVITY_HOST "8.8.8.8"
#define ARKTUBE_CONNECTIVITY_PORT 53
#define ARKTUBE_CONNECTIVITY_TIMEOUT_MS 2000
#define ARKTUBE_CONNECTIVITY_RETRY_INTERVAL_MS 3000

/* Once a check has succeeded, arktube_connectivity_thread() no longer
   exits -- it keeps polling for the rest of the app's life so a
   connection that drops mid-session (not just at boot) is still caught
   -- but there's no reason to hammer connect(2) every
   ARKTUBE_CONNECTIVITY_RETRY_INTERVAL_MS while things are fine. This
   separate, longer interval is used for that steady-state "are we still
   online" polling; the shorter retry interval above is kept for the
   already-offline case, where recovering quickly matters more than
   saving a few wakeups. */
#define ARKTUBE_CONNECTIVITY_MONITOR_INTERVAL_MS 5000

/* Persisted window state -- currently just "was the window fullscreen
   last time the app quit", read on startup and written whenever F11 or
   Escape changes it, so the toggle survives a restart. Native
   replacement for Neutralino.storage, per docs/PORTING-NOTES.md's
   "Persisted settings generally" note. */
#define ARKTUBE_CONFIG_DIR_NAME "arktube_linux"
#define ARKTUBE_CONFIG_FILE_NAME "config.ini"
#define ARKTUBE_CONFIG_GROUP "window"
#define ARKTUBE_CONFIG_KEY_FULLSCREEN "fullscreen"

#endif /* ARKTUBE_CONFIG_H */
