#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "minictl.h"

/*
 * util.c
 *
 * Small helper functions:
 *   - die(): print error and exit.
 *   - write_string_to_file(): convenience wrapper for config/cgroup writes.
 *   - parse_mem_string(): parse memory strings like "256M", "1G", or plain bytes.
 */

void die(const char *msg) {
    perror(msg);
    exit(1);
}

int write_string_to_file(const char *path, const char *s) {
    FILE *f = fopen(path, "w");
    if (!f) {
        return -1;
    }
    if (fputs(s, f) < 0) {
        fclose(f);
        return -1;
    }
    if (fclose(f) != 0) {
        return -1;
    }
    return 0;
}

/*
 * parse_mem_string("256M") -> 268435456
 * parse_mem_string("1G")   -> 1073741824
 * parse_mem_string("1234") -> 1234
 *
 * Returns -1 on parse error.
 */
long parse_mem_string(const char *s) {
    if (!s || !*s) return -1;

    char *end = NULL;
    long val = strtol(s, &end, 10);
    if (end == s) return -1; // no digits

    if (*end == 'M' || *end == 'm') {
        val *= 1024L * 1024L;
        end++;
    } else if (*end == 'G' || *end == 'g') {
        val *= 1024L * 1024L * 1024L;
        end++;
    }

    if (*end != '\0') {
        // Unexpected trailing characters; treat as error for simplicity.
        return -1;
    }
    return val;
}
