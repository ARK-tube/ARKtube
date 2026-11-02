#ifndef ARKTUBE_UI_INPUT_H
#define ARKTUBE_UI_INPUT_H

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

/* F11 toggles fullscreen; Escape only ever backs out of it, mirroring
   the old app-init.js onKeyDown()/exitFullScreenIfActive() -- Escape
   never quits the app outright. Also remaps a handful of smart-TV
   remote keysyms (Home/Search/Program) that arrive as ordinary GDK key
   events -- see input.c for the full rationale. Intended to be
   connected on the GtkWindow itself (rather than the WebView) so it
   fires ahead of youtube.com/tv's own keydown handling. */
gboolean on_window_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

/* Single-process shutdown: closing the window just quits, the way a
   normal native application does. */
gboolean on_window_delete(GtkWidget *widget, GdkEvent *event, gpointer user_data);

#endif /* ARKTUBE_UI_INPUT_H */
