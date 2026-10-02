# The model, and why it is the right one

## 1. The setup

Road `[0, l]`. Walking 1 m costs 1 s regardless of charge. Clearing 1 m costs
one unit of charge. A working station resets the charge to `k` for free. Each
day begins at `p` with an empty battery, every segment must be cleared, and
Bajtazar may stop anywhere. Output the minimum time for that day.

## 2. Primitive costs

With

    f(g,k) = g + sum_{j>=1} max(0, g - j*k) = g + m*g - k*m*(m+1)/2,   m = (g-1)//k

the exact cost of a gap of length `g` is:

| gap | action | cost |
|---|---|---|
| road-end (one station at the near end) | out and back | `Eclose = 2f(g)` |
| road-end | finish inside | `Eopen = 2f(g) - g` |
| interior (working stations both ends) | out and back | `Iclose = 2f(g/2) + 2f(g-g/2)` |
| interior | pass through | `Ipass = g + 2(f(a) + f(b))`, `a = m'/2`, `b = m'-a`, `m' = max(0,g-k)` |
| interior | finish inside | `Iopen = f(g)` |

`f` is the cost of walking `g` clear metres starting with a full battery and
never recharging: `g` seconds walking, plus the charge that expires unused
before each of the `m` recharge boundaries.

**`Iclose` is an even split, and that is a theorem, not a guess.** Minimising
`f(a,k) + f(g-a,k)` over integer `a` is convex, so the optimum is the balanced
point `a = floor(g/2)`. It was verified against a binary search on 500 200
cases before the search was deleted, which is where the 192 s → 12.3 s win on a
20k×20k run came from. Anything convex and symmetric is worth a closed form
before it is worth a search.

## 3. The day structure

Let `S_0 < ... < S_{m-1}` be the working stations. Gaps are
`G_0 = [0,S_0]`, `G_j = [S_{j-1},S_j]` for `1 <= j <= m-1`, `G_m = [S_{m-1},l]`.

**Claim.** Walking straight from `p` to a station costs `|p - S_s|` and no
detour before the first station can ever be cheaper.

Recharging is free at every working station, so the walk to the *first* station
touched accrues no clearing cost at all — it is pure travel, and the shortest
path from `p` to `S_s` is the segment between them. Any detour only adds travel.

**Consequence.** An optimal day touches a first station `S_s`, then sweeps
monotonically, treating every gap it crosses as a closed excursion and finishing
inside one gap. So

    answer(p) = min_s ( |p - S_s| + C_s )

where `C_s` is the day cost starting at `S_s` with a full battery.

## 4. The reformulation that makes it fast

`answer(p)` is a **lower envelope of V-shapes**, one per candidate station. On
its own that reads as O(n) per query, and that reading is where a long effort
went to die. But break the coordinate space at the fixed data points:

for `p` in `[S_t, S_{t+1})` the set of stations left of `p` is **fixed**, so
`|p - S_s| = p - S_s` throughout the interval, and the envelope collapses to two
coefficients:

    answer(p) = min( p + A_t ,  -p + B_t )
    A_t = min_{s <= t} (C_s - S_s)          B_t = min_{s > t} (C_s + S_s)

`A_t` and `B_t` contain **no `p`**. They are functions of the fixed station set
only, so they belong in a tree and every node summary is reusable across days.
One day is now a single split lookup plus O(log n).

This is the whole problem. A per-query term in the objective is a *formulation
smell*, not a complexity tax: the fix is to push the parameter out of the node
state, not to find a cleverer way to summarise it.

## 5. The tree

Leaves `0..n` over the **fixed station slots**. Leaf 0 holds the leading
road-end gap. Leaf `c` (`1 <= c <= n`) is slot `c-1`: if that station works, the
leaf holds the station *and* the gap after it. An inactive slot's leaf is empty,
so active stations are a subsequence of the leaves and the prefix sums telescope
to the right values.

A repair or breakage changes at most three leaves: that slot's own, the
**preceding** active station's (its following gap changes length), and leaf 0
when the first working station changes. Three point updates per event.

### Node state

Relative to `tLo` = the node's first-gap prefix, with `U` = the node's sum of
`ps - cl`:

    P       = sum (ps - cl) over the node's gaps
    clSum   = sum cl over the node's gaps
    aMinPos = min(-T_cs + S)    carries -T
    aMinNeg = min(-T_cs - S)    carries -T
    uMinPos = min(+T_cs + S)    carries +T
    uMinNeg = min(+T_cs - S)    carries +T
    cMinT   = min(+T_j - cl_j + OP_j)      carries +T
    dMinT   = min(-T_j - ps_j + OP_j)      carries -T
    abPos   = min_{cell(s)<=j}(aMinPos + cMinT)     +1, station left of gap
    abNeg   = min_{cell(s)<=j}(aMinNeg + cMinT)
    cdPos   = min_{j<cell(s)} (dMinT + uMinPos)     -1, gap left of station
    cdNeg   = min_{j<cell(s)} (dMinT + uMinNeg)

### The merge, and why the signs are the hard part

    aMinPos = min(L.aMinPos, R.aMinPos - L.P)     dMinT = min(L.dMinT, R.dMinT  - L.P)
    aMinNeg = min(L.aMinNeg, R.aMinNeg - L.P)
    uMinPos = min(L.uMinPos, R.uMinPos + L.P)     cMinT = min(L.cMinT,  R.cMinT  + L.P)
    uMinNeg = min(L.uMinNeg, R.uMinNeg + L.P)
    abPos   = min(L.abPos, R.abPos, L.aMinPos + L.P + R.cMinT)
    abNeg   = min(L.abNeg, R.abNeg, L.aMinNeg + L.P + R.cMinT)
    cdPos   = min(L.cdPos, R.cdPos, L.dMinT  + L.P + R.uMinPos)
    cdNeg   = min(L.cdNeg, R.cdNeg, L.dMinT  + L.P + R.uMinNeg)
    P = L.P + R.P ;  clSum = L.clSum + R.clSum

**A `-T` term shifts by `-U`. A `+T` term shifts by `+U`.** Pair values take no
shift at all, because a pair is one `+T` plus one `-T` and the frame cancels.
Applying one uniform shift to every aggregate is the classic bug: it never
crashes, it is wrong by a prefix-like amount, and it compounds up the tree. It
cost roughly a dozen variants to find, and roughly 40% of random cases pass
under it — so pass-rate, not pass/fail, is the signal to watch.

`proofs/monoid_vs_sweep.cpp` checks this over 500 000 cases.

### The query

With `p0` = the leaf of the last working station at or before `p` (0 if none),
query `[0,p0]` as `Q0` and `[p0+1,n]` as `Q1`:

    A_t = cltot + min( min(Q0.abNeg, Q0.aMinNeg + Q0.P + Q1.cMinT), Q0.cdNeg )
    B_t = cltot + min( Q1.abPos, min(Q1.cdPos, Q0.dMinT + Q0.P + Q1.uMinPos) )
    answer = min( p + A_t , -p + B_t )

`cdNeg` needs no cross term because `j <= s <= t` forces `j <= p0`; `abPos` needs
none because `j >= cell(s) >= p0+1`. Those two are the only places the split is
load-bearing, and both are one-line arguments.

## 6. Input convention

Per night, three lines: `z u p`, then the sorted list of the **z repaired**
stations (previously **broken**), then the sorted list of the **u damaged**
stations (previously **working**). Disjoint, at least one station always
working, 1-based ids, either list may be empty.

This is worth stating explicitly because the two lists mean opposite things,
and reading them backwards passes every structural check — only a semantic test
catches it. `tests/test_zu_convention.py` builds cases where swapping the
convention changes the answer, and confirms the solver matches the reference
read this way. Getting this backwards cost three flip-flops in one sitting.

## 7. Bounds

Worst measured cost is about 1.0e18 against a 9.2e18 int64 ceiling
(`proofs/overflow_audit.cpp`). The road-end gap of 1e9 at `k = 1` is the
binding case: `2f(1e9,1) = 1.000000001e18`.
