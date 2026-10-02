
"""Ground-truth oracle for OI XXVIII 'Surowa zima' (sur).

State = (position, charge, cleared_mask) over road [0,L].
  move +-1 (cost 1); if the crossed segment is uncleared and charge>0, it is
  cleared and charge drops by 1.
  at a WORKING station you may refill to k (cost 0).
Terminal: cleared_mask == full (optionally requiring a final position).
"""
from heapq import heappush, heappop


def solve_day(L, k, working, p, end_at=None):
    working = set(working)
    full = (1 << L) - 1
    s0 = (p, 0, 0)
    dist = {s0: 0}
    pq = [(0, p, 0, 0)]
    while pq:
        d, pos, ch, mask = heappop(pq)
        s = (pos, ch, mask)
        if d > dist.get(s, 1 << 60):
            continue
        if mask == full and (end_at is None or pos == end_at):
            return d

        def relax(ns, nd):
            if nd < dist.get(ns, 1 << 60):
                dist[ns] = nd
                heappush(pq, (nd,) + ns)

        if pos in working and ch < k:
            relax((pos, k, mask), d)
        for nq in (pos - 1, pos + 1):
            if not 0 <= nq <= L:
                continue
            seg = min(pos, nq)
            nm, nc = mask, ch
            if not (mask >> seg) & 1 and ch > 0:
                nm = mask | (1 << seg)
                nc = ch - 1
            relax((nq, nc, nm), d + 1)
    return -1


def tri(x, k):
    """sum_{j>=1} max(0, x - j*k)"""
    if x <= 0:
        return 0
    m = (x - 1) // k
    return m * x - k * m * (m + 1) // 2


def f(x, k):
    return x + tri(x, k)


def one_end_close(g, k):
    return 2 * f(g, k)


def one_end_open(g, k):
    return 2 * f(g, k) - g


def gap_close(g, k):
    return min(2 * f(D, k) + 2 * f(g - D, k) for D in range(g + 1))


def gap_open(g, k):
    return min(2 * f(D, k) + 2 * f(g - D, k) - max(D, g - D) for D in range(g + 1))
