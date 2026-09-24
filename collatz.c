/*
 * collatz.c
 * -----------------------------------------------------------------------
 * Zeba Academy - Parallel Computing - Practicum: The Amdahl Reality Gap
 * OpenMP parallel implementation (Phase 3: scaling, Phase 4: false sharing
 * and scheduling experiments)
 *
 * Student ID: 240103142  -> last 4 digits = 3142
 * N = 10,000,000 + (3142 * 1,000) = 13,142,000
 *
 * Compile (macOS, Apple Silicon, after `brew install libomp`):
 *   clang -O2 -Xpreprocessor -fopenmp \
 *         -I/opt/homebrew/opt/libomp/include \
 *         -L/opt/homebrew/opt/libomp/lib -lomp \
 *         collatz.c -o collatz
 *
 * Usage:
 *   ./collatz <mode> <threads> [chunk_size]
 *
 *   mode:
 *     base        - plain parallel for, max+checksum only (Phase 3 scaling)
 *     naive       - Experiment A, Variant 1: false sharing via hit_count[tid]++
 *     reduction   - Experiment A, Variant 2: reduction(+:total_hits)
 *     padded      - Experiment A, Variant 2 (alt): cache-line padded struct
 *     sched       - Experiment B: pass schedule kind via chunk_size arg below
 *
 *   For mode=sched, pass an extra argument selecting the schedule:
 *     ./collatz sched <threads> static0        (schedule(static))
 *     ./collatz sched <threads> static1000      (schedule(static,1000))
 *     ./collatz sched <threads> dynamic100      (schedule(dynamic,100))
 *     ./collatz sched <threads> dynamic10000    (schedule(dynamic,10000))
 *     ./collatz sched <threads> guided          (schedule(guided))
 *
 * Examples:
 *   ./collatz base 4
 *   ./collatz naive 8
 *   ./collatz reduction 8
 *   ./collatz padded 8
 *   ./collatz sched 8 dynamic100
 * -----------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <omp.h>

#define N 13142000UL
#define MOD 1000000007UL
#define MAX_THREADS 64
#define HIT_THRESHOLD 100

static inline uint32_t collatz_steps(uint64_t n) {
    uint32_t steps = 0;
    while (n > 1) {
        if ((n & 1) == 0) n >>= 1;
        else n = 3 * n + 1;
        steps++;
    }
    return steps;
}

/* Cache-line padded counter: on Apple Silicon the physical cache line is
 * typically 128 bytes, not the textbook 64 bytes. We pad to 128 to be safe
 * on M-series chips; on x86 128 bytes still fully covers a 64-byte line. */
typedef struct {
    long count;
    char pad[128 - sizeof(long)];
} PaddedCounter;

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr,
            "Usage: %s <base|naive|reduction|padded|sched> <threads> [sched_kind]\n",
            argv[0]);
        return 1;
    }

    const char *mode = argv[1];
    int threads = atoi(argv[2]);
    const char *sched_kind = (argc >= 4) ? argv[3] : "static0";

    omp_set_num_threads(threads);

    uint64_t max_steps = 0;
    uint64_t checksum = 0;
    long total_hits = 0;

    double t_start, t_end;

    if (strcmp(mode, "base") == 0) {
        /* ---- Phase 3: plain scaling benchmark ---- */
        t_start = omp_get_wtime();

        #pragma omp parallel for reduction(max:max_steps) reduction(+:checksum) \
                                  schedule(static)
        for (uint64_t i = 1; i <= N; i++) {
            uint32_t s = collatz_steps(i);
            if (s > max_steps) max_steps = s;
            checksum = (checksum + s) % MOD;
        }

        t_end = omp_get_wtime();

        printf("[base] threads=%d max_steps=%lu checksum=%lu time=%.6f\n",
               threads, (unsigned long)max_steps, (unsigned long)checksum,
               t_end - t_start);

    } else if (strcmp(mode, "naive") == 0) {
        /* ---- Experiment A, Variant 1: FALSE SHARING ----
         * hit_count[tid]++ : each thread hammers its own slot in a shared
         * array. Adjacent slots (8 bytes apart for `int`... here `long`)
         * live on the SAME 64/128-byte cache line, so every increment
         * invalidates the line in every other core's cache (MESI
         * Invalid->Modified transitions), forcing constant bus traffic. */
        long hit_count[MAX_THREADS];
        memset(hit_count, 0, sizeof(hit_count));

        t_start = omp_get_wtime();

        #pragma omp parallel for reduction(max:max_steps) reduction(+:checksum) \
                                  schedule(static)
        for (uint64_t i = 1; i <= N; i++) {
            uint32_t s = collatz_steps(i);
            if (s > max_steps) max_steps = s;
            checksum = (checksum + s) % MOD;
            if (s > HIT_THRESHOLD) {
                hit_count[omp_get_thread_num()]++;   /* <-- false sharing site */
            }
        }

        t_end = omp_get_wtime();

        for (int t = 0; t < threads; t++) total_hits += hit_count[t];

        printf("[naive] threads=%d hits=%ld max_steps=%lu checksum=%lu time=%.6f\n",
               threads, total_hits, (unsigned long)max_steps,
               (unsigned long)checksum, t_end - t_start);

    } else if (strcmp(mode, "reduction") == 0) {
        /* ---- Experiment A, Variant 2a: OpenMP reduction ----
         * Each thread accumulates into a private register/stack copy,
         * combined once at the end. No shared cache line is touched
         * inside the hot loop. */
        t_start = omp_get_wtime();

        #pragma omp parallel for reduction(max:max_steps) reduction(+:checksum) \
                                  reduction(+:total_hits) schedule(static)
        for (uint64_t i = 1; i <= N; i++) {
            uint32_t s = collatz_steps(i);
            if (s > max_steps) max_steps = s;
            checksum = (checksum + s) % MOD;
            if (s > HIT_THRESHOLD) total_hits++;
        }

        t_end = omp_get_wtime();

        printf("[reduction] threads=%d hits=%ld max_steps=%lu checksum=%lu time=%.6f\n",
               threads, total_hits, (unsigned long)max_steps,
               (unsigned long)checksum, t_end - t_start);

    } else if (strcmp(mode, "padded") == 0) {
        /* ---- Experiment A, Variant 2b: cache-line padded struct ----
         * Each thread's counter is forced onto its own cache line via
         * padding, so increments never invalidate a neighbor's line. */
        static PaddedCounter counters[MAX_THREADS];
        for (int t = 0; t < MAX_THREADS; t++) counters[t].count = 0;

        t_start = omp_get_wtime();

        #pragma omp parallel for reduction(max:max_steps) reduction(+:checksum) \
                                  schedule(static)
        for (uint64_t i = 1; i <= N; i++) {
            uint32_t s = collatz_steps(i);
            if (s > max_steps) max_steps = s;
            checksum = (checksum + s) % MOD;
            if (s > HIT_THRESHOLD) {
                counters[omp_get_thread_num()].count++;
            }
        }

        t_end = omp_get_wtime();

        for (int t = 0; t < threads; t++) total_hits += counters[t].count;

        printf("[padded] threads=%d hits=%ld max_steps=%lu checksum=%lu time=%.6f\n",
               threads, total_hits, (unsigned long)max_steps,
               (unsigned long)checksum, t_end - t_start);

    } else if (strcmp(mode, "sched") == 0) {
        /* ---- Experiment B: scheduling clause comparison ----
         * We can't select a runtime schedule from a variable with a plain
         * `schedule()` clause portably without OMP_SCHEDULE, so we branch
         * over five compiled variants selected by sched_kind. */
        t_start = omp_get_wtime();

        if (strcmp(sched_kind, "static0") == 0) {
            #pragma omp parallel for reduction(max:max_steps) reduction(+:checksum) \
                                      schedule(static)
            for (uint64_t i = 1; i <= N; i++) {
                uint32_t s = collatz_steps(i);
                if (s > max_steps) max_steps = s;
                checksum = (checksum + s) % MOD;
            }
        } else if (strcmp(sched_kind, "static1000") == 0) {
            #pragma omp parallel for reduction(max:max_steps) reduction(+:checksum) \
                                      schedule(static, 1000)
            for (uint64_t i = 1; i <= N; i++) {
                uint32_t s = collatz_steps(i);
                if (s > max_steps) max_steps = s;
                checksum = (checksum + s) % MOD;
            }
        } else if (strcmp(sched_kind, "dynamic100") == 0) {
            #pragma omp parallel for reduction(max:max_steps) reduction(+:checksum) \
                                      schedule(dynamic, 100)
            for (uint64_t i = 1; i <= N; i++) {
                uint32_t s = collatz_steps(i);
                if (s > max_steps) max_steps = s;
                checksum = (checksum + s) % MOD;
            }
        } else if (strcmp(sched_kind, "dynamic10000") == 0) {
            #pragma omp parallel for reduction(max:max_steps) reduction(+:checksum) \
                                      schedule(dynamic, 10000)
            for (uint64_t i = 1; i <= N; i++) {
                uint32_t s = collatz_steps(i);
                if (s > max_steps) max_steps = s;
                checksum = (checksum + s) % MOD;
            }
        } else if (strcmp(sched_kind, "guided") == 0) {
            #pragma omp parallel for reduction(max:max_steps) reduction(+:checksum) \
                                      schedule(guided)
            for (uint64_t i = 1; i <= N; i++) {
                uint32_t s = collatz_steps(i);
                if (s > max_steps) max_steps = s;
                checksum = (checksum + s) % MOD;
            }
        } else {
            fprintf(stderr, "Unknown sched_kind: %s\n", sched_kind);
            return 1;
        }

        t_end = omp_get_wtime();

        printf("[sched=%s] threads=%d max_steps=%lu checksum=%lu time=%.6f\n",
               sched_kind, threads, (unsigned long)max_steps,
               (unsigned long)checksum, t_end - t_start);

    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        return 1;
    }

    return 0;
}
