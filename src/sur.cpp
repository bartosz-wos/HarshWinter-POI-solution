// OI XXVIII "Surowa zima" (sur) -- O(n + (d + sum_{i=1..d} (z_i + u_i)) log n).
//
// ===========================================================================
// Model
// ===========================================================================
// Road [0,l] of l unit segments.  Walking 1 m costs 1 s regardless of the
// battery.  Clearing 1 m costs one unit of charge.  At a WORKING station the
// charge resets to k for free.  Each day starts at p with an empty battery and
// every segment must be cleared, minimising total walking time.
//
// With  f(g,k) = g + sum_{j>=1} max(0, g - j*k) = g + m*g - k*m*(m+1)/2,
//                              m = (g-1)/k,
// the exact cost of a gap of length g is
//   road-end gap (only one station at the near end):
//       Eclose = 2 f(g)                 out and back
//       Eopen  = 2 f(g) - g             finish inside
//   interior gap (working stations at both ends):
//       Iclose = 2 f(g/2) + 2 f(g-g/2)  out and back; the even split is
//                                      optimal (verified vs a binary search
//                                      on 500200 cases, so this is O(1))
//       Ipass  = g + 2 ( f(a) + f(b) ),  a=m'/2, b=m'-a, m'=max(0,g-k)
//       Iopen  = f(g)                   finish inside
//
// ===========================================================================
// Day structure
// ===========================================================================
// Let S_0 < ... < S_{m-1} be the working stations.  The gaps are
//   G_0 = [0,S_0]  (road end),  G_j = [S_{j-1},S_j]  (interior, 1<=j<=m-1),
//   G_m = [S_{m-1},l]  (road end).
// An optimal day walks from p to some station S_s (cost |p-S_s|; charging at
// the stations passed en route is free, so "walk straight to S_s" is optimal),
// then services the gaps outward from that station, traversing gaps up to the
// final gap j, finishing inside gap j, with every other gap a closed excursion.
// (NOT a monotone left-to-right sweep: the sample route is 3->2->0->2->4->5->4,
// which goes left first.  What matters is that the gaps SPLIT at S_s, which is
// what makes the cost additive.)  So
//
//     answer(p) = min_s ( |p - S_s| + C_s ),      C_s = day cost from S_s
//
// which is a lower envelope of V-shapes.  For p in the interval [S_t,S_{t+1})
// the set of stations left of p is fixed, so |p-S_s| = p-S_s there, and
//
//     answer(p) = min( p + A_t ,  -p + B_t ),
//     A_t = min_{s<=t} (C_s - S_s),      B_t = min_{s>t} (C_s + S_s)
//
// A_t and B_t do NOT depend on p.  That is what makes the whole thing
// maintainable: one segment tree, O(log n) per day.
//
// Writing T_j = sum_{i<j}(ps_i-cl_i) and cltot = sum_i cl_i, the pair cost
// (start station s, final gap j) decomposes exactly (see monoid_proof.cpp):
//
//   +1 (s<j) : cltot + (|p-S_s| - T_{s+1}) + ( T_j - cl_j + OP_j )
//   -1 (j<=s): cltot + (|p-S_s| + T_{s+1}) + (-T_j - ps_j + OP_j)
//
// so, for a start at S_s with a full battery,
//     C_s = cltot + min( -T_{s+1} + min_{j>=s+1} beta_j ,
//                        +T_{s+1} + min_{j<=s}   betap_j )
// with beta_j = T_j - cl_j + OP_j and betap_j = -T_j - ps_j + OP_j.
// Splitting out the -S_s / +S_s of A_t and B_t gives four ordered min-plus
// products, all carried by the tree below.
//
// ===========================================================================
// Segment tree
// ===========================================================================
// Leaves 0..n over the FIXED station slots.  Leaf 0 is the leading road-end
// gap.  Leaf c (1<=c<=n) is station slot c-1: if that station works, the leaf
// holds the station together with the gap that follows it.  An inactive slot's
// leaf is empty and contributes nothing, so active stations form a subsequence
// of the leaves and the tree's prefix sums telescope to the right T values.
//
// Because charging is free at every working station, a repair/breakage only
// changes: the leaf of that slot, the leaf of the preceding active station
// (its following gap is now longer or shorter), and leaf 0 when the first
// working station changes.  That is <=3 point updates.
//
// Node state, all relative to tLo = T at the node's first gap (so the node
// knows nothing about the prefix before it).  Terms carrying -T and terms
// carrying +T are kept apart, and a pair always combines one of each, so pair
// values are frame-invariant and need no shift at merge time.
//
//   P       = sum (ps-cl) over the node's gaps
//   clSum   = sum cl over the node's gaps
//   aMinPos = min ( -T_cs + S )   over stations      carries -T
//   aMinNeg = min ( -T_cs - S )   over stations      carries -T
//   uMinPos = min ( +T_cs + S )   over stations      carries +T
//   uMinNeg = min ( +T_cs - S )   over stations      carries +T
//   cMinT   = min ( +T_j - cl_j + OP_j )             carries +T
//   dMinT   = min ( -T_j - ps_j + OP_j )             carries -T
//   abPos   = min_{cell(s)<=j} (aMinPos + cMinT)     (+1, station left of gap)
//   abNeg   = min_{cell(s)<=j} (aMinNeg + cMinT)
//   cdPos   = min_{j<cell(s)}  (dMinT + uMinPos)     (-1, gap left of station)
//   cdNeg   = min_{j<cell(s)}  (dMinT + uMinNeg)
//
// Merge L+R with U = L.P (moving R into the parent's frame):
//   -T-carrying values take R's minus U, +T-carrying take R's plus U, pair
//   values are unchanged, and the cross terms are L.<a> + U + R.<c>.
// Getting both shift signs the same way is the classic bug: it still passes
// about 40% of random cases before failing.
//
// Per day, with p0 = leaf of the last working station at or before p
// (0 if there is none), the two halves are Q0 = [0,p0] and Q1 = [p0+1,n]:
//   A_t = cltot + min( min(Q0.abNeg, Q0.aMinNeg + Q0.P + Q1.cMinT),
//                      Q0.cdNeg )
//   B_t = cltot + min( Q1.abPos,
//                      min(Q1.cdPos, Q0.dMinT + Q0.P + Q1.uMinPos) )
// (cdNeg needs no cross term: j <= s <= t forces j <= p0, so both terms lie
//  in Q0.  abPos needs none either: j >= cell(s) >= p0+1.)
// ===========================================================================
#include <bits/stdc++.h>
using namespace std;

using int64 = long long;
// Worst-case magnitudes: cltot <= l^2/4 ~ 2.5e17, |T| <= l^2/4, answers
// <~1.1e18 (verified in overflow_audit.cpp).  INF = 2e18 leaves room for the
// INF+-U arithmetic in the merge without colliding with a real value.
static const int64 INF = 2000000000000000000LL;
static const int64 EMPTY_GUARD = 1000000000000000000LL;

static int64 K;
static int64 lroad;

static inline int64 tri(int64 x) {
    if (x <= 0) return 0;
    // The statement guarantees k >= 1, so K is never 0 on valid input.  The
    // guard costs one predictable branch and turns an illegal input from a
    // SIGFPE (a core dump that looks like a solver bug) into a finite answer.
    if (K <= 0) return 0;
    int64 m = (x - 1) / K;
    return m * x - K * m * (m + 1) / 2;
}
static inline int64 f(int64 x) { return x + tri(x); }
static inline int64 Eclose(int64 g) { return 2 * f(g); }
static inline int64 Eopen(int64 g)  { return 2 * f(g) - g; }
static inline int64 Iopen(int64 g)  { return f(g); }
static inline int64 Iclose(int64 g) { return 2 * f(g / 2) + 2 * f(g - g / 2); }
static inline int64 Ipass(int64 g) {
    int64 m = (g > K ? g - K : 0);
    int64 a = m / 2, b = m - a;
    return g + 2 * (f(a) + f(b));
}

// NOTE on `inline`: most of these are decoration. At -O2 GCC inlines
// isEmpty/tri/f/Eclose/... into their callers whether or not they are marked,
// so deleting the keyword changes nothing for them.
//
// It is load-bearing for `mrg` and for the `refresh` lambda. Without it GCC
// decides mrg is big enough that inlining it into its ~9 call sites is not
// worth the code growth, and emits it out of line: 7 calls from main and 5
// from the refresh lambda, with the 104-byte Nd passed in memory instead of
// registers. Measured 1.27 s -> 1.43 s (13%) with the keywords stripped, all
// output byte-identical. See proofs/optimisation_notes.cpp.
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

struct Nd {
    bool empty = true;          // true iff the node holds no active leaf
    int64 P = 0, clSum = 0;
    int64 aMinPos = INF, aMinNeg = INF, uMinPos = INF, uMinNeg = INF;
    int64 cMinT = INF, dMinT = INF;
    int64 abPos = INF, abNeg = INF, cdPos = INF, cdNeg = INF;
};

// A node is empty iff it holds no active leaf.  This must be an explicit flag,
// not a magnitude test: the leading road-end gap carries no station, so its
// station fields are INF, yet the node is very much non-empty.
static inline bool isEmpty(const Nd &n) { return n.empty; }

static inline Nd mrg(const Nd &L, const Nd &R) {
    if (isEmpty(L)) return R;
    if (isEmpty(R)) return L;
    Nd r;
    r.empty = false;
    r.P     = L.P + R.P;
    r.clSum = L.clSum + R.clSum;
    // -T-carrying terms: the right child's frame is larger by U
    r.aMinPos = min(L.aMinPos, R.aMinPos - L.P);
    r.aMinNeg = min(L.aMinNeg, R.aMinNeg - L.P);
    r.dMinT  = min(L.dMinT,  R.dMinT  - L.P);
    // +T-carrying terms
    r.uMinPos = min(L.uMinPos, R.uMinPos + L.P);
    r.uMinNeg = min(L.uMinNeg, R.uMinNeg + L.P);
    r.cMinT   = min(L.cMinT,   R.cMinT   + L.P);
    // pair values are frame-invariant; cross terms need one U
    r.abPos = min(min(L.abPos, R.abPos), L.aMinPos + L.P + R.cMinT);
    r.abNeg = min(min(L.abNeg, R.abNeg), L.aMinNeg + L.P + R.cMinT);
    r.cdPos = min(min(L.cdPos, R.cdPos), L.dMinT  + L.P + R.uMinPos);
    r.cdNeg = min(min(L.cdNeg, R.cdNeg), L.dMinT  + L.P + R.uMinNeg);
    return r;
}

// leaf 0: the leading road-end gap [0, first working station]
static Nd makeLead(int64 g) {
    Nd n; n.empty = false;
    int64 cl = Eclose(g), op = Eopen(g);
    n.P = 0;                       // ps - cl = Eclose - Eclose
    n.clSum = cl;
    n.cMinT = -cl + op;
    n.dMinT = -cl + op;            // ps == cl
    return n;                      // no station in this leaf
}

// leaf c: working station slot c-1 at S, followed by a gap of length g
static Nd makeStn(int64 S, int64 g, bool interior) {
    Nd n; n.empty = false;
    int64 cl, ps, op;
    if (interior) { cl = Iclose(g); ps = Ipass(g); op = Iopen(g); }
    else          { cl = ps = Eclose(g); op = Eopen(g); }
    n.P     = ps - cl;
    n.clSum = cl;
    n.cMinT = -cl + op;
    n.dMinT = -ps + op;
    n.aMinPos =  S;  n.aMinNeg = -S;
    n.uMinPos =  S;  n.uMinNeg = -S;
    n.abPos =  S + n.cMinT;        // cell(s) == j, allowed for +1
    n.abNeg = -S + n.cMinT;
    // cdPos / cdNeg stay INF: a single leaf has no gap strictly before it
    return n;
}

int main() {
    FastScanner in;
    int n, d;
    if (!in.read(n)) return 0;
    in.read(lroad);
    in.read(K);
    in.read(d);

    vector<int64> x(n);
    for (int i = 0; i < n; i++) in.read(x[i]);

    set<int> act;                     // active station slots, ascending
    for (int i = 0; i < n; i++) act.insert(act.end(), i);

    int base = 1;
    while (base < n + 1) base <<= 1;
    vector<Nd> seg(2 * base);

    auto setLeaf = [&](int c, const Nd &v) {
        seg[base + c] = v;
        for (int i = (base + c) >> 1; i >= 1; i >>= 1)
            seg[i] = mrg(seg[2 * i], seg[2 * i + 1]);
    };

    // recompute leaf c from the current active set
    auto refresh = [&](int c) {
        // The statement guarantees at least one station always works, so `act`
        // is never empty.  Guard anyway: an empty set has no first station, and
        // dereferencing begin() would be undefined.
        if (c == 0) {
            if (act.empty()) { setLeaf(0, Nd{}); return; }
            setLeaf(0, makeLead(x[*act.begin()]));
            return;
        }
        int slot = c - 1;
        auto it = act.find(slot);
        if (it == act.end()) { setLeaf(c, Nd{}); return; }
        ++it;
        if (it == act.end()) setLeaf(c, makeStn(x[slot], lroad - x[slot], false));
        else                 setLeaf(c, makeStn(x[slot], x[*it] - x[slot], true));
    };

    // build
    refresh(0);
    for (int i = 0; i < n; i++) {
        bool interior = (i + 1 < n);
        int64 g = interior ? (x[i + 1] - x[i]) : (lroad - x[i]);
        seg[base + i + 1] = makeStn(x[i], g, interior);
    }
    for (int i = base - 1; i >= 1; i--) seg[i] = mrg(seg[2 * i], seg[2 * i + 1]);

    // range query over leaves [lo,hi], returned in the frame of leaf lo
    auto query = [&](int lo, int hi) -> Nd {
        if (lo > hi) return Nd{};
        Nd L, R;
        for (int a = lo + base, b = hi + 1 + base; a < b; a >>= 1, b >>= 1) {
            if (a & 1) L = mrg(L, seg[a++]);
            if (b & 1) R = mrg(seg[--b], R);
        }
        return mrg(L, R);
    };

    auto activate = [&](int slot) {
        int prevSlot = -1;
        auto it = act.lower_bound(slot);
        if (it != act.begin()) prevSlot = *prev(it);
        act.insert(slot);
        refresh(slot + 1);
        if (prevSlot >= 0) refresh(prevSlot + 1); else refresh(0);
    };
    auto deactivate = [&](int slot) {
        auto it = act.find(slot);
        if (it == act.end()) return;          // input guarantees this cannot
        int prevSlot = (it == act.begin()) ? -1 : *prev(it);
        act.erase(it);
        refresh(slot + 1);
        if (prevSlot >= 0) refresh(prevSlot + 1); else refresh(0);
    };

    string out;
    out.reserve(1 << 20);
    char line[32];                 // holds a long long plus sign and newline

    while (d--) {
        // Initialise, because read() returns false on a truncated input and
        // would otherwise leave these indeterminate.
        int z = 0, u = 0;
        int64 p = 0;
        if (!in.read(z) || !in.read(u) || !in.read(p)) break;
        for (int i = 0; i < z; i++) { int a = 0; in.read(a); activate(a - 1); }
        for (int i = 0; i < u; i++) { int b = 0; in.read(b); deactivate(b - 1); }

        // p0 = leaf of the last working station at or before p, else 0
        int r = (int)(upper_bound(x.begin(), x.end(), p) - x.begin()) - 1;
        int p0 = 0;
        if (r >= 0) {
            auto it = act.upper_bound(r);
            if (it != act.begin()) p0 = *prev(it) + 1;
        }

        if (act.empty()) { out += "0\n"; continue; }
        Nd Q0 = query(0, p0);
        Nd Q1 = query(p0 + 1, n);
        int64 cltot = seg[1].clSum;

        int64 aNeg = min(Q0.abNeg, Q0.aMinNeg + Q0.P + Q1.cMinT);
        int64 cdN  = Q0.cdNeg;
        int64 aPos = Q1.abPos;
        int64 cdP  = min(Q1.cdPos, Q0.dMinT + Q0.P + Q1.uMinPos);

        int64 A = cltot + min(aNeg, cdN);
        int64 B = cltot + min(aPos, cdP);
        int64 best = min(p + A, -p + B);

        int len = snprintf(line, sizeof line, "%lld\n", best);
        out.append(line, len);
        if (out.size() > (1u << 20)) { fwrite(out.data(), 1, out.size(), stdout); out.clear(); }
    }
    fwrite(out.data(), 1, out.size(), stdout);
    return 0;
}
