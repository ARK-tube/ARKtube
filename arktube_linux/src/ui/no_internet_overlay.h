#ifndef ARKTUBE_UI_NO_INTERNET_OVERLAY_H
#define ARKTUBE_UI_NO_INTERNET_OVERLAY_H

#include <gtk/gtk.h>

/* No-internet overlay: no_internet.png stretched to exactly fill the
   window in place of the browser. No spinner on this one: the retry
   loop runs silently in the background (see net/connectivity.c) and
   this overlay is simply replaced the moment that loop finds a
   connection. Returns NULL if no_internet.png isn't shipped this run. */
GtkWidget *arktube_create_no_internet_overlay(void);

#endif /* ARKTUBE_UI_NO_INTERNET_OVERLAY_H */
