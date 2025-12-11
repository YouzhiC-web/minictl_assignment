#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include "minictl.h"

/*
 * Part 4 — Run an image:
 *
 * images/<image_name>/rootfs
 * images/<image_name>/config.txt
 *
 * config.txt may contain:
 *   entrypoint=/bin/sh
 *   args=-c echo hello
 *   hostname=mycontainer
 *   mem_limit=256M
 *   cpu_limit=20
 *
 * Command-line overrides take precedence.
 */

static char *safe_strdup(const char *s) {
    return s ? strdup(s) : NULL;
}

int cmd_run_image(const char *image_name, struct run_opts *override)
{
    char image_dir[PATH_MAX];
    char rootfs[PATH_MAX];
    char config_path[PATH_MAX];

    snprintf(image_dir, sizeof(image_dir), "images/%s", image_name);
    snprintf(rootfs, sizeof(rootfs), "%s/rootfs", image_dir);
    snprintf(config_path, sizeof(config_path), "%s/config.txt", image_dir);

    // ==== DEFAULT VALUES ====
    char *entrypoint = safe_strdup("/bin/sh");
    char *args = NULL;
    char *hostname = NULL;
    char *cpu_limit = NULL;
    char *mem_limit = NULL;

    // ==== READ CONFIG FILE ====
    FILE *f = fopen(config_path, "r");
    if (f) {
        char line[1024];

        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (!eq) continue;

            *eq = '\0';
            char *key = line;
            char *value = eq + 1;

            // Remove trailing newline
            value[strcspn(value, "\r\n")] = '\0';

            if (strcmp(key, "entrypoint") == 0) {
                free(entrypoint);
                entrypoint = safe_strdup(value);
            }
            else if (strcmp(key, "args") == 0) {
                free(args);
                args = safe_strdup(value);
            }
            else if (strcmp(key, "hostname") == 0) {
                free(hostname);
                hostname = safe_strdup(value);
            }
            else if (strcmp(key, "cpu_limit") == 0) {
                free(cpu_limit);
                cpu_limit = safe_strdup(value);
            }
            else if (strcmp(key, "mem_limit") == 0) {
                free(mem_limit);
                mem_limit = safe_strdup(value);
            }
        }

        fclose(f);
    }

    // ==== APPLY OVERRIDES ====
    if (override->hostname)
        hostname = safe_strdup(override->hostname);

    if (override->cpu_limit)
        cpu_limit = safe_strdup(override->cpu_limit);

    if (override->mem_limit)
        mem_limit = safe_strdup(override->mem_limit);

    // ==== BUILD ARGV ====
    // entrypoint + args string → split into tokens

    int argc = 1;
    char *tmp = NULL;

    // Duplicate args string since strtok mutates
    char *args_copy = args ? safe_strdup(args) : NULL;

    // Count arguments
    if (args_copy) {
        tmp = strtok(args_copy, " ");
        while (tmp) {
            argc++;
            tmp = strtok(NULL, " ");
        }
        free(args_copy);
    }

    // Allocate argv array (+1 for NULL terminator)
    char **argv = calloc(argc + 1, sizeof(char*));
    if (!argv) {
        perror("calloc argv");
        return 1;
    }

    // argv[0] = entrypoint
    argv[0] = safe_strdup(entrypoint);

    // Tokenize a second time to populate
    int pos = 1;
    if (args) {
        char *tok;
        char *copy2 = safe_strdup(args);

        tok = strtok(copy2, " ");
        while (tok) {
            argv[pos++] = safe_strdup(tok);
            tok = strtok(NULL, " ");
        }
        free(copy2);
    }

    argv[pos] = NULL;

    // ==== PREPARE run_opts ====

    struct run_opts opts = {
        .rootfs = rootfs,
        .argv = argv,
        .hostname = hostname,
        .cpu_limit = cpu_limit,
        .mem_limit = mem_limit
    };

    // ==== RUN CONTAINER ====
    int result = cmd_run(&opts);

    // Cleanup memory
    for (int i = 0; i < pos; i++)
        free(argv[i]);
    free(argv);

    free(entrypoint);
    free(args);
    free(hostname);
    free(cpu_limit);
    free(mem_limit);

    return result;
}
