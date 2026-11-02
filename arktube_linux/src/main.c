/*
    ARKtube Linux -- native port entry point.

    This replaces the Neutralino + hybrid embedded-Chrome shell (see
    ../ARKtube and its docs/bugs-caught) with a single native GTK3 +
    WebKit2GTK process:

      - One process, one window, one WebView. No second Chrome process
        to spawn, reparent (via xdotool/wmctrl/xbindkeys) and keep in
        sync -- see ../docs/bugs-caught for the class of bugs that
        reparenting trick caused. That also means this works unchanged
        under Wayland, where reparenting another client's window isn't
        possible at all.
      - No Node.js/npm/neu CLI build step and no prebuilt Neutralino
        server binary to ship -- just a small C program linked against
        GTK3 and WebKit2GTK, both of which are already on virtually
        every Linux desktop (WebKitGTK is what GNOME Web / Epiphany
        uses).
      - WebKitGTK's user agent setting is a full replacement, not an
        append -- see ui/webview.c -- so this can reliably ask for
        YouTube's TV/Leanback interface the same way the old "chrome"
        mode's --user-agent flag did, without the window-mode limitation
        the old shell's README documented (Neutralino's own webview
        could only *extend* its user agent).

    Scope of this first port (see ../docs/PORTING-NOTES.md for the full
    file-by-file mapping): window creation, loading youtube.com/tv,
    fullscreen (F11) and Escape-to-unfullscreen, and the injected
    resources/js/user-script.js carrying over the parts of the old
    app-init.js that are pure page-side JS with no Neutralino API
    dependency (Home-key SPA navigation, cursor auto-hide, and the
    gamepad/remote-to-keyboard remap). The tray icon and the on-screen
    Immersive Mode lockdown are intentionally not ported yet -- they
    depended on Neutralino.os.setTray and need a native replacement
    (AppIndicator or similar), tracked as follow-up work.

    Connectivity is also monitored for the app's whole lifetime, not
    just at boot: the background thread started below keeps polling
    after its first successful check instead of exiting, so
    no_internet.png (see ui/no_internet_overlay.c) can reappear over an
    already-loaded page if the connection drops mid-session, the same
    way it already covered a genuinely offline boot.

    Persisted settings (the other Neutralino.storage-dependent piece
    PORTING-NOTES.md flagged) are ported for the one setting that
    currently needs it: whether the window was fullscreen last run, so
    the F11 toggle survives a restart the same way it would have if the
    old shell's window state had been wired up to Neutralino.storage.
    It's a GKeyFile under g_get_user_config_dir() (see
    platform/settings.c), the native fit that doc's "Persisted settings
    generally" note already called out.

    This file is deliberately just orchestration now: it wires GTK up
    and calls into the modules under platform/, net/ and ui/, each of
    which owns one piece of the app (resource lookup, persisted
    settings, connectivity polling, the webview, the splash/no-internet
    overlays, and keyboard/window input) and can be read and changed on
    its own.
*/

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include <stdlib.h>

#include "config.h"
#include "net/connectivity.h"
#include "platform/resource_paths.h"
#include "platform/settings.h"
#include "ui/input.h"
#include "ui/splash_overlay.h"
#include "ui/webview.h"

int main(int argc, char **argv) {
    /* Must happen before gtk_init(): GDK reads g_get_prgname() while
       setting up the X11 WM_CLASS hint (and the Wayland xdg_toplevel
       app-id) for every window this process creates, and that's the
       identity GNOME Shell's dock matches against installed .desktop
       files -- see the ARKTUBE_WM_CLASS comment in config.h. Setting it
       here covers every window (splash included) consistently, rather
       than patching one window after the fact. */
    g_set_prgname(ARKTUBE_WM_CLASS);

    gtk_init(&argc, &argv);

    g_set_application_name(ARKTUBE_TITLE);

    /* Default icon for every window this process creates, looked up by
       name from the icon theme -- the same "arktube-linux" name the
       .desktop file's Icon= key resolves, and the same PNG that gets
       installed to share/icons/hicolor/256x256/apps/ (see
       CMakeLists.txt). Consistent with the WM_CLASS/app-id fix above:
       one name, matched in both places, rather than the window's own
       XWMHints icon (set below, from a raw file path) being the only
       thing that ever agreed with the .desktop file. */
    gtk_window_set_default_icon_name(ARKTUBE_WM_CLASS);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), ARKTUBE_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(window), ARKTUBE_DEFAULT_WIDTH,
                                 ARKTUBE_DEFAULT_HEIGHT);
    /* Boots maximized by default -- same reasoning as the old shell's
       neutralino.config.json ("maximize": true, "fullScreen": false):
       the window manager's chrome stays visible until F11 is pressed
       explicitly. But if the window was fullscreen when the app last
       quit, honor that instead, the same way the on-screen fullscreen
       button's state would have round-tripped through
       Neutralino.storage in the old shell (see
       platform/settings.c's arktube_save_fullscreen_pref(), called from
       ui/input.c's on_window_key_press()). */
    if (arktube_load_fullscreen_pref()) {
        gtk_window_fullscreen(GTK_WINDOW(window));
    } else {
        gtk_window_maximize(GTK_WINDOW(window));
    }

    gchar *icon_path = arktube_find_resource("icons/appIcon.png");
    if (icon_path) {
        /* Belt-and-suspenders on top of gtk_window_set_default_icon_name()
           above: this covers running straight out of the build tree or an
           install whose icon cache hasn't been regenerated yet, where an
           icon-theme lookup by name would find nothing. Installed-and-
           cached runs get the same picture either way, since this is the
           same file the hicolor icon is a copy of. */
        GError *error = NULL;
        if (!gtk_window_set_icon_from_file(GTK_WINDOW(window), icon_path, &error)) {
            g_warning("ARKtube: could not load window icon '%s': %s", icon_path,
                       error ? error->message : "unknown error");
            g_clear_error(&error);
        }
        g_free(icon_path);
    } else {
        g_warning("ARKtube: icons/appIcon.png not found next to the executable "
                   "or under a share/arktube_linux install prefix; running "
                   "without a window icon.");
    }

    WebKitUserContentManager *ucm = webkit_user_content_manager_new();
    gchar *script_path = arktube_find_resource("js/user-script.js");
    if (script_path) {
        arktube_webview_inject_user_script(ucm, script_path);
        g_free(script_path);
    } else {
        g_warning("ARKtube: js/user-script.js not found; Home-key navigation, "
                   "cursor auto-hide, and gamepad/remote input will be "
                   "unavailable this run.");
    }

    GtkWidget *webview = arktube_webview_new(ucm);

    /* GtkOverlay owns both layers now: the WebView fills it as the base
       child, and (if boot-logo.png shipped) the splash backdrop sits on
       top of it as an overlay child -- both inside this one, real,
       already-maximized/fullscreen GtkWindow (set above), instead of the
       splash being a second, separate, fixed-size top-level window. See
       ui/splash_overlay.c's comment for the bug that used to cause: the
       splash not actually filling/matching the app's own window, and
       appearing as a small box with the desktop visible around it. */
    GtkWidget *root_overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window), root_overlay);
    gtk_container_add(GTK_CONTAINER(root_overlay), webview);

    g_signal_connect(window, "key-press-event", G_CALLBACK(on_window_key_press), NULL);
    g_signal_connect(window, "delete-event", G_CALLBACK(on_window_delete), NULL);

    /* Connected before load_uri() below so "load-changed" can't possibly
       fire WEBKIT_LOAD_FINISHED before anything is listening for it. */
    ArktubeSplashState *splash_state = g_new0(ArktubeSplashState, 1);
    g_signal_connect(webview, "load-changed",
                      G_CALLBACK(on_webview_load_changed), splash_state);

    /* The WebView is never pointed at ARKTUBE_URL directly here anymore
       -- arktube_on_internet_ready() (run from the background
       connectivity thread below, via g_idle_add, see net/connectivity.c)
       is what actually calls webkit_web_view_load_uri() and shows the
       boot splash, and only once a connectivity check has confirmed
       there's a connection. Until then, the no-internet overlay is shown
       in the same full-bleed overlay spot instead, so a genuinely
       offline machine never sits on an empty or endlessly "loading"
       browser. */
    ArktubeConnectivityState *connectivity_state = g_new0(ArktubeConnectivityState, 1);
    connectivity_state->root_overlay = root_overlay;
    connectivity_state->webview = webview;
    connectivity_state->splash_state = splash_state;
    g_thread_new("arktube-connectivity", arktube_connectivity_thread, connectivity_state);

    /* Show the real window immediately, already in its final
       maximized/fullscreen state (set above) -- whatever's on top
       (the no-internet screen or the boot splash, once the connectivity
       thread's first check reports back) is what actually becomes
       visible; nothing is deferred behind a second window anymore. */
    gtk_widget_show_all(window);

    gtk_main();

    g_free(connectivity_state);

    g_free(splash_state);
    return EXIT_SUCCESS;
}
