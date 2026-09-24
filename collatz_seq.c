/*
 * collatz_seq.c
 * -----------------------------------------------------------------------
 * Zeba Academy - Parallel Computing - Practicum: The Amdahl Reality Gap
 * Sequential baseline implementation (Phase 2)
 *
 * Student ID: 240103142  -> last 4 digits = 3142
 * N = 10,000,000 + (3142 * 1,000) = 13,142,000
 *
 * Compile (macOS, Apple Silicon, after `brew install libomp`):
 *   clang -O2 -Xpreprocessor -fopenmp \
 *         -I/opt/homebrew/opt/libomp/include \
 *         -L/opt/homebrew/opt/libomp/lib -lomp \
 *         collatz_seq.c -o collatz_seq
 *
 * (We still link against omp here only to use omp_get_wtime() for
 *  high-precision timing, even though this file runs single-threaded.)
 *
 * Run:
 *   ./collatz_seq
 * -----------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <omp.h>

#define N 13142000UL
#define MOD 1000000007UL

static inline uint32_t collatz_steps(uint64_t n) {
    uint32_t steps = 0;
    while (n > 1) {
        if ((n & 1) == 0) n >>= 1;
        else n = 3 * n + 1;
        steps++;
    }
    return steps;
}

int main(void) {
    uint64_t max_steps = 0;
    uint64_t max_i = 0;
    uint64_t checksum = 0;

    double t_start = omp_get_wtime();

    for (uint64_t i = 1; i <= N; i++) {
        uint32_t s = collatz_steps(i);
        if (s > max_steps) {
            max_steps = s;
            max_i = i;
        }
        checksum = (checksum + s) % MOD;
    }

    double t_end = omp_get_wtime();

    printf("N                = %lu\n", (unsigned long)N);
    printf("Max steps        = %lu (at i = %lu)\n",
           (unsigned long)max_steps, (unsigned long)max_i);
    printf("Checksum (mod)   = %lu\n", (unsigned long)checksum);
    printf("Elapsed time (s) = %.6f\n", t_end - t_start);

    return 0;
}
