#include "splash_overlay.h"

#include "../config.h"
#include "spinner.h"
#include "stretch_image.h"

/* Recomputes the spinner's position every time the stretched boot-logo
   image is resized, using independent horizontal and vertical scale
   factors (allocation size / native size on each axis) rather than one
   shared scale -- because the image is no longer scaled uniformly, a
   single shared factor would drift off the tagline as soon as the
   window's aspect ratio stopped matching the artwork's own 1672:941.

   Horizontally: centered on the tagline's own midpoint, not its left
   edge. Vertically: anchored to the *bottom* of the frame by
   ARKTUBE_SPLASH_SPINNER_BOTTOM_MARGIN_NATIVE rather than hung just
   under the text, so it reads as belonging to the composition as a
   whole instead of crowding the tagline. */
static void arktube_splash_reposition_spinner(GtkWidget *widget, GdkRectangle *allocation,
                                               gpointer user_data) {
    (void)widget;
    GtkWidget *spinner = GTK_WIDGET(user_data);

    if (allocation->width <= 0 || allocation->height <= 0) {
        return;
    }

    gdouble scale_x = (gdouble)allocation->width / (gdouble)ARKTUBE_SPLASH_LOGO_NATIVE_WIDTH;
    gdouble scale_y = (gdouble)allocation->height / (gdouble)ARKTUBE_SPLASH_LOGO_NATIVE_HEIGHT;
    gdouble text_center_native =
        (ARKTUBE_SPLASH_TEXT_LEFT_NATIVE + ARKTUBE_SPLASH_TEXT_RIGHT_NATIVE) / 2.0;

    gint margin_start = (gint)(text_center_native * scale_x) - (ARKTUBE_SPLASH_SPINNER_SIZE / 2);
    gint margin_top = allocation->height -
        (gint)(ARKTUBE_SPLASH_SPINNER_BOTTOM_MARGIN_NATIVE * scale_y) -
        ARKTUBE_SPLASH_SPINNER_SIZE;

    gtk_widget_set_margin_start(spinner, margin_start);
    gtk_widget_set_margin_top(spinner, margin_top);
}

/* Splash overlay: boot-logo.png stretched to exactly fill the window
   with a spinning "loading" indicator overlaid just under the tagline,
   drawn as a full-bleed layer *inside* the main window (layered on top
   of the WebView via the GtkOverlay set up by the caller, not as a
   second, separate top-level window) so it always exactly fills and
   matches whatever state the real window is actually in this run --
   maximized, fullscreen, or a plain resizable window -- instead of
   floating as its own small, fixed-size, always-centered-on-the-screen
   window regardless of where or how big the real window ends up.
   Returns NULL (and leaves nothing on screen) if the boot logo isn't
   shipped this run. */
static GtkWidget *arktube_create_splash_overlay(void) {
    GtkWidget *backdrop = gtk_event_box_new();
    gtk_widget_set_hexpand(backdrop, TRUE);
    gtk_widget_set_vexpand(backdrop, TRUE);
    gtk_widget_set_halign(backdrop, GTK_ALIGN_FILL);
    gtk_widget_set_valign(backdrop, GTK_ALIGN_FILL);
    gtk_widget_set_name(backdrop, "arktube-splash-backdrop");

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(
        css, "#arktube-splash-backdrop { background-color: #0f0f0f; }", -1, NULL);
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(backdrop),
        GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    /* GtkOverlay so the spinner floats on top of the stretched image at
       an exact pixel position instead of sharing a box layout with it.
       Fills the backdrop completely (hexpand/vexpand + ALIGN_FILL)
       rather than shrink-wrapping to a fixed size and centering, so the
       image beneath it stretches to the real window's actual size. */
    GtkWidget *card = gtk_overlay_new();
    gtk_widget_set_hexpand(card, TRUE);
    gtk_widget_set_vexpand(card, TRUE);
    gtk_widget_set_halign(card, GTK_ALIGN_FILL);
    gtk_widget_set_valign(card, GTK_ALIGN_FILL);
    gtk_container_add(GTK_CONTAINER(backdrop), card);

    GtkWidget *image_area = arktube_create_stretch_image_area(
        "splash screen/boot-logo.png",
        "ARKtube: 'splash screen/boot-logo.png' not found; skipping the "
        "splash screen this run.");
    if (!image_area) {
        return NULL;
    }
    gtk_container_add(GTK_CONTAINER(card), image_area);

    /* The branded loading spinner: centered under the tagline's
       midpoint, anchored to the bottom of the frame rather than hung
       directly under the text. Repositioned on every resize of
       image_area, since the stretched image's scale changes with the
       window's own size. */
    GtkWidget *spinner = arktube_create_spinner_widget();
    gtk_overlay_add_overlay(GTK_OVERLAY(card), spinner);
    g_signal_connect(image_area, "size-allocate",
                      G_CALLBACK(arktube_splash_reposition_spinner), spinner);

    return backdrop;
}

/* Defensive ceiling (see ARKTUBE_SPLASH_MAX_WAIT_MS): reveals whatever's
   underneath even if the WebView never reports WEBKIT_LOAD_FINISHED for
   some reason. */
static gboolean on_splash_fallback_timeout(gpointer user_data) {
    ArktubeSplashState *state = (ArktubeSplashState *)user_data;
    state->fallback_id = 0; /* this source is already being removed by returning below */
    arktube_dismiss_splash(state);
    return G_SOURCE_REMOVE;
}

void arktube_splash_show(GtkWidget *root_overlay, ArktubeSplashState *state) {
    GtkWidget *splash = arktube_create_splash_overlay();
    if (!splash) {
        return;
    }

    gtk_overlay_add_overlay(GTK_OVERLAY(root_overlay), splash);
    gtk_widget_show_all(splash);
    state->splash = splash;
    state->fallback_id = g_timeout_add(
        ARKTUBE_SPLASH_MAX_WAIT_MS, on_splash_fallback_timeout, state);
}

void arktube_dismiss_splash(ArktubeSplashState *state) {
    if (!state->splash) {
        return;
    }
    gtk_widget_destroy(state->splash);
    state->splash = NULL;
    if (state->fallback_id) {
        g_source_remove(state->fallback_id);
        state->fallback_id = 0;
    }
}

void on_webview_load_changed(WebKitWebView *webview, WebKitLoadEvent load_event,
                              gpointer user_data) {
    (void)webview;
    if (load_event != WEBKIT_LOAD_FINISHED) {
        return;
    }
    arktube_dismiss_splash((ArktubeSplashState *)user_data);
}
