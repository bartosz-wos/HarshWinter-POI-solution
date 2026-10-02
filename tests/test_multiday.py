"""Generate strictly legal multi-day trials for sur, and check every binary.

Legal per the statement:
  - the z listed stations were previously BROKEN  (repaired)
  - the u listed stations were previously WORKING (damaged)
  - the two lists are disjoint, each increasing, ids in 1..n
  - at least one station always remains working
  - sum(z+u) over the whole run <= 500000
"""
import os, sys, subprocess, random
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(ROOT, "reference"))
from day_model import day_O_n          # noqa: E402

BUDGET = 500000


def gen_trial(rng, n, d, l, k):
    xs = sorted(rng.sample(range(0, l + 1), n))
    lines = [f"{n} {l} {k} {d}", " ".join(map(str, xs))]
    work = set(range(1, n + 1))
    want = []
    for _ in range(d):
        p = rng.randint(0, l)
        broken = sorted(set(range(1, n + 1)) - work)
        z = set(rng.sample(broken, rng.randint(0, len(broken)))) if broken else set()
        # Damage only stations that are working AFTER this morning's repairs.
        pool = sorted((work | z))
        u = set(rng.sample(pool, rng.randint(0, len(pool)))) if pool else set()
        u -= z                                   # lists must be disjoint
        # Keep at least one station working: if this would empty the set,
        # retract the last damage (i.e. that station simply stays up).
        if not ((work | z) - u) and u:
            u.discard(max(u))
        work = (work | z) - u
        assert work, "generator produced an all-down state"
        z, u = sorted(z), sorted(u)
        lines.append(f"{len(z)} {len(u)} {p}")
        lines.append(" ".join(map(str, z)))
        lines.append(" ".join(map(str, u)))
        want.append(day_O_n(l, k, [xs[s - 1] for s in sorted(work)], p))
    return "\n".join(lines) + "\n", want


def run(exe, src):
    r = subprocess.run([exe], input=src, capture_output=True, text=True, timeout=120)
    if r.returncode != 0:
        raise RuntimeError(f"{exe} exited {r.returncode}\n{src[:300]}\n{r.stderr[:200]}")
    return [int(v) for v in r.stdout.split()]


def main(trials=2000, seed=20261002):
    rng = random.Random(seed)
    cases = []
    for _ in range(trials):
        l = rng.choice([2, 3, 5, 8, 13, 30, 100, 1000, 10000])
        n = rng.randint(1, min(l + 1, 60))
        k = rng.choice([1, l, max(1, l // 2), rng.randint(1, l)])
        d = rng.randint(1, 8)
        cases.append(gen_trial(rng, n, d, l, k))

    exes = [("sur", os.path.join(ROOT, "build", "sur")),
            ("sur_bitmap", os.path.join(ROOT, "build", "sur_bitmap")),
            ("sur_sweep", os.path.join(ROOT, "build", "sur_sweep"))]
    results = {}
    for name, exe in exes:
        with ThreadPoolExecutor(max_workers=16) as pool:
            results[name] = list(pool.map(lambda s: run(exe, s), (c[0] for c in cases)))

    total_bad = 0
    for name, _ in exes:
        bad = tot = 0
        for i, ((_, w), g) in enumerate(zip(cases, results[name])):
            tot += len(w)
            if len(g) != len(w):
                bad += 1
                if bad <= 3:
                    print(f"  LENGTH MISMATCH [{name}] trial {i}: want {len(w)} got {len(g)}")
                continue
            for j, v in enumerate(w):
                if g[j] != v:
                    bad += 1
                    if bad <= 3:
                        print(f"  MISMATCH [{name}] trial {i} day {j}: want {v} got {g[j]}")
        print(f"multi-day mismatches [{name}]: {bad} / {tot} over {len(cases)} trials")
        total_bad += bad
    return 1 if total_bad else 0


if __name__ == "__main__":
    sys.exit(main())
