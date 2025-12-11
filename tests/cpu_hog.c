/*
 * cpu_hog.c
 *
 * Simple CPU-intensive loop to test CPU limits in cgroups.
 */
#include <stdio.h>

int main(void) {
    volatile unsigned long long x = 0;
    printf("Starting CPU hog (looping)...\n");
    while (1) {
        x++;
    }
    return 0;
}
