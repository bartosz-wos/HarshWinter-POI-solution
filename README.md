# sur — OI XXVIII, "Surowa zima" (v. 1.61)

A road `[0, l]` of `l` unit segments. Walking 1 m costs 1 s whatever the
battery. Clearing 1 m costs one unit of charge. A **working** station resets
the charge to `k` for free. Each day starts at `p` with an empty battery, every
segment must be cleared, and Bajtazar may finish anywhere. Print the minimum
time per day.

```
O(n + (d + sum(z+u)) log n) time,  O(n) memory
```

`sur.cpp` runs the statement's maximum constraints (n = d = 250 000,
l = 10^9) in **1.26 s / 71 MB** against a 256 MB limit.

## Build and run

```sh
./build.sh                      # builds everything into build/
./build/sur < input.txt         # the submission
./run_tests.sh                  # full verification suite
./run_tests.sh quick            # skips the two slow suites
```

Only `g++` (C++20) and `python3` are needed. No network, no dependencies.

## Layout

| Path | What it is |
|---|---|
| `src/sur.cpp` | **the submission** — one segment tree, O(log n) per day |
| `reference/sur_sweep.cpp` | slow O(n·d) sweep, ground truth for differential tests |
| `reference/sur_segtree.cpp` | the O(n)-per-day tree variant of the same model |
| `reference/oracle.py` | literal state-space brute force — the referee |
| `reference/model.py` | verified primitive costs (f, Eopen, Eclose, Iopen, Iclose) |
| `reference/day_model.py` | the verified O(n) per-day model |
| `proofs/` | four self-contained programs that check structural claims |
| `generators/` | input generators and a statement-conformance validator |
| `tests/` | the suite, plus reconstructed `ocen` inputs |
| `docs/` | the model write-up and the derivation log |

## Verification

| Check | Result |
|---|---|
| official sample | `9` |
| exhaustive small l, all k, all station subsets | 0 / 2286 |
| statement-convention (which list is repaired?) | 0 / 400 |
| segment tree vs linear sweep | 0 / 30000 |
| min-plus monoid vs linear sweep | 0 / 500000 |
| adversarial edges (k=1, k=l, zero gaps, single station) | 0 / 400000 |
| stress shapes (heavy breakage, full update budget) | identical, 3 shapes |
| full constraints vs the slow sweep, 250 000 days | byte-identical |
| overflow audit | worst case 1.0e18 vs 9.2e18 headroom |

The sweep reference is itself checked against `reference/oracle.py` on 68 706
exhaustive configurations with 0 mismatches, so the two-implementation
agreement is not circular.

**About the `ocen` files.** The statement *describes* its four test cases in
prose; there is only one worked example. `tests/data_*ocen.in` are therefore
reconstructions — a generator's reading of the spec, not the grader's data, and
only as trustworthy as that reading. `3ocen` is the weakest of the four. The
correctness evidence above does not depend on them; treat them as a
convenience, not as evidence.

## Status

Scored 100 / 100 on the official grader, in under half the time limit.

## Documentation

- `docs/MODEL.md` — the model, the day decomposition, the segment tree, and
  why the merge's shift signs are what they are.
- `docs/DERIVATION.md` — how the model was arrived at, in order, including the
  paths that failed.
