#ifndef ARKTUBE_NET_CONNECTIVITY_H
#define ARKTUBE_NET_CONNECTIVITY_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "../ui/splash_overlay.h"

/* Gates the WebView ever being pointed at ARKTUBE_URL behind a
   connectivity check, so a genuinely offline machine shows
   no_internet.png -- never an empty or perpetually-loading browser --
   and only starts the real load (WebView + boot splash) once a check
   actually succeeds.

   That check doesn't stop once the boot-time gate passes, though:
   arktube_connectivity_thread() keeps polling for as long as the app
   runs, and is_online is the flag it flips on every transition. That
   makes a connection dropping mid-session (the WebView already loaded,
   the user mid-video) show exactly the same no-internet screen
   boot-time offline already did, instead of that screen only ever
   being reachable before the first successful load. */
typedef struct {
    GtkWidget          *root_overlay;
    GtkWidget          *webview;
    ArktubeSplashState *splash_state;
    GtkWidget          *no_internet_overlay; /* main-thread-only; NULL when not shown */
    gboolean            webview_started;     /* main-thread-only; guards a single load_uri() */
    gboolean            is_online;           /* main-thread-only; TRUE once any check -- boot-time
                                                 or later -- has succeeded, FALSE from the moment
                                                 any check (including a post-boot one) fails. */
} ArktubeConnectivityState;

/* Runs entirely off the main thread for as long as the app does. Only
   touches cs's main-thread-owned fields via g_idle_add(); everything
   else is plain BSD-socket syscalls, safe to run concurrently with the
   GTK main loop. Intended to be started with g_thread_new(). */
gpointer arktube_connectivity_thread(gpointer user_data);

#endif /* ARKTUBE_NET_CONNECTIVITY_H */
