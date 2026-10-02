
"""Correct O(n), derived from the instrumented identity.

For d=+1, station s, run = [s+1 .. nG-1], j = run[t]:
  terms: OP[j]  for i=j
         PS[i]  for i in run[:t] = [s+1 .. j-1]
         CL[i]  for all other i  (i.e. [0..s] and [j+1..nG-1])
  cost = |p-W_s| + OP[j] + (preps[j]-preps[s+1]) + (precl[s+1]) + (sufcl[j+1])

For d=-1, station s, run = [0 .. s], j = run[t] (t = s-j):
  run[:t] = [j+1 .. s]
  terms: OP[j]
         PS[i] for i in [j+1 .. s]  = preps[s+1]-preps[j+1]
         CL[i] for i in [0..j-1] and [s+1..nG-1] = precl[j] + (sufcl[s+1])
  cost = |p-W_s| + OP[j] + (preps[s+1]-preps[j+1]) + precl[j] + sufcl[s+1]

Now:
  d=+1: cost = |p-W_s| + (preps[j]-preps[s+1]) + precl[s+1] + sufcl[j+1] + OP[j]
  d=-1: cost = |p-W_s| + (preps[s+1]-preps[j+1]) + precl[j]     + sufcl[s+1] + OP[j]

Group by s for d=+1:  [ -preps[s+1] + precl[s+1] ] depends on s;
                      [ preps[j] + sufcl[j+1] + OP[j] ] depends on j.
  => cost = |p-W_s| + K_s + V_j,   s <= j-1
     K_s = precl[s+1]-preps[s+1]
     V_j = preps[j]+sufcl[j+1]+OP[j]
Group by s for d=-1:  [ preps[s+1] + sufcl[s+1] ] + |p-W_s|  depends on s;
                      [ -preps[j+1] + precl[j] + OP[j] ]  on j.
  => cost = |p-W_s| + M_s + U_j,   s >= j
     M_s = preps[s+1]+sufcl[s+1]
     U_j = precl[j]-preps[j+1]+OP[j]
Two linear scans with running minima. VERIFY."""
import sys, random
from oracle import solve_day
from model import costs, day_slow  # noqa

INF = float("inf")


def day_O_n(L, k, W, p):
    CL, OP, PS = costs(L, k, W)
    n = len(W)
    nG = n + 1
    precl = [0] * (nG + 1)
    preps = [0] * (nG + 1)
    for i in range(nG):
        precl[i + 1] = precl[i] + CL[i]
        preps[i + 1] = preps[i] + PS[i]
    sufcl = [0] * (nG + 1)
    for i in range(nG - 1, -1, -1):
        sufcl[i] = sufcl[i + 1] + CL[i]
    K = [precl[s + 1] - preps[s + 1] for s in range(n)]
    M = [preps[s + 1] + sufcl[s + 1] for s in range(n)]
    V = [preps[j] + sufcl[j + 1] + OP[j] for j in range(nG)]
    U = [precl[j] - preps[j + 1] + OP[j] for j in range(nG)]
    best = INF
    runmin = INF
    for j in range(1, nG):
        s = j - 1
        a = abs(p - W[s]) + K[s]
        if a < runmin:
            runmin = a
        v = runmin + V[j]
        if v < best:
            best = v
    runmin = INF
    for j in range(n - 1, -1, -1):
        s = j
        a = abs(p - W[s]) + M[s]
        if a < runmin:
            runmin = a
        v = runmin + U[j]
        if v < best:
            best = v
    return best


if __name__ == "__main__":
    print("sample:", solve_day(5, 2, [2, 5], 3), day_O_n(5, 2, [2, 5], 3))
    random.seed(331)
    bad = 0
    N = 8000
    for it in range(N):
        L = random.randint(1, 18)
        k = random.randint(1, L)
        nst = random.randint(1, min(L + 1, 5))
        W = sorted(random.sample(range(L + 1), nst))
        p = random.randint(0, L)
        a = solve_day(L, k, W, p)
        b = day_slow(L, k, W, p)
        c = day_O_n(L, k, W, p)
        if not (a == b == c):
            bad += 1
            if bad <= 8:
                print("L=%d k=%d W=%s p=%d brute=%d slow=%d On=%d"
                      % (L, k, W, p, a, b, c))
    print("mismatches: %d / %d" % (bad, N))
