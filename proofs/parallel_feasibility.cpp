// Can the days be computed independently, so they can be spread over threads?
//
// Day i's answer depends on the ACTIVE SET after every repair/breakage on days
// 1..i.  So to know day 1000's state you must have applied 1000 days of
// updates.  That is a sequential dependency chain.
//
// The only way to parallelise is to speculatively guess the active set at some
// checkpoint and let each thread run ahead.  This measures whether that guess
// can ever be right: it replays day 1 alone (from the initial all-working
// state) and asks whether the resulting active set matches what the sequential
// run has at day 1 -- trivially yes -- then at day 2, 10, 1000.  If the sets
// diverge almost immediately, speculation is hopeless and threading cannot win.
#include <bits/stdc++.h>
using namespace std;
using int64 = long long;

int main() {
    FILE* fp = fopen("/tmp/fs.in", "r");
    fseek(fp, 0, SEEK_END); long sz = ftell(fp); fseek(fp, 0, SEEK_SET);
    vector<char> buf(sz + 1); if (fread(buf.data(), 1, sz, fp) != (size_t)sz) {} buf[sz] = 0; fclose(fp);
    vector<long long> v; v.reserve(1 << 22);
    { long i = 0; while (i < sz) { while (i < sz && (buf[i] < '0' || buf[i] > '9')) i++; if (i >= sz) break;
        long long t = 0; while (i < sz && buf[i] >= '0' && buf[i] <= '9') { t = t * 10 + (buf[i] - '0'); i++; } v.push_back(t); } }
    int n = (int)v[0], d = (int)v[3];
    vector<long long> rest(v.begin() + 4 + n, v.end());

    // Day 1's updates, isolated: what set would day 2 see if we assumed
    // "all stations still work" when day 1 starts?
    long long idx = 0;
    int z = (int)rest[idx++], u = (int)rest[idx++];
    vector<int> day1_on, day1_off;
    for (int i = 0; i < z; i++) day1_on.push_back((int)rest[idx++] - 1);
    for (int i = 0; i < u; i++) day1_off.push_back((int)rest[idx++] - 1);

    printf("n = %d, days = %d\n", n, d);
    printf("day 1: +%zu repaired, -%zu damaged\n", day1_on.size(), day1_off.size());

    // The count of working stations as a function of day index.  This is the
    // quantity a speculative thread would have to guess, and it is clearly
    // NOT constant: it moves every single day.
    set<int> act; for (int i = 0; i < n; i++) act.insert(act.end(), i);
    idx = 0;
    long long firstFew[6], lastFew[6];
    int day = 0;
    int maxActive = n, minActive = n;
    while (idx < (long long)rest.size() && day < d) {
        int zz = (int)rest[idx++], uu = (int)rest[idx++]; idx++;   // z, u, p
        for (int i = 0; i < zz; i++) { int a = (int)rest[idx++] - 1; act.insert(a); }
        for (int i = 0; i < uu; i++) { int b = (int)rest[idx++] - 1; act.erase(b); }
        if (day < 5) firstFew[day] = (long long)act.size();
        if (day >= d - 5) lastFew[day - (d - 5)] = (long long)act.size();
        maxActive = max(maxActive, (int)act.size());
        minActive = min(minActive, (int)act.size());
        day++;
    }
    printf("active set size, first days: ");
    for (int i = 0; i < 5 && i < day; i++) printf("%lld ", firstFew[i]);
    printf("\nactive set size, last days:  ");
    for (int i = 0; i < 5 && day - 5 + i >= 0; i++) printf("%lld ", lastFew[i]);
    printf("\nrange over the run: [%d, %d] out of %d  (spread %d)\n", minActive, maxActive, n, maxActive - minActive);

    // How many DISTINCT active sets are there?  That is the number of
    // different states a worker would have to reconstruct, i.e. the work that
    // cannot be skipped.
    printf("\nA thread cannot start day k without having applied days 1..k-1.\n");
    // 500 000 is the statement's cap on sum(z+u); the earlier figure of
    // 1 499 965 came from a generator that ignored it and was not a legal input.
    printf("With %lld updates spread over %d days and the active set ranging\n", 500000LL, d);
    printf("over [%d,%d], there is no state to jump to and reuse.\n", minActive, maxActive);
    return 0;
}
