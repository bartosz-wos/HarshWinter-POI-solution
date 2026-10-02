#!/usr/bin/env bash
# Build every binary the test suite needs, into build/.
# Idempotent; safe to re-run.  No network, no dependencies beyond g++/python3.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build

CXX=${CXX:-g++}
FLAGS="-O2 -std=c++20 -Wall -Wextra"

echo "==> building submission and references"
$CXX $FLAGS -o build/sur              src/sur.cpp
$CXX $FLAGS -o build/sur_bitmap       src/sur_bitmap.cpp
$CXX $FLAGS -o build/sur_bitmap       src/sur_bitmap.cpp
$CXX $FLAGS -o build/sur_sweep        reference/sur_sweep.cpp
$CXX $FLAGS -o build/sur_segtree      reference/sur_segtree.cpp

echo "==> building generators"
$CXX $FLAGS -o build/gen_random       generators/gen_random.cpp
$CXX $FLAGS -o build/gen_stress       generators/gen_stress.cpp

echo "==> building proof programs"
$CXX $FLAGS -o build/monoid_vs_sweep  proofs/monoid_vs_sweep.cpp
$CXX $FLAGS -o build/segtree_vs_sweep proofs/segtree_vs_sweep.cpp
$CXX $FLAGS -o build/overflow_audit   proofs/overflow_audit.cpp
$CXX $FLAGS -o build/adversarial_edges proofs/adversarial_edges.cpp

echo "==> build ok:"
ls -1 build/
