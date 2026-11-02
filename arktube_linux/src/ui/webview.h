#ifndef ARKTUBE_UI_WEBVIEW_H
#define ARKTUBE_UI_WEBVIEW_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

/* Reads user-script.js from path and registers it to be injected at
   document-start into every frame of the WebView that owns ucm, ahead
   of any of the page's own scripts, so the user-agent and
   navigator/screen spoof set up below is in place before the page's
   bootstrap JS ever reads it. */
void arktube_webview_inject_user_script(WebKitUserContentManager *ucm, const char *path);

/* Creates the main WebView: the TV/Leanback user agent (full
   replacement, not an append), developer extras, media stream and
   WebAudio support, and hardware-accelerated compositing explicitly
   forced ALWAYS-on -- see webview.c for why each setting matters. */
GtkWidget *arktube_webview_new(WebKitUserContentManager *ucm);

#endif /* ARKTUBE_UI_WEBVIEW_H */
