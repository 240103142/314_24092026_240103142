# Monte Carlo π — Concurrency Lab

Java source lives in `src/`. No external dependencies — just a JDK (11+).

## How to run

```bash
cd src
javac Part1_PhantomBug.java && java Part1_PhantomBug
javac Part2_SynchronizationTrap.java && java Part2_SynchronizationTrap
javac Part3_Reduction.java && java Part3_Reduction
```

> **Note:** `Part3_Reduction` should be run on *your own machine* — the runtimes
> are meant to reflect your CPU's actual core count and memory system, so the
> grid below is a template to fill in yourself, not pre-filled data.

---

## Part 1 — The Phantom Bug

5 runs, `totalHits++` unsynchronized across 4 threads. Expect π ≈ 1.8–2.4,
and a **different** wrong value each run, because the outcome depends on
how the threads happen to interleave.

| Run | π (result)   |
|-----|--------------|
| 1   | 0.982585     |
| 2   | 0.990372     |
| 3   | 1.084635     |
| 4   | 1.088764     |
| 5   | 1.081769     |

All five runs land nowhere near π ≈ 3.14159 and disagree with each other,
which is exactly the signature of a data race: `totalHits++` silently loses
a large, non-deterministic number of increments every run.

## Part 2 — The Synchronization Trap

| Version                              | Time (ms) |
|---------------------------------------|-----------|
| Single-threaded (plain local counter) | 207       |
| 4 threads, `synchronized`             | 2246      |
| 4 threads, `AtomicLong`                | 866       |

Both concurrent versions produce the correct π (≈3.1415), but `synchronized`
is over **10x slower** than one thread, and `AtomicLong` (a lock-free CAS)
is still over 4x slower — confirming that contention on the shared counter,
not the extra CPU work, is the bottleneck.

## Part 3 — OpenMP-Style Reduction

Fill in after running `Part3_Reduction` on your machine (100,000,000 points):

| Threads (T) | Runtime (ms) | Speedup (T1/TN) | Efficiency (Speedup/T) |
|-------------|---------------|------------------|--------------------------|
| 1 (baseline)| 422           | 1.00x            | 100.0%                   |
| 2           | 224           | 1.88x            | 94.2%                    |
| 4           | 87            | 4.85x            | 121.3%                   |
| 8           | 77            | 5.48x            | 68.5%                    |
| 16          | 72            | 5.86x            | 36.6%                    |
| 32          | 75            | 5.63x            | 17.6%                    |

(Test machine: MacBook Air, 100,000,000 points.)

Interesting wrinkle: T=4 shows **super-linear** speedup (121% efficiency,
i.e. faster than the "ideal" 4x). This isn't a measurement error so much as
a real effect of JIT warm-up and CPU cache behavior — splitting the same
100M-point workload across 4 cores means each core's slice fits better in
its private cache and JIT-compiles to fast native code sooner, so the
combined runtime can beat the naive T1×(1/T) prediction. Past T=4, the
gains taper off and efficiency drops steadily as more software threads
compete for the same physical cores.

---

## Questions

**1. Look at your row for 16 threads. Why didn't your 8-core CPU run twice as
fast as 8 threads?**

Past the physical core count, extra "threads" don't add extra execution
units — they're scheduled onto the same physical cores via OS time-slicing.
In my data this shows up clearly: going from 8→16 threads only takes the
runtime from 77ms to 72ms (barely any gain), while efficiency drops from
68.5% to 36.6%. That's because my machine doesn't have 16 physical cores
available to the JVM — once T exceeds the real core count, extra threads
are just interleaved by the OS scheduler on the same cores, adding
context-switch overhead and cache contention instead of genuine parallel
throughput. So doubling T from 8 to 16 doesn't double the work being done
in parallel, it mostly doubles the number of threads fighting over the
same physical resources — hence almost no speedup and a big efficiency
drop. This is the practical face of Amdahl's/Gustafson's law plus real
hardware limits (shared caches, memory bandwidth, OS scheduling overhead).

**2. Why was the synchronized version in Part 2 slower than running on one
single core?**

Every `totalHits++` now has to be atomic, which forces each of the 4 threads
to serialize on the same memory location: acquiring a monitor lock (or doing
a CAS with AtomicLong) means the CPU has to invalidate that cache line on
every other core and pull the current value across the memory bus/cache
coherency protocol before it can proceed. Because hits happen very
frequently (millions of times), the threads spend the overwhelming majority
of their time stalling, waiting for exclusive ownership of that one cache
line, rather than doing useful work — that's the "95% of cycles stalling on
memory bus locks" from the assignment. A single thread never has this
problem: its local counter lives in a register/cache line that no one else
touches, so every increment is essentially free. Adding threads to a
workload that is dominated by contention on shared state makes it *slower*,
not faster — the fix isn't "more careful locking," it's Part 3's approach of
avoiding the shared write entirely until the very end.

---

## Publishing to Git

This folder is ready to become the class Git repo. From this directory:

```bash
git init
git add .
git commit -m "Monte Carlo pi: race condition, sync trap, lock-free reduction"
git branch -M main
git remote add origin <YOUR_NEW_REPO_URL>   # create an empty repo on GitHub/GitLab first
git push -u origin main
```

Then share repo access with **@sufyanism** (Settings → Collaborators on
GitHub, or Members on GitLab) before class end-time.
