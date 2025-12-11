#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "minictl.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr,
            "Usage:\n"
            "  %s chroot <rootfs> <cmd> [args...]\n"
            "  %s run [--hostname=NAME] [--mem-limit=BYTES|XM|XG] [--cpu-limit=PCT] <rootfs> <cmd> [args...]\n"
            "  %s run-image <image-name> [--hostname=...] [--mem-limit=...] [--cpu-limit=...]\n",
            argv[0], argv[0], argv[0]);
        exit(1);
    }

    // ----------------------
    // PART 1: chroot mode
    // ----------------------
    if (strcmp(argv[1], "chroot") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s chroot <rootfs> <cmd> [args...]\n", argv[0]);
            exit(1);
        }
        return cmd_chroot(argv[2], &argv[3]);
    }

    // ----------------------
    // PART 2–3: run mode
    // ----------------------
    if (strcmp(argv[1], "run") == 0) {
        struct run_opts opts;
        memset(&opts, 0, sizeof(opts));

        opts.hostname = NULL;
        opts.cpu_limit = NULL;
        opts.mem_limit = NULL;

        int i = 2;

        // Parse optional flags
        for (; i < argc; i++) {
            if (strncmp(argv[i], "--hostname=", 11) == 0) {
                opts.hostname = argv[i] + 11;
            } else if (strncmp(argv[i], "--mem-limit=", 12) == 0) {
                opts.mem_limit = argv[i] + 12;
            } else if (strncmp(argv[i], "--cpu-limit=", 12) == 0) {
                opts.cpu_limit = argv[i] + 12;
            } else {
                break;
            }
        }

        if (i >= argc - 1) {
            fprintf(stderr, "Usage: %s run [flags] <rootfs> <cmd> [args...]\n", argv[0]);
            exit(1);
        }

        opts.rootfs = argv[i];
        opts.argv = &argv[i + 1];

        return cmd_run(&opts);
    }

    // ----------------------
    // PART 4: run-image mode
    // ----------------------
    if (strcmp(argv[1], "run-image") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s run-image <image-name> [... flags ...]\n", argv[0]);
            exit(1);
        }

        struct run_opts override;
        memset(&override, 0, sizeof(override));
        override.hostname = NULL;
        override.cpu_limit = NULL;
        override.mem_limit = NULL;

        // Parse overrides after image-name
        for (int j = 3; j < argc; j++) {
            if (strncmp(argv[j], "--hostname=", 11) == 0) {
                override.hostname = argv[j] + 11;
            } else if (strncmp(argv[j], "--mem-limit=", 12) == 0) {
                override.mem_limit = argv[j] + 12;
            } else if (strncmp(argv[j], "--cpu-limit=", 12) == 0) {
                override.cpu_limit = argv[j] + 12;
            }
        }

        return cmd_run_image(argv[2], &override);
    }

    fprintf(stderr, "Unknown command: %s\n", argv[1]);
    exit(1);
}
