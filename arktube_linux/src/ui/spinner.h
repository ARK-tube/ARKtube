#ifndef ARKTUBE_UI_SPINNER_H
#define ARKTUBE_UI_SPINNER_H

#include <gtk/gtk.h>

/* The branded loading spinner: an 8-blade pinwheel shipped as
   resources/icons/arktube-spinner.svg, rasterized once at
   ARKTUBE_SPLASH_SPINNER_SIZE and then just rotated per frame, the same
   "pre-render once, transform per frame" trade GtkSpinner itself makes
   internally. Falls back to a plain (but re-tinted) GtkSpinner if the
   SVG can't be loaded -- e.g. a minimal system missing the gdk-pixbuf
   SVG loader (librsvg) -- so a missing icon loader degrades the *style*
   of the loading indicator rather than losing it outright. */
GtkWidget *arktube_create_spinner_widget(void);

#endif /* ARKTUBE_UI_SPINNER_H */
