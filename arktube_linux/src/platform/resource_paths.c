#include "resource_paths.h"

#include <limits.h>
#include <unistd.h>

gchar *arktube_find_resource(const char *relative) {
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

    if (len > 0) {
        exe_path[len] = '\0';
        gchar *exe_dir = g_path_get_dirname(exe_path);

        gchar *candidate = g_build_filename(exe_dir, "resources", relative, NULL);
        if (g_file_test(candidate, G_FILE_TEST_EXISTS)) {
            g_free(exe_dir);
            return candidate;
        }
        g_free(candidate);

        candidate = g_build_filename(
            exe_dir, "..", "share", "arktube_linux", "resources", relative, NULL);
        if (g_file_test(candidate, G_FILE_TEST_EXISTS)) {
            g_free(exe_dir);
            return candidate;
        }
        g_free(candidate);
        g_free(exe_dir);
    }

    gchar *cwd_candidate = g_build_filename("resources", relative, NULL);
    if (g_file_test(cwd_candidate, G_FILE_TEST_EXISTS)) {
        return cwd_candidate;
    }
    g_free(cwd_candidate);

    return NULL;
}
