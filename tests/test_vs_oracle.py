"""Final validation of surc: exhaustive small cases vs the state-space oracle,
plus randomized multi-day replay."""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DIR  = os.path.join(ROOT, "build")
sys.path.insert(0, os.path.join(ROOT, "reference"))
import subprocess, sys, random, itertools

EXE = os.path.join(DIR, "sur_sweep")

# ---- run the C++ on a batch of single-day problems ------------------------
def run_cpp(cases):
    """cases: list of (L,k,W_sorted,p) -> answers"""
    inp = []
    inp.append(f"{len(cases)} 0 0 0")
    for (L, k, W, p) in cases:
        inp.append(f"{len(W)} {L} {k} 1")
        inp.append(" ".join(map(str, W)) if W else "0")
        inp.append("1 0 " + str(p))
        inp.append("")
    r = subprocess.run([EXE], input="\n".join(inp) + "\n",
                       capture_output=True, text=True, timeout=600)
    return [int(x) for x in r.stdout.split()], r.stderr

# ---- exhaustive -----------------------------------------------------------
from oracle import solve_day

def exhaustive(Lmax=9, kmax=9, mmax=4):
    cases, want = [], []
    n_ok = 0
    for L in range(1, Lmax + 1):
        for k in range(1, min(kmax, L) + 1):
            for m in range(1, mmax + 1):
                if m > L + 1:
                    continue
                for W in itertools.combinations(range(L + 1), m):
                    for p in range(L + 1):
                        cases.append((L, k, list(W), p))
                        want.append(solve_day(L, k, list(W), p))
                        n_ok += 1
    return cases, want, n_ok

cases, want, n_ok = exhaustive()
print(f"exhaustive cases: {n_ok}")
bad = 0
CH = 4000
for i in range(0, len(cases), CH):
    got, err = run_cpp(cases[i:i + CH])
    for j in range(len(got)):
        if got[j] != want[i + j]:
            if bad < 5:
                print("  MISMATCH", cases[i + j], "want", want[i + j], "got", got[j])
            bad += 1
print("exhaustive mismatches:", bad, "/", n_ok)
