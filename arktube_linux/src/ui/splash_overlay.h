#ifndef ARKTUBE_UI_SPLASH_OVERLAY_H
#define ARKTUBE_UI_SPLASH_OVERLAY_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

typedef struct {
    GtkWidget *splash;      /* NULL once dismissed (or if none was ever shown) */
    guint      fallback_id; /* the ARKTUBE_SPLASH_MAX_WAIT_MS safety timer's source id, or 0 */
} ArktubeSplashState;

/* Builds the splash overlay (boot-logo.png stretched to fill the
   window, with the branded spinner positioned under the tagline),
   layers it onto root_overlay, shows it, and arms the
   ARKTUBE_SPLASH_MAX_WAIT_MS defensive fallback timer that dismisses it
   even if the WebView never reports WEBKIT_LOAD_FINISHED. A no-op if
   boot-logo.png isn't shipped this run -- state->splash is simply left
   NULL, so packaging without it still boots straight to the bare
   webview with no overlay at all. */
void arktube_splash_show(GtkWidget *root_overlay, ArktubeSplashState *state);

/* Tears the splash overlay down, whichever caller gets there first --
   safe to call more than once; the second call is just a no-op. */
void arktube_dismiss_splash(ArktubeSplashState *state);

/* "load-changed" handler: dismisses the splash exactly when
   youtube.com/tv has actually finished loading. user_data must be the
   ArktubeSplashState* passed to arktube_splash_show(). Connect this to
   the WebView before arktube_splash_show() is ever called, so
   WEBKIT_LOAD_FINISHED can't possibly fire before anything is listening
   for it. */
void on_webview_load_changed(WebKitWebView *webview, WebKitLoadEvent load_event,
                              gpointer user_data);

#endif /* ARKTUBE_UI_SPLASH_OVERLAY_H */
