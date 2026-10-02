"""Validate a sur input file against every rule the statement guarantees."""
import sys


def check(path, verbose=True):
    txt = open(path).read().split("\n")
    n, l, k, d = map(int, txt[0].split())
    errs = []
    if not (1 <= n <= 250000): errs.append(f"n={n} out of range")
    if not (1 <= l <= 10**9):  errs.append(f"l={l} out of range")
    if not (1 <= k <= l):      errs.append(f"k={k} out of range")
    if not (1 <= d <= 250000): errs.append(f"d={d} out of range")
    xs = list(map(int, txt[1].split()))
    if len(xs) != n: errs.append(f"x has {len(xs)} values, expected {n}")
    if xs != sorted(xs): errs.append("x not increasing")
    if len(set(xs)) != len(xs): errs.append("x has duplicates")
    if xs and (xs[0] < 0 or xs[-1] > l): errs.append("x outside [0,l]")
    work = set(range(1, n + 1))          # all stations work before night 1
    idx = 2
    total_zu = 0
    for day in range(d):
        if idx + 2 >= len(txt) and day < d:
            errs.append(f"day {day+1}: truncated input")
            break
        z, u, p = map(int, txt[idx].split()); idx += 1
        Z = list(map(int, txt[idx].split())) if txt[idx].strip() else []; idx += 1
        U = list(map(int, txt[idx].split())) if txt[idx].strip() else []; idx += 1
        # the statement: line 2 = repaired (previously broken),
        #                line 3 = damaged  (previously working)
        if len(Z) != z: errs.append(f"day {day+1}: z says {z}, list has {len(Z)}")
        if len(U) != u: errs.append(f"day {day+1}: u says {u}, list has {len(U)}")
        if Z != sorted(Z): errs.append(f"day {day+1}: z list not increasing")
        if U != sorted(U): errs.append(f"day {day+1}: u list not increasing")
        if set(Z) & set(U): errs.append(f"day {day+1}: z and u overlap")
        if any(s in work for s in Z): errs.append(f"day {day+1}: z lists a WORKING station")
        if any(s not in work for s in U): errs.append(f"day {day+1}: u lists a BROKEN station")
        if any(not (1 <= s <= n) for s in Z + U): errs.append(f"day {day+1}: station id out of 1..n")
        if not (0 <= p <= l): errs.append(f"day {day+1}: p={p} outside [0,l]")
        work = (work | set(Z)) - set(U)
        if not work: errs.append(f"day {day+1}: ALL stations down")
        total_zu += z + u
    if total_zu > 500000: errs.append(f"sum(z+u) = {total_zu} > 500000")
    if verbose:
        print(f"{path}: {'OK' if not errs else 'PROBLEMS'}")
        for e in errs[:6]:
            print("   ", e)
    return errs


if __name__ == "__main__":
    bad = 0
    for p in sys.argv[1:]:
        bad += len(check(p))
    print("total problems:", bad)
