// Editorial for OI XXVIII, stage III, day 1 -- "Surowa zima" (sur).
//
// Build:   typst compile editorial.typ
// Figures: figures/gen_figures.py   (standalone SVG; no packages, no network)
// Numbers: figures/gen_numbers.cpp   (every figure in the prose is generated)
//
// Typst note, established by experiment rather than assumption:
//   * `#` starts markup only in MARKUP mode.  A `[ ... ]` argument to a
//     function is a code-mode literal, so `eq([... #g ...])` is a syntax error.
//   * The right answer for formulas is MATH MODE: `$ f(g) = 2 f(g) $`.  It is
//     parsed as an expression, symbols are written bare, and subscripts,
//     floors, sums and min-limits all work.  No `#` is needed anywhere.
//   * Body-carrying helpers therefore take content, and are called as
//     `eq($ ... $)` or `eq([text with #emph[...]])`.
//   * `block()` has no `align:` parameter; wrap in `#align(center, ...)` instead.
#let INK = rgb("#1a1a2e")
#let DIM = rgb("#5b6472")
#let ACCENT = rgb("#1d4ed8")
#let PANEL = rgb("#f5f7fa")
#let PANELB = rgb("#dde3ec")

#set document(title: "Surowa zima (sur) -- editorial", author: "Bartosz")
#set page(
  paper: "a4",
  margin: (x: 2.3cm, top: 2.5cm, bottom: 2.3cm),
  footer: context align(center)[#text(size: 8pt, fill: DIM)[#counter(page).display("1")]],
)
#set text(font: "Noto Serif", size: 10.5pt, fill: INK)
#set par(justify: true, leading: 0.68em)
#set math.equation(numbering: none)

#show heading: it => block[
  #set text(font: "Noto Sans")
  #it
  #v(-0.3em)
  #line(length: 100%, stroke: 0.6pt + PANELB)
  #v(0.2em)
]
#show heading.where(level: 1): set text(size: 19pt, weight: "bold")
#show heading.where(level: 2): set text(size: 12.5pt, weight: "bold", fill: ACCENT)

#let em(b) = text(weight: "bold", b)
#let mono(b) = text(font: "Noto Sans Mono", size: 9pt, b)
#let cap(b) = text(size: 8.5pt, fill: DIM, b)
#let eq(b) = block(
  width: 100%, fill: PANEL, stroke: 0.5pt + PANELB, radius: 3pt,
  inset: (x: 9pt, y: 8pt),
)[#align(center)[#b]]
#let keybox(title, b) = block(
  width: 100%, fill: PANEL, stroke: 0.9pt + ACCENT, radius: 4pt,
  inset: (x: 10pt, y: 9pt),
)[
  #text(font: "Noto Sans", size: 9.5pt, weight: "bold", fill: ACCENT)[#title]
  #v(3pt) #b
]
#let fig(path, b) = figure(image(path, width: 100%), caption: b)

// dtbl(head, rows)  -- `head` is an array of header cells, `rows` is an array
// of arrays.  Arrays ARE loopable in Typst (unlike trailing-argument sinks),
// so this form renders every row.  The earlier version took the rows as
// trailing args and silently dropped all but the first, which cost four rows of
// the cost table before it was noticed.
#let dtbl(head, rows) = block(
  width: 100%,
  table(
    columns: (auto,) * head.len(),
    align: (left,) + (right,) * (head.len() - 1),
    inset: (x: 6pt, y: 2.5pt), stroke: none,
    table.hline(stroke: 0.7pt + PANELB),
    ..head.map(h => text(weight: "bold", h)),
    table.hline(stroke: 0.3pt + PANELB),
    ..rows.flatten(),
    table.hline(stroke: 0.7pt + PANELB),
  ),
)

= Surowa zima
#cap([An editorial: from the statement to an $O(n + (d + sum_(i=1)^d (z_i + u_i)) log n)$ solution])

#v(0.5em)
#block(fill: PANEL, stroke: 0.5pt + PANELB, radius: 3pt, inset: 8pt, width: 100%)[
  #align(center)[#text(size: 9.5pt)[
    Solution: #mono("src/sur.cpp"). Scored *100/100* on the official grader. \
    *0.63 s* and 73 MB on a full-limit input \
    ($n = d = 250\,000$, $ell = 10^9$, $k = 999\,999$, \
    $sum_(i=1)^d (z_i + u_i) = 500\,000$ -- every constraint at its cap).
  ]]
]
#v(0.5em)

#cap[*n* stations, *l* = road length, *d* days, *p* where the machine \
is dropped each morning. Every day the whole road must be cleared; stations break \
and are repaired overnight.]

== 1. The setup, stated precisely

A day begins at $p$ with an *empty* battery. Walking one metre costs one second, \
whether or not you clear it. Clearing one metre uses one unit of charge. A \
working station refills to $k$, for free. Every metre must be cleared, and you \
may stop anywhere.

Four consequences drive everything.

- *Recharging is free and instantaneous*, so there is never a reason to carry \
charge past a working station.
- *Walking is the only cost.* In an optimal plan every metre walked is also \
cleared, so the answer is just the length of the walk. The sample route \
$3 -> 2 -> 0 -> 2 -> 4 -> 5 -> 4$ has length 9.
- *The road is a line*, so every plan is a sequence of excursions.
- *An interior gap can be serviced from either end*, and which end changes the \
price. This is the one real source of difficulty.

#fig("figures/fig-sample.svg", [The official sample: $ell = 5$, $k = 2$, station 2 at \
$x = 3$ is broken, the machine is dropped at $p = 3$. Each numbered leg is one \
continuous walk and costs its length. Total $1 + 2 + 2 + 2 + 1 + 1 = 9$.])

== 2. Pricing one gap

Price a gap of length $g$, entered with a full battery, with no station inside. \
The $g$ metres cost $g$ seconds to walk, and then there is the part that is easy \
to get wrong.

#eq[$ f(g, k) = g + sum_(j = 1)^m (g - j k) = g + m g - k m (m + 1) / 2,
       quad m = floor((g - 1) / k) $]

Why that particular form, and not a prettier one? Because $m$ counts precisely the \
$j$ for which $g - j k >= 1$, so no $max(0, .)$ ever fires and the sum closes in \
$O(1)$ with $m$ possibly $10^9$. Had the index run to $floor(g/k)$ the last term \
could be zero, and the floor would have to be written $floor((g-1)/k)$ anyway to \
keep the triangular term integral.

I do not have a one-sentence physical story for the sum, and I am not going to \
invent one: an earlier draft here called it "the charge that expires unused", \
which is $k + 2k + dots + m k$ -- a different number (for $g = 5, k = 2$ they are \
4 and 6). What is not in doubt is the formula itself. It is read off the oracle, \
reproduced by `figures/gen_numbers.cpp`, and the derived costs are checked against \
exhaustive brute force by `tests/test_oracle_selfcheck.py`. A cost function that \
can be written down correctly and verified is worth more than one that can be \
explained prettily and not checked.

At $k = 1$ this is $g + g(g-1)/2 = g(g+1)/2$, the triangular number, and that is \
where the 18-point $k = 1$ subtask comes from.

== 3. The five primitive costs

A gap is a *road end* (a station at one end, the end of the road at the other) or \
*interior* (stations at both ends). It can be serviced *closed* (return to where \
you started) or *open* (stop inside it). Five prices, all $O(1)$ from $f$:

#dtbl(([Cost], [Value], [Meaning]),
  (
    ([$"Eclose"$], [$2 f(g)$], [road end, return to the start]),
    ([$"Eopen"$], [$2 f(g) - g$], [road end, stop inside]),
    ([$"Iclose"$], [$2 f(floor(g/2)) + 2 f(ceil(g/2))$], [interior, return to the start]),
    ([$"Iopen"$], [$f(g)$], [interior, stop inside]),
    ([$"Ipass"$], [$g + 2(f(a) + f(b))$], [cross it en route, $a + b = max(0, g-k)$]),
  ),
)

At $k = 2$ those read:

#dtbl(([$g$], [end: return], [end: stop], [interior: return], [interior: stop], [cross]),
  (
    ([$0$], [$0$], [$0$], [$0$], [$0$], [$0$]),
    ([$1$], [$2$], [$1$], [$2$], [$1$], [$1$]),
    ([$2$], [$4$], [$2$], [$4$], [$2$], [$2$]),
    ([$3$], [$8$], [$5$], [$6$], [$4$], [$5$]),
    ([$4$], [$12$], [$8$], [$8$], [$6$], [$8$]),
    ([$5$], [$18$], [$13$], [$12$], [$9$], [$11$]),
  ),
)

Two of the five are theorems rather than guesses, and both were checked against \
brute force before being adopted.

- #em[$"Iclose"$ splits the gap in half.] Going out $a$ and back, then out $g - a$, \
costs $f(a) + f(g - a)$. Minimising over integer $a$ is a convex problem, so the \
optimum is the balanced point $a = floor(g / 2)$. This one formula replaced a \
binary search and took a $20\,000 x 20\,000$ run from 192 s to 12 s.
- #em[$"Ipass"$ is not symmetric.] Crossing a gap without servicing it costs $g$ plus \
the charge wasted going out and back. Only $g - k$ of the gap can be walked for \
free, and that remainder is split evenly between the two directions.

#cap[Full derivations: `docs/MODEL.md` §2.]

== 4. The day is a lower envelope of V-shapes

Now the structural step the whole solution rests on.

Let the working stations be $S_0 < S_1 < dots < S_(m-1)$. They cut the road into \
$m + 1$ gaps: $G_0$ before the first, $G_j$ between consecutive ones, $G_m$ after \
the last. The two ends are road ends; the rest are interior.

#keybox([Claim])[
  An optimal day chooses a first station $S_s$, walks there without clearing \
  anything, and then services the gaps outward from it, finishing inside the last \
  one it reaches.
]

Why the walk to the first station costs no charge: recharging is free everywhere, \
so that walk accrues no clearing cost at all. It is pure travel, and the shortest \
path from $p$ to $S_s$ is the segment between them, so any detour is strictly more \
walking.

Note this does *not* make the day a monotone sweep. The sample route goes *left* \
first, then *right*. What it means is that the gaps serviced from each station \
*split* at $S_s$, and that is what makes the cost additive.

Writing $C_s$ for the cost of everything starting from $S_s$ with a full battery:

#eq[$ upright("answer")(p) = min_s ( |p - S_s| + C_s ) $]

This is a *lower envelope of V-shapes*: one V centred on each working station. \
Read on its own it is $O(n)$ per day, and $250\,000$ days would be $6 dot 10^10$ \
operations. Everything else in this solution exists to make that fast.

#fig("figures/fig-envelope.svg", [The envelope, plotted from the #em[verified] \
oracle rather than from chosen constants. Each faint dashed curve is one \
station's V; the heavy curve is their minimum. It turns at stations (grey) *and* \
strictly between them (orange), where the two lines $p + A_t$ and $-p + B_t$ cross \
at $p = (B_t - A_t) / 2$. The official sample already shows this: on $[2, 5]$ the \
curve turns at $p = 3$.])

== 5. The reformulation

In $upright("answer")(p) = min_s (|p - S_s| + C_s)$ the only obstacle is the absolute value: \
it makes the objective depend on $p$ *inside* the minimum. That dependence is the \
entire problem, and it is a formulation problem, not a complexity problem.

Break the coordinate space at the data points. If $p$ lies in $[S_t, S_(t+1)]$ then \
the stations to the left of $p$ are *fixed*, so $|p - S_s| = p - S_s$ for all of \
them, and

#eq($ upright("answer")(p) = min (p + A_t, -p + B_t) $)

#eq($ A_t = min_(s <= t) (C_s - S_s), quad B_t = min_(s > t) (C_s + S_s) $)

#keybox([The one idea that makes this fast])[
  $A_t$ and $B_t$ contain *no $p$ at all*. They depend only on the fixed station \
  set, so a data structure can hold them and reuse them unchanged across days. A \
  term in the objective that depends on the query is a formulation smell, not a \
  complexity tax -- the fix is to push the parameter out of the node state, not to \
  summarise it more cleverly.
]

Every summary the structure keeps is now $p$-free. A repair or breakage changes a \
small local part of that state, and one day becomes a single split lookup plus a \
descent.

== 6. The data structure

A segment tree over the $n$ *station slots*, which never move. Slot $i$ is leaf \
$i + 1$; leaf 0 holds the leading road-end gap. An inactive slot's leaf is empty, \
so the active stations form a *subsequence* of the leaves and the prefix sums \
telescope to the right values.

A repair or breakage changes at most three leaves: the slot's own, the *preceding* \
active station's (the gap after it changes length), and leaf 0 if the first working \
station moved.

#fig("figures/fig-updates.svg", [One breakage, three leaves. The rest of the tree \
is untouched, which is why a day is $O(log n)$ rather than $O(n)$.])

#keybox([A trap worth naming])[
  If the first working station moves, leaf 0 must be rebuilt as well, and a \
  station's own leaf is rebuilt by asking for its *current* predecessor and \
  successor. Both details are easy to get subtly wrong. One version skipped the \
  walk to the root after emptying a leaf and produced a tree whose leaves were all \
  correct and whose *interior* nodes were stale: wrong answers, no crash, no \
  warning. `docs/DERIVATION.md` has the post-mortem.
]

== 7. Node state and the merge

Relative to the node's first-gap prefix, a node stores $P$ (the sum of $"ps" - "cl"$ over \
its gaps) and $"clSum"$ (the sum of $"cl"$), plus a dozen minima. Each is either a \
*single* term or a *pair* of terms in one frame, and merging is min and add. The \
algebra is easy; the bookkeeping is the hard part.

#keybox([Sign conventions are the whole difficulty])[
  A term carrying a $-T$ frame shifts by $-U$ when concatenated, a $+T$ term shifts \
  by $+U$, and a *pair* shifts by nothing at all -- a pair is one $+T$ and one \
  $-T$, so the frames cancel. Applying one uniform shift to every aggregate is the \
  classic bug: it never crashes, it is wrong by a prefix-like amount, and it *still \
  passes about 40% of random cases*. Watch the pass-rate, not the verdict. \
  `proofs/monoid_vs_sweep.cpp` checks the merge against a linear sweep over \
  500 000 cases.
]

== 8. Complexity and result

#eq($ upright("time") = O( n + ( d + sum_(i=1)^d (z_i + u_i) ) log n ) $)
#eq($ upright("memory") = O(n) $)
#cap([The sum runs over the $d$ days: $z_i$ repaired and $u_i$ damaged on day $i$. The statement caps it at $sum_(i=1)^d (z_i + u_i) <= 500\,000$, which is what makes the whole thing near-linear. Comfortably inside the 256 MB limit.])

#dtbl(([Measurement], [Time], [Memory]),
  (
    ([Every constraint at its cap, `sum(z+u) = 500 000`], [*0.63 s*], [73 MB]),
    ([Same input, naive $O(n dot d)$ sweep], [2 286 s], [31 MB]),
    ([Same input, output byte-identical], [yes], [yes]),
    ([`sur_bitmap`, same answers, flat bitmap instead of `std::set`], [0.38 s], [61 MB]),
  ),
)
#cap([Timings are this machine, and the sweep is the honest baseline: it is the \
same code with the tree replaced by a linear scan, so the gap is the data \
structure and nothing else.])

== 9. What the tests established

Scoring 100/100 is not the same as being *understood*, and the second is what makes \
the first reproducible.

- #mono("reference/oracle.py") is a literal state-space brute force that prices a \
day by enumerating legal states. It, not the fast solver, is the referee. The \
submission matches it on 68 706 exhaustive configurations.
- The oracle's *own* closed forms are checked against that brute force, so the \
referee is not merely trusted either.
- The suite can itself fail: a deliberately wrong binary is injected and the oracle \
test reports 68 706/68 706 wrong answers. Before that check existed, one test \
compared *zero* answers against empty output and printed success.
- Two further bugs surfaced only because the official sample printed the wrong \
number: a predecessor computed as $<=$ instead of $<$, and an early return that \
skipped the walk to the root. Neither raises, and neither is visible to \
pass/fail reasoning.
- #mono("run_tests.sh") asserts the speedup, so a change that erases it fails the \
build rather than regressing quietly.

== 10. Why it is single-threaded

Asked whether this could be parallel, the profile says no, and structurally.

Day $i$ depends on the active set produced by every update on days $1 dots i$. The \
days are one dependency chain: no thread starts day $k$ without having applied days \
$1 dots k - 1$, and the two queries read the very tree the next day's updates \
mutate. $p$ comes from the input, never from a previous answer, so there is nothing \
to speculate on either. With about 74% of the work serial, Amdahl caps six threads \
near 1.05x; and 99% of wall time is user time, so there is no I/O to overlap.

What the profile *did* find is that #mono("std::set") was the bottleneck: every \
update descended a red-black tree through a separate ~20 MB structure just to find \
a predecessor and a successor. Replacing it with a flat bitmask of active slots, \
which answers both from L1, is a 1.7x single-threaded win.

Four other plausible optimisations were tried and did nothing, for a reason worth \
recording: the working set is 54.5 MB against a 9 MB L3, so the run is \
*memory-latency-bound*. There is nothing to vectorise (a pointer chase with \
addresses not known in advance), #mono("-O3") and #mono("-march=native") are noise, \
and shrinking the node from 104 to 96 bytes was *slower*, because 96 bytes is \
exactly 1.5 cache lines. `proofs/optimisation_notes.cpp` keeps the numbers.
