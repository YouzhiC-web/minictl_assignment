#define _GNU_SOURCE
#include <sched.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "minictl.h"

#define STACK_SIZE (1024 * 1024)

struct child_ctx {
    struct run_opts *opts;
    int rootfs_fd;
};

/* -------------------- CHILD FUNCTION ---------------------- */

static int child_main(void *arg) {
    struct child_ctx *ctx = arg;
    struct run_opts *opts = ctx->opts;

    /* 1. Stop immediately so parent can write uid/gid maps */
    if (raise(SIGSTOP) != 0) {
        perror("raise(SIGSTOP)");
        _exit(1);
    }

    /* 2. Set hostname */
    if (opts->hostname) {
        if (sethostname(opts->hostname, strlen(opts->hostname)) < 0) {
            perror("sethostname");
            _exit(1);
        }
    }

    /* 3. Make mounts private */
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0) {
        perror("mount private");
        _exit(1);
    }

    /* 4. Enter rootfs via fchdir + chroot(".") */
    if (fchdir(ctx->rootfs_fd) < 0) {
        perror("fchdir");
        _exit(1);
    }

    if (chroot(".") < 0) {
        perror("chroot");
        _exit(1);
    }

    if (chdir("/") < 0) {
        perror("chdir /");
        _exit(1);
    }

    /* 5. Ensure /proc exists */
    mkdir("/proc", 0555);

    /* 6. Mount /proc inside the new root */
    if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL) < 0) {
        perror("mount /proc");
        _exit(1);
    }

    /* 7. Execute command */
    execvp(opts->argv[0], opts->argv);
    perror("execvp");
    _exit(1);
}

/* -------------------- USERNS SETUP ---------------------- */

static int setup_user_namespace(pid_t pid) {
    char path[256];
    char buf[256];
    uid_t uid = getuid();
    gid_t gid = getgid();

    /* Write "deny" to setgroups first */
    snprintf(path, sizeof(path), "/proc/%d/setgroups", pid);
    write_string_to_file(path, "deny\n");

    /* UID map */
    snprintf(path, sizeof(path), "/proc/%d/uid_map", pid);
    snprintf(buf, sizeof(buf), "0 %d 1\n", uid);
    write_string_to_file(path, buf);

    /* GID map */
    snprintf(path, sizeof(path), "/proc/%d/gid_map", pid);
    snprintf(buf, sizeof(buf), "0 %d 1\n", gid);
    write_string_to_file(path, buf);

    return 0;
}

/* -------------------- MAIN CMD_RUN ---------------------- */

int cmd_run(struct run_opts *opts) {
    int rootfs_fd = open(opts->rootfs, O_RDONLY | O_DIRECTORY);
    if (rootfs_fd < 0) {
        perror("open rootfs");
        return 1;
    }

    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc stack");
        return 1;
    }
    char *stack_top = stack + STACK_SIZE;

    struct child_ctx ctx = {
        .opts = opts,
        .rootfs_fd = rootfs_fd
    };

    int flags = CLONE_NEWUSER | CLONE_NEWPID |
                CLONE_NEWNS | CLONE_NEWUTS | SIGCHLD;

    pid_t child_pid = clone(child_main, stack_top, flags, &ctx);
    if (child_pid < 0) {
        perror("clone");
        return 1;
    }

    /* Wait for SIGSTOP */
    int status;
    waitpid(child_pid, &status, WUNTRACED);

    /* Set up user namespace */
    setup_user_namespace(child_pid);

    /* Resume child */
    kill(child_pid, SIGCONT);

    /* Apply cgroups only if limits specified */
    if (opts->cpu_limit || opts->mem_limit) {
        cgroup_setup(child_pid, opts);
    }

    /* Wait for child to exit */
    waitpid(child_pid, &status, 0);

    free(stack);
    close(rootfs_fd);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);

    return 1;
}

