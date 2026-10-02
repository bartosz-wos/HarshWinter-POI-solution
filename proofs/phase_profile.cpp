// Where does the 1.27 s actually go?  Time each phase by stubbing the others.
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

int main(int argc,char**argv){
  // argv[1]: phase to measure  (0=all,1=read only,2=read+build,3=all+queries)
  int phase = argc>1?atoi(argv[1]):0;
  auto T0=chrono::steady_clock::now();
  FILE*fp=fopen("/tmp/fs.in","r");
  fseek(fp,0,SEEK_END); long sz=ftell(fp); fseek(fp,0,SEEK_SET);
  vector<char> buf(sz+1); fread(buf.data(),1,sz,fp); buf[sz]=0; fclose(fp);
  auto T1=chrono::steady_clock::now();
  // Proper token scan: all numbers in this format are non-negative, so a
  // non-digit is a separator.  (Reading 64-bit words over the char buffer is
  // meaningless -- alignment and padding do not line up.)
  vector<long long> v; v.reserve(1<<22);
  {
    long i=0;
    while(i<sz){
      while(i<sz && (buf[i]<'0'||buf[i]>'9')) i++;
      if(i>=sz) break;
      long long x=0;
      while(i<sz && buf[i]>='0' && buf[i]<='9'){ x=x*10+(buf[i]-'0'); i++; }
      v.push_back(x);
    }
  }
  auto T2=chrono::steady_clock::now();
  int n=(int)v[0]; lroad=v[1]; K=v[2]; int d=(int)v[3];
  vector<int64> x(n); for(int i=0;i<n;i++) x[i]=v[4+i];
  vector<long long> rest(v.begin()+4+n, v.end());
  auto T3=chrono::steady_clock::now();

  set<int> act; for(int i=0;i<n;i++) act.insert(act.end(),i);
  int base=1; while(base<n+1) base<<=1;
  vector<Nd> seg(2*base);
  seg[base]=makeLead(x[*act.begin()]);
  for(int i=0;i<n;i++){ bool in=(i+1<n); int64 g=in?(x[i+1]-x[i]):(lroad-x[i]); seg[base+i+1]=makeStn(x[i],g,in); }
  for(int i=base-1;i>=1;i--) seg[i]=mrg(seg[2*i],seg[2*i+1]);
  auto T4=chrono::steady_clock::now();

  auto refresh=[&](int c){
    if(c==0){ setleaf: seg[base]=act.empty()?Nd{}:makeLead(x[*act.begin()]);
              for(int i=(base)>>1;i>=1;i>>=1) seg[i]=mrg(seg[2*i],seg[2*i+1]); return; }
    int slot=c-1; auto it=act.find(slot);
    if(it==act.end()){ seg[base+c]=Nd{}; }
    else { ++it; seg[base+c] = (it==act.end())? makeStn(x[slot],lroad-x[slot],false) : makeStn(x[slot],x[*it]-x[slot],true); }
    for(int i=(base+c)>>1;i>=1;i>>=1) seg[i]=mrg(seg[2*i],seg[2*i+1]);
  };
  auto setleaf_unused = [&]{};
  long long idx=0; long long acc=0;
  auto query=[&](int lo,int hi)->Nd{
    if(lo>hi) return Nd{};
    Nd L,R; for(int a=lo+base,b=hi+1+base;a<b;a>>=1,b>>=1){ if(a&1) L=mrg(L,seg[a++]); if(b&1) R=mrg(seg[--b],R);} return mrg(L,R);
  };
  int days=0; long long nup=0;
  while(idx<(long long)rest.size()){
    int z=(int)rest[idx++]; int u=(int)rest[idx++]; int64 p=rest[idx++]; days++;
    auto ap=[&](int slot){ int pv=-1; auto it=act.lower_bound(slot); if(it!=act.begin()) pv=*std::prev(it); act.insert(slot); refresh(slot+1); if(pv>=0) refresh(pv+1); else refresh(0); };
    auto dp=[&](int slot){ auto it=act.find(slot); if(it==act.end()) return; int pv=(it==act.begin())?-1:*std::prev(it); act.erase(it); refresh(slot+1); if(pv>=0) refresh(pv+1); else refresh(0); };
    for(int i=0;i<z;i++){ nup++; ap((int)rest[idx++]-1); }
    for(int i=0;i<u;i++){ nup++; dp((int)rest[idx++]-1); }
    if(phase>=3){
      int r=(int)(upper_bound(x.begin(),x.end(),p)-x.begin())-1; int p0=0;
      if(r>=0){ auto it=act.upper_bound(r); if(it!=act.begin()) p0=*std::prev(it)+1; }
      if(act.empty()) continue;
      Nd Q0=query(0,p0), Q1=query(p0+1,n); int64 cltot=seg[1].clSum;
      int64 aNeg=min(Q0.abNeg,Q0.aMinNeg+Q0.P+Q1.cMinT), aPos=Q1.abPos;
      int64 cdP=min(Q1.cdPos,Q0.dMinT+Q0.P+Q1.uMinPos);
      int64 A=cltot+min(aNeg,Q0.cdNeg), B=cltot+min(aPos,cdP);
      acc+=min(p+A,-p+B);
    }
  }
  auto T5=chrono::steady_clock::now();
  auto ms=[](auto a,auto b){ return chrono::duration<double,milli>(b-a).count(); };
  fprintf(stderr,"days=%d  updates=%lld  slop=%lld\n",days,nup,acc);
  printf("read file      %7.1f ms\n",ms(T0,T1));
  printf("tokenise       %7.1f ms\n",ms(T1,T2));
  printf("setup x[]      %7.1f ms\n",ms(T2,T3));
  printf("build tree     %7.1f ms\n",ms(T3,T4));
  printf("days+queries   %7.1f ms\n",ms(T4,T5));
  printf("TOTAL          %7.1f ms\n",ms(T0,T5));
  return 0;
}
