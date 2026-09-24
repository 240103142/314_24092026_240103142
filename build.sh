#!/bin/bash
# -----------------------------------------------------------------------
# build.sh - compiles collatz_seq.c and collatz.c on Apple Silicon (M1)
# using Homebrew's libomp, since Apple Clang has no built-in OpenMP.
# -----------------------------------------------------------------------
set -e

LIBOMP_PREFIX=$(brew --prefix libomp)

echo "Using libomp from: $LIBOMP_PREFIX"

clang -O2 -Xpreprocessor -fopenmp \
      -I"$LIBOMP_PREFIX/include" \
      -L"$LIBOMP_PREFIX/lib" -lomp \
      collatz_seq.c -o collatz_seq

clang -O2 -Xpreprocessor -fopenmp \
      -I"$LIBOMP_PREFIX/include" \
      -L"$LIBOMP_PREFIX/lib" -lomp \
      collatz.c -o collatz

echo "Build OK: ./collatz_seq and ./collatz"
