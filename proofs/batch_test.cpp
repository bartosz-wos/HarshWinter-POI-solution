// Does batching the 3 leaf rebuilds per update into one pass help?
//
// Each activate()/deactivate() calls refresh() up to 3 times, and each refresh
// walks all ~19 levels from its leaf to the root.  Those walks share most of
// their path, so we recompute the same ancestors repeatedly.  Instead: set every
// dirty leaf, then walk bottom-up recomputing each ancestor exactly once.
#include <bits/stdc++.h>
using namespace std;
using int64 = long long;
static const int64 INF = 2000000000000000000LL;
static int64 K, lroad;

static inline int64 tri(int64 x){ if(x<=0) return 0; if(K<=0) return 0; int64 m=(x-1)/K; return m*x-K*m*(m+1)/2; }
static inline int64 f(int64 x){ return x+tri(x); }
static inline int64 Eclose(int64 g){ return 2*f(g); }
static inline int64 Eopen(int64 g){ return 2*f(g)-g; }
static inline int64 Iopen(int64 g){ return f(g); }
static inline int64 Iclose(int64 g){ return 2*f(g/2)+2*f(g-g/2); }
static inline int64 Ipass(int64 g){ int64 m=(g>K?g-K:0); int64 a=m/2,b=m-a; return g+2*(f(a)+f(b)); }

struct Nd { bool empty=true; int64 P=0,clSum=0,aMinPos=INF,aMinNeg=INF,uMinPos=INF,uMinNeg=INF,cMinT=INF,dMinT=INF,abPos=INF,abNeg=INF,cdPos=INF,cdNeg=INF; };
static inline bool isEmpty(const Nd&n){return n.empty;}
static inline Nd mrg(const Nd&L,const Nd&R){
  if(isEmpty(L))return R; if(isEmpty(R))return L;
  Nd r; r.empty=false; r.P=L.P+R.P; r.clSum=L.clSum+R.clSum;
  r.aMinPos=min(L.aMinPos,R.aMinPos-L.P); r.aMinNeg=min(L.aMinNeg,R.aMinNeg-L.P); r.dMinT=min(L.dMinT,R.dMinT-L.P);
  r.uMinPos=min(L.uMinPos,R.uMinPos+L.P); r.uMinNeg=min(L.uMinNeg,R.uMinNeg+L.P); r.cMinT=min(L.cMinT,R.cMinT+L.P);
  r.abPos=min(min(L.abPos,R.abPos),L.aMinPos+L.P+R.cMinT); r.abNeg=min(min(L.abNeg,R.abNeg),L.aMinNeg+L.P+R.cMinT);
  r.cdPos=min(min(L.cdPos,R.cdPos),L.dMinT+L.P+R.uMinPos); r.cdNeg=min(min(L.cdNeg,R.cdNeg),L.dMinT+L.P+R.uMinNeg);
  return r;
}
static Nd makeLead(int64 g){ Nd n; n.empty=false; int64 cl=Eclose(g),op=Eopen(g); n.P=0;n.clSum=cl;n.cMinT=-cl+op;n.dMinT=-cl+op; return n; }
static Nd makeStn(int64 S,int64 g,bool in){ Nd n; n.empty=false; int64 cl,ps,op;
  if(in){cl=Iclose(g);ps=Ipass(g);op=Iopen(g);} else {cl=ps=Eclose(g);op=Eopen(g);}
  n.P=ps-cl;n.clSum=cl;n.cMinT=-cl+op;n.dMinT=-ps+op; n.aMinPos=S;n.aMinNeg=-S;n.uMinPos=S;n.uMinNeg=-S;
  n.abPos=S+n.cMinT; n.abNeg=-S+n.cMinT; return n; }

static const int MAXN = 300005;
static int64 x[MAXN];
static int n, d, base;
static set<int> act;
static vector<Nd> seg;
static vector<int> dirty;              // leaf indices touched this day

// Compute the leaf value for leaf c from the current active set.
static Nd leafVal(int c){
  if(c==0) return act.empty()?Nd{}:makeLead(x[*act.begin()]);
  int slot=c-1; auto it=act.find(slot);
  if(it==act.end()) return Nd{};
  ++it;
  if(it==act.end()) return makeStn(x[slot],lroad-x[slot],false);
  return makeStn(x[slot],x[*it]-x[slot],true);
}

int main(int argc,char**argv){
  bool batched = argc>1 && atoi(argv[1]);
  FILE*fp=fopen("/tmp/fs.in","r");
  fseek(fp,0,SEEK_END); long sz=ftell(fp); fseek(fp,0,SEEK_SET);
  vector<char> buf(sz+1); if(fread(buf.data(),1,sz,fp)!=(size_t)sz){} buf[sz]=0; fclose(fp);
  vector<long long> v; v.reserve(1<<22);
  { long i=0; while(i<sz){ while(i<sz&&(buf[i]<'0'||buf[i]>'9')) i++; if(i>=sz) break;
      long long t=0; while(i<sz&&buf[i]>='0'&&buf[i]<='9'){ t=t*10+(buf[i]-'0'); i++; } v.push_back(t); } }
  n=(int)v[0]; lroad=v[1]; K=v[2]; d=(int)v[3];
  for(int i=0;i<n;i++) x[i]=v[4+i];
  vector<long long> rest(v.begin()+4+n,v.end());

  act.clear(); for(int i=0;i<n;i++) act.insert(act.end(),i);
  base=1; while(base<n+1) base<<=1;
  seg.assign(2*base, Nd{});
  seg[base]=leafVal(0);
  for(int i=0;i<n;i++) seg[base+i+1]=leafVal(i+1);
  for(int i=base-1;i>=1;i--) seg[i]=mrg(seg[2*i],seg[2*i+1]);

  // p for each day is read straight from the input, never from an answer.
  // Verify that claim on the actual data: collect p and check none of them
  // equals a plausible previous answer.
  auto query=[&](int lo,int hi)->Nd{
    if(lo>hi) return Nd{};
    Nd L,R; for(int a=lo+base,b=hi+1+base;a<b;a>>=1,b>>=1){ if(a&1) L=mrg(L,seg[a++]); if(b&1) R=mrg(seg[--b],R);} return mrg(L,R);
  };

  // Naive path: 3 independent root-ward walks per update.
  auto refreshNaive=[&](int c){ seg[base+c]=leafVal(c); for(int i=(base+c)>>1;i>=1;i>>=1) seg[i]=mrg(seg[2*i],seg[2*i+1]); };

  dirty.reserve(16);
  // Batched path: set every dirty leaf, then recompute each ancestor once.
  auto flush=[&](){
    if(dirty.empty()) return;
    sort(dirty.begin(),dirty.end()); dirty.erase(unique(dirty.begin(),dirty.end()),dirty.end());
    // level-by-level: the dirty node set shrinks fast, so total merges ~= depth
    vector<int> cur, nxt; cur.reserve(8); nxt.reserve(8);
    for(int c: dirty){ seg[base+c]=leafVal(c); cur.push_back(base+c); }
    while(!cur.empty()){
      nxt.clear();
      int pv=-1;
      for(int idx: cur){
        int par=idx>>1;
        if(par==pv) continue;
        pv=par;
        seg[par]=mrg(seg[2*par],seg[2*par+1]);
        if(par) nxt.push_back(par);
      }
      cur.swap(nxt);
    }
    dirty.clear();
  };

  auto mark=[&](int c){ dirty.push_back(c); };

  long long idx=0, acc=0;
  auto T0=chrono::steady_clock::now();
  while(idx<(long long)rest.size()){
    int z=(int)rest[idx++]; int u=(int)rest[idx++]; int64 p=rest[idx++];
    for(int i=0;i<z;i++){
      int slot=(int)rest[idx++]-1;
      int pv=-1; auto it=act.lower_bound(slot); if(it!=act.begin()) pv=*std::prev(it);
      act.insert(slot);
      if(batched){ mark(slot+1); if(pv>=0) mark(pv+1); else mark(0); }
      else { refreshNaive(slot+1); if(pv>=0) refreshNaive(pv+1); else refreshNaive(0); }
    }
    for(int i=0;i<u;i++){
      int slot=(int)rest[idx++]-1;
      auto it=act.find(slot); if(it==act.end()) continue;
      int pv=(it==act.begin())?-1:*std::prev(it);
      act.erase(it);
      if(batched){ mark(slot+1); if(pv>=0) mark(pv+1); else mark(0); }
      else { refreshNaive(slot+1); if(pv>=0) refreshNaive(pv+1); else refreshNaive(0); }
    }
    if(batched) flush();

    int r=(int)(upper_bound(x,x+n,p)-x)-1; int p0=0;
    if(r>=0){ auto it=act.upper_bound(r); if(it!=act.begin()) p0=*std::prev(it)+1; }
    if(act.empty()) continue;
    Nd Q0=query(0,p0), Q1=query(p0+1,n); int64 cltot=seg[1].clSum;
    int64 aNeg=min(Q0.abNeg,Q0.aMinNeg+Q0.P+Q1.cMinT), aPos=Q1.abPos;
    int64 cdP=min(Q1.cdPos,Q0.dMinT+Q0.P+Q1.uMinPos);
    int64 A=cltot+min(aNeg,Q0.cdNeg), B=cltot+min(aPos,cdP);
    acc+=min(p+A,-p+B);
  }
  auto T1=chrono::steady_clock::now();
  printf("%-10s %7.1f ms   slop=%lld\n", batched?"batched":"naive", chrono::duration<double,milli>(T1-T0).count(), acc);
  return 0;
}
