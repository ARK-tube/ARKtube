#ifndef ARKTUBE_PLATFORM_SETTINGS_H
#define ARKTUBE_PLATFORM_SETTINGS_H

#include <glib.h>

/* Reads the remembered fullscreen state. Defaults to FALSE (matching
   the old shell's boot config, "maximize": true / "fullScreen": false)
   on first run, a missing file, or a corrupt one -- a persisted-setting
   read failing should fail open to today's existing behavior, not
   silently force a mode the user never asked for. */
gboolean arktube_load_fullscreen_pref(void);

/* Writes the current fullscreen state so the next launch can restore
   it. Re-reads the existing file first so unrelated keys/groups a
   future setting might add aren't clobbered by this one save. */
void arktube_save_fullscreen_pref(gboolean fullscreen);

#endif /* ARKTUBE_PLATFORM_SETTINGS_H */
