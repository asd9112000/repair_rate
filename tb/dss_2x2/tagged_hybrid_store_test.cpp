#include "Vtagged_hybrid_store.h"
#include <cstdlib>
#include <iostream>
namespace { void require(bool v, const char *m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
void tick(Vtagged_hybrid_store &d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();} }
int main(){ Vtagged_hybrid_store d; d.rst_ni=0;d.clear_i=0;d.write_valid_i=0;tick(d);d.rst_ni=1;
 d.fault_ref_i=9;d.fault_row_i=17;d.fault_col_i=23;d.descriptor_i=1;d.pivot_ptr_i=2;d.cfg_valid_i=0x06;d.write_valid_i=1;tick(d);d.write_valid_i=0;d.eval();
 require(d.occupancy_o==1 && (d.valid_o&1),"Hybrid record was not stored once");
 require((d.cfg_valid_flat_o[0]&0x7f)==0x06 && (d.pivot_ptrs_flat_o&7)==2,"Hybrid tag metadata changed");
 require((d.rows_flat_o[0]&0x3ff)==17 && (d.cols_flat_o[0]&0x3ff)==23,"Hybrid address payload changed");
 std::cout<<"tagged_hybrid_store_test PASS\n"; }
