#include "no_internet_overlay.h"

#include "stretch_image.h"

GtkWidget *arktube_create_no_internet_overlay(void) {
    GtkWidget *backdrop = gtk_event_box_new();
    gtk_widget_set_hexpand(backdrop, TRUE);
    gtk_widget_set_vexpand(backdrop, TRUE);
    gtk_widget_set_halign(backdrop, GTK_ALIGN_FILL);
    gtk_widget_set_valign(backdrop, GTK_ALIGN_FILL);
    gtk_widget_set_name(backdrop, "arktube-no-internet-backdrop");

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(
        css, "#arktube-no-internet-backdrop { background-color: #0f0f0f; }", -1, NULL);
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(backdrop),
        GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    GtkWidget *image_area = arktube_create_stretch_image_area(
        "splash screen/no_internet.png",
        "ARKtube: 'splash screen/no_internet.png' not found; cannot show "
        "the no-internet screen this run.");
    if (!image_area) {
        return NULL;
    }
    gtk_container_add(GTK_CONTAINER(backdrop), image_area);

    return backdrop;
}
