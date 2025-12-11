/*
 * mem_hog.c
 *
 * Allocate memory in chunks until allocation fails, to test memory limits.
 * IMPORTANT: We touch each chunk so that cgroup memory.current actually increases.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK (10 * 1024 * 1024) // 10 MB per chunk

int main(void) {
    printf("Starting memory hog...\n");
    size_t total = 0;

    while (1) {
        void *p = malloc(CHUNK);
        if (!p) {
            printf("Allocation failed after %zu MB\n", total / (1024 * 1024));
            break;
        }

        // Touch the memory so the kernel must back it with real pages.
        memset(p, 0xAA, CHUNK);

        total += CHUNK;
        printf("Allocated %zu MB\n", total / (1024 * 1024));
    }

    printf("Exiting mem_hog.\n");
    return 0;
}
