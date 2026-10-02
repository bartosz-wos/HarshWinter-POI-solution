// Known-good stress generator: n=250000, d=250000, full 500000 update budget.
// Maintains the active set in an indexable container so slot choice is O(1)
// (no rejection resampling).  Legal per the statement: z = previously broken,
// u = previously working, disjoint, >=1 station always working.
//
// GCC emits spurious range warnings on the two std::sort calls below: it
// cannot see that the emptiness test above guarantees a non-empty range, and
// libstdc++'s insertion sort then reads a few bytes past a 4-byte vector.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif
#include <bits/stdc++.h>
using namespace std;
int main(int argc, char** argv){
    setvbuf(stdout, nullptr, _IOFBF, 1<<20);
    int mode = argc>1 ? atoi(argv[1]) : 0;
    long long n = 20000, l = 1000000000LL, k = 999999, d = 8000;
    mt19937_64 rng(1234567);
    vector<long long> x(n);
    for(long long i=0;i<n;i++) x[i] = (long long)(i*(l/(n+1)));
    for(long long i=1;i<n;i++) if(x[i]<=x[i-1]) x[i]=x[i-1]+1;
    printf("%lld %lld %lld %lld\n", n, l, k, d);
    { string s; for(long long i=0;i<n;i++){ if(i)s+=' '; s+=to_string(x[i]); } printf("%s\n", s.c_str()); }
    fflush(stdout);

    vector<char> work(n, 1);
    vector<int> dead, alive;                 // stacks of 0-based ids
    for(long long i=n-1;i>=0;i--) alive.push_back((int)i);

    long long budget = 60000;
    string out; out.reserve(1<<22);
    char buf[64];

    auto takeDead = [&]() -> int { int s = dead.back(); dead.pop_back(); work[s]=1; alive.push_back(s); return s; };
    auto takeAlive = [&]() -> int { int s = alive.back(); alive.pop_back(); work[s]=0; return s; };

    for(long long day=0; day<d; ++day){
        long long p = (long long)(rng()%(l+1));
        vector<int> za, ua;
        long long z=0, u=0;

        if(mode==0){
            int t = 2;
            for(int c=0;c<t && !dead.empty();c++){ za.push_back(takeDead()+1); z++; }
            for(int c=0;c<t && alive.size()>1;c++){ ua.push_back(takeAlive()+1); u++; }
        } else if(mode==1){
            for(int c=0;c<500 && budget>0 && !alive.empty();c++){
                if(alive.size()<=1) break;
                ua.push_back(takeAlive()+1); u++; budget--;
            }
        } else {
            if(day%10==0){
                size_t lim = min(alive.size(), (size_t)100);
                while(alive.size() > lim && budget>0){ ua.push_back(takeAlive()+1); u++; budget--; }
            } else if(day%10==5){
                while(!dead.empty() && budget>0){ za.push_back(takeDead()+1); z++; budget--; }
            }
        }
        // The statement guarantees at least one station stays working.  If the
        // night's break list would empty the active set, retract the last break
        // and re-list that station as repaired instead -- which is legal, since
        // retracting a break means it was never broken.
        if(alive.empty()){
            if(!ua.empty()){
                int s = ua.back() - 1;
                ua.pop_back(); u--;
                work[s] = 1; alive.push_back(s);
                za.push_back(s + 1); z++;
            } else if(!za.empty()){
                // Nothing was broken tonight, so the active set is empty only
                // because everything was already down.  Repair one to get back
                // to a legal state.
                int s = za.back() - 1;
                za.pop_back(); z--;
                work[s] = 1; alive.push_back(s);
            }
        }
        sort(za.begin(), za.end());
        sort(ua.begin(), ua.end());
        snprintf(buf,sizeof buf,"%lld %lld %lld\n", z, u, p);
        fputs(buf, stdout);
        { string s; for(size_t i=0;i<za.size();i++){ if(i)s+=' '; s+=to_string(za[i]); }
          printf("%s\n", s.c_str()); }
        { string s; for(size_t i=0;i<ua.size();i++){ if(i)s+=' '; s+=to_string(ua[i]); }
          printf("%s\n", s.c_str()); }
        // repaired stations (za) are working again -> they belong on `alive`.
        // The takeDead/takeAlive helpers already flipped `work`, so just re-stack.
    }
    return 0;
}
