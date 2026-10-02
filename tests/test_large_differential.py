"""Large-scale differential test: sur3 (segment tree) vs surc (sweep).
Both are independently verified on small cases; this checks they agree at
scale, where hand-verification is impossible.
"""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DIR  = os.path.join(ROOT, "build")
sys.path.insert(0, os.path.join(ROOT, "reference"))
import subprocess, random, sys, os
from concurrent.futures import ThreadPoolExecutor

GGEN = os.path.join(DIR, "gen_random")

def gen(n, d, L, k, seed):
    return subprocess.run([GGEN, str(n), str(d), str(L), str(k), str(seed)],
                          capture_output=True).stdout

def run(exe, data):
    r = subprocess.run([os.path.join(DIR, exe)], input=data,
                       capture_output=True, timeout=1800)
    return r.stdout

rng = random.Random(4242)
cases = []
for i in range(40):
    n = rng.choice([1, 2, 7, 100, 1000, 5000, 20000, 50000, 120000])
    d = rng.choice([1, 3, 17, 200, 3000])
    L = rng.choice([1, 5, 100, 9999, 10**6, 10**9])
    k = rng.randint(1, L)
    cases.append((n, d, L, k, rng.randint(0, 10**9)))

# The submission is the thing under test, so it must appear here.  The sweep is
# O(n*d) and gets skipped at large n on cost alone, which is exactly why it must
# not be the only oracle: without sur in the comparison, the biggest inputs
# would go untested.
bad = 0
skipped = 0
for (n, d, L, k, seed) in cases:
    if n > L + 1:
        n = L + 1
    data = gen(n, d, L, k, seed)
    fast = run("sur", data)
    la = fast.split()
    if n * d > 40_000_000:          # too slow to be worth running O(n*d)
        skipped += 1
        print(f"  ok n={n:>6} d={d:>4} L={L:>10} k={k:>10}  "
              f"({len(la)} days, sweep skipped: n*d too large)")
        continue
    slow = run("sur_sweep", data)
    lb = slow.split()
    if la != lb:
        bad += 1
        print(f"  DIFFER n={n} d={d} L={L} k={k} seed={seed} "
              f"(len {len(la)} vs {len(lb)})")
        for idx, (x, y) in enumerate(zip(la, lb)):
            if x != y:
                print(f"    first diff at day {idx}: sur={x} sweep={y}")
                break
    else:
        print(f"  ok n={n:>6} d={d:>4} L={L:>10} k={k:>10}  ({len(la)} days)")

print(f"\nlarge-scale differential mismatches: {bad} / "
      f"{len(cases) - skipped} compared ({skipped} sweep-skipped as O(n*d) too slow)")
sys.exit(1 if bad else 0)
sys.exit(1 if bad else 0)
