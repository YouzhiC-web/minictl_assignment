#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include "minictl.h"

/*
 * Correct Part 1 implementation
 */

int cmd_chroot(const char *rootfs, char **argv) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // --- CHILD ---

        // Move into rootfs directory
        if (chdir(rootfs) < 0) {
            perror("chdir rootfs");
            _exit(1);
        }

        // chroot to the CURRENT directory (.) — not the path!
        if (chroot(".") < 0) {
            perror("chroot");
            _exit(1);
        }

        // Enter root directory of the chroot
        if (chdir("/") < 0) {
            perror("chdir /");
            _exit(1);
        }

        // Execute command
        execvp(argv[0], argv);
        perror("execvp");
        _exit(1);
    }

    // --- PARENT ---
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
}

