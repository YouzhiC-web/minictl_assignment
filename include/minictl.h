#ifndef MINICTL_H
#define MINICTL_H

#include <sys/types.h>

/*
 * Shared options for running a container-like environment.
 * You will gradually fill in the behavior that uses this struct.
 */
struct run_opts {
    const char *rootfs;      // path to rootfs
    char **argv;             // command + args
    const char *hostname;    // --hostname=

    const char *cpu_limit;   // --cpu-limit=
    const char *mem_limit;   // --mem-limit=
};


/*
 * Part 1: Simple chroot-based sandbox.
 *
 * TODO (Part 1): Implement this in src/chroot_cmd.c
 *   - fork()
 *   - child: chdir(rootfs), chroot(rootfs), chdir("/"), execvp(argv[0], argv)
 *   - parent: waitpid()
 */
int cmd_chroot(const char *rootfs, char **argv);

/*
 * Part 2–3: Namespace-based container runner with optional cgroup limits.
 *
 * TODO (Part 2): Implement this in src/run_cmd.c
 *   - Use clone() with:
 *       CLONE_NEWUSER | CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD
 *   - In parent:
 *       write uid_map, gid_map, setgroups for child PID
 *   - In child:
 *       sethostname()
 *       set up mount namespace, switch to rootfs, mount /proc
 *       execvp(opts->argv[0], opts->argv)
 *
 * TODO (Part 3): Integrate cgroups by calling cgroup_setup() before waitpid().
 */
int cmd_run(struct run_opts *opts);

/*
 * Part 4: Run an image by name.
 *
 * TODO (Part 4): Implement this in src/image.c
 *   - Resolve images/<name>/rootfs and images/<name>/config.txt
 *   - Parse config file (entrypoint, args, mem_limit, cpu_limit, hostname)
 *   - Build a run_opts struct and call cmd_run().
 */
int cmd_run_image(const char *image_name, struct run_opts *override);

/*
 * Part 3 helper: set up a cgroup for a given pid.
 *
 * TODO (Part 3): Implement this in src/cgroup.c
 *   - Create /sys/fs/cgroup/minictl-<pid> directory.
 *   - If mem_bytes > 0: write to memory.max.
 *   - If cpu_percent > 0: compute cpu.max = "<quota> <period>" and write.
 *   - Write pid into cgroup.procs.
 */
int cgroup_setup(pid_t pid, struct run_opts *opts);


/*
 * Utility helpers declared here, implemented in src/util.c
 */
void die(const char *msg);
int write_string_to_file(const char *path, const char *s);
long parse_mem_string(const char *s);  // e.g., "256M", "1G", or plain bytes

#endif // MINICTL_H
