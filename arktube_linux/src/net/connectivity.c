#include "connectivity.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../config.h"
#include "../ui/no_internet_overlay.h"

/* Internet reachability check via a raw non-blocking connect(2) to
   ARKTUBE_CONNECTIVITY_HOST:ARKTUBE_CONNECTIVITY_PORT, bounded by
   poll(2) so a silently-dropping network can't hang this past
   ARKTUBE_CONNECTIVITY_TIMEOUT_MS. Deliberately just the raw BSD
   socket syscalls instead of asking WebKit to load the real page and
   inferring the answer from its failure mode -- this is meant to run
   in a background thread, repeatedly, well before (and independently
   of) any WebView load. Always called off the main thread; nothing
   here touches GTK. */
static gboolean arktube_check_internet_now(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return FALSE;
    }

    int flags = fcntl(sock, F_GETFL, 0);
    if (flags != -1) {
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ARKTUBE_CONNECTIVITY_PORT);
    if (inet_pton(AF_INET, ARKTUBE_CONNECTIVITY_HOST, &addr.sin_addr) != 1) {
        close(sock);
        return FALSE;
    }

    gboolean online = FALSE;
    int rc = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == 0) {
        online = TRUE;
    } else if (errno == EINPROGRESS) {
        struct pollfd pfd = { .fd = sock, .events = POLLOUT, .revents = 0 };
        int pr = poll(&pfd, 1, ARKTUBE_CONNECTIVITY_TIMEOUT_MS);
        if (pr > 0 && (pfd.revents & POLLOUT)) {
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len) == 0 &&
                so_error == 0) {
                online = TRUE;
            }
        }
    }

    close(sock);
    return online;
}

/* The one place that actually shows or hides no_internet.png, driven
   purely off cs->is_online. Called after every flag change (both
   directions) so it's equally at home clearing the overlay a moment
   after boot as it is bringing it back mid-session: idempotent either
   way, since it only acts when the overlay's current on/off state
   disagrees with the flag. */
static void arktube_sync_no_internet_overlay(ArktubeConnectivityState *cs) {
    if (cs->is_online) {
        if (cs->no_internet_overlay) {
            gtk_widget_destroy(cs->no_internet_overlay);
            cs->no_internet_overlay = NULL;
        }
        return;
    }

    if (!cs->no_internet_overlay) {
        GtkWidget *overlay = arktube_create_no_internet_overlay();
        if (overlay) {
            gtk_overlay_add_overlay(GTK_OVERLAY(cs->root_overlay), overlay);
            gtk_widget_show_all(overlay);
            cs->no_internet_overlay = overlay;
        }
    }
}

/* Main-thread idle callback: flips the flag, then (for the very first
   success only) starts the real app -- the WebView load + boot splash.
   A later success, after the WebView is already up and a mid-session
   no_internet.png was showing over it, just clears that overlay and
   leaves the still-loaded page underneath alone -- there's no reason to
   reload it just because the connection blipped. */
static gboolean arktube_on_internet_ready(gpointer user_data) {
    ArktubeConnectivityState *cs = (ArktubeConnectivityState *)user_data;

    cs->is_online = TRUE;
    arktube_sync_no_internet_overlay(cs);

    if (!cs->webview_started) {
        cs->webview_started = TRUE;
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(cs->webview), ARKTUBE_URL);
        arktube_splash_show(cs->root_overlay, cs->splash_state);
    }

    return G_SOURCE_REMOVE;
}

/* Main-thread idle callback: flips the flag the other way and lets
   arktube_sync_no_internet_overlay() show the screen -- unconditionally
   now, not just before the first successful load, so a connection that
   disappears after boot (mid-video, mid-browse) is caught exactly the
   same way a genuinely offline boot always was. */
static gboolean arktube_on_internet_unavailable(gpointer user_data) {
    ArktubeConnectivityState *cs = (ArktubeConnectivityState *)user_data;

    cs->is_online = FALSE;
    arktube_sync_no_internet_overlay(cs);

    return G_SOURCE_REMOVE;
}

gpointer arktube_connectivity_thread(gpointer user_data) {
    ArktubeConnectivityState *cs = (ArktubeConnectivityState *)user_data;

    for (;;) {
        if (arktube_check_internet_now()) {
            g_idle_add(arktube_on_internet_ready, cs);
            g_usleep(ARKTUBE_CONNECTIVITY_MONITOR_INTERVAL_MS * 1000);
        } else {
            g_idle_add(arktube_on_internet_unavailable, cs);
            g_usleep(ARKTUBE_CONNECTIVITY_RETRY_INTERVAL_MS * 1000);
        }
    }

    return NULL; /* unreachable: the loop above never breaks */
}
