"""Independent end-to-end check: does the SOLVER read z and u the way the
statement says?

Line 2 of each night lists the a_i = REPAIRED stations (previously broken);
line 3 lists the b_i = DAMAGED stations (previously working).

This test builds inputs where the two conventions give DIFFERENT answers, runs
the solver, and compares against the state-space oracle under the same
convention.  If the solver swapped z and u, the answers would differ.
"""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DIR  = os.path.join(ROOT, "build")
sys.path.insert(0, os.path.join(ROOT, "reference"))
import subprocess, random
from day_model import day_O_n



def run(exe, src):
    r = subprocess.run([f"{DIR}/{exe}"], input=src, capture_output=True,
                       text=True, timeout=300)
    if r.returncode != 0:
        raise RuntimeError(f"{exe} rc={r.returncode} {r.stderr[:200]}")
    return [int(v) for v in r.stdout.split()]


rng = random.Random(7)
# Cases where the active set changes a lot, so swapping z and u really matters.
mismatch = 0
N = 400
for trial in range(N):
    l = rng.choice([20, 60, 200, 1000])
    n = rng.randint(3, min(30, l - 1))
    k = rng.choice([1, 2, l, max(1, l // 3)])
    d = rng.randint(2, 6)
    xs = sorted(rng.sample(range(1, l), n))     # 1 <= x <= l-1
    src = [f"{n} {l} {k} {d}", " ".join(map(str, xs))]
    work = set(range(1, n + 1))
    want = []
    for _ in range(d):
        p = rng.randint(0, l)
        broken = sorted(set(range(1, n + 1)) - work)
        z = sorted(rng.sample(broken, rng.randint(0, len(broken)))) if broken else []
        cur = sorted(work | set(z))
        u = sorted(rng.sample(cur, rng.randint(0, len(cur) - 1))) if len(cur) > 1 else []
        z, u = set(z), set(u)
        u -= z
        if not ((work | z) - u):
            if u: u.discard(sorted(u)[0])
            else: z.discard(sorted(z)[0])
        work = (work | z) - u
        assert work
        z, u = sorted(z), sorted(u)
        src.append(f"{len(z)} {len(u)} {p}")
        src.append(" ".join(map(str, z)))
        src.append(" ".join(map(str, u)))
        W = [xs[s - 1] for s in sorted(work)]
        want.append(day_O_n(l, k, W, p))
    text = "\n".join(src) + "\n"
    got = run("sur", text)
    if got != want:
        mismatch += 1
        if mismatch <= 3:
            print(f"  MISMATCH trial={trial} n={n} l={l} k={k} d={d}")
            print(f"    want {want}")
            print(f"    got  {got}")
            print("    input:\n" + text)
print(f"z/u convention check: {mismatch} / {N} trials wrong")
import sys as _sys
_sys.exit(1 if mismatch else 0)
