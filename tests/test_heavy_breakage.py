"""Heavy-breakage stress: most stations broken each day, so the active set is
small and changes constantly.  Checks the multi-day state machine (repair then
break, same-day ordering, the >=1-working invariant) against the Python model.
"""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DIR  = os.path.join(ROOT, "build")
sys.path.insert(0, os.path.join(ROOT, "reference"))
import subprocess, random
from day_model import day_O_n


def run(exe, src):
    return [int(v) for v in
            subprocess.run([f"{DIR}/{exe}"], input=src, capture_output=True,
                           text=True, timeout=600).stdout.split()]

rng = random.Random(13579)
bad = 0
total = 0
for trial in range(400):
    L = rng.choice([10, 50, 400, 3000])
    n = rng.randint(2, min(L + 1, 120))
    k = rng.randint(1, L)
    d = rng.randint(1, 10)
    xs = sorted(rng.sample(range(L + 1), n))
    lines = [f"{n} {L} {k} {d}", " ".join(map(str, xs))]
    ok = [True] * n
    want = []
    for _ in range(d):
        p = rng.randint(0, L)
        broken = [i + 1 for i in range(n) if not ok[i]]
        rng.shuffle(broken)
        # repair heavily: most broken stations come back
        z = broken[: rng.randint(0, len(broken))]
        for i in z:
            ok[i - 1] = True
        work = [i + 1 for i in range(n) if ok[i]]
        rng.shuffle(work)
        # break heavily, but keep at least one
        u = work[: rng.randint(0, len(work))]
        if len(u) >= len(work):
            u = u[: max(0, len(work) - 1)]
        for i in u:
            ok[i - 1] = False
        lines.append(f"{len(z)} {len(u)} {p}")
        lines.append(" ".join(map(str, z)))
        lines.append(" ".join(map(str, u)))
        W = [xs[i] for i in range(n) if ok[i]]
        want.append(day_O_n(L, k, W, p) if W else 0)
    src = "\n".join(lines) + "\n"
    g1 = run("sur_sweep", src)
    g2 = run("sur_segtree", src)
    total += len(want)
    if g1 != want:
        bad += 1
        if bad <= 3:
            print(f"  surc WRONG trial={trial} n={n} d={d} L={L} k={k}")
            print(f"    want {want[:8]}")
            print(f"    got  {g1[:8]}")
    if g2 != g1:
        bad += 1
        if bad <= 6:
            print(f"  sur3 DIFFERS trial={trial} n={n} d={d} L={L} k={k}")
            print(f"    surc {g1[:8]}")
            print(f"    sur3 {g2[:8]}")
print(f"heavy-breakage stress: {bad} problems, {total} day-answers checked")
