// Stress edge cases: heavy breakage (few active stations), zero gaps,
// duplicate-ish spacing, k=1, k=L, n=1, and a single active station.
#include <bits/stdc++.h>
using namespace std;
using ll = long long;
static const ll INF = 4000000000000000000LL;
static ll K;
static inline ll tri(ll x){ if(x<=0) return 0; ll m=(x-1)/K; return m*x-K*m*(m+1)/2; }
static inline ll f(ll x){ return x+tri(x); }
static inline ll ecl(ll g){ return 2*f(g); }
static inline ll eop(ll g){ return 2*f(g)-g; }
static inline ll iop(ll g){ return f(g); }
static ll ic(ll g){ return 2*f(g/2)+2*f(g-g/2); }
static inline ll ipass(ll g){ ll m=(g>K?g-K:0); ll a=m/2,b=m-a; return g+2*(f(a)+f(b)); }
struct GC{ ll cl, ps, op; };
static GC gapc(ll g, bool one){ if(g<=0) return {0,0,0};
  if(one){ ll c=ecl(g); return {c,c,eop(g)}; } return {ic(g), ipass(g), iop(g)}; }
static ll sweep(const vector<ll>& S, const vector<GC>& G, ll p){
  int m=(int)S.size(), nG=m+1;
  vector<ll> precl(nG+1,0),preps(nG+1,0),sufcl(nG+1,0);
  for(int i=0;i<nG;i++){ precl[i+1]=precl[i]+G[i].cl; preps[i+1]=preps[i]+G[i].ps; }
  for(int i=nG-1;i>=0;i--) sufcl[i]=sufcl[i+1]+G[i].cl;
  ll best=INF,rm=INF;
  for(int j=1;j<nG;j++){ int s=j-1;
    ll a=llabs(p-S[s])+(precl[s+1]-preps[s+1]); rm=min(rm,a);
    best=min(best,rm+preps[j]+sufcl[j+1]+G[j].op); }
  rm=INF;
  for(int j=m-1;j>=0;j--){ ll a=llabs(p-S[j])+(preps[j+1]+sufcl[j+1]); rm=min(rm,a);
    best=min(best,rm+precl[j]-preps[j+1]+G[j].op); }
  return best;
}
struct Nd{ ll P=0,aMin=INF,cMin=INF,ab=INF,qMin=INF,dMin=INF,cd=INF; };
static inline Nd mrg(const Nd& L, const Nd& R){
  Nd r; ll U=L.P;
  r.P=L.P+R.P; r.aMin=min(L.aMin,R.aMin-U); r.cMin=min(L.cMin,R.cMin+U);
  r.qMin=min(L.qMin,R.qMin+U); r.dMin=min(L.dMin,R.dMin-U);
  r.ab=min(min(L.ab,R.ab),L.aMin+R.cMin+U);
  r.cd=min(min(L.cd,R.cd),L.dMin+R.qMin+U);
  return r;
}
int main(){
  mt19937_64 rng(2718);
  ll bad=0, N=0, zeros=0;
  for(ll it=0; it<400000; ++it){
    int L=1+(int)(rng()%120);
    int kk = (int)(rng()%4);
    K = (kk==0)?1 : (kk==1)?L : 1+(int)(rng()%L);
    int m=1+(int)(rng()%12); if(m>L+1) m=L+1;
    // sometimes force tight clustering (zero and unit gaps)
    int mode=(int)(rng()%3);
    vector<ll> S;
    if(mode==0){ for(int i=0;i<m;i++) S.push_back(rng()%(L+1)); }
    else if(mode==1){ for(int i=0;i<m;i++) S.push_back((ll)(i%(L+1))); }
    else { for(int i=0;i<m;i++) S.push_back(rng()%min(3LL,(ll)L+1)); }
    sort(S.begin(),S.end()); S.erase(unique(S.begin(),S.end()),S.end());
    m=S.size(); if(m==0) continue;
    int nG=m+1; vector<GC> G(nG);
    for(int i=0;i<nG;i++){ ll g=(i==0)?S[0]:(i==m?(ll)L-S[m-1]:S[i]-S[i-1]);
      G[i]=gapc(g,i==0||i==m); }
    ll p=(ll)(rng()%(L+1));
    ll want=sweep(S,G,p);
    int N0=1; while(N0<nG) N0<<=1;
    vector<Nd> seg(2*N0);
    for(int i=0;i<N0;i++){
      if(i<nG){ Nd n; n.P=G[i].ps-G[i].cl; bool hs=(i>=1);
        ll d=hs?llabs(p-S[i-1]):0;
        n.aMin=hs?d:INF; n.cMin=-G[i].cl+G[i].op; n.qMin=hs?d:INF;
        n.dMin=-G[i].ps+G[i].op; n.ab=hs?(n.aMin+n.cMin):INF; n.cd=INF;
        seg[N0+i]=n; }
      else seg[N0+i]=Nd{};
    }
    for(int i=N0-1;i>=1;i--) seg[i]=mrg(seg[2*i],seg[2*i+1]);
    ll cltot=0; for(int i=0;i<nG;i++) cltot+=G[i].cl;
    ll got=cltot+min(seg[1].ab,seg[1].cd);
    N++;
    if(want==0) zeros++;
    if(got!=want){ if(bad<5) printf("L=%d K=%lld m=%d p=%lld mode=%d sweep=%lld seg=%lld\n",
        L,K,m,p,mode,want,got); ++bad; }
  }
  printf("edge-case mismatches: %lld / %lld   (zero-answer cases: %lld)\n", bad, N, zeros);
  return 0;
}
