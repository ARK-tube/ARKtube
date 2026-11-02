#ifndef ARKTUBE_PLATFORM_RESOURCE_PATHS_H
#define ARKTUBE_PLATFORM_RESOURCE_PATHS_H

#include <glib.h>

/* Locate a file this app ships (icons, the injected JS) at runtime.
   Checked in order:
     1. next to the executable, under resources/<relative>       (build tree)
     2. ../share/arktube_linux/resources/<relative>               (installed,
        e.g. /usr/bin/arktube_linux + /usr/share/arktube_linux/...)
     3. resources/<relative> under the current working directory  (running
        straight out of the source tree)
   Returns a newly allocated path the caller must g_free(), or NULL if
   none of the above exist. */
gchar *arktube_find_resource(const char *relative);

#endif /* ARKTUBE_PLATFORM_RESOURCE_PATHS_H */
