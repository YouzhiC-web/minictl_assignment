#ifndef MINICTL_H
#define MINICTL_H

#include <sys/types.h>

struct run_opts {
    const char *rootfs;       // path to rootfs
    char **argv;              // command + args
    const char *hostname;     // --hostname=

    const char *cpu_limit;    // --cpu-limit=STRING (e.g., "50")
    const char *mem_limit;    // --mem-limit=STRING (e.g., "256M")
    int pid_ns_enabled;
};

int cmd_chroot(const char *rootfs, char **argv);
int cmd_run(struct run_opts *opts);
int cmd_run_image(const char *image_name, struct run_opts *override);
int cgroup_setup(pid_t pid, struct run_opts *opts);

void die(const char *msg);
int write_string_to_file(const char *path, const char *s);
long parse_mem_string(const char *s);

#endif

