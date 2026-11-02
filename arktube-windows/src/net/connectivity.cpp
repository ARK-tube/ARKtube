#include "connectivity.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include "../config.h"

#pragma comment(lib, "ws2_32.lib")

/* Internet reachability check via a raw non-blocking connect(2) to
   ARKTUBE_CONNECTIVITY_HOST:ARKTUBE_CONNECTIVITY_PORT, bounded by
   select() so a silently-dropping network can't hang this past
   ARKTUBE_CONNECTIVITY_TIMEOUT_MS -- the Winsock equivalent of
   ../../../arktube_linux/src/net/connectivity.c's
   arktube_check_internet_now(), which uses poll() for the same bound.
   Deliberately plain sockets rather than asking WebView2 to navigate
   and inferring the answer from its failure mode, for the same reason
   given there: this runs in a background thread, repeatedly, well
   before (and independently of) any WebView2 navigation. */
static bool ArktubeCheckInternetNow() {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return false;
    }

    u_long nonBlocking = 1;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ARKTUBE_CONNECTIVITY_PORT);
    if (InetPtonA(AF_INET, ARKTUBE_CONNECTIVITY_HOST, &addr.sin_addr) != 1) {
        closesocket(sock);
        return false;
    }

    bool online = false;
    int rc = connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    if (rc == 0) {
        online = true;
    } else if (WSAGetLastError() == WSAEWOULDBLOCK) {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);
        fd_set errorSet;
        FD_ZERO(&errorSet);
        FD_SET(sock, &errorSet);

        timeval timeout{};
        timeout.tv_sec = ARKTUBE_CONNECTIVITY_TIMEOUT_MS / 1000;
        timeout.tv_usec = (ARKTUBE_CONNECTIVITY_TIMEOUT_MS % 1000) * 1000;

        int selectResult = select(0, nullptr, &writeSet, &errorSet, &timeout);
        if (selectResult > 0 && FD_ISSET(sock, &writeSet) && !FD_ISSET(sock, &errorSet)) {
            online = true;
        }
    }

    closesocket(sock);
    return online;
}

/* The only place that actually posts a change to the main thread, and
   only when the flag genuinely flips -- idempotent the same way
   arktube_sync_no_internet_overlay() on Linux only acts when the
   overlay's current state disagrees with cs->is_online, just moved
   here since Windows has no main-thread-owned struct a background
   thread could safely read back from to compare against (unlike GLib's
   g_idle_add(), which runs this struct's reads and writes on the main
   thread only -- see connectivity.h's comment on field ownership). The
   window proc (main.cpp) is the one source of truth for the last state
   it applied, via its own static bool, so a redundant post here is
   merely wasted work, never incorrect. */
DWORD WINAPI ArktubeConnectivityThreadProc(LPVOID param) {
    auto *state = static_cast<ArktubeConnectivityState *>(param);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 1;
    }

    for (;;) {
        bool online = ArktubeCheckInternetNow();
        PostMessageW(
            state->mainWindow, WM_ARKTUBE_CONNECTIVITY_CHANGED,
            online ? TRUE : FALSE, 0);

        Sleep(online ? ARKTUBE_CONNECTIVITY_MONITOR_INTERVAL_MS
                     : ARKTUBE_CONNECTIVITY_RETRY_INTERVAL_MS);
    }

    // Unreachable: the loop above never breaks, matching
    // arktube_connectivity_thread()'s own infinite for(;;) on Linux --
    // the thread runs for the process's whole life and is torn down
    // implicitly on exit, not joined.
    WSACleanup();
    return 0;
}
