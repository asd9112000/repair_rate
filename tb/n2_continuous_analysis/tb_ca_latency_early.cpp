#define STR2(x) #x
#define STR(x) STR2(x)
#include STR(CA_LATENCY_HEADER)
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
using D=CA_LATENCY_DUT;
struct R{bool seen=0,dup=0;uint8_t c=0,p=0,r=0,v=0;uint32_t a[3]={};};static R x[4],y[4];static long cyc=0,st[4];static std::vector<int> lat;static unsigned rankc[3]={};
static void put(R&q,uint8_t c,uint8_t p,uint8_t r,uint8_t v,const uint32_t*a){if(q.seen){q.dup=1;return;}q.seen=1;q.c=c;q.p=p;q.r=r;q.v=v;for(int i=0;i<3;++i)q.a[i]=a[i];}
static void cap(D&d){if(d.canonical_commit_o)put(x[d.canonical_sa_o],d.canonical_config_o,d.canonical_pattern_o,d.canonical_is_row_o,d.canonical_line_valid_o,d.canonical_address_o);if(d.live_commit_o){put(y[d.live_sa_o],d.live_config_o,d.live_pattern_o,d.live_is_row_o,d.live_line_valid_o,d.live_address_o);int sl=d.live_scan_slot_o;int r=sl==1?0:sl==0?1:2;rankc[r]++;lat.push_back(int(cyc-st[d.live_sa_o]));}}
static void tick(D&d){d.clk_i=0;d.eval();cap(d);d.clk_i=1;d.eval();d.clk_i=0;d.eval();++cyc;}static void reset(D&d){for(int i=0;i<4;++i){x[i]=R{};y[i]=R{};}d.rst_ni=0;d.canonical_start_i=0;d.state_update_i=0;d.test_done_valid_i=0;d.seeds_i=0;tick(d);tick(d);d.rst_ni=1;}
static bool same(){for(int i=0;i<4;++i){if(x[i].seen!=y[i].seen||x[i].dup||y[i].dup||x[i].c!=y[i].c||x[i].p!=y[i].p||x[i].r!=y[i].r||x[i].v!=y[i].v)return 0;for(int j=0;j<3;++j)if(x[i].a[j]!=y[i].a[j])return 0;}return 1;}static uint32_t nx(uint32_t&z){z^=z<<13;z^=z>>17;z^=z<<5;return z;}
int main(){uint32_t z=0x20260928;D d;for(unsigned k=0;k<10000;++k){reset(d);d.seeds_i=nx(z);d.canonical_start_i=1;tick(d);d.canonical_start_i=0;bool cd=0;for(int i=0;i<24&&!cd;++i){tick(d);cd=d.canonical_done_o;}if(!cd)return 1;for(unsigned sa=0;sa<4&&!d.live_done_o;++sa){bool f=(k^sa)&1;d.state_update_i=1;d.state_sa_i=sa;d.test_done_valid_i=f;d.test_done_sa_i=sa;tick(d);st[sa]=cyc-1;d.state_update_i=0;d.test_done_valid_i=0;for(int i=0;i<8&&!d.live_done_o&&d.live_active_sa_o==sa;++i){if(!f&&d.live_scan_active_o){d.test_done_valid_i=1;d.test_done_sa_i=sa;}tick(d);d.test_done_valid_i=0;}}if(!d.live_done_o||d.canonical_repairable_o!=d.live_repairable_o||!same()){std::cerr<<"MISMATCH="<<k<<'\n';return 1;}}std::sort(lat.begin(),lat.end());long sum=0;for(int n:lat)sum+=n;auto q=[&](double p){return lat[size_t((lat.size()-1)*p)];};std::cout<<"LATENCY_REPLAY_PASS vectors=10000 corpus_seed=0x20260928 samples="<<lat.size()<<" rank1="<<rankc[0]<<" rank2="<<rankc[1]<<" rank3="<<rankc[2]<<" min="<<lat.front()<<" mean="<<double(sum)/lat.size()<<" median="<<q(.5)<<" p95="<<q(.95)<<" max="<<lat.back()<<" mismatches=0\n";}
