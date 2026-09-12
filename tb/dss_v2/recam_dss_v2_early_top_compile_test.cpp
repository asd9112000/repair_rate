#include "Vrecam_dss_v2_early_top.h"
#include <iostream>

int main() {
    Vrecam_dss_v2_early_top dut;
    dut.clk_i = 0;
    dut.rst_ni = 0;
    dut.start_i = 0;
    dut.pivot_valid_i = 0;
    dut.pivot_rows_flat_i = 0;
    dut.pivot_cols_flat_i = 0;
    dut.row_gt1_i = dut.row_gt2_i = dut.row_gt3_i = 0;
    dut.col_gt1_i = dut.col_gt2_i = dut.col_gt3_i = 0;
    dut.hybrid_valid_i = 0;
    dut.hybrid_pointer_flat_i = 0;
    dut.hybrid_descriptor_i = 0;
    dut.hybrid_differing_flat_i = 0;
    dut.conventional_overflow_i = 0;
    dut.eval();
    std::cout << "DSS_V2_PHASE4E_EARLY_TOP_COMPILE PASS\n";
}
