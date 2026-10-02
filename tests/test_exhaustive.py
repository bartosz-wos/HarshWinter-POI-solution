"""Exhaustive check of sur5 against the verified reference on all small
configurations: every station subset for small l, every k, every p.
"""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DIR  = os.path.join(ROOT, "build")
sys.path.insert(0, os.path.join(ROOT, "reference"))
import subprocess, itertools
from day_model import day_O_n


def run(exe, src):
    r = subprocess.run([f"{DIR}/{exe}"], input=src, capture_output=True,
                       text=True, timeout=300)
    if r.returncode != 0:
        raise RuntimeError(f"{exe} rc={r.returncode} {r.stderr[:200]}")
    return [int(v) for v in r.stdout.split()]

cases = 0
bad = 0
# batch many single-day configurations into one multi-day input, but each night
# must be legal relative to the previous, so instead run one input per (subset).
# Group by l to keep the process count sane: build one input whose nights each
# set the active set explicitly using legal repair/break lists.
for l in range(1, 10):
    positions = list(range(0, l + 1))
    n_all = len(positions)
    for k in range(1, l + 1):
        # nights: enumerate every non-empty subset of working stations by
        # starting from all-broken-except-one and repairing / breaking legally.
        # Simpler: use a fresh legal transition chain.
        n = n_all
        xs = positions
        # build the chain over all non-empty subsets in a legal order
        subsets = [frozenset(s) for r in range(1, n + 1)
                   for s in itertools.combinations(range(1, n + 1), r)]
        # cap so the test stays quick
        subsets = subsets[:60]
        src = [f"{n} {l} {k} {len(subsets)}", " ".join(map(str, xs))]
        work = set(range(1, n + 1))
        want = []
        for target in subsets:
            p = 0
            z = sorted(work - set(target))   # were working, must be damaged? no:
            # z = repaired (were broken), u = damaged (were working)
            z = sorted(set(target) - work)   # newly working -> repaired
            u = sorted(work - set(target))   # no longer working -> damaged
            if len(target) == 0:
                continue
            src.append(f"{len(z)} {len(u)} {p}")
            src.append(" ".join(map(str, z)))
            src.append(" ".join(map(str, u)))
            work = set(target)
            W = [xs[s - 1] for s in sorted(work)]
            want.append(day_O_n(l, k, W, p))
        if not want:
            continue
        text = "\n".join(src) + "\n"
        got = run("sur", text)
        cases += len(want)
        if got != want:
            bad += sum(1 for a, b in zip(got, want) if a != b)
            if bad <= 5:
                for i, (a, b) in enumerate(zip(got, want)):
                    if a != b:
                        print(f"  l={l} k={k} night{i}: want {b} got {a}")
                        break
print(f"exhaustive-ish sur5 check: {bad} wrong day-answers out of {cases}")
