// Replace std::set with a flat bitmap of active slots.
//
// The set is used for: is-empty, first, predecessor, successor, insert, erase,
// and successor-of-p.  A two-level bitmask (a uint64 word per 64 slots, plus a
// summary word marking which words are non-empty) answers all of them in O(1)
// with pure register/L1 work, instead of ~19 levels of pointer chasing.
//
// This is the honest test of whether std::set is the bottleneck.  If it is not,
// the win has to come from elsewhere.
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

// ---- flat two-level bitmap over [0,n) ----
struct ActMap {
    vector<uint64_t> w;        // bit per slot
    vector<uint64_t> sum;      // bit per word: word is non-zero
    int nw;
    void init(int n){ nw=(n+63)/64; w.assign(nw,~0ULL); sum.assign((nw+63)/64,0);
        // clear bits past n
        if(n&63) w[nw-1] = (n&63)==64?~0ULL:((1ULL<<(n&63))-1);
        rebuild(); }
    void rebuild(){ for(size_t i=0;i<sum.size();i++) sum[i]=0;
        for(int i=0;i<nw;i++) if(w[i]) sum[i>>6] |= 1ULL<<(i&63); }
    inline bool empty() const { for(uint64_t s:sum) if(s) return false; return true; }
    inline bool test(int i) const { return (w[i>>6]>>(i&63))&1ULL; }
    inline void set(int i){ w[i>>6] |= 1ULL<<(i&63); sum[i>>12] |= 1ULL<<((i>>6)&63); }
    inline void clr(int i){ w[i>>6] &= ~(1ULL<<(i&63)); if(!w[i>>6]) sum[i>>12] &= ~(1ULL<<((i>>6)&63)); }
    // smallest set bit >= from, or -1
    inline int nextAtLeast(int from) const {
        if(from<0) from=0;
        int wi=from>>6; if(wi>=nw) return -1;
        uint64_t v=w[wi] & (~0ULL<<(from&63));
        if(v) return (wi<<6)+__builtin_ctzll(v);
        int gi=wi>>6; if(gi>=(int)sum.size()) return -1;
        uint64_t s=sum[gi] & (~0ULL<<((wi&63)+1));
        if(wi&63==63) s=sum[gi]>>1;   // avoid shift by 64
        if(!s){ for(++gi; gi<(int)sum.size(); gi++) if(sum[gi]) { s=sum[gi]; break; } if(!s) return -1; s = s<<1 | 1ULL<<0; /*recheck below*/ }
        if(!s) return -1;
        int wj = (gi<<6) + __builtin_ctzll(s);
        if(wj>=nw || !w[wj]) { for(int k=wi+1;k<nw;k++) if(w[k]) return (k<<6)+__builtin_ctzll(w[k]); return -1; }
        return (wj<<6)+__builtin_ctzll(w[wj]);
    }
    inline int prevAtMost(int i) const {
        if(i<0) return -1;
        if(i>=nw*64) i=nw*64-1;
        int wi=i>>6; uint64_t v=w[wi] & ((i&63)==63?~0ULL:((1ULL<<((i&63)+1))-1));
        if(v) return (wi<<6)+63-__builtin_clzll(v);
        for(int k=wi-1;k>=0;k--) if(w[k]) return (k<<6)+63-__builtin_clzll(w[k]);
        return -1;
    }
    inline int first() const { return nextAtLeast(0); }
};

int main(int argc,char**argv){
  bool useBitmap = !(argc>1 && atoi(argv[1])==0);
  FILE*fp=fopen("/tmp/fs.in","r");
  fseek(fp,0,SEEK_END); long sz=ftell(fp); fseek(fp,0,SEEK_SET);
  vector<char> buf(sz+1); if(fread(buf.data(),1,sz,fp)!=(size_t)sz){} buf[sz]=0; fclose(fp);
  vector<long long> v; v.reserve(1<<22);
  { long i=0; while(i<sz){ while(i<sz&&(buf[i]<'0'||buf[i]>'9')) i++; if(i>=sz) break;
      long long t=0; while(i<sz&&buf[i]>='0'&&buf[i]<='9'){ t=t*10+(buf[i]-'0'); i++; } v.push_back(t); } }
  int n=(int)v[0]; lroad=v[1]; K=v[2]; int d=(int)v[3];
  vector<int64> x(n); for(int i=0;i<n;i++) x[i]=v[4+i];
  vector<long long> rest(v.begin()+4+n,v.end());

  set<int> S; ActMap A;
  if(useBitmap) A.init(n); else for(int i=0;i<n;i++) S.insert(S.end(),i);

  // lambdas that abstract the two
  auto isEmpty=[&](){ return useBitmap? A.empty() : S.empty(); };
  auto firstOf=[&](){ return useBitmap? A.first() : *S.begin(); };
  auto succOf=[&](int s){ if(useBitmap){ int q=A.nextAtLeast(s+1); return q; } auto it=S.find(s); ++it; return it==S.end()?-1:*it; };
  auto predOf=[&](int s){ if(useBitmap) return A.prevAtMost(s-1); auto it=S.lower_bound(s); if(it==S.begin()) return -1; return *std::prev(it); };
  auto insert_=[&](int s){ if(useBitmap) A.set(s); else S.insert(s); };
  auto erase_=[&](int s){ if(useBitmap) A.clr(s); else S.erase(s); };
  auto has=[&](int s){ if(useBitmap) return A.test(s); return S.find(s)!=S.end(); };
  // successor of p among active slots at or before r
  auto predLE=[&](int r){ if(useBitmap){ int q=A.prevAtMost(r); return q; } auto it=S.upper_bound(r); if(it==S.begin()) return -1; return *std::prev(it); };

  int base=1; while(base<n+1) base<<=1;
  vector<Nd> seg(2*base);
  auto leafVal=[&](int c)->Nd{
    if(c==0) return isEmpty()?Nd{}:makeLead(x[firstOf()]);
    int slot=c-1; if(!has(slot)) return Nd{};
    int q=succOf(slot);
    if(q<0) return makeStn(x[slot],lroad-x[slot],false);
    return makeStn(x[slot],x[q]-x[slot],true);
  };
  auto setLeaf=[&](int c,const Nd&val){ seg[base+c]=val; for(int i=(base+c)>>1;i>=1;i>>=1) seg[i]=mrg(seg[2*i],seg[2*i+1]); };
  setLeaf(0,leafVal(0));
  for(int i=0;i<n;i++) setLeaf(i+1,leafVal(i+1));

  auto query=[&](int lo,int hi)->Nd{
    if(lo>hi) return Nd{};
    Nd L,R; for(int a=lo+base,b=hi+1+base;a<b;a>>=1,b>>=1){ if(a&1) L=mrg(L,seg[a++]); if(b&1) R=mrg(seg[--b],R);} return mrg(L,R);
  };

  long long idx=0,acc=0;
  auto T0=chrono::steady_clock::now();
  while(idx<(long long)rest.size()){
    int z=(int)rest[idx++]; int u=(int)rest[idx++]; int64 p=rest[idx++];
    for(int i=0;i<z;i++){ int s=(int)rest[idx++]-1; int pv=predOf(s); insert_(s);
      setLeaf(s+1,leafVal(s+1)); if(pv>=0) setLeaf(pv+1,leafVal(pv+1)); else setLeaf(0,leafVal(0)); }
    for(int i=0;i<u;i++){ int s=(int)rest[idx++]-1; if(!has(s)) continue; int pv=predOf(s); erase_(s);
      setLeaf(s+1,leafVal(s+1)); if(pv>=0) setLeaf(pv+1,leafVal(pv+1)); else setLeaf(0,leafVal(0)); }
    int r=(int)(upper_bound(x.begin(),x.end(),p)-x.begin())-1; int p0=0;
    if(r>=0){ int q=predLE(r); if(q>=0) p0=q+1; }
    if(isEmpty()) continue;
    Nd Q0=query(0,p0),Q1=query(p0+1,n); int64 cltot=seg[1].clSum;
    int64 aNeg=min(Q0.abNeg,Q0.aMinNeg+Q0.P+Q1.cMinT),aPos=Q1.abPos;
    int64 cdP=min(Q1.cdPos,Q0.dMinT+Q0.P+Q1.uMinPos);
    int64 A_=cltot+min(aNeg,Q0.cdNeg),B_=cltot+min(aPos,cdP);
    acc+=min(p+A_,-p+B_);
  }
  auto T1=chrono::steady_clock::now();
  printf("%-8s %7.1f ms   slop=%lld\n",useBitmap?"bitmap":"set",chrono::duration<double,milli>(T1-T0).count(),acc);
  return 0;
}
