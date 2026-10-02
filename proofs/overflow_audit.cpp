// Overflow / magnitude audit: worst-case values the constraints allow.
#include <bits/stdc++.h>
using namespace std;
using ll = long long;
int main(){
  setvbuf(stdout,NULL,_IONBF,0);
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
  printf("single station, road-end gap g=1e9, k=1: Eclose=%lld (fits: %d)\n",
         worst, worst < LLONG_MAX);
  // Intermediate quantities in the sweep: precl up to cltot (~1e17+)
  printf("max plausible cltot with one 1e9 gap: %lld\n", worst);
  // Check the multiplications inside tri for overflow at the limit
  ll mm=(L-1)/1;                    // 999999999
  __int128 t = (__int128)mm*L - (__int128)1*mm*(mm+1)/2;
  printf("tri(1e9,k=1) as __int128 = %lld  fits in ll: %d\n", (ll)t, t < (__int128)LLONG_MAX);
  // f(g,k) for the largest gap: safe?
  __int128 ff = (__int128)L + t;
  printf("f(1e9,k=1) as __int128 = %lld  fits in ll: %d\n", (ll)ff, ff < (__int128)LLONG_MAX);
  __int128 dd = 2*ff;
  printf("2f = %lld  fits: %d\n", (ll)dd, dd < (__int128)LLONG_MAX);
  return 0;
}
