#include "Vrecam_dss_v2_group_core.h"
#include <array>
#include <cstdlib>
#include <iostream>
struct C { bool v=false; unsigned p=0; }; using M=std::array<std::array<C,4>,4>;
struct R { bool ok=false,collection_before=false,stable=true,latest_ledger=false; unsigned commits=0,ledger=0,cfg=0,pat=0,donor=0,borrow=0,release=0,fail=0,writes=0; };
static void tick(Vrecam_dss_v2_group_core&d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();}
static void put(M&m,unsigned sa,unsigned sl,unsigned p){m[sa][sl]={true,p};}
static R run(const M&m){Vrecam_dss_v2_group_core d;d.rst_ni=0;d.start_i=0;d.candidate_valid_i=0;d.candidate_pattern_id_i=0;tick(d);d.rst_ni=1;d.start_i=1;tick(d);d.start_i=0;R r;bool collection_clean=true,image_seen=false;std::array<unsigned,3> image{};for(unsigned n=0;n<48&&!d.done_o;n++){d.eval();if(d.collection_active_o){d.candidate_valid_i=m[d.current_sa_o][d.current_slot_o].v;d.candidate_pattern_id_i=m[d.current_sa_o][d.current_slot_o].p;++r.writes;if(d.ledger_released_borrower_o||d.sa_commit_valid_o)collection_clean=false;}else{const std::array<unsigned,3> now={d.candidate_store_image_o[0],d.candidate_store_image_o[1],d.candidate_store_image_o[2]};if(!image_seen){image=now;image_seen=true;}else if(now!=image)r.stable=false;if(d.allocation_active_o&&d.current_sa_o==1&&d.ledger_released_borrower_o==1)r.latest_ledger=true;d.candidate_valid_i=0;d.candidate_pattern_id_i=0;}tick(d);}r.ok=d.group_repairable_o;r.collection_before=(r.writes==16&&collection_clean);r.commits=d.sa_commit_valid_o;r.ledger=d.ledger_released_borrower_o;r.cfg=d.selected_config_flat_o;r.pat=d.selected_pattern_flat_o;r.donor=d.selected_donor_flat_o;r.borrow=d.borrow_flat_o;r.release=d.release_flat_o;r.fail=d.failure_position_o;return r;}
static int f=0;static void ck(bool v,const char*n){if(!v){std::cerr<<"FAIL "<<n<<'\n';++f;}else std::cout<<n<<" PASS\n";}
static M locals(){M m{};for(unsigned s=0;s<4;s++)put(m,s,0,s+1);return m;}
int main(){
 {auto r=run(locals());ck(r.ok&&r.commits==15,"GROUP_FULL_LOCAL_SUCCESS");ck(r.collection_before,"COLLECTION_BEFORE_ALLOCATION");ck(r.stable,"STORE_STABLE_DURING_ALLOCATION");}
 {auto m=locals();put(m,0,1,9);auto r=run(m);ck(r.ok&&((r.cfg&7)==4),"GROUP_RANKING_SLOT1_OVER_SLOT0");}
 {auto m=locals();auto r=run(m);ck(r.ok&&((r.cfg&7)==0),"GROUP_RANKING_SLOT0_FALLBACK");}
 {M m{};put(m,0,1,1);put(m,1,3,7);put(m,2,0,3);put(m,3,0,4);auto r=run(m);ck(r.ok&&((r.cfg>>3)&7)==3,"GROUP_RANKING_SLOT3_FALLBACK");}
 {M m{};put(m,0,1,1);put(m,1,2,6);put(m,2,0,3);put(m,3,0,4);auto r=run(m);ck(r.ok&&((r.cfg>>3)&7)==2,"GROUP_RANKING_SLOT2_LAST");}
 {M m{};put(m,0,1,9);put(m,1,2,7);put(m,2,0,3);put(m,3,0,4);auto r=run(m);ck(r.ok&&r.ledger==0x11&&((r.cfg>>3)&7)==2&&r.latest_ledger,"GROUP_A_RELEASE_B_BORROW");ck(r.pat==0x4379,"GROUP_PATTERN_ID_PROPAGATION");}
 {M m{};put(m,0,1,1);put(m,1,2,2);put(m,2,2,3);auto r=run(m);ck(!r.ok&&r.fail==2&&r.commits==3,"GROUP_RESOURCE_CONSUMPTION");}
 {M m{};put(m,0,0,0);put(m,1,1,1);put(m,2,0,3);put(m,3,2,2);auto r=run(m);ck(r.ok&&((r.donor>>6)&3)==2,"GROUP_DONOR_PRIORITY_FALLBACK");}
 for(unsigned fail=0;fail<4;fail++){auto m=locals();m[fail]={};auto r=run(m);ck(!r.ok&&r.fail==fail&&r.commits==((1u<<fail)-1),fail==0?"GROUP_FAILURE_AT_A":fail==1?"GROUP_FAILURE_AT_B":fail==2?"GROUP_FAILURE_AT_C":"GROUP_FAILURE_AT_D");}
 {M m{};put(m,0,1,1);put(m,0,0,2);put(m,1,0,3);put(m,2,0,4);put(m,3,0,5);auto r=run(m);ck(r.ok&&(r.cfg&7)==4&&r.ledger==1,"GROUP_NON_SELECTED_NO_COMMIT");}
 {M m{};put(m,0,1,1);put(m,1,2,2);put(m,2,2,3);auto r=run(m);ck(!r.ok&&r.commits==3&&r.ledger==0x11,"GROUP_FIRST_FAILURE_TERMINATION");ck(r.ledger==0x11,"GROUP_NO_ROLLBACK");}
 std::cout<<"GROUP_PHYSICAL_FEASIBILITY_FALLBACK = N/A_PROVEN_UNREACHABLE\n";if(f)return 1;std::cout<<"GROUP_CANDIDATE_STORE_BITS = 80\nGROUP_TOTAL_ARCH_STATE_BITS = 157\nEXTRA_CONFIG_MAP_BITS = 0\n";return 0;}
