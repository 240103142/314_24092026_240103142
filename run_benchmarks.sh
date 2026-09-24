#!/bin/bash
# -----------------------------------------------------------------------
# run_benchmarks.sh
# Automates Phase 2-4 benchmarking for the Amdahl Reality Gap practicum.
# Student ID: 240103142 | N = 13,142,000
#
# Run this AFTER compiling with build.sh (or manually, see comments).
# It performs the Warmup Rule itself: Run 1 is discarded, T = avg(Run2,Run3).
#
# Usage:
#   chmod +x run_benchmarks.sh
#   ./run_benchmarks.sh
#
# Output: results.csv in the current directory.
# -----------------------------------------------------------------------
set -e

OUT=results.csv
echo "table,config,run1_or_note,run2,run3,avg" > "$OUT"

# ---- helper: run a command 3 times, parse "time=X.XXXXXX" from stdout ----
run3() {
    local label="$1"; shift
    local r1 r2 r3
    r1=$("$@" | grep -o 'time=[0-9.]*' | cut -d= -f2)
    r2=$("$@" | grep -o 'time=[0-9.]*' | cut -d= -f2)
    r3=$("$@" | grep -o 'time=[0-9.]*' | cut -d= -f2)
    local avg
    avg=$(echo "scale=6; ($r2 + $r3) / 2.0" | bc)
    echo "  $label: run1(cold,discarded)=$r1  run2=$r2  run3=$r3  avg=$avg"
    echo "$label,,$r1,$r2,$r3,$avg" >> "$OUT"
}

echo "== Phase 2: Sequential baseline (collatz_seq) =="
r1=$(./collatz_seq | grep -o 'Elapsed time (s) = [0-9.]*' | grep -o '[0-9.]*$')
r2=$(./collatz_seq | grep -o 'Elapsed time (s) = [0-9.]*' | grep -o '[0-9.]*$')
r3=$(./collatz_seq | grep -o 'Elapsed time (s) = [0-9.]*' | grep -o '[0-9.]*$')
avg=$(echo "scale=6; ($r2 + $r3) / 2.0" | bc)
echo "  T_seq: run1(discarded)=$r1 run2=$r2 run3=$r3 avg=$avg"
echo "T_seq,k=1(sequential),$r1,$r2,$r3,$avg" >> "$OUT"

echo ""
echo "== Phase 3: OpenMP scaling (Table 1) =="
for k in 1 2 4 8; do
    run3 "T_k,k=$k" ./collatz base $k
done

echo ""
echo "== Phase 4a: False sharing experiment (Table 2), threads=8 =="
run3 "FalseSharing,naive(hits[tid]++)" ./collatz naive 8
run3 "FalseSharing,reduction" ./collatz reduction 8
run3 "FalseSharing,padded_struct" ./collatz padded 8

echo ""
echo "== Phase 4b: Scheduling experiment (Table 3), threads=8 =="
run3 "Scheduling,static_default" ./collatz sched 8 static0
run3 "Scheduling,static_1000" ./collatz sched 8 static1000
run3 "Scheduling,dynamic_100" ./collatz sched 8 dynamic100
run3 "Scheduling,dynamic_10000" ./collatz sched 8 dynamic10000
run3 "Scheduling,guided" ./collatz sched 8 guided

echo ""
echo "Done. Raw results written to $OUT"
echo "NOTE: Apple M1 has no SMT, so there is no k=16 logical-thread tier;"
echo "      your worksheet's k=16 row should be marked N/A with a one-line"
echo "      explanation (see Q2)."
