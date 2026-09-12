#include "Vrecam_dss_v2_group_top.h"

#include <cstdlib>
#include <iostream>

static void tick(Vrecam_dss_v2_group_top& dut) {
    dut.clk_i = 0; dut.eval();
    dut.clk_i = 1; dut.eval();
}
static void clear_analyzer_inputs(Vrecam_dss_v2_group_top& dut) {
    dut.pivot_valid_i = 0; dut.pivot_rows_flat_i = 0; dut.pivot_cols_flat_i = 0;
    dut.row_gt1_i = dut.row_gt2_i = dut.row_gt3_i = 0;
    dut.col_gt1_i = dut.col_gt2_i = dut.col_gt3_i = 0;
    dut.hybrid_valid_i = 0; dut.hybrid_pointer_flat_i = 0;
    dut.hybrid_descriptor_i = 0; dut.hybrid_differing_flat_i = 0;
}
static void start(Vrecam_dss_v2_group_top& dut, bool overflow) {
    dut.rst_ni = 0; dut.start_i = 0; dut.conventional_overflow_i = overflow;
    tick(dut); dut.rst_ni = 1; dut.start_i = 1; tick(dut); dut.start_i = 0;
}
int main() {
    Vrecam_dss_v2_group_top dut;
    clear_analyzer_inputs(dut);
    start(dut, false);
    for (unsigned cycle = 0; cycle != 32 && !dut.done_o; ++cycle) tick(dut);
    if (!dut.done_o || !dut.group_repairable_o || dut.sa_commit_valid_o != 0xf ||
        dut.selected_config_flat_o != 0x84c || dut.selected_pattern_flat_o != 0x1111 ||
        dut.ledger_released_borrower_o != 0x00f) return 1;
    std::cout << "V2_GROUP_FULL_LOCAL PASS\n";

    start(dut, true);
    for (unsigned cycle = 0; cycle != 32 && !dut.done_o; ++cycle) tick(dut);
    if (!dut.done_o || dut.group_repairable_o || dut.sa_commit_valid_o != 0 ||
        dut.failure_position_o != 0 || dut.ledger_released_borrower_o != 0) return 1;
    std::cout << "V2_GROUP_FAILURE_A PASS\n";
}
