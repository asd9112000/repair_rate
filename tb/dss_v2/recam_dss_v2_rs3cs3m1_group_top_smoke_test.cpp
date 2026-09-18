#include "Vrecam_dss_v2_rs3cs3m1_group_top.h"
#include <iostream>
namespace {
void tick(Vrecam_dss_v2_rs3cs3m1_group_top& d) { d.clk_i=0; d.eval(); d.clk_i=1; d.eval(); }
void init(Vrecam_dss_v2_rs3cs3m1_group_top& d) {
    d.pivot_valid_i=0; d.pivot_rows_flat_i=0; d.pivot_cols_flat_i=0;
    d.row_gt1_i=d.row_gt2_i=d.row_gt3_i=d.row_gt4_i=0;
    d.col_gt1_i=d.col_gt2_i=d.col_gt3_i=d.col_gt4_i=0;
    d.hybrid_valid_i=0; d.hybrid_pointer_flat_i=0; d.hybrid_descriptor_i=0;
    d.hybrid_differing_flat_i[0]=0; d.hybrid_differing_flat_i[1]=0; d.conventional_overflow_i=0;
}
}
int main() {
    Vrecam_dss_v2_rs3cs3m1_group_top dut; init(dut); dut.rst_ni=0; dut.start_i=0; tick(dut); dut.rst_ni=1;
    dut.start_i=1; tick(dut); dut.start_i=0;
    for (unsigned i=0; i<48 && !dut.done_o; ++i) tick(dut);
    const bool pass = dut.done_o && dut.group_repairable_o && dut.sa_commit_valid_o == 0xf &&
                      dut.selected_pattern_flat_o == 0x041041;
    std::cout << "TARGET_GROUP_ALL_LOCAL " << (pass ? "PASS" : "FAIL") << '\n';
    return pass ? 0 : 1;
}
