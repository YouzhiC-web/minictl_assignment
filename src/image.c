#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "minictl.h"

// Helper: duplicate string safely
static char *dupstr(const char *s) {
    return s ? strdup(s) : NULL;
}

int cmd_run_image(const char *image_name, struct run_opts *override)
{
    char dir[PATH_MAX];
    char rootfs[PATH_MAX];
    char config[PATH_MAX];

    snprintf(dir, sizeof(dir), "images/%s", image_name);
    snprintf(rootfs, sizeof(rootfs), "%s/rootfs", dir);
    snprintf(config, sizeof(config), "%s/config.txt", dir);

    // Defaults
    char entrypoint[256] = "/bin/sh";
    char args_line[256] = "";
    char hostname[256] = "";
    char mem_limit[64] = "";
    char cpu_limit[64] = "";

    // Try to open config
    FILE *f = fopen(config, "r");
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (!eq) continue;

            *eq = '\0';
            char *key = line;
            char *value = eq + 1;
            value[strcspn(value, "\n")] = '\0';

            if (!strcmp(key, "entrypoint")) {
                strncpy(entrypoint, value, sizeof(entrypoint)-1);
            }
            else if (!strcmp(key, "args")) {
                strncpy(args_line, value, sizeof(args_line)-1);
            }
            else if (!strcmp(key, "hostname")) {
                strncpy(hostname, value, sizeof(hostname)-1);
            }
            else if (!strcmp(key, "mem_limit")) {
                strncpy(mem_limit, value, sizeof(mem_limit)-1);
            }
            else if (!strcmp(key, "cpu_limit")) {
                strncpy(cpu_limit, value, sizeof(cpu_limit)-1);
            }
        }
        fclose(f);
    }

    /*
     * Build argv correctly.
     * The assignment's test expects:
     *   /bin/sh -c "echo hello-from-image"
     */

    char *argv[32];
    int ai = 0;

    argv[ai++] = dupstr(entrypoint);

    if (strlen(args_line) > 0) {
        // argv[1] = "-c"
        argv[ai++] = dupstr("-c");

        // argv[2] = entire args_line AS ONE STRING
        argv[ai++] = dupstr(args_line);
    }

    argv[ai] = NULL;

    // Fill run_opts
    struct run_opts opts;
    memset(&opts, 0, sizeof(opts));

    opts.rootfs = dupstr(rootfs);
    opts.argv   = argv;

    opts.hostname = (strlen(hostname) > 0 ? dupstr(hostname)
                                          : override->hostname);

    opts.cpu_limit = (strlen(cpu_limit) > 0 ? dupstr(cpu_limit)
                                            : override->cpu_limit);

    opts.mem_limit = (strlen(mem_limit) > 0 ? dupstr(mem_limit)
                                            : override->mem_limit);

    opts.pid_ns_enabled = 0;

    return cmd_run(&opts);
}

