#include "Vrecam_early_solution_selector.h"
#include <cstdint>
#include <iostream>
#include <stdexcept>
static void req(bool v,const char*s){if(!v)throw std::runtime_error(s);}
static void clear(Vrecam_early_solution_selector&d){for(auto&w:d.config_pattern_map_i)w=0;}
static void put(Vrecam_early_solution_selector&d,unsigned i,bool valid,bool sol,bool rep,unsigned cfg,unsigned pat,unsigned cand){unsigned b=i*20,w=b/32,o=b%32;std::uint64_t x=(std::uint64_t(cand)<<10)|(std::uint64_t(pat)<<6)|(rep<<5)|(sol<<4)|(cfg<<1)|valid;d.config_pattern_map_i[w]|=x<<o;if(o>12)d.config_pattern_map_i[w+1]|=x>>(32-o);}
static void check(Vrecam_early_solution_selector&d,unsigned i,unsigned cfg,unsigned pat,unsigned cand){d.eval();req(d.early_solution_valid_o,"solution invalid");req(d.selected_map_index_o==i,"index");req(d.selected_config_id_o==cfg&&d.selected_pattern_id_o==pat,"alignment");req(d.selected_candidate_valid_o==cand,"bitmap");}
int main(){try{Vrecam_early_solution_selector d;
 clear(d);put(d,0,1,1,1,0,1,0x3f);put(d,1,1,1,1,4,2,3);check(d,0,0,1,0x3f);std::cout<<"CFG0 FIRST PASS\n";
 clear(d);put(d,1,1,1,1,4,7,0x153);put(d,2,1,1,1,5,3,2);check(d,1,4,7,0x153);std::cout<<"SECOND ENTRY PASS\n";
 clear(d);put(d,2,1,1,1,1,6,0x2aa);check(d,2,1,6,0x2aa);std::cout<<"THIRD ENTRY PASS\n";
 clear(d);put(d,3,1,1,1,3,9,0x155);check(d,3,3,9,0x155);std::cout<<"FOURTH ENTRY PASS\n";
 clear(d);put(d,0,0,1,1,0,1,3);d.eval();req(!d.early_solution_valid_o&&!d.selected_repairable_o&&d.selected_candidate_valid_o==0,"valid qualification/default");std::cout<<"NO SOLUTION AND VALID QUALIFICATION PASS\n";
 std::cout<<"PHASE3D_EARLY_FUNCTIONAL_REGRESSION PASS\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
