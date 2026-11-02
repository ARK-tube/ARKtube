#include "spinner.h"

#include <math.h>

#include "../config.h"
#include "../platform/resource_paths.h"

/* Draws the current frame of the branded spinner: the pixbuf stashed on
   this widget rotated about its own center by "arktube-angle" degrees.
   Rotating a pre-rasterized pixbuf here, rather than re-rendering the
   SVG every frame, is what keeps a ~33fps redraw cheap. */
static gboolean arktube_spinner_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)user_data;
    GdkPixbuf *pixbuf = GDK_PIXBUF(g_object_get_data(G_OBJECT(widget), "arktube-pixbuf"));
    if (!pixbuf) {
        return FALSE;
    }

    gdouble angle = *(gdouble *)g_object_get_data(G_OBJECT(widget), "arktube-angle");
    gint w = gdk_pixbuf_get_width(pixbuf);
    gint h = gdk_pixbuf_get_height(pixbuf);

    cairo_translate(cr, ARKTUBE_SPLASH_SPINNER_SIZE / 2.0, ARKTUBE_SPLASH_SPINNER_SIZE / 2.0);
    cairo_rotate(cr, angle * G_PI / 180.0);
    gdk_cairo_set_source_pixbuf(cr, pixbuf, -w / 2.0, -h / 2.0);
    cairo_paint(cr);
    return FALSE;
}

/* Timer callback: advances the rotation angle and repaints. Stops
   itself (G_SOURCE_REMOVE) once the widget is gone -- the splash
   overlay is destroyed the moment youtube.com/tv finishes loading, and
   without this check the timer would otherwise keep firing against a
   freed widget. */
static gboolean arktube_spinner_tick(gpointer user_data) {
    GtkWidget *widget = GTK_WIDGET(user_data);
    if (!GTK_IS_WIDGET(widget)) {
        return G_SOURCE_REMOVE;
    }

    gdouble *angle = (gdouble *)g_object_get_data(G_OBJECT(widget), "arktube-angle");
    *angle = fmod(*angle + ARKTUBE_SPLASH_SPINNER_STEP_DEG, 360.0);
    gtk_widget_queue_draw(widget);
    return G_SOURCE_CONTINUE;
}

GtkWidget *arktube_create_spinner_widget(void) {
    gchar *path = arktube_find_resource("icons/arktube-spinner.svg");
    GdkPixbuf *pixbuf = NULL;
    if (path) {
        GError *error = NULL;
        pixbuf = gdk_pixbuf_new_from_file_at_scale(
            path, ARKTUBE_SPLASH_SPINNER_SIZE, ARKTUBE_SPLASH_SPINNER_SIZE, TRUE, &error);
        if (!pixbuf) {
            g_warning("ARKtube: could not load 'icons/arktube-spinner.svg': %s",
                      error ? error->message : "unknown error");
            g_clear_error(&error);
        }
        g_free(path);
    }

    if (!pixbuf) {
        GtkWidget *fallback = gtk_spinner_new();
        gtk_widget_set_size_request(fallback, ARKTUBE_SPLASH_SPINNER_SIZE,
                                     ARKTUBE_SPLASH_SPINNER_SIZE);
        gtk_widget_set_halign(fallback, GTK_ALIGN_START);
        gtk_widget_set_valign(fallback, GTK_ALIGN_START);
        gtk_spinner_start(GTK_SPINNER(fallback));

        GtkCssProvider *css = gtk_css_provider_new();
        gtk_css_provider_load_from_data(
            css, "spinner { color: #E8221A; }", -1, NULL);
        gtk_style_context_add_provider(
            gtk_widget_get_style_context(fallback),
            GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(css);
        return fallback;
    }

    GtkWidget *area = gtk_drawing_area_new();
    gtk_widget_set_size_request(area, ARKTUBE_SPLASH_SPINNER_SIZE, ARKTUBE_SPLASH_SPINNER_SIZE);
    gtk_widget_set_halign(area, GTK_ALIGN_START);
    gtk_widget_set_valign(area, GTK_ALIGN_START);
    /* Background must stay transparent so the boot-logo shows through
       around the pinwheel's own silhouette. */
    gtk_widget_set_app_paintable(area, TRUE);

    g_object_set_data_full(G_OBJECT(area), "arktube-pixbuf", pixbuf, g_object_unref);
    gdouble *angle = g_new0(gdouble, 1);
    g_object_set_data_full(G_OBJECT(area), "arktube-angle", angle, g_free);

    g_signal_connect(area, "draw", G_CALLBACK(arktube_spinner_draw), NULL);
    guint tick_id = g_timeout_add(
        ARKTUBE_SPLASH_SPINNER_INTERVAL_MS, arktube_spinner_tick, area);
    /* g_timeout_add() itself already stops (via arktube_spinner_tick()'s
       GTK_IS_WIDGET() check) once "area" is destroyed, but removing the
       source explicitly on destroy avoids leaving it registered for the
       few-hundred-ms window between the widget dying and its next
       scheduled tick. */
    g_signal_connect_swapped(area, "destroy", G_CALLBACK(g_source_remove),
                              GUINT_TO_POINTER(tick_id));

    return area;
}
