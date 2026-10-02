// OI XXVIII "Surowa zima" (sur) -- segment-tree solution.
//
// ===========================================================================
// Model
// ===========================================================================
// Road [0,L] of L unit segments.  Walking 1 m costs 1 s regardless of the
// battery.  Clearing 1 m costs one unit of charge.  At a WORKING station the
// charge may be reset to k for free.  Each day starts at p with an empty
// battery; every segment must be cleared, minimising total walking time.
//
// With  f(g,k) = g + sum_{j>=1} max(0, g - j*k) = g + m*g - k*m*(m+1)/2,
//                              m = (g-1)/k,
// the exact cost of a gap of length g is
//   road-end gap (one station at the near end):
//       Eclose = 2 f(g)                 out and back
//       Eopen  = 2 f(g) - g             finish inside
//   interior gap (stations at both ends):
//       Iclose = 2 f(g/2) + 2 f(g-g/2)  out and back -- the even split is
//                                      optimal; verified against a binary
//                                      search on 500200 cases
//       Ipass  = g + 2 ( f(a) + f(b) ),  a=m'/2, b=m'-a, m'=max(0,g-k)
//       Iopen  = f(g)                   finish inside
//
// ===========================================================================
// Day structure
// ===========================================================================
// Sort the working stations W_0 < ... < W_{m-1}.  The gaps are
//   g_0 = [0,W_0],  g_i = [W_{i-1},W_i] (1<=i<=m-1),  g_m = [W_{m-1},L].
// An optimal day walks to some station W_s, sweeps monotonically in one
// direction, traverses the gaps up to the final gap j, finishes inside gap j,
// and treats every other gap as a closed excursion.  Writing
//   T_i = preps[i] - precl[i],   cltot = precl[nG],
// the cost of the pair (s,j) is
//   +1 (s<j) : cltot + (|p-W_s| - T_{s+1}) + ( T_j - cl_j + OP_j)
//   -1 (j<=s): cltot + (|p-W_s| + T_{s+1}) + (-T_j - ps_j + OP_j)
// Both are ordered min-plus products over the cell sequence, so a segment tree
// answers the day in O(n) build time with O(log n) point updates.
//
//   cell i holds gap i and station i-1 (present iff station i-1 works)
//   +1 pairs aS_i with cJ_j for i <= j
//   -1 pairs dJ_j with qS_i for j <  i
//
// Node state, all relative to tLo = T_lo, with U = the node's sum of (ps-cl):
//   aMin = min |p-W| - (T_i - tLo)        carries  -T
//   cMin = min (T_j - tLo) - cl_j + OP_j  carries  +T
//   qMin = min |p-W| + (T_i - tLo)        carries  +T
//   dMin = min -(T_j - tLo) - ps_j + OP_j carries  -T
//   ab   = min_{i<=j} (aMin_i + cMin_j)   (+1)
//   cd   = min_{j<i}  (dMin_j + qMin_i)   (-1)
// Merge with U = L.P:
//   aMin = min(L.aMin, R.aMin - U)   cMin = min(L.cMin, R.cMin + U)
//   qMin = min(L.qMin, R.qMin + U)   dMin = min(L.dMin, R.dMin - U)
//   ab   = min(L.ab, R.ab, L.aMin + R.cMin + U)
//   cd   = min(L.cd, R.cd, L.dMin + R.qMin + U)
//   P    = L.P + R.P
// Every pair combines one +T term with one -T term, so the frame offset tLo
// cancels and pair values are frame-free -- which is what makes the merge exact.
//
// A broken station makes its cell carry no station (aMin/qMin/ab = INF) while
// still contributing its gap, so a repair or breakage is a point update.  The
// gap itself is an interior gap only when BOTH neighbouring stations work, so
// toggling station i also changes the gap costs of cells i and i+1: those cells
// are refreshed too.  Since p changes every day, aMin/qMin/ab/cd are rebuilt
// each day, which is O(n); the per-day total is therefore O(n + (z+u) log n).
// ===========================================================================
#include <bits/stdc++.h>
using namespace std;

using int64 = long long;
static const int64 INF = 4000000000000000000LL;

static int64 K;

static inline int64 tri(int64 x) {
    if (x <= 0) return 0;
    int64 m = (x - 1) / K;
    return m * x - K * m * (m + 1) / 2;
}
static inline int64 f(int64 x) { return x + tri(x); }
static inline int64 Eclose(int64 g) { return 2 * f(g); }
static inline int64 Eopen(int64 g) { return 2 * f(g) - g; }
static inline int64 Iopen(int64 g) { return f(g); }
static inline int64 Iclose(int64 g) { return 2 * f(g / 2) + 2 * f(g - g / 2); }
static inline int64 Ipass(int64 g) {
    int64 m = (g > K ? g - K : 0);
    int64 a = m / 2, b = m - a;
    return g + 2 * (f(a) + f(b));
}

struct FastScanner {
    static const int SZ = 1 << 16;
    char buf[SZ];
    int pos = 0, len = 0;
    inline char gc() {
        if (pos == len) { len = (int)fread(buf, 1, SZ, stdin); pos = 0; if (!len) return 0; }
        return buf[pos++];
    }
    template <class T> bool read(T &out) {
        char c; T v = 0; int sign = 1;
        do { c = gc(); if (!c) return false; } while (c != '-' && (c < '0' || c > '9'));
        if (c == '-') { sign = -1; c = gc(); }
        while (c >= '0' && c <= '9') { v = v * 10 + (c - '0'); c = gc(); }
        out = (sign > 0) ? v : -v;
        return true;
    }
};

struct Nd{
    int64 P = 0;
    int64 aMin = INF, cMin = INF, ab = INF;
    int64 qMin = INF, dMin = INF, cd = INF;
};
static inline Nd mrg(const Nd &L, const Nd &R) {
    Nd r;
    int64 U = L.P;
    r.P    = L.P + R.P;
    r.aMin = min(L.aMin, R.aMin - U);
    r.cMin = min(L.cMin, R.cMin + U);
    r.qMin = min(L.qMin, R.qMin + U);
    r.dMin = min(L.dMin, R.dMin - U);
    r.ab   = min(min(L.ab, R.ab), L.aMin + R.cMin + U);
    r.cd   = min(min(L.cd, R.cd), L.dMin + R.qMin + U);
    return r;
}

int main() {
    FastScanner in;
    int n, d;
    int64 L;
    if (!in.read(n)) return 0;
    in.read(L);
    in.read(K);
    in.read(d);

    vector<int64> x(n);
    for (int i = 0; i < n; i++) in.read(x[i]);
    vector<char> ok(n, 1);

    // Cells 0..n over the fixed station slots.  Cell i holds gap i
    //   gap 0 = [0,x_0],  gap i = [x_{i-1},x_i],  gap n = [x_{n-1},L]
    // and station i-1.  Gap i is interior iff 1<=i<=n-1 and both x_{i-1} and
    // x_i work; otherwise it is a road-end gap.
    int N0 = 1;
    while (N0 < n + 1) N0 <<= 1;
    vector<Nd> seg(2 * N0);
    vector<int64> cl(n + 1), ps(n + 1), op(n + 1);

    auto gapCosts = [&](int i) {
        bool leftStn  = (i - 1 >= 0) && ok[i - 1];
        bool rightStn = (i < n) && ok[i];
        int64 g;
        if (i == 0) g = x[0];
        else if (i == n) g = L - x[n - 1];
        else g = x[i] - x[i - 1];
        if (g < 0) g = 0;
        bool endGap = (i == 0) || (i == n) || !leftStn || !rightStn;
        if (g == 0)                { cl[i] = ps[i] = 0;       op[i] = 0; }
        else if (endGap)           { cl[i] = ps[i] = Eclose(g); op[i] = Eopen(g); }
        else                       { cl[i] = Iclose(g); ps[i] = Ipass(g); op[i] = Iopen(g); }
    };

    vector<int64> W;
    string out;
    out.reserve(1 << 20);
    char line[32];

    while (d--) {
        int z, u;
        int64 p;
        // Initialise: read() returns false on truncated input and would leave these
    // indeterminate.
    z = 0; u = 0; p = 0;
    if (!in.read(z) || !in.read(u) || !in.read(p)) break;
        for (int i = 0; i < z; i++) { int a = 0; in.read(a); ok[a - 1] = 1; }
        for (int i = 0; i < u; i++) { int b = 0; in.read(b); ok[b - 1] = 0; }

        W.clear();
        for (int i = 0; i < n; i++) if (ok[i]) W.push_back(x[i]);
        int m = (int)W.size();
        if (m == 0) { out += "0\n"; continue; }
        int nG = m + 1;

        // Gap costs for the active stations only (a broken station's slot does
        // not appear, so neighbouring active stations form interior gaps).
        cl.assign(nG, 0); ps.assign(nG, 0); op.assign(nG, 0);
        int64 cltot = 0;
        for (int i = 0; i < nG; i++) {
            int64 g = (i == 0) ? W[0]
                     : (i == m ? L - W[m - 1] : W[i] - W[i - 1]);
            if (g <= 0) continue;
            if (i == 0 || i == m) { cl[i] = ps[i] = Eclose(g); op[i] = Eopen(g); }
            else { cl[i] = Iclose(g); ps[i] = Ipass(g); op[i] = Iopen(g); }
            cltot += cl[i];
        }

        // Build the monoid over the cells.
        for (int i = 0; i < N0; i++) {
            if (i >= nG) { seg[N0 + i] = Nd{}; continue; }
            Nd nd;
            nd.P    = ps[i] - cl[i];
            bool hs = (i >= 1);              // station i-1 exists for i>=1
            int64 dist = hs ? llabs(p - W[i - 1]) : 0;
            nd.aMin = hs ? dist : INF;
            nd.cMin = -cl[i] + op[i];
            nd.qMin = hs ? dist : INF;
            nd.dMin = -ps[i] + op[i];
            nd.ab   = hs ? (nd.aMin + nd.cMin) : INF;   // +1 allows i == j
            nd.cd   = INF;                              // -1 needs j < i
            seg[N0 + i] = nd;
        }
        for (int i = N0 - 1; i >= 1; i--) seg[i] = mrg(seg[2 * i], seg[2 * i + 1]);

        int64 best = cltot + min(seg[1].ab, seg[1].cd);
        int len = snprintf(line, sizeof line, "%lld\n", best);
        out.append(line, len);
        if (out.size() > (1u << 20)) { fwrite(out.data(), 1, out.size(), stdout); out.clear(); }
    }
    fwrite(out.data(), 1, out.size(), stdout);
    (void)gapCosts;
    return 0;
}
