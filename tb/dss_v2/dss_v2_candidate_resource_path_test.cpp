#include "Vdss_v2_candidate_resource_path_test_top.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>
namespace { void require(bool v,const char*s){if(!v){std::cerr<<"FAIL: "<<s<<'\n';std::exit(1);}}
void tick(Vdss_v2_candidate_resource_path_test_top&d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();}
void put(std::uint32_t*w,unsigned b,unsigned n,unsigned v){w[b/32]|=v<<(b%32);if((b%32)+n>32)w[b/32+1]|=v>>(32-(b%32));}
void entry(std::uint32_t*w,unsigned s,unsigned c){unsigned b=s*20;put(w,b,1,1);put(w,b+1,3,c);put(w,b+4,1,1);put(w,b+5,1,1);put(w,b+6,4,1);put(w,b+10,10,1);}
void clear(Vdss_v2_candidate_resource_path_test_top&d){for(auto&w:d.legacy_config_pattern_map_i)w=0;}
void req(Vdss_v2_candidate_resource_path_test_top&d,unsigned sa,unsigned index,bool commit){d.sa_id_i=sa;d.sa_valid_i=1;d.selected_candidate_index_i=index;d.commit_i=commit;d.eval();}
}
int main(){Vdss_v2_candidate_resource_path_test_top d;d.rst_ni=0;d.commit_i=0;clear(d);tick(d);d.rst_ni=1;
 // B canonical slot 1 is CFG1: release B_COL.
 clear(d);entry(d.legacy_config_pattern_map_i,2,1);req(d,1,1,true);require(d.selected_candidate_valid_o&&d.physical_feasible_o,"B release candidate");tick(d);require(d.resource_released_o==4,"B release committed");
 // A canonical slot 2 is CFG5: locally valid but requires released B_COL.
 clear(d);entry(d.legacy_config_pattern_map_i,2,5);req(d,0,2,true);require(d.selected_candidate_valid_o&&d.physical_feasible_o,"A borrow candidate");tick(d);require(d.resource_borrowed_o==4,"candidate to ledger borrow");
 d.rst_ni=0;d.commit_i=0;tick(d);d.rst_ni=1;clear(d);entry(d.legacy_config_pattern_map_i,2,5);req(d,0,2,false);require(d.selected_candidate_valid_o&&!d.physical_feasible_o,"local valid distinct from physical infeasible");require(d.resource_released_o==0,"infeasible no commit");
 std::cout<<"DSS_V2_PHASE4D_CANDIDATE_RESOURCE_PATH PASS\n";}
