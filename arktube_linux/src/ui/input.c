#include "input.h"

#include "../platform/settings.h"

static gboolean arktube_window_is_fullscreen(GtkWidget *window) {
    GdkWindow *gdk_window = gtk_widget_get_window(window);
    if (!gdk_window) {
        return FALSE;
    }
    return (gdk_window_get_state(gdk_window) & GDK_WINDOW_STATE_FULLSCREEN) != 0;
}

/* Cheap Bluetooth/IR Fire TV Stick-style remotes (and similar smart-TV
   remotes) register as an ordinary Linux input "keyboard" device --
   `evtest` on one shows plain evdev KEY_* codes, not a HID gamepad --
   so none of resources/js/user-script.js's Gamepad-API remap applies
   to them; they arrive here as normal GDK key events instead. Most of
   the buttons already Just Work because their evdev codes land on
   keysyms youtube.com/tv's own JS already understands (KEY_UP/DOWN/
   LEFT/RIGHT -> GDK_KEY_Up/Down/Left/Right, KEY_KPENTER ->
   GDK_KEY_KP_Enter, which WebKitGTK's keyval-to-DOM-`key` table maps
   to "Enter" the same as a plain Return).

   Two buttons don't, and both are dedicated hardware keys with no
   video-game-controller equivalent, so the Gamepad-API path could
   never have covered them either: the remote's Home button reports
   XKB's XF86HomePage keysym, and its search/voice (Alexa/Google
   Assistant) button reports XF86Search. Checked against WebKitGTK's
   own GDK-keyval switch (Source/WebCore/platform/gtk/
   PlatformKeyboardEventGtk.cpp upstream, mirroring the older
   KeyEventGtk.cpp this project inherited its WebKitGTK from): neither
   XF86HomePage nor XF86Search is one of the cases handled there, so
   left alone they'd reach the page as an unrecognized/empty key
   the page's own JS has nothing to match against, the same failure
   mode Samsung/Logitech keyboard-on-smart-TV threads report for other
   XF86 media keys reaching a YouTube-flavored webview.

   The fix is done here rather than in user-script.js: this handler is
   connected on the GtkWindow (see on_window_key_press below) and so
   runs *before* the event ever reaches WebKitWebView, letting a keyval
   swap stand in for a page-side remap -- rewrite the keyval to one
   WebKitGTK's table *does* already translate correctly, then let it
   fall through unhandled so it continues on to the WebView exactly like
   a real keypress of that key would:

     - XF86HomePage -> Home, so it lands on user-script.js's existing
       onKeyDown()/goHome() (the same handler that already answers a
       real Home key), sending youtube.com/tv back to its `#/` root.
     - XF86Search -> the literal '/' character, "Go to the search box"
       on youtube.com's own documented global keyboard shortcuts and
       the closest evdev-remote equivalent of a search/Assistant
       button available without a matching native voice API to hook
       into. youtube.com/tv's YouTube Leanback predecessor similarly
       treated an ordinary keypress (any alphanumeric, or Up from the
       grid) as "start a search", so a remapped '/' degrades safely --
       at worst a no-op, never a stray character typed somewhere -- if
       this build of the page doesn't bind it. */
static void arktube_remap_remote_keyval(GdkEventKey *event) {
    switch (event->keyval) {
        /* GTK/GDK generates these two off whatever X11/XF86keysym.h the
           system had at GDK-build time, so the identifier isn't always
           the "GDK_KEY_XF86*" name docs/older code samples use --
           confirmed against this project's actual target (Ubuntu 24.04's
           libgtk-3-dev, gdk/gdkkeysyms.h) where the generated defines
           are plain GDK_KEY_HomePage / GDK_KEY_Search, no XF86 prefix,
           for the same 0x1008ff18 / 0x1008ff1b keysyms. */
        case GDK_KEY_HomePage:
            event->keyval = GDK_KEY_Home;
            break;

        case GDK_KEY_Search:
            event->keyval = GDK_KEY_slash;
            break;

        default:
            break;
    }

    /* Remote's PROGRAM/guide button: unlike Home/Search above, evdev
       KEY_PROGRAM has no keysym bound to it in the stock XKB tables, so
       it arrives as GDK_KEY_VoidSymbol -- catch it by hardware keycode
       (X keycode 370 = evdev code 362 + 8) and remap to F11 instead. */
    if (event->keyval == GDK_KEY_VoidSymbol && event->hardware_keycode == 370) {
        event->keyval = GDK_KEY_F11;
    }
}

gboolean on_window_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)user_data;

    arktube_remap_remote_keyval(event);

    switch (event->keyval) {
        case GDK_KEY_F11:
            if (arktube_window_is_fullscreen(widget)) {
                gtk_window_unfullscreen(GTK_WINDOW(widget));
                arktube_save_fullscreen_pref(FALSE);
            } else {
                gtk_window_fullscreen(GTK_WINDOW(widget));
                arktube_save_fullscreen_pref(TRUE);
            }
            return TRUE;

        case GDK_KEY_Escape:
            if (arktube_window_is_fullscreen(widget)) {
                gtk_window_unfullscreen(GTK_WINDOW(widget));
                arktube_save_fullscreen_pref(FALSE);
                return TRUE;
            }
            return FALSE;

        default:
            return FALSE;
    }
}

gboolean on_window_delete(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    (void)widget;
    (void)event;
    (void)user_data;
    gtk_main_quit();
    return FALSE;
}
