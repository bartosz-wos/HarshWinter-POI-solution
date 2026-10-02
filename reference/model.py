
import sys, random
from oracle import solve_day


def tri(x, k):
    if x <= 0:
        return 0
    m = (x - 1) // k
    return m * x - k * m * (m + 1) // 2


def f(x, k):
    return x + tri(x, k)


def ECLOSE(g, k):
    return 2 * f(g, k)


def EOPEN(g, k):
    return 2 * f(g, k) - g


def IPASS(g, k):
    m = max(0, g - k)
    a = m // 2
    b = m - a
    return g + 2 * (f(a, k) + f(b, k))


def ICLOSE(g, k):
    return min(2 * f(D, k) + 2 * f(g - D, k) for D in range(g + 1))


def IOPEN(g, k):
    return f(g, k)


def costs(L, k, W):
    n = len(W)
    ends = [W[0]] + [W[i] - W[i - 1] for i in range(1, n)] + [L - W[-1]]
    nG = n + 1
    CL, OP, PS = [], [], []
    for i, g in enumerate(ends):
        if g == 0:
            CL.append(0); OP.append(0); PS.append(0)
        elif i == 0 or i == nG - 1:
            CL.append(ECLOSE(g, k)); OP.append(EOpenV(g, k)); PS.append(ECLOSE(g, k))
        else:
            CL.append(ICLOSE(g, k)); OP.append(IOPEN(g, k)); PS.append(IPASS(g, k))
    return CL, OP, PS


def EOpenV(g, k):
    return EOPEN(g, k)


def day_slow(L, k, W, p):
    CL, OP, PS = costs(L, k, W)
    n = len(W)
    nG = n + 1
    best = None
    for s in range(n):
        for d in (1, -1):
            run = ([s + 1 + t for t in range(nG - s - 1)] if d == 1
                   else [s - t for t in range(s + 1)])
            for t in range(len(run)):
                j = run[t]
                cost = 0
                for i in range(nG):
                    if i == j:
                        cost += OP[i]
                    elif i in run[:t]:
                        cost += PS[i]
                    else:
                        cost += CL[i]
                c = abs(p - W[s]) + cost
                if best is None or c < best:
                    best = c
    return best


def day_fast(L, k, W, p):
    CL, OP, PS = costs(L, k, W)
    n = len(W)
    nG = n + 1
    CLtot = sum(CL)
    delta = [PS[i] - CL[i] for i in range(nG)]
    save = [OP[i] - CL[i] for i in range(nG)]
    best = None
    for s in range(n):
        for j in range(s + 1, nG):
            c = abs(p - W[s]) + CLtot + save[j] - delta[j]
            if best is None or c < best:
                best = c
        for j in range(0, s + 1):
            c = abs(p - W[s]) + CLtot + save[j] + delta[j]
            if best is None or c < best:
                best = c
    return best


if __name__ == "__main__":
    print("sample:", solve_day(5, 2, [2, 5], 3), day_fast(5, 2, [2, 5], 3))
    random.seed(269)
    bad = 0
    N = 4000
    for it in range(N):
        L = random.randint(1, 16)
        k = random.randint(1, L)
        nst = random.randint(1, min(L + 1, 4))
        W = sorted(random.sample(range(L + 1), nst))
        p = random.randint(0, L)
        a = solve_day(L, k, W, p)
        b = day_slow(L, k, W, p)
        c = day_fast(L, k, W, p)
        if not (a == b == c):
            bad += 1
            if bad <= 8:
                print("L=%d k=%d W=%s p=%d brute=%d slow=%d fast=%d"
                      % (L, k, W, p, a, b, c))
    print("mismatches: %d / %d" % (bad, N))
