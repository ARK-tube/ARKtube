#include "webview.h"

#include "../config.h"

void arktube_webview_inject_user_script(WebKitUserContentManager *ucm, const char *path) {
    gchar *contents = NULL;
    gsize length = 0;
    GError *error = NULL;

    if (!g_file_get_contents(path, &contents, &length, &error)) {
        g_warning("ARKtube: could not read user script '%s': %s", path,
                   error ? error->message : "unknown error");
        g_clear_error(&error);
        return;
    }

    WebKitUserScript *script = webkit_user_script_new(
        contents,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        NULL, NULL);
    webkit_user_content_manager_add_script(ucm, script);
    webkit_user_script_unref(script);
    g_free(contents);
}

GtkWidget *arktube_webview_new(WebKitUserContentManager *ucm) {
    GtkWidget *webview = webkit_web_view_new_with_user_content_manager(ucm);
    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview));

    /* Full replacement, matching the old shell's "chrome mode" user
       agent so YouTube's server-side device detection serves the same
       TV/Leanback interface this app is built around. */
    webkit_settings_set_user_agent(settings, ARKTUBE_USER_AGENT);
    webkit_settings_set_enable_developer_extras(settings, TRUE);
    webkit_settings_set_enable_media_stream(settings, TRUE);
    webkit_settings_set_enable_webaudio(settings, TRUE);

    /* ALWAYS is WebKitGTK's own default since 2.16, but state it
       explicitly so a future WebKit default change, or a distro/session
       compositing override, can't silently fall back to software
       rendering underneath us -- that's the single biggest source of
       the stutter this app is built to avoid. See
       docs/HARDWARE-ACCELERATION.md for the system packages (GStreamer
       VA-API) this still depends on for smooth video decode, and the
       WEBKIT_DISABLE_DMABUF_RENDERER escape hatch for the rarer case
       where accelerated compositing itself is the problem. */
    webkit_settings_set_hardware_acceleration_policy(
        settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);

    return webview;
}
