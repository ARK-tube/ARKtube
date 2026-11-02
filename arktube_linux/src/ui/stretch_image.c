#include "stretch_image.h"

#include "../platform/resource_paths.h"

/* Paints whatever GdkPixbuf was stashed on this GtkDrawingArea (see
   "arktube-pixbuf" below) scaled to exactly the widget's current
   allocated size -- width and height independently, so it stretches to
   fill the window edge to edge on every resize regardless of the
   window's actual resolution or aspect ratio, rather than the
   artwork's own 1672:941 ratio being preserved and letter/pillar-boxed
   in the middle of it. */
static gboolean arktube_stretch_image_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)user_data;
    GdkPixbuf *original = GDK_PIXBUF(g_object_get_data(G_OBJECT(widget), "arktube-pixbuf"));
    if (!original) {
        return FALSE;
    }

    gint width = gtk_widget_get_allocated_width(widget);
    gint height = gtk_widget_get_allocated_height(widget);
    if (width <= 0 || height <= 0) {
        return FALSE;
    }

    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(original, width, height, GDK_INTERP_BILINEAR);
    if (scaled) {
        gdk_cairo_set_source_pixbuf(cr, scaled, 0, 0);
        cairo_paint(cr);
        g_object_unref(scaled);
    }
    return FALSE;
}

GtkWidget *arktube_create_stretch_image_area(const char *relative_resource_path,
                                              const char *missing_warning) {
    gchar *path = arktube_find_resource(relative_resource_path);
    if (!path) {
        g_warning("%s", missing_warning);
        return NULL;
    }

    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, &error);
    g_free(path);

    if (!pixbuf) {
        g_warning("ARKtube: could not load '%s': %s", relative_resource_path,
                  error ? error->message : "unknown error");
        g_clear_error(&error);
        return NULL;
    }

    GtkWidget *area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(area, TRUE);
    gtk_widget_set_vexpand(area, TRUE);
    gtk_widget_set_halign(area, GTK_ALIGN_FILL);
    gtk_widget_set_valign(area, GTK_ALIGN_FILL);
    /* Ownership moves to the widget: the pixbuf is unref'd automatically
       when the drawing area is destroyed. */
    g_object_set_data_full(G_OBJECT(area), "arktube-pixbuf", pixbuf, g_object_unref);
    g_signal_connect(area, "draw", G_CALLBACK(arktube_stretch_image_draw), NULL);

    return area;
}
