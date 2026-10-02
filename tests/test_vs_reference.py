"""Validate sur5 (segment tree, O((d + sum(z+u)) log n)) against the verified
O(n)-per-day reference surc.

Every generated case obeys the statement's guarantees:
  - repaired stations (z) were previously BROKEN
  - broken stations (u) were previously WORKING
  - the two sets are disjoint
  - at least one station always remains working
Station numbers in the input are 1-based; the internal set holds 1-based ids and
W is built in ascending order.
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
                       text=True, timeout=600)
    if r.returncode != 0:
        raise RuntimeError(f"{exe} rc={r.returncode} stderr={r.stderr[:300]}")
    return [int(v) for v in r.stdout.split()]


def gen(rng, n, d, l, k):
    xs = sorted(rng.sample(range(l + 1), n))
    o = [f"{n} {l} {k} {d}", " ".join(map(str, xs))]
    work = set(range(1, n + 1))          # 1-based ids of working stations
    want = []
    for _ in range(d):
        p = rng.randint(0, l)
        broken = sorted(set(range(1, n + 1)) - work)
        z = set(rng.sample(broken, rng.randint(0, len(broken)))) if broken else set()
        working = sorted(work | z)
        u = set()
        if len(working) > 1:
            u = set(rng.sample(working, rng.randint(0, len(working) - 1)))
        u -= z
        remaining = (work | z) - u
        if not remaining and u:
            u.discard(sorted(u)[0])
        work = (work | z) - u
        z, u = sorted(z), sorted(u)
        assert not (set(z) & set(u)), (z, u)
        assert work, "no working station"
        o.append(f"{len(z)} {len(u)} {p}")
        o.append(" ".join(map(str, z)))
        o.append(" ".join(map(str, u)))
        W = [xs[s - 1] for s in sorted(work)]
        assert W == sorted(W)
        want.append(day_O_n(l, k, W, p))
    return "\n".join(o) + "\n", want


rng = random.Random(20210414)
bad5 = badref = 0
N = 800
for trial in range(N):
    l = rng.choice([1, 2, 3, 5, 8, 12, 20, 40, 100, 500, 5000])
    n = rng.randint(1, min(l + 1, 80))
    k = rng.choice([1, l, max(1, l // 2), rng.randint(1, l)])
    d = rng.randint(1, 8)
    src, want = gen(rng, n, d, l, k)
    try:
        got5 = run("sur", src)
    except RuntimeError as e:
        print(f"  sur5 CRASH trial={trial} n={n} d={d} l={l} k={k}: {e}")
        bad5 += 1
        continue
    try:
        gref = run("sur_sweep", src)
    except RuntimeError as e:
        print(f"  surc CRASH trial={trial} n={n} d={d} l={l} k={k}: {e}")
        badref += 1
        continue
    if got5 != want:
        bad5 += 1
        if bad5 <= 3:
            print(f"  sur5 WRONG trial={trial} n={n} d={d} l={l} k={k}")
            print(f"    want {want}")
            print(f"    got  {got5}")
            print("    src:\n" + src)
    if gref != want:
        badref += 1
        if badref <= 3:
            print(f"  surc WRONG trial={trial} n={n} d={d} l={l} k={k}: "
                  f"want {want} got {gref}")
print(f"sur5 vs reference: {bad5} / {N} trials wrong")
print(f"surc vs reference: {badref} / {N} trials wrong")
sys.exit(1 if (bad5 or badref) else 0)
