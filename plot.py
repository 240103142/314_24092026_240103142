#!/usr/bin/env python3
"""
plot.py (fixed)
-----------------------------------------------------------------------
Reads results.csv (produced by run_benchmarks.sh), derives the parallel
fraction p from the k=2 empirical speedup, computes S_theo(k) via
Amdahl's Law, and plots S_emp(k) vs S_theo(k) vs the Linear Ideal.

This version parses each line by splitting on commas and always takes
the LAST field as the avg value (this is robust even though some
labels like "T_k,k=1" or "FalseSharing,naive(hits[tid]++)" contain an
embedded comma, which shifted the middle columns in the raw CSV -- the
last field is unaffected by that shift).

Usage:
    python3 plot.py
-----------------------------------------------------------------------
"""
import sys

try:
    import matplotlib.pyplot as plt
except ImportError:
    sys.exit("matplotlib not found. Install it with: pip3 install matplotlib")

with open("results.csv") as f:
    lines = [ln.strip() for ln in f.readlines() if ln.strip()]

data_lines = lines[1:]  # skip header

T_seq = None
T_k = {}

for line in data_lines:
    fields = line.split(",")
    avg = float(fields[-1])
    kind = fields[0]

    if kind == "T_seq":
        T_seq = avg
    elif kind == "T_k":
        # fields[1] looks like "k=1"
        k = int(fields[1].split("=")[1])
        T_k[k] = avg

if T_seq is None:
    sys.exit("Could not find T_seq row in results.csv")

k_values = sorted(T_k.keys())
if not k_values:
    sys.exit("Could not find any T_k rows in results.csv")

S_emp = {k: T_seq / T_k[k] for k in k_values}

# Derive p from k = 2 empirical speedup (fallback to smallest k>1 if 2 missing)
ref_k = 2 if 2 in S_emp else min(x for x in k_values if x > 1)
S_ref = S_emp[ref_k]
p = (ref_k / (ref_k - 1)) * (1.0 - (1.0 / S_ref))

def s_theo(k, p):
    return 1.0 / ((1.0 - p) + (p / k))

print(f"T_seq              = {T_seq:.6f} s")
for k in k_values:
    print(f"T_{k:<2}   = {T_k[k]:.6f} s   S_emp({k}) = {S_emp[k]:.4f}")

print(f"\nDerived parallel fraction p (from k={ref_k}) = {p:.4f}")

print("\nTable 1 (fill-in-ready):")
print(f"{'k':>4} {'T_k(s)':>10} {'S_emp':>8} {'S_theo':>8} {'Delta':>8}")
for k in k_values:
    st = s_theo(k, p)
    delta = st - S_emp[k]
    print(f"{k:>4} {T_k[k]:>10.6f} {S_emp[k]:>8.4f} {st:>8.4f} {delta:>8.4f}")

# ---- Plot ----
s_emp_plot = [S_emp[k] for k in k_values]
s_theo_plot = [s_theo(k, p) for k in k_values]
s_linear_plot = k_values

plt.figure(figsize=(7, 5))
plt.plot(k_values, s_linear_plot, "k--", label="Linear Ideal (S=k)")
plt.plot(k_values, s_theo_plot, "o-", color="tab:blue",
         label=f"S_theo(k)  [p={p:.3f}]")
plt.plot(k_values, s_emp_plot, "s-", color="tab:red", label="S_emp(k) (measured)")

plt.xlabel("Threads (k)")
plt.ylabel("Speedup S(k)")
plt.title("Amdahl Reality Gap — Student ID 240103142 (N=13,142,000)")
plt.legend()
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig("speedup_plot.png", dpi=150)
print("\nSaved plot to speedup_plot.png")
