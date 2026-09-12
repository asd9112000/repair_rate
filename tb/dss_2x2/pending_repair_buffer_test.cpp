#include "Vpending_repair_buffer.h"
#include <cstdlib>
#include <iostream>
static void tick(Vpending_repair_buffer&d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();}
static void require(bool v,const char*m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
int main(){
    Vpending_repair_buffer d;d.rst_ni=0;d.clear_i=0;d.capture_group_i=0;d.group_success_i=0;
    for(auto&w:d.pivot_rows_flat_i)w=0;for(auto&w:d.pivot_cols_flat_i)w=0;for(auto&w:d.cam_reuse_flat_i)w=0;
    for(auto&w:d.hybrid_rows_flat_i)w=0;for(auto&w:d.hybrid_cols_flat_i)w=0;
    for(auto&w:d.hybrid_ptrs_flat_i)w=0;for(auto&w:d.hybrid_cfg_valid_flat_i)w=0;
    for(auto&w:d.row_must_by_cfg_i)w=0;for(auto&w:d.col_must_by_cfg_i)w=0;
    d.hybrid_valid_i=0;d.hybrid_descriptors_i=0;
    d.selected_configs_i=0x321;d.selected_patterns_i=0x4321;d.pivot_valid_i=1;d.cam_reuse_valid_i=1;tick(d);d.rst_ni=1;
    d.capture_group_i=1;d.group_success_i=0;tick(d);
    require(!d.pending_valid_o&&!d.final_program_valid_o,"failed group escaped to eFuse staging");
    d.group_success_i=1;tick(d);
    require(d.pending_valid_o&&d.final_program_valid_o,"successful group was not staged atomically");
    require(d.selected_configs_o==0x321&&d.selected_patterns_o==0x4321,"pending IDs differ from selection");
    d.capture_group_i=0;tick(d);require(d.pending_valid_o&&!d.final_program_valid_o,"program-valid was not a one-cycle pulse");
    std::cout<<"pending_repair_buffer_test PASS\n";
}
