"""Generate the 4 official 'ocen' tests from the statement.

Per the statement: the z stations listed as a_1..a_z were previously BROKEN and
are repaired; the u stations listed as b_1..b_u were previously WORKING and are
damaged.  The two sets are disjoint, each list is increasing, station ids are
1-based, and at least one station always remains working.
"""


def ocen1():
    # n=5, l=12, k=1, d=5; x=[1,3,6,9,11];
    # on day i (1..5) only station i is not working; p is x_i.
    n, l, k, d = 5, 12, 1, 5
    x = [1, 3, 6, 9, 11]
    o = [f"{n} {l} {k} {d}", " ".join(map(str, x))]
    work = set(range(1, n + 1))
    for i in range(1, n + 1):
        cur = {i}                              # only station i is down tonight
        repair = sorted(cur - work)            # were broken, now work
        damage = sorted(work - cur)            # were working, now fail
        work = cur
        o.append(f"{len(repair)} {len(damage)} {x[i-1]}")
        o.append(" ".join(map(str, repair)))
        o.append(" ".join(map(str, damage)))
    return "\n".join(o) + "\n"


def ocen2():
    # n=11, l=100, k=1, d=26; xi = 10(i-1);
    # odd days only odd-numbered stations work, even days only even ones;
    # day i asks position 4(i-1).  All stations work before night 1.
    n, l, k, d = 11, 100, 1, 26
    x = [10 * i for i in range(n)]
    o = [f"{n} {l} {k} {d}", " ".join(map(str, x))]
    work = set(range(1, n + 1))
    for day in range(1, d + 1):
        keep = 1 if day % 2 == 1 else 0        # odd day -> odd stations
        cur = {s for s in range(1, n + 1) if s % 2 == keep}
        repair = sorted(work - cur)            # working now, must break -> NO
        damage = sorted(cur - work)            # broken now, must repair -> NO
        # repair list a = stations that were BROKEN and now work
        # damage list b = stations that were WORKING and now fail
        repair, damage = damage, repair
        work = cur
        o.append(f"{len(repair)} {len(damage)} {4*(day-1)}")
        o.append(" ".join(map(str, repair)))
        o.append(" ".join(map(str, damage)))
    return "\n".join(o) + "\n"


def ocen3():
    n, l, k, d = 45, 2**23, 4, 2**13 + 1
    x = list(range(20, 42)) + [l - v for v in range(42, 19, -1)]
    assert len(x) == 45 and x == sorted(set(x))
    o = [f"{n} {l} {k} {d}", " ".join(map(str, x))]
    for day in range(1, d + 1):
        o.append(f"0 0 {2**10*(day-1)}")
        o.append("")
        o.append("")
    return "\n".join(o) + "\n"


def ocen4():
    # night 1: all but the first station break.  night 2: all repair.
    n, l, k, d = 250000, 10**9, 1, 2
    x = [4000 * i for i in range(n)]
    o = [f"{n} {l} {k} {d}", " ".join(map(str, x))]
    rest = list(range(2, n + 1))
    o.append(f"0 {len(rest)} 0")               # nothing repaired, 2..n damaged
    o.append("")
    o.append(" ".join(map(str, rest)))
    o.append(f"{len(rest)} 0 0")               # 2..n repaired
    o.append(" ".join(map(str, rest)))
    o.append("")
    return "\n".join(o) + "\n"


if __name__ == "__main__":
    from pathlib import Path
    # Write into the build directory, never the caller's CWD.  These used to be
    # dumped as ./in_*ocen.txt, so running the generator from the repo root
    # littered the working tree and `git add -A` committed a 5.5 MB copy of
    # 4ocen -- the very file the README says is deliberately not committed.
    out = Path(__file__).resolve().parent.parent / "build" / "ocen"
    out.mkdir(parents=True, exist_ok=True)
    for name, fn in (("1ocen", ocen1), ("2ocen", ocen2), ("3ocen", ocen3), ("4ocen", ocen4)):
        t = fn()
        f = out / f"in_{name}.txt"
        f.write_text(t)
        print(f"wrote {f}  header: {t.splitlines()[0]}")
