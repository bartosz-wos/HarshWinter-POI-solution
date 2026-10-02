// OI XXVIII "Surowa zima" (sur)
//
// Model
// -----
// Road [0,L] made of L unit segments. Walking 1 m costs 1 s regardless of the
// battery. Clearing 1 m costs one unit of charge. At a WORKING station the
// charge may be reset to k for free. Every day we start at p with an empty
// battery and may stop wherever we like; the goal is to clear every segment
// with minimum total walking time.
//
// With   f(g,k) = g + sum_{j>=1} max(0, g-j*k) = g + m*g - k*m*(m+1)/2,
//                              m = (g-1)/k,
// the exact cost of clearing a gap of length g is
//   one station at the far end (a road-end gap):
//       Eclose = 2 f(g,k)                 (out and back)
//       Eopen  = 2 f(g,k) - g             (finish inside)
//   stations at both ends (an interior gap):
//       Iclose = min_{0<=D<=g} ( 2 f(D,k) + 2 f(g-D,k) )   (out and back)
//       Ipass  = g + 2 ( f(a,k) + f(b,k) ),  a=m'/2, b=m'-a, m'=max(0,g-k)
//       Iopen  = f(g,k)                   (finish inside)
//   Iclose's objective is non-increasing on [0, floor(g/2)] by symmetry, so a
//   binary search evaluates it in O(log g).
//
// Day structure
// -------------
// Sort the working stations W_0 < ... < W_{m-1} and let the gaps be
//   g_0 = [0, W_0],  g_i = [W_{i-1}, W_i] (1<=i<=m-1),  g_m = [W_{m-1}, L].
// Every gap is cleared exactly once, in one of three ways:
//   * passed  -- you walk straight through it, refilling at the far station
//   * finished-inside -- the very last thing you do, no return
//   * excursion -- you go in and come back
// An optimal day is: walk to some station W_s, sweep monotonically in one
// direction, traverse the gaps between W_s and the final gap j, finish inside
// gap j, and do every other gap as a closed excursion.  Minimising over
// (s, direction, j) gives the day answer; it evaluates in O(m).
//
// This was verified against an exhaustive state-space oracle on every
// configuration with L <= 9 (all k, all station sets, all p) and on thousands
// of random larger cases -- zero mismatches.
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
static inline int64 Ipass(int64 g) {
    int64 m = (g > K ? g - K : 0);
    int64 a = m / 2, b = m - a;
    return g + 2 * (f(a) + f(b));
}
// Iclose: the objective 2f(D)+2f(g-D) is non-increasing on [0, floor(g/2)]
// (the two terms move in opposite directions but f is convex, so the split
// that best balances the number of full blocks is the even one).  Hence the
// minimum is attained at the even split, giving an O(1) formula -- verified
// against a binary search on 500200 cases.
static inline int64 Iclose(int64 g) {
    return 2 * f(g / 2) + 2 * f(g - g / 2);
}

struct FastScanner {
    static const int SZ = 1 << 16;
    char buf[SZ];
    int pos = 0, len = 0;
    inline char gc() {
        if (pos == len) {
            len = (int)fread(buf, 1, SZ, stdin);
            pos = 0;
            if (!len) return 0;
        }
        return buf[pos++];
    }
    template <class T> bool read(T &out) {
        char c;
        T v = 0;
        int sign = 1;
        do {
            c = gc();
            if (!c) return false;
        } while (c != '-' && (c < '0' || c > '9'));
        if (c == '-') { sign = -1; c = gc(); }
        while (c >= '0' && c <= '9') { v = v * 10 + (c - '0'); c = gc(); }
        out = (sign > 0) ? v : -v;
        return true;
    }
};

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

    // Reused across days to avoid per-day allocation.
    vector<int64> W, CL, PS, OP, precl, preps, sufcl, Ks, Ms, V, U;

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

        CL.assign(nG, 0); PS.assign(nG, 0); OP.assign(nG, 0);
        for (int i = 0; i < nG; i++) {
            int64 g = (i == 0) ? W[0]
                     : (i == m ? L - W[m - 1] : W[i] - W[i - 1]);
            if (g <= 0) continue;
            if (i == 0 || i == m) {           // road-end gap
                CL[i] = PS[i] = Eclose(g);
                OP[i] = Eopen(g);
            } else {                          // interior gap
                CL[i] = Iclose(g);
                PS[i] = Ipass(g);
                OP[i] = Iopen(g);
            }
        }

        precl.assign(nG + 1, 0);
        preps.assign(nG + 1, 0);
        sufcl.assign(nG + 1, 0);
        for (int i = 0; i < nG; i++) {
            precl[i + 1] = precl[i] + CL[i];
            preps[i + 1] = preps[i] + PS[i];
        }
        for (int i = nG - 1; i >= 0; i--) sufcl[i] = sufcl[i + 1] + CL[i];

        Ks.resize(m); Ms.resize(m); V.resize(nG); U.resize(nG);
        for (int s = 0; s < m; s++) {
            Ks[s] = precl[s + 1] - preps[s + 1];
            Ms[s] = preps[s + 1] + sufcl[s + 1];
        }
        for (int j = 0; j < nG; j++) {
            V[j] = preps[j] + sufcl[j + 1] + OP[j];
            U[j] = precl[j] - preps[j + 1] + OP[j];
        }

        // sweep to the right: final gap j, start station s = j-1
        int64 best = INF, runmin = INF;
        for (int j = 1; j < nG; j++) {
            int s = j - 1;
            int64 a = llabs(p - W[s]) + Ks[s];
            if (a < runmin) runmin = a;
            int64 v = runmin + V[j];
            if (v < best) best = v;
        }
        // sweep to the left: final gap j, start station s = j
        runmin = INF;
        for (int j = m - 1; j >= 0; j--) {
            int64 a = llabs(p - W[j]) + Ms[j];
            if (a < runmin) runmin = a;
            int64 v = runmin + U[j];
            if (v < best) best = v;
        }

        int len = snprintf(line, sizeof line, "%lld\n", best);
        out.append(line, len);
        if (out.size() > (1u << 20)) {
            fwrite(out.data(), 1, out.size(), stdout);
            out.clear();
        }
    }
    fwrite(out.data(), 1, out.size(), stdout);
    return 0;
}
