"""Exhaustive oracle check for the SUBMISSION (src/sur.cpp).

Each (L, k, working-station-set, p) configuration is fed to the compiled binary
as its own complete, statement-legal input, and the answer is compared against
reference/oracle.py -- a literal state-space brute force.

Why one input per invocation: the C++ program reads a single test case per
process (a header line `n l k d`, the station list, then `d` nights).  An
earlier version of this file packed many configurations into one file behind a
`0 0 0` sentinel header; the binary produced no output, the comparison loop ran
zero times, and the suite reported "0 mismatches / 68706" while checking
nothing.  A test that cannot fail is worse than no test, so the runner now
asserts that the number of answers returned equals the number of questions
asked, and fails loudly otherwise.

Run directly, or via ./run_tests.sh.
"""
import os, sys, subprocess, itertools
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
BUILD = os.path.join(ROOT, "build")
sys.path.insert(0, os.path.join(ROOT, "reference"))

from oracle import solve_day          # noqa: E402

EXE = os.path.join(BUILD, "sur")
BITMAP = os.path.join(BUILD, "sur_bitmap")


def one_case(L, k, W, p):
    """A complete, legal single-day input: all stations work, no updates."""
    n = len(W)
    return (f"{n} {L} {k} 1\n"
            + " ".join(map(str, W)) + "\n"
            + "0 0 " + str(p) + "\n\n\n")


def run_batch(triples, exe=EXE):
    """Ask the binary each case in its own process.

    `triples` is a list of (L, k, W, p).  Returns a list of answers, or raises
    if the binary failed or answered the wrong number of questions.
    """
    def one(item):
        (L, k, W, p) = item
        src = one_case(L, k, W, p)
        r = subprocess.run([exe], input=src, capture_output=True,
                           text=True, timeout=60)
        if r.returncode != 0:
            raise RuntimeError(
                f"{os.path.basename(exe)} exited {r.returncode} on\n{src}"
                f"stderr: {r.stderr[:300]}")
        got = r.stdout.split()
        if len(got) != 1:
            raise RuntimeError(
                f"expected exactly 1 answer, got {len(got)} ({got!r}) from\n{src}"
                f"\nA wrong answer count means the harness and the binary "
                f"disagree about the input format -- stop and fix that, do not "
                f"let the comparison loop silently compare nothing.")
        return int(got[0])

    # One process per case is required (the binary reads a single test case),
    # so overlap them: 68k serial subprocesses dominates the runtime otherwise.
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        return list(pool.map(one, triples))


def exhaustive(Lmax=9, kmax=9, mmax=4):
    cases = []
    for L in range(1, Lmax + 1):
        for k in range(1, min(kmax, L) + 1):
            for m in range(1, mmax + 1):
                if m > L + 1:
                    continue
                for W in itertools.combinations(range(L + 1), m):
                    for p in range(L + 1):
                        cases.append((L, k, list(W), p))
    return cases


def check(exe, cases, want, label):
    bad = 0
    CH = 500
    for i in range(0, len(cases), CH):
        chunk = cases[i:i + CH]
        got = run_batch(chunk, exe)
        # run_batch already refuses to return the wrong number of answers
        for j, g in enumerate(got):
            if g != want[i + j]:
                if bad < 5:
                    print(f"  MISMATCH [{label}] {chunk[j]}: "
                          f"want {want[i+j]}, got {g}")
                bad += 1
    print(f"exhaustive mismatches [{label}]: {bad} / {len(cases)}")
    return bad


def main():
    cases = exhaustive()
    want = [solve_day(L, k, W, p) for (L, k, W, p) in cases]
    print(f"exhaustive cases: {len(cases)}")

    bad = check(EXE, cases, want, "sur")
    # The flat-bitmap variant shares the segment tree and the whole cost model
    # with the submission; only the active-station set is represented
    # differently.  Checking it here means a bug in the bitmap's next()/prev()
    # cannot hide behind the fact that the sample and a few big runs agree.
    if os.path.exists(BITMAP):
        bad += check(BITMAP, cases, want, "sur_bitmap")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
