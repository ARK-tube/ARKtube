#include "settings.h"

#include "../config.h"

/* Path to the persisted config file, creating its parent directory if
   needed (g_key_file_save_to_file() does not create directories).
   Returns a newly allocated path the caller must g_free(). */
static gchar *arktube_config_path(void) {
    gchar *dir = g_build_filename(g_get_user_config_dir(), ARKTUBE_CONFIG_DIR_NAME, NULL);
    g_mkdir_with_parents(dir, 0700);

    gchar *path = g_build_filename(dir, ARKTUBE_CONFIG_FILE_NAME, NULL);
    g_free(dir);
    return path;
}

gboolean arktube_load_fullscreen_pref(void) {
    gchar *path = arktube_config_path();

    GKeyFile *keyfile = g_key_file_new();
    gboolean loaded = g_key_file_load_from_file(keyfile, path, G_KEY_FILE_NONE, NULL);

    gboolean fullscreen = FALSE;
    if (loaded) {
        GError *error = NULL;
        fullscreen = g_key_file_get_boolean(
            keyfile, ARKTUBE_CONFIG_GROUP, ARKTUBE_CONFIG_KEY_FULLSCREEN, &error);
        if (error) {
            fullscreen = FALSE;
            g_clear_error(&error);
        }
    }

    g_key_file_free(keyfile);
    g_free(path);
    return fullscreen;
}

void arktube_save_fullscreen_pref(gboolean fullscreen) {
    gchar *path = arktube_config_path();

    GKeyFile *keyfile = g_key_file_new();
    g_key_file_load_from_file(keyfile, path, G_KEY_FILE_NONE, NULL);
    g_key_file_set_boolean(
        keyfile, ARKTUBE_CONFIG_GROUP, ARKTUBE_CONFIG_KEY_FULLSCREEN, fullscreen);

    GError *error = NULL;
    if (!g_key_file_save_to_file(keyfile, path, &error)) {
        g_warning("ARKtube: could not save '%s': %s", path,
                  error ? error->message : "unknown error");
        g_clear_error(&error);
    }

    g_key_file_free(keyfile);
    g_free(path);
}
