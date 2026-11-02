#ifndef ARKTUBE_UI_INPUT_H
#define ARKTUBE_UI_INPUT_H

#include <windows.h>

/* Windows counterpart of ../../../arktube_linux/src/ui/input.h. F11
   toggles fullscreen; Escape only ever backs out of it -- same
   behavior, same reasoning (mirrors the old app-init.js
   onKeyDown()/exitFullScreenIfActive()) as on_window_key_press() on
   Linux.

   Unlike GTK's "key-press-event" (connected directly on the
   GtkWindow, ahead of the WebView, see ../../../arktube_linux/src/main.c),
   WebView2 by default handles keyboard input itself and never gives
   the host window's WndProc a chance to see it first -- see
   ArktubeHandleAcceleratorKey()'s header comment for how this app gets
   the same "native shortcuts win" ordering back via WebView2's
   ICoreWebView2Controller4::add_AcceleratorKeyPressed. */

/* Returns TRUE (and toggles/exits fullscreen, persisting the new state
   via ArktubeSaveFullscreenPref()) if key was one of the shortcuts this
   function owns -- F11 or Escape-while-fullscreen -- so the caller
   (ArktubeHandleAcceleratorKey() in webview.cpp, or WM_KEYDOWN in
   main.cpp for input that reaches the host window directly, e.g. while
   a splash/no-internet overlay has focus) knows to suppress the key
   from reaching the page. FALSE means "not handled here, let it
   through normally". */
BOOL ArktubeHandleWindowKeyDown(HWND window, WPARAM virtualKey);

/* Toggles the window between its current state and fullscreen --
   borderless, taskbar-covering, matching gtk_window_fullscreen()/
   gtk_window_unfullscreen() on Linux. Saves the restored (non-
   fullscreen) window placement in prevPlacement so it can put the
   window back exactly where it was. */
void ArktubeSetFullscreen(HWND window, BOOL fullscreen, WINDOWPLACEMENT *prevPlacement);

#endif // ARKTUBE_UI_INPUT_H
