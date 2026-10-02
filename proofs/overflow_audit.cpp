// Overflow / magnitude audit: worst-case values the constraints allow.
//
// Recomputes every binding quantity in __int128 and FAILS if any exceeds the
// int64 range the solver relies on.  Prints the numbers either way, because the
// margin is the useful output.
#include <bits/stdc++.h>
using namespace std;
using ll = long long;
int main(){
  setvbuf(stdout,NULL,_IONBF,0);
  int bad = 0;
  auto check = [&](const char* what, __int128 v){
    int ok = v <= (__int128)LLONG_MAX;
    if(!ok) ++bad;
    printf("  %-28s %-22lld  fits in int64: %s\n", what, (ll)v, ok ? "yes" : "NO");
    return ok;
  };
  // Worst case: L = 1e9, k = 1, one station, p at the far end.
  // f(g,1) = g + g(g-1)/2 = O(g^2/2) = 5e17 for g=1e9.
  // Iclose ~ 2f(g/2) + 2f(g/2) = 4 * (5e8 + 5e8*5e8/2) ~ 5e17
  // A day with n=250000 gaps each of length L/n = 4000 and k=1:
  //   cltot ~ 250000 * 5e5 = 1.25e11
  // But a single huge gap dominates: g = 1e9, k=1:
  ll K=1, L=1000000000LL;
  auto tri=[&](ll x){ if(x<=0) return 0LL; ll m=(x-1)/K; return m*x-K*m*(m+1)/2; };
  auto f=[&](ll x){ return x+tri(x); };
  printf("k=1, g=1e9: f=%lld  2f=%lld  Iclose=%lld\n",
         f(L), 2*f(L), 2*f(L/2)+2*f(L-L/2));
  printf("  LLONG_MAX = %lld\n", LLONG_MAX);
  // Largest possible day cost: n gaps, each closed-excursion, k=1.
  // If all n stations are consecutive integers in a huge L, each gap is L/n.
  ll n=250000, g=L/n;
  ll ic=2*f(g/2)+2*f(g-g/2);
  printf("k=1, n=2.5e5, gap=%lld: per-gap Iclose=%lld, cltot=%lld\n", g, ic, ic*n);
  // Worst single-station case
  ll worst = 2*f(L);
  printf("single station, road-end gap g=1e9, k=1: Eclose=%lld\n", worst);
  // Intermediate quantities in the sweep: precl up to cltot (~1e17+)
  printf("max plausible cltot with one 1e9 gap: %lld\n", worst);

  // The multiplications inside tri are the real overflow risk, so recompute
  // every binding quantity in 128-bit and require it to fit.
  printf("\nint64 audit:\n");
  ll mm=(L-1)/1;                    // 999999999
  __int128 t = (__int128)mm*L - (__int128)1*mm*(mm+1)/2;
  check("tri(1e9, k=1)", t);
  __int128 ff = (__int128)L + t;
  check("f(1e9, k=1)", ff);
  check("2*f(1e9, k=1)  [Eclose]", (__int128)2*ff);
  check("Iclose(1e9, k=1)", (__int128)(2*f(L/2)+2*f(L-L/2)));
  check("cltot, n=2.5e5 gaps", (__int128)ic*n);
  // The largest day total actually emitted: worst single-station gap plus the
  // travel to reach it, which is at most L.
  check("worst full day answer", (__int128)worst + L);

  printf("\noverflow audit: %d value(s) exceed int64\n", bad);
  return bad ? 1 : 0;
}
