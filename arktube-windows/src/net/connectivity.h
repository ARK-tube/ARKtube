#ifndef ARKTUBE_NET_CONNECTIVITY_H
#define ARKTUBE_NET_CONNECTIVITY_H

#include <windows.h>

/* Windows counterpart of
   ../../../arktube_linux/src/net/connectivity.h: gates the WebView2
   navigation to ARKTUBE_URL behind a connectivity check the same way,
   using Winsock instead of BSD sockets, and keeps polling for the
   app's whole lifetime so a connection dropping mid-session is caught
   too -- see that header's comment for the full reasoning, which
   applies unchanged here.

   All fields below are main-thread-owned, exactly as on Linux: the
   background thread (ArktubeConnectivityThreadProc) never touches them
   directly, only via PostMessageW(..., WM_ARKTUBE_CONNECTIVITY_CHANGED,
   ...) to the main window, matching the g_idle_add() pattern on Linux. */
#define WM_ARKTUBE_CONNECTIVITY_CHANGED (WM_APP + 1)

/* wParam of WM_ARKTUBE_CONNECTIVITY_CHANGED: TRUE if this update means
   "online now", FALSE if it means "offline now". */
struct ArktubeConnectivityState {
    HWND mainWindow = nullptr;
    bool webviewStarted = false; // main-thread-only; guards a single Navigate() call
    bool isOnline = false;       // main-thread-only; see connectivity.h on Linux for semantics
};

/* Runs on its own thread for as long as the app does (started via
   CreateThread() in main.cpp). Does no GUI work itself -- every
   state change is marshaled to the main thread with PostMessageW() so
   the window proc (src/ui/input.cpp / main.cpp) can react on the UI
   thread, the same split ArktubeConnectivityState keeps on Linux
   between this struct's main-thread fields and the thread's own,
   purely-socket-level work. */
DWORD WINAPI ArktubeConnectivityThreadProc(LPVOID param);

#endif // ARKTUBE_NET_CONNECTIVITY_H
