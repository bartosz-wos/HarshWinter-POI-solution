// Fast full-scale input generator (valid: legal repairs/breaks, >=1 working).
//
//     gen_random <n> <d> <L> <k> <seed>   > input.txt
#include <bits/stdc++.h>
using namespace std;
int main(int argc, char** argv){
    if (argc < 6) {
        fprintf(stderr,
                "usage: %s <n> <d> <L> <k> <seed>\n"
                "  n    stations      (1 <= n <= L+1)\n"
                "  d    days          (1 <= d)\n"
                "  L    road length   (1 <= L <= 1e9)\n"
                "  k    charge capacity (1 <= k <= L)\n"
                "  seed any integer\n", argv[0]);
        return 2;
    }
    long long n = atoll(argv[1]), d = atoll(argv[2]);
    long long L = atoll(argv[3]), k = atoll(argv[4]);
    unsigned long long seed = atoll(argv[5]);
    mt19937_64 rng(seed);
    vector<long long> xs(n);
    {   // sample distinct positions in [0,L]
        unordered_set<long long> seen;
        seen.reserve(n*2);
        for(long long i=0;i<n;i++){
            long long v;
            do { v = (long long)(rng() % (unsigned long long)(L+1)); }
            while(!seen.insert(v).second);
            xs[i]=v;
        }
    }
    sort(xs.begin(), xs.end());
    string out;
    out.reserve(1u<<26);
    char buf[64];
    auto put = [&](long long v, char sep){ int l=snprintf(buf,sizeof buf,"%lld%c",v,sep); out.append(buf,l); };
    put(n,' '); put(L,' '); put(k,' '); put(d,'\n');
    for(long long i=0;i<n;i++) put(xs[i], i+1==n?'\n':' ');
    vector<char> ok(n,1);
    vector<long long> z, u;
    // The statement caps sum over days of (z_i + u_i) at 500000.  This
    // generator previously allowed up to 6 changes a day with no global cap,
    // so at d = 250000 it emitted 1 499 965 -- an input the grader would never
    // produce and one that measures the wrong thing.  The budget is shared
    // across the whole run, so a long run simply spreads the same allowance
    // over more days instead of accumulating.
    const long long ZUMAX = 500000;
    long long budget = ZUMAX;
    for(long long day=0; day<d; ++day){
        long long p = (long long)(rng() % (unsigned long long)(L+1));
        z.clear(); u.clear();
        long long room = max(0LL, min(6LL, budget));
        for(long long i=0;i<n && (long long)z.size()<room && (long long)(z.size()+u.size())<room;i++)
            if(!ok[i] && (rng()%100)<40) z.push_back(i);
        for(long long i=0;i<n && (long long)u.size()<room && (long long)(z.size()+u.size())<room;i++)
            if(ok[i] && (rng()%100)<40) u.push_back(i);
        // never break the last working station
        {   long long working=0; for(long long i=0;i<n;i++) if(ok[i]) working++;
            for(long long i=0;i<n;i++) if(ok[i]) working--;
            long long nb = (long long)u.size();
            if(nb >= working && working>0) { /* trim to leave one */ u.resize(max(0LL, working-1)); }
        }
        put((long long)z.size(),' '); put((long long)u.size(),' '); put(p,'\n');
        for(size_t i=0;i<z.size();i++) put(z[i]+1, i+1==z.size()?'\n':' ');
        if(z.empty()) out += '\n';
        for(size_t i=0;i<u.size();i++) put(u[i]+1, i+1==u.size()?'\n':' ');
        if(u.empty()) out += '\n';
        for(long long i: z) ok[i]=1;
        for(long long i: u) ok[i]=0;
        budget -= (long long)(z.size() + u.size());
    }
    fwrite(out.data(),1,out.size(),stdout);
    return 0;
}
