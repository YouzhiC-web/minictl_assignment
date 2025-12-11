#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "minictl.h"

void die(const char *msg) {
    perror(msg);
    exit(1);
}

int write_string_to_file(const char *path, const char *s) {
    FILE *f = fopen(path, "w");
    if (!f)
        return -1;
    if (fputs(s, f) < 0) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

long parse_mem_string(const char *s) {
    long val = atol(s);
    size_t len = strlen(s);

    if (len > 1) {
        char suffix = s[len - 1];
        if (suffix == 'M' || suffix == 'm')
            return val * 1024 * 1024;
        if (suffix == 'G' || suffix == 'g')
            return val * 1024L * 1024L * 1024L;
    }
    return val;
}

