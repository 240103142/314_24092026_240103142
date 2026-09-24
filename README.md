# Lab 1 — The Amdahl Reality Gap

**Student ID:** 240103142
**Workload N:** 13,142,000 (10,000,000 + 3142 × 1,000)
**Hardware:** Apple M1 (4 Performance + 4 Efficiency cores, no SMT, 128-byte cache line)

## Repository contents

| File | Purpose |
|---|---|
| `hw_info.txt` | Raw sysctl hardware dump |
| `collatz_seq.c` | Sequential baseline (Phase 2) |
| `collatz.c` | OpenMP parallel version — scaling, false sharing, scheduling (Phase 3-4) |
| `build.sh` | Compiles both binaries with libomp on Apple Silicon |
| `run_benchmarks.sh` | Runs all benchmarks, writes results.csv |
| `plot.py` | Derives parallel fraction p, generates speedup_plot.png |
| `results.csv` | Raw benchmark output |
| `speedup_plot.png` | Amdahl Reality Gap chart |

## Reproduce

    brew install libomp
    chmod +x build.sh run_benchmarks.sh
    ./build.sh
    ./run_benchmarks.sh
    python3 plot.py

## Key results

- Derived parallel fraction **p = 0.9411** (from S_emp(2))
- S_emp(8) = 5.50 vs S_theo(8) = 5.67
- No SMT tier: k=16 not applicable (physical = logical = 8 threads)
