"""Final multi-day validation: legal day sequences, C++ vs verified replay.

One input file = one instance (n, L, k, d) followed by d day lines.
Trials are run one instance per subprocess call but batched in parallel."""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DIR  = os.path.join(ROOT, "build")
sys.path.insert(0, os.path.join(ROOT, "reference"))
import subprocess, random
from concurrent.futures import ThreadPoolExecutor
from day_model import day_O_n as day

EXE = os.path.join(DIR, "sur_sweep")

def gen_trial(rng):
    L = rng.randint(1, 30)
    n = rng.randint(1, min(7, L + 1))
    x = sorted(rng.sample(range(L + 1), n))
    k = rng.randint(1, L)
    d = rng.randint(1, 6)
    days = []
    ok = [True] * n
    for _ in range(d):
        p = rng.randint(0, L)
        zr = [i + 1 for i in range(n) if not ok[i]]
        rng.shuffle(zr)
        z = zr[: rng.randint(0, len(zr))]
        after = list(ok)
        for i in z:
            after[i - 1] = True
        # u may only name stations working AFTER repairs, and must leave
        # at least one working station (the statement's invariant).
        ub = [i + 1 for i in range(n) if after[i]]
        rng.shuffle(ub)
        u = ub[: rng.randint(0, len(ub))]
        if len(u) == len(ub):        # would leave nothing working
            u = u[1:]                # keep one
        for i in u:
            after[i - 1] = False
        days.append((p, z, u, after))
        ok = after
    return L, k, x, days

def build(L, k, x, days):
    out = [f"{len(x)} {L} {k} {len(days)}", " ".join(map(str, x))]
    for (p, z, u, after) in days:
        out.append(f"{len(z)} {len(u)} {p}")
        out.append(" ".join(map(str, z)))
        out.append(" ".join(map(str, u)))
    return "\n".join(out) + "\n"

def replay(L, k, x, days):
    res = []
    for (p, z, u, after) in days:
        W = [x[i] for i in range(len(x)) if after[i]]
        res.append(day(L, k, W, p) if W else 0)
    return res

rng = random.Random(20261002)
trials = [gen_trial(rng) for _ in range(4000)]
inputs = [build(*t) for t in trials]
wants = [replay(*t) for t in trials]

def run(inp):
    r = subprocess.run([EXE], input=inp, capture_output=True,
                       text=True, timeout=120)
    return [int(v) for v in r.stdout.split()]

with ThreadPoolExecutor(max_workers=16) as ex:
    gots = list(ex.map(run, inputs))

bad = 0
tot = 0
for i, (w, g) in enumerate(zip(wants, gots)):
    tot += len(w)
    if len(g) != len(w):
        print("  LENGTH MISMATCH trial", i, len(w), len(g))
        bad += 1
        continue
    for j, v in enumerate(w):
        if g[j] != v:
            if bad < 5:
                print("  MISMATCH trial", i, "day", j, "want", v, "got", g[j],
                      trials[i][:3])
            bad += 1
print("multi-day mismatches:", bad, "/", tot, "over", len(trials), "trials")
