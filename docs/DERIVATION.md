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

## What actually mattered

Ranked by contribution:

1. **The oracle, built first, and printing its own routes.** Every formula
   decision was settled by a number instead of by intuition.
2. **Reading the real statement.** Reconstructed constraints cost more time than
   any amount of algorithmic work in this project.
3. **Measuring the failure rather than guessing at it** — the 83.4% figure, the
   per-node change counts, the error signs.
4. **Validating the inputs, not just the solver.**

The elegant part — the envelope collapse — took an afternoon once the
reformulation was visible. The unglamorous parts took the other half of the
project.
