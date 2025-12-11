#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "minictl.h"

/*
 * All cgroup v2 controllers live under:
 *     /sys/fs/cgroup/minictl/
 *
 * For each run, we create:
 *     /sys/fs/cgroup/minictl/<pid>/
 *
 * Inside that:
 *     cgroup.procs        <-- add PID
 *     memory.max          <-- mem limit
 *     cpu.max             <-- cpu limit
 *
 */

static int mkdir_p(const char *path, mode_t mode) {
    if (mkdir(path, mode) == 0)
        return 0;

    if (errno == EEXIST)
        return 0;

    return -1;
}

/*
 * Convert --mem-limit format:
 *   100000000 (bytes)
 *   200M
 *   1G
 */
static long parse_memory(const char *s) {
    long val = 0;
    char unit = 0;

    if (sscanf(s, "%ld%c", &val, &unit) == 2) {
        switch (unit) {
            case 'G': case 'g': val *= 1024L * 1024L * 1024L; break;
            case 'M': case 'm': val *= 1024L * 1024L; break;
            case 'K': case 'k': val *= 1024L; break;
            default: break;
        }
    } else {
        val = atol(s);
    }

    return val;
}

/*
 * Parse CPU limit:
 *   --cpu-limit=50
 *
 * cgroup v2 cpu.max expects:
 *   "quota period"
 *
 * We use:
 *   period = 100000 (100ms)
 *   quota = period * pct / 100
 */
static int write_cpu_limit(int fd, const char *pct_str) {
    int pct = atoi(pct_str);

    if (pct <= 0 || pct > 100)
        return -1;

    long period = 100000;             // 100ms
    long quota = (period * pct) / 100;

    char buf[64];
    snprintf(buf, sizeof(buf), "%ld %ld\n", quota, period);

    if (write(fd, buf, strlen(buf)) < 0)
        return -1;

    return 0;
}

int cgroup_setup(pid_t pid, struct run_opts *opts) {
    char base[256];
    char cgpath[256];
    char file[256];

    // Root cgroup directory
    snprintf(base, sizeof(base), "/sys/fs/cgroup/minictl");
    if (mkdir_p(base, 0755) < 0) {
        perror("mkdir base cgroup");
        return -1;
    }

    // Per-process sub-cgroup
    snprintf(cgpath, sizeof(cgpath), "%s/%d", base, pid);
    if (mkdir_p(cgpath, 0755) < 0) {
        perror("mkdir pid cgroup");
        return -1;
    }

    //
    // 1. CPU limit
    //
    if (opts->cpu_limit) {
        snprintf(file, sizeof(file), "%s/cpu.max", cgpath);
        int fd = open(file, O_WRONLY);
        if (fd < 0) {
            perror("open cpu.max");
            return -1;
        }

        if (write_cpu_limit(fd, opts->cpu_limit) < 0) {
            perror("write cpu limit");
            close(fd);
            return -1;
        }
        close(fd);
    }

    //
    // 2. Memory limit
    //
    if (opts->mem_limit) {
        long bytes = parse_memory(opts->mem_limit);

        snprintf(file, sizeof(file), "%s/memory.max", cgpath);
        int fd = open(file, O_WRONLY);
        if (fd < 0) {
            perror("open memory.max");
            return -1;
        }

        char buf[64];
        snprintf(buf, sizeof(buf), "%ld\n", bytes);

        if (write(fd, buf, strlen(buf)) < 0) {
            perror("write memory limit");
            close(fd);
            return -1;
        }

        close(fd);
    }

    //
    // 3. Add child PID to cgroup.procs
    //
    snprintf(file, sizeof(file), "%s/cgroup.procs", cgpath);
    int fd = open(file, O_WRONLY);
    if (fd < 0) {
        perror("open cgroup.procs");
        return -1;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%d\n", pid);

    if (write(fd, buf, strlen(buf)) < 0) {
        perror("write cgroup.procs");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}
