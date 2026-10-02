// Fast legality checker for a generated input.  Same rules as
// generators/validate_input.py, but linear in the input size so it can sit in
// the routine test suite: the Python one is O(n*d) and takes minutes on a
// 250 000 x 250 000 input, which is exactly the case worth checking.
//
//     validate_input <input.txt> [...]   ->  exit 0 if legal, 1 otherwise
//
// Why this exists: gen_random once emitted 1 499 965 updates against the
// statement's 500 000 cap, and the full-limit timing test happily measured that
// illegal input and called it "at the statement's limits".
#include <bits/stdc++.h>
using namespace std;

static const long long NMAX = 250000, LMAX = 1000000000LL, DMAX = 250000, ZUMAX = 500000;
static const size_t MAXERR = 12;   // a badly broken file would otherwise print
                                   // one line per day; report and stop

static char buf[1 << 22];

static bool readline(FILE* f) {
    if (!fgets(buf, sizeof buf, f)) return false;
    size_t n = strlen(buf);
    while (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = 0;
    return true;
}

static void split(vector<long long>& v) {
    v.clear();
    long long cur = 0; bool any = false, neg = false;
    for (char c : string(buf)) {
        if (c == '-') { neg = true; continue; }
        if (c >= '0' && c <= '9') { cur = cur * 10 + (c - '0'); any = true; }
        else if (any) { v.push_back(neg ? -cur : cur); cur = 0; any = false; neg = false; }
    }
    if (any) v.push_back(neg ? -cur : cur);
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <input.txt> [...]\n", argv[0]); return 2; }
    int rc = 0;
    for (int a = 1; a < argc; ++a) {
        FILE* f = fopen(argv[a], "r");
        if (!f) { printf("%s: cannot open\n", argv[a]); rc = 1; continue; }

        vector<string> errs;
        auto bad = [&](const string& e) { if (errs.size() < MAXERR) errs.push_back(e); };

        vector<long long> hdr, xs, Z, U, zu;
        if (!readline(f)) { printf("%s: empty\n", argv[a]); fclose(f); rc = 1; continue; }
        split(hdr);
        if (hdr.size() < 4) { printf("%s: bad header\n", argv[a]); fclose(f); rc = 1; continue; }
        long long n = hdr[0], l = hdr[1], k = hdr[2], d = hdr[3];
        if (!(1 <= n && n <= NMAX)) bad("n out of range");
        if (!(1 <= l && l <= LMAX)) bad("l out of range");
        if (!(1 <= k && k <= l))    bad("k out of range");
        if (!(1 <= d && d <= DMAX)) bad("d out of range");

        if (!readline(f)) { printf("%s: no coordinate line\n", argv[a]); fclose(f); rc = 1; continue; }
        split(xs);
        if ((long long)xs.size() != n)
            bad("x has " + to_string(xs.size()) + " values, expected " + to_string(n));
        for (size_t i = 1; i < xs.size(); ++i)
            if (xs[i] <= xs[i - 1]) { bad("x not strictly increasing"); break; }
        if (!xs.empty() && (xs.front() < 0 || xs.back() > l)) bad("x outside [0,l]");

        vector<char> work(n + 2, 1);
        long long working = n, total = 0, day;
        for (day = 0; day < d; ++day) {
            if (!readline(f)) { bad("day " + to_string(day + 1) + ": truncated"); break; }
            split(zu);
            if (zu.size() < 3) { bad("day " + to_string(day + 1) + ": truncated"); break; }
            long long z = zu[0], u = zu[1], p = zu[2];
            if (!readline(f)) { bad("day " + to_string(day + 1) + ": truncated"); break; } split(Z);
            if (!readline(f)) { bad("day " + to_string(day + 1) + ": truncated"); break; } split(U);
            if ((long long)Z.size() != z) bad("day " + to_string(day + 1) + ": z count mismatch");
            if ((long long)U.size() != u) bad("day " + to_string(day + 1) + ": u count mismatch");
            for (int side = 0; side < 2; ++side) {
                const vector<long long>& v = side ? U : Z;
                for (size_t i = 0; i < v.size(); ++i) {
                    if (v[i] < 1 || v[i] > n) { bad("day " + to_string(day + 1) + ": id out of 1..n"); break; }
                    if (i && v[i] <= v[i - 1]) { bad("day " + to_string(day + 1) + ": list not increasing"); break; }
                }
            }
            for (long long s : Z) {
                if (s < 1 || s > n) continue;
                if (work[s]) { bad("day " + to_string(day + 1) + ": z lists a WORKING station"); break; }
                work[s] = 1; working++;
            }
            for (long long s : U) {
                if (s < 1 || s > n) continue;
                if (!work[s]) { bad("day " + to_string(day + 1) + ": u lists a BROKEN station"); break; }
                work[s] = 0; working--;
            }
            if (working <= 0) { bad("day " + to_string(day + 1) + ": all stations down"); break; }
            if (p < 0 || p > l) bad("day " + to_string(day + 1) + ": p out of [0,l]");
            total += z + u;
        }
        if (total > ZUMAX) bad("sum(z+u) = " + to_string(total) + " > 500000");

        if (errs.empty())
            printf("%s: OK  (n=%lld d=%lld, sum(z+u)=%lld)\n", argv[a], n, d, total);
        else {
            printf("%s: PROBLEMS\n", argv[a]);
            for (auto& e : errs) printf("    %s\n", e.c_str());
            if (errs.size() >= MAXERR) printf("    ...more suppressed\n");
            rc = 1;
        }
        fclose(f);
    }
    return rc;
}
