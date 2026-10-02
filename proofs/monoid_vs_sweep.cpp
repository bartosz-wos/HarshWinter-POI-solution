// Monoid for the sur day model -- corrected frame signs.
//
// Pair values (all frame-free, tLo cancels):
//   +1:  aTerm_i + cTerm_j  where i = s+1, i <= j
//         = |p-W| - (T_i - tLo) + (T_j - tLo) - cl_j + OP_j
//         = |p-W| - T_i + T_j - cl_j + OP_j          ( = |p-W| - T_{s+1} + ... )
//   -1:  dTerm_j + qTerm_i  where j < i
//         = -(T_j - tLo) - ps_j + OP_j + |p-W| + (T_i - tLo)
//         = |p-W| + T_i - T_j - ps_j + OP_j
//
// Node over [lo..hi], base tLo = T_lo, U = sum of (ps-cl) over the node.
// Converting a value from R's base (T_lo + U) to L's base (T_lo):
//   aTerm (uses -T_i) : -U      cTerm (uses +T_j) : +U
//   qTerm (uses +T_i) : +U      dTerm (uses -T_j) : -U
// A PAIR is invariant (one +T and one -T), so ab and cd need no shift.
#include <bits/stdc++.h>
using namespace std;
using ll = long long;
static const ll INF = 4000000000000000000LL;
static ll K;
static inline ll tri(ll x){ if(x<=0) return 0; ll m=(x-1)/K; return m*x-K*m*(m+1)/2; }
static inline ll f(ll x){ return x+tri(x); }
static ll ic(ll g){ return 2*f(g/2)+2*f(g-g/2); }
static inline ll ipass(ll g){ ll m=(g>K?g-K:0); ll a=m/2,b=m-a; return g+2*(f(a)+f(b)); }
static inline ll ecl(ll g){ return 2*f(g); }
static inline ll eop(ll g){ return 2*f(g)-g; }
struct GC{ ll cl, ps, op; };
static GC gapc(ll g, bool one){ if(g<=0) return {0,0,0};
  if(one){ ll c=ecl(g); return {c,c,eop(g)}; } return {ic(g), ipass(g), f(g)}; }

static ll sweep(const vector<ll>& S, const vector<GC>& G, ll p){
  int m=(int)S.size(), nG=m+1;
  vector<ll> precl(nG+1,0), preps(nG+1,0), sufcl(nG+1,0);
  for(int i=0;i<nG;i++){ precl[i+1]=precl[i]+G[i].cl; preps[i+1]=preps[i]+G[i].ps; }
  for(int i=nG-1;i>=0;i--) sufcl[i]=sufcl[i+1]+G[i].cl;
  ll best=INF, runmin=INF;
  for(int j=1;j<nG;j++){ int s=j-1;
    ll a = llabs(p-S[s])+(precl[s+1]-preps[s+1]);
    runmin=min(runmin,a);
    best=min(best, runmin+preps[j]+sufcl[j+1]+G[j].op); }
  runmin=INF;
  for(int j=m-1;j>=0;j--){
    ll a = llabs(p-S[j])+(preps[j+1]+sufcl[j+1]);
    runmin=min(runmin,a);
    best=min(best, runmin+precl[j]-preps[j+1]+G[j].op); }
  return best;
}

struct Nd{
  ll P=0;
  ll aMin=INF, cMin=INF, ab=INF;
  ll qMin=INF, dMin=INF, cd=INF;
  bool empty=true;
};

static Nd leaf(const GC& g, ll dPrev, bool hasStn){
  Nd n; n.empty=false;
  n.P    = g.ps - g.cl;
  n.aMin = hasStn ? dPrev      : INF;      // |p-W| - 0
  n.cMin = -g.cl + g.op;                   //  0 - cl + OP
  n.qMin = hasStn ? dPrev      : INF;      // |p-W| + 0
  n.dMin = -g.ps + g.op;                   // -0 - ps + OP
  n.ab   = hasStn ? (n.aMin + n.cMin) : INF;   // i == j allowed
  n.cd   = INF;                               // -1 needs j < i
  return n;
}

static Nd mrg(const Nd& L, const Nd& R){
  if(L.empty) return R;
  if(R.empty) return L;
  Nd r; r.empty=false;
  ll U = L.P;
  r.P    = L.P + R.P;
  r.aMin = min(L.aMin, R.aMin - U);     // carries -T
  r.cMin = min(L.cMin, R.cMin + U);     // carries +T
  r.qMin = min(L.qMin, R.qMin + U);     // carries +T
  r.dMin = min(L.dMin, R.dMin - U);     // carries -T
  r.ab   = min(min(L.ab, R.ab), L.aMin + R.cMin + U);
  r.cd   = min(min(L.cd, R.cd), L.dMin + R.qMin + U);
  return r;
}

static ll monotree(const vector<ll>& S, const vector<GC>& G, ll p){
  int m=(int)S.size(), nG=m+1;
  ll cltot=0; for(int i=0;i<nG;i++) cltot += G[i].cl;
  Nd root;
  for(int i=0;i<=m;i++)
    root = mrg(root, leaf(G[i], i>=1 ? llabs(p-S[i-1]) : 0, i>=1));
  return cltot + min(root.ab, root.cd);
}

int main(){
  mt19937_64 rng(31337);
  ll bad=0, N=500000;
  for(ll it=0; it<N; ++it){
    int L=1+(int)(rng()%60); K=1+(int)(rng()%L);
    int m=1+(int)(rng()%9); if(m>L+1) m=L+1;
    vector<ll> S; set<ll> seen;
    while((int)seen.size()<m){ ll v=(ll)(rng()%(L+1)); if(seen.insert(v).second) S.push_back(v); }
    sort(S.begin(),S.end());
    int nG=m+1; vector<GC> G(nG);
    for(int i=0;i<nG;i++){ ll g=(i==0)?S[0]:(i==m?(ll)L-S[m-1]:S[i]-S[i-1]);
      G[i]=gapc(g, i==0||i==m); }
    ll p=(ll)(rng()%(L+1));
    ll a=sweep(S,G,p), b=monotree(S,G,p);
    if(a!=b){ if(bad<4) printf("L=%d K=%lld m=%d p=%lld sweep=%lld mono=%lld\n",L,K,m,p,a,b); ++bad; }
  }
  printf("monoid mismatches: %lld / %lld\n", bad, N);
  return bad ? 1 : 0;
}
