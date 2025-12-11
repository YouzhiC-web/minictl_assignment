#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include "minictl.h"

int cmd_chroot(const char *rootfs, char **argv) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // CHILD
        if (chdir(rootfs) < 0) {
            perror("chdir rootfs");
            _exit(1);
        }
        if (chroot(rootfs) < 0) {
            perror("chroot");
            _exit(1);
        }
        if (chdir("/") < 0) {
            perror("chdir /");
            _exit(1);
        }

        execvp(argv[0], argv);
        perror("execvp");
        _exit(1);
    }

    // PARENT
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);

    return 1;
}
