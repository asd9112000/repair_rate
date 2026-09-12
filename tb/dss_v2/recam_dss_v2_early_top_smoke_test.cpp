#include "Vrecam_dss_v2_early_top.h"
#include <cstdlib>
#include <iostream>
static void tick(Vrecam_dss_v2_early_top& d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();}
int main(){Vrecam_dss_v2_early_top d;d.rst_ni=0;d.start_i=0;d.pivot_valid_i=0;d.pivot_rows_flat_i=0;d.pivot_cols_flat_i=0;d.row_gt1_i=d.row_gt2_i=d.row_gt3_i=0;d.col_gt1_i=d.col_gt2_i=d.col_gt3_i=0;d.hybrid_valid_i=0;d.hybrid_pointer_flat_i=0;d.hybrid_descriptor_i=0;d.hybrid_differing_flat_i=0;d.conventional_overflow_i=0;tick(d);d.rst_ni=1;d.start_i=1;tick(d);d.start_i=0;for(int i=0;i<20&&!d.done_o;i++)tick(d);if(!d.done_o||!d.group_repairable_o||d.sa_commit_valid_o!=15||d.selected_config_flat_o!=0||d.selected_pattern_flat_o!=0x1111||d.ledger_released_borrower_o!=0)return 1;std::cout<<"V2_EARLY_FULL_LOCAL PASS\n";
d.rst_ni=0;d.start_i=0;d.conventional_overflow_i=1;tick(d);d.rst_ni=1;d.start_i=1;tick(d);d.start_i=0;for(int i=0;i<8&&!d.done_o;i++)tick(d);if(!d.done_o||d.group_repairable_o||d.sa_commit_valid_o!=0||d.failure_position_o!=0||d.ledger_released_borrower_o!=0)return 1;std::cout<<"V2_EARLY_FAILURE_A PASS\n";}
