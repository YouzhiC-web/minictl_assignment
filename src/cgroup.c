#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "minictl.h"

int cgroup_setup(pid_t pid, struct run_opts *opts) {
    char cgpath[256];
    char file[256];
    
    // Create the cgroup directory
    snprintf(cgpath, sizeof(cgpath), "/sys/fs/cgroup/minictl-%d", pid);
    if (mkdir(cgpath, 0755) < 0) {
        perror("mkdir cgroup");
        return -1;
    }

    // Set memory limit if specified
    if (opts->mem_limit) {
        snprintf(file, sizeof(file), "%s/memory.max", cgpath);
        long mem_bytes = parse_mem_string(opts->mem_limit);
        if (mem_bytes > 0) {
            FILE *f = fopen(file, "w");
            if (!f) {
                perror("open memory.max");
                return -1;
            }
            fprintf(f, "%ld\n", mem_bytes);
            fclose(f);
        }
    }

    // Set CPU limit if specified
    if (opts->cpu_limit) {
        snprintf(file, sizeof(file), "%s/cpu.max", cgpath);
        long cpu_percent = atoi(opts->cpu_limit);
        if (cpu_percent > 0) {
            FILE *f = fopen(file, "w");
            if (!f) {
                perror("open cpu.max");
                return -1;
            }
            long period = 100000; // default period (in microseconds)
            long quota = (cpu_percent * period) / 100;
            fprintf(f, "%ld %ld\n", quota, period);
            fclose(f);
        }
    }

    // Add the process to the cgroup
    snprintf(file, sizeof(file), "%s/cgroup.procs", cgpath);
    FILE *f = fopen(file, "w");
    if (!f) {
        perror("open cgroup.procs");
        return -1;
    }
    fprintf(f, "%d\n", pid);
    fclose(f);

    return 0;
}

