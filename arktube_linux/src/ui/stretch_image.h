#ifndef ARKTUBE_UI_STRETCH_IMAGE_H
#define ARKTUBE_UI_STRETCH_IMAGE_H

#include <gtk/gtk.h>

/* Loads a resource PNG at its full native resolution and wraps it in a
   hexpand/vexpand GtkDrawingArea that stretches -- non-uniformly if
   need be -- to exactly fill whatever container it's added to, on
   every resize, regardless of the window's actual resolution or aspect
   ratio. Shared by the boot splash and the no-internet screen, since
   both want the same "cover the whole window, however big or oddly
   shaped it is" artwork behavior.

   Returns NULL (having already warned with missing_warning) if the
   resource is missing or fails to load. */
GtkWidget *arktube_create_stretch_image_area(const char *relative_resource_path,
                                              const char *missing_warning);

#endif /* ARKTUBE_UI_STRETCH_IMAGE_H */
