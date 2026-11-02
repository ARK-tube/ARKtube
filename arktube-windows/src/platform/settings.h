#ifndef ARKTUBE_PLATFORM_SETTINGS_H
#define ARKTUBE_PLATFORM_SETTINGS_H

/* Windows counterpart of
   ../../../arktube_linux/src/platform/settings.h: persists "was the
   window fullscreen when the app last quit" under
   HKEY_CURRENT_USER\<ARKTUBE_REGISTRY_KEY> (see config.h) instead of a
   GKeyFile, so the F11 toggle survives a restart the same way it does
   on Linux. */

/* Reads the remembered fullscreen state. Defaults to false (matching
   the app's normal boot state: maximized, not fullscreen) on first
   run, a missing key, or any registry read error -- a persisted-setting
   read failing should fail open to today's existing behavior, not
   silently force a mode the user never asked for. */
bool ArktubeLoadFullscreenPref();

/* Writes the current fullscreen state so the next launch can restore
   it. */
void ArktubeSaveFullscreenPref(bool fullscreen);

#endif // ARKTUBE_PLATFORM_SETTINGS_H
