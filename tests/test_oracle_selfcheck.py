"""Check that the closed forms in reference/oracle.py match brute force.

oracle.py is the referee for the whole project, so its helper formulas must
themselves be trustworthy -- tri/f/gap_close/gap_open were fitted, not proven.

NOTE on scope: solve_day is NOT checked against the road-end formulas here.
That check is invalid.  With one station at x and a start at p, the road splits
into a gap of |p-x| and a gap of L-|p-x|, but the pieces adjacent to p are
road-end gaps only when p is at a physical road end.  When p sits at the
station, the day is not "one road-end gap plus another" at all: the optimum
refills at the station before each leg, which those formulas do not model.  A
check built on that decomposition reports thousands of false failures and is
worse than no check.  solve_day is instead validated end-to-end by
tests/test_vs_oracle.py, which compares the compiled submission against it on
68 706 configurations.

What IS checked here: the arithmetic in tri/f, and that gap_close/gap_open
agree with an independent O(g) minimisation, including the even-split form the
solver relies on.
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(ROOT, "reference"))

from oracle import tri, f, gap_close, gap_open      # noqa: E402

bad = 0


def chk(cond, msg):
    global bad
    if not cond:
        bad += 1
        if bad <= 10:
            print("  FAIL:", msg)


# 1. tri against its own definition, term by term.
def tri_direct(x, k):
    return sum(max(0, x - j * k) for j in range(1, x // k + 2))


for k in range(1, 13):
    for x in range(0, 40):
        chk(tri(x, k) == tri_direct(x, k),
            f"tri({x},{k}) = {tri(x,k)} != {tri_direct(x,k)}")

# 2. gap_close: must equal a straight O(g) minimisation, AND equal the even
#    split the solver uses in O(1).
for k in range(1, 13):
    for g in range(0, 30):
        direct = min(2 * f(D, k) + 2 * f(g - D, k) for D in range(g + 1))
        chk(gap_close(g, k) == direct,
            f"gap_close({g},{k}) = {gap_close(g,k)} != {direct}")
        even = 2 * f(g // 2, k) + 2 * f(g - g // 2, k)
        chk(gap_close(g, k) == even,
            f"even split != min for gap_close({g},{k}): {even} vs {direct}")

# 3. gap_open likewise.
for k in range(1, 13):
    for g in range(0, 30):
        direct = min(2 * f(D, k) + 2 * f(g - D, k) - max(D, g - D)
                     for D in range(g + 1))
        chk(gap_open(g, k) == direct,
            f"gap_open({g},{k}) = {gap_open(g,k)} != {direct}")

# 4. f is the "clear g metres with no recharge" cost: monotone, and at least g.
for k in range(1, 13):
    prev = -1
    for x in range(0, 40):
        chk(f(x, k) >= x, f"f({x},{k}) = {f(x,k)} < {x}")
        chk(f(x, k) >= prev, f"f not monotone at x={x}, k={k}")
        prev = f(x, k)

print(f"oracle self-check: {bad} failures")
sys.exit(1 if bad else 0)
