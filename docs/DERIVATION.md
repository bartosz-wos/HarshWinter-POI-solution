# Derivation log

How the model in `MODEL.md` was arrived at, in order — **including the paths
that failed**, because those are the parts that generalise.

## Phase 1 — the oracle, before any math

`reference/oracle.py` is a literal state-space search: enumerate states, walk
the road, respect the charge, take the minimum. No cleverness, no formulas.

It was validated against the official sample first. An oracle that does not
reproduce the stated output is worthless, and that check is the one thing that
must happen before anything else is built on top of it.

The oracle then **printed its own optimal routes**. This was the highest-value
move in the whole project: the routes, not the numbers, showed which structural
shapes were actually possible. Reading them killed three plausible-looking
models that no amount of numeric testing would have flagged as wrong.

## Phase 2 — primitives in isolation

Each primitive cost was fitted against the oracle alone, over a grid of small
parameters, and verified exhaustively before being used in any composition.

The first one was wrong: `EOpen(2,1)` was fitted as 5 and is 4. It surfaced via
a single decisive case — `a=2, b=1, k=1`, where the oracle said 6 and the
candidates said 7 and 5. Two candidates, one high and one low, means the
candidate set is *too wide*, not off by a constant.

**Read the sign of the error.** Consistently overestimating means a legal action
is excluded; consistently underestimating means an illegal action is admitted.
Either way the fix is a category, not an individual case. This narrows the
search far more than the mismatch count does.

Composing unverified primitives only compounds unverified error, so each was
locked down before the next was attempted.

## Phase 3 — the composition

`reference/day_model.py` is the verified O(n) day model: for each starting
station, sweep right, treat crossed gaps as closed excursions, finish inside one
gap. Verified exhaustively on 34 774 small configurations, 0 mismatches.

`ICLOSE` was the last piece. Binary search worked but was slow; testing showed
the optimum is a plain **even split**, and the closed form matched the search on
500 200 cases before the search was deleted.

## Phase 4 — the optimisation, and the long way round

The O(n) sweep was correct and verified, and it scored **30/100**. The failing
subtasks were exactly the small-`d` ones, which says *asymptotics*, not *logic*.

The first instinct — a segment tree over the same day model — went nowhere for a
long time. Roughly a dozen monoid variants failed. The useful diagnosis:

    nodes changed when only p changed: 3416 / 4095  (83.4%)

That number is a verdict on the **formulation**, not on the tree. The day
objective genuinely depended on `p`, so no merge could make the summaries
reusable. Trying a one-sided split at `p` also failed, because optimal pairs
legitimately straddle the split.

**The real constraints had never been read.** The bounds in play were
reconstructed from an earlier conversation — guesses wearing the costume of
facts — and they said `n, d <= 250000`, which made the sweep structurally dead.
Getting the statement confirmed the limit and killed the remaining hope of
tuning the old approach. That was the turning point, and it was a *statement*
problem, not an algorithmic one.

## Phase 5 — the reformulation

See `MODEL.md` §4. Rewriting the day objective as a lower envelope of V-shapes,
then partitioning the coordinate space at the station positions, makes `p` fall
out of the node state entirely: `answer(p) = min(p + A_t, -p + B_t)`.

Everything the earlier attempts had been reaching for follows immediately. The
segment tree that had resisted a dozen designs works on the first try against
the reformulated objective — the difficulty was never the merge, it was the
`p` in the leaves.

    n = d = 250000, l = 1e9     2383 s  →  1.26 s
    output                       byte-identical over all 250000 days

The two implementations share a decomposition, so their agreement is strong but
not independent; it is still the measurement that separates "correct" from
"correct on the cases I tried".

## Phase 6 — the bug that the tests were hiding

The test generators had the repair and damage lists **backwards** for most of
the project. Every structural check passed. It surfaced only because ASan
reported a heap overflow on a path the statement guarantees unreachable — which
is a statement about the *generator*, not the solver.

The tempting fix was an `if (empty) return 0;` guard. That would have converted
a loud, precise failure into a silent wrong answer and sent the next person
debugging a solver that was never wrong. Instead: write a validator, run it on
the offending input, fix the generator, then add the guard as a belt-and-braces
measure.

`generators/validate_input.py` exists for exactly this, and it is worth running
on any generated input *before* believing any downstream mismatch.

## Phase 8 — why this is single-threaded, and what made it fast instead

Asked whether the solver could be made multithreaded, the honest answer is no,
and the profile is what proves it rather than a guess.

`proofs/phase_profile.cpp` splits the 1.28 s at the statement's limits:

    read file          5 ms
    tokenise          23 ms
    setup x[]          9 ms
    build tree        44 ms
    days+queries    1200 ms      <-- 94%

Turning the per-day queries off inside the day loop separates the rest:

    point updates    ~940 ms      <-- 74% of everything
    the two queries  ~260 ms
    1 499 965 updates over 250 000 days, ~6 per day

Day *i*'s answer depends on the active set produced by every repair and breakage
on days 1..*i*. The days are one long dependency chain: no thread can start day
*k* without first applying days 1..*k-1*, and the two range queries read the very
tree the next day's updates mutate. `p` comes from the input, never from a
previous answer, so there is no data to speculate on either — a speculative
thread would have to guess a state the active set never revisits. With 74% of
the work serial, Amdahl puts a 6-thread ceiling at about 1.05x. 99% of wall time
is user time on one core, so it is not I/O either.

What did pay off was `std::set`. The active set was a red-black tree, and each
update walks it for a predecessor and a successor, then the segment tree walks
~19 levels for the three changed leaves — all pointer chasing through a
72 MB working set. Replacing it with a flat two-level bitmask (`src/sur_bitmap.cpp`:
one `uint64` per 64 slots, plus a summary word per 64 words) makes every
operation a handful of register and L1 operations.

    sur   (std::set)   1.35 s   72.6 MB
    sur_bitmap         0.79 s   60.9 MB     1.71x, byte-identical output

Two bugs were caught on the way, both by the sample printing the wrong number:

  - `act.prev(slot)` returns the largest active slot <= slot, which INCLUDES
    slot itself. The original's `lower_bound` then `*prev(it)` was a strict
    predecessor. The 68 706-case oracle test caught this; reasoning about it did
    not.
  - an early `return` in the "station is broken" branch of `refresh()` skipped
    the walk to the root, so every ancestor of an emptied leaf kept the old
    value. Identical inputs, identical active set, identical leaves after build
    — and a different answer, because the internal nodes above the emptied leaf
    were stale. Dumping the whole tree node by node located it in one step;
    two rounds of reasoning about it did not.

`proofs/batch_test.cpp` also tested batching the three leaf rebuilds per update
into a single bottom-up pass: correct, and about 1.5x on the update phase, but
subsumed by the bitmap and not worth the extra code.

## Phase 9 — what optimisation actually bought

Tried: `[[assume]]`, `[[likely]]`/`[[unlikely]]`, hand-written intrinsics, SIMD
over the merge, `-O3`, `-march=native`, and packing the segment tree node from
104 bytes down to 96. Four of those did nothing, and the reasons are structural
rather than matters of tuning.

The workload is memory-latency-bound. The tree is 524288 nodes x 104 B = 54.5 MB
against a 9 MB L3, each update touches ~57 nodes, and the run makes ~171 M
cache-line fetches that nearly all miss. 99% of wall time is user time, so
there is no I/O to overlap either. At `-O2` the default target is
`-march=x86-64`, so AVX2 was not even available to the compiler.

    baseline                       1.27 s   72.6 MB
    -O3                            1.27 s
    -march=native (AVX2/BMI2)      1.25 s
    -O3 -march=native              1.25 s
    96-byte node (empty bit-packed
      into the sign of clSum)      1.28 s   WORSE
    flat-bitmap active set         0.76 s   1.71x   <-- the only real win

The 96-byte node is the instructive failure. The `empty` flag is not
redundant: a node is non-empty iff it holds an active leaf, and `clSum == 0`
does not imply otherwise, because zero-length gaps make a live node's clSum 0
too. But `clSum` is a sum of non-negative closed-excursion costs, so its sign
bit is never set on a live node, and INT64_MIN is free as the marker. That
really does remove 8 bytes per node. It was still slower, because 96 B is
exactly 1.5 cache lines: every second node straddles a line boundary, where 104 B
is 1.625 lines, so line efficiency is ~98% either way and the saved bytes were
bytes nobody re-read. **The only lever that matters here is touching fewer
nodes, not smaller nodes.**

There is nothing for SIMD to do either. An update is a 19-level pointer chase
whose addresses are not known in advance; AVX2 cannot gather them, and the work
between loads is 12 min/adds on values already in registers. The 171 M dependent
DRAM loads are latency, not throughput, so wider execution units cannot hide
them. `[[likely]]` has nothing to annotate: the hot path has no unpredictable
branch left in it that the profile points at.

The bitmap won for a different reason than expected, and it is worth stating
precisely: it did not make the segment tree faster at all. It deleted a second
data structure. Every update was doing a red-black-tree descent -- up to 19 more
levels of pointer chasing, through a separate ~20 MB structure -- purely to find
a predecessor and a successor. The bitmap answers both from L1. That is the
difference between touching the same nodes faster and touching the same nodes
without paying for a second structure.

`proofs/optimisation_notes.cpp` records the numbers, and
`proofs/make_lean_variant.py` generates the 96-byte variant so the claim can be
re-checked rather than taken on trust.

## Phase 7 — the tests that could not fail

Moving the work into a repository exposed a defect the numbers had been hiding.
`tests/test_vs_oracle.py` reported "exhaustive mismatches: 0 / 68706" — the
figure quoted above as proof the slow reference was oracle-checked — while its
comparison loop ran **zero** times. It packed many configurations into a single
file behind a `0 0 0` sentinel header; the binary read the header, produced no
output, `got` was empty, and every case passed vacuously.

Three things had to be true before that was visible:

- the suite had to be *run from a fresh clone*, because the file that broke it
  worked on the machine that wrote it;
- every test and proof program had to actually exit non-zero, because they all
  printed a count and returned 0;
- the multi-day tests had to include the submission at all, because they had
  only ever run the two references, leaving repairs, breakages and state
  carried between days untested on the code that ships.

`tests/test_harness_can_fail.py` now injects a binary that always prints 0 and
requires the suite to catch it. It does, 68706/68706. The general lesson is
not about this bug: **a test whose loop can execute zero times and still report
success is indistinguishable from a passing test**, and a green suite is
evidence about the suite before it is evidence about the solver.

## What actually mattered

Ranked by contribution:

1. **The oracle, built first, and printing its own routes.** Every formula
   decision was settled by a number instead of by intuition.
2. **Reading the real statement.** Reconstructed constraints cost more time than
   any amount of algorithmic work in this project.
3. **Measuring the failure rather than guessing at it** — the 83.4% figure, the
   per-node change counts, the error signs.
4. **Validating the inputs, not just the solver.**
5. **Running the suite from somewhere else.** Portability is a test, and it was
   the only thing that exposed the vacuous oracle test.

The elegant part — the envelope collapse — took an afternoon once the
reformulation was visible. The unglamorous parts took the other half of the
project.
