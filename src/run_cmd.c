#define _GNU_SOURCE
#include <sched.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "minictl.h"

#define STACK_SIZE (1024 * 1024)

struct child_ctx {
    struct run_opts *opts;
    int rootfs_fd;      // <-- FD to rootfs directory
};

/*
 * CHILD PROCESS
 * Runs inside new namespaces. Never returns.
 */
static int child_main(void *arg) {
    struct child_ctx *ctx = (struct child_ctx *)arg;
    struct run_opts *opts = ctx->opts;

    // ---- Stop until parent sets UID/GID maps ----
    if (raise(SIGSTOP) != 0) {
        perror("raise(SIGSTOP)");
        _exit(1);
    }

    // -------- 1. Set hostname (UTS namespace) --------
    if (opts->hostname) {
        if (sethostname(opts->hostname, strlen(opts->hostname)) < 0) {
            perror("sethostname");
            _exit(1);
        }
    }

    // -------- 2. Make mount namespace private --------
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0) {
        perror("mount private");
        _exit(1);
    }

    // -------- 3. Use saved FD to enter rootfs --------
    if (fchdir(ctx->rootfs_fd) < 0) {
        perror("fchdir rootfs_fd");
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

    // -------- 4. Mount /proc inside container --------
    if (mount("proc", "/proc", "proc", 0, NULL) < 0) {
        perror("mount /proc");
        _exit(1);
    }

    // -------- 5. Exec the command --------
    execvp(opts->argv[0], opts->argv);

    perror("execvp");
    _exit(1);
}


/*
 * Parent-side: setup user namespace
 */
static int setup_user_namespace(pid_t child_pid) {
    char path[256];
    char buf[256];

    uid_t uid = getuid();
    gid_t gid = getgid();

    // 1. Write "deny" to setgroups
    snprintf(path, sizeof(path), "/proc/%d/setgroups", child_pid);
    if (write_string_to_file(path, "deny\n") < 0) {
        perror("write setgroups");
        return -1;
    }

    // 2. uid_map
    snprintf(path, sizeof(path), "/proc/%d/uid_map", child_pid);
    snprintf(buf, sizeof(buf), "0 %d 1\n", uid);
    if (write_string_to_file(path, buf) < 0) {
        perror("write uid_map");
        return -1;
    }

    // 3. gid_map
    snprintf(path, sizeof(path), "/proc/%d/gid_map", child_pid);
    snprintf(buf, sizeof(buf), "0 %d 1\n", gid);
    if (write_string_to_file(path, buf) < 0) {
        perror("write gid_map");
        return -1;
    }

    return 0;
}


/*
 * MAIN: cmd_run
 */
int cmd_run(struct run_opts *opts) {
    // ---- Open rootfs directory BEFORE clone ----
    int rootfs_fd = open(opts->rootfs, O_RDONLY | O_DIRECTORY);
    if (rootfs_fd < 0) {
        perror("open rootfs");
        return 1;
    }

    // ---- Allocate stack ----
    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc stack");
        close(rootfs_fd);
        return 1;
    }
    char *stack_top = stack + STACK_SIZE;

    // ---- Prepare context ----
    struct child_ctx ctx;
    ctx.opts = opts;
    ctx.rootfs_fd = rootfs_fd;

    int flags = SIGCHLD
              | CLONE_NEWUSER      // userns (needs stop + mapping)
              | CLONE_NEWUTS       // hostname
              | CLONE_NEWPID       // PID 1 inside container
              | CLONE_NEWNS;       // mount namespace

    // ---- Create child ----
    pid_t child_pid = clone(child_main, stack_top, flags, &ctx);
    if (child_pid < 0) {
        perror("clone");
        free(stack);
        close(rootfs_fd);
        return 1;
    }

    // ---- Wait for SIGSTOP from child ----
    int status;
    if (waitpid(child_pid, &status, WUNTRACED) < 0) {
        perror("waitpid WUNTRACED");
        free(stack);
        close(rootfs_fd);
        return 1;
    }

    // ---- Write UID/GID mappings ----
    if (setup_user_namespace(child_pid) < 0) {
        fprintf(stderr, "Failed to set up user namespace\n");
    }

    // -------- PART 3: Setup cgroups --------
    if (cgroup_setup(child_pid, opts) < 0) {
        fprintf(stderr, "Warning: failed to configure cgroup limits\n");
    }

    // ---- Resume child ----
    if (kill(child_pid, SIGCONT) < 0) {
        perror("kill SIGCONT");
        free(stack);
        close(rootfs_fd);
        return 1;
    }

    // ---- Wait for normal exit ----
    if (waitpid(child_pid, &status, 0) < 0) {
        perror("waitpid");
        free(stack);
        close(rootfs_fd);
        return 1;
    }

    free(stack);
    close(rootfs_fd);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);

    return 1;
}
