#include "Vrecam_dss_hyp02_static_global_top.h"

#include <cstdlib>
#include <iostream>

namespace {

void tick(Vrecam_dss_hyp02_static_global_top& dut) {
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void initialize(Vrecam_dss_hyp02_static_global_top& dut, bool overflow) {
    dut.start_i = 0;
    dut.pivot_valid_i = 0;
    dut.pivot_rows_flat_i = 0;
    dut.pivot_cols_flat_i = 0;
    dut.row_gt1_i = 0;
    dut.row_gt2_i = 0;
    dut.row_gt3_i = 0;
    dut.col_gt1_i = 0;
    dut.col_gt2_i = 0;
    dut.col_gt3_i = 0;
    dut.hybrid_valid_i = 0;
    dut.hybrid_pointer_flat_i = 0;
    dut.hybrid_descriptor_i = 0;
    dut.hybrid_differing_flat_i = 0;
    dut.conventional_overflow_i = overflow ? 1U : 0U;
    dut.rst_ni = 0;
    tick(dut);
    dut.rst_ni = 1;
    tick(dut);
}

void run_case(Vrecam_dss_hyp02_static_global_top& dut, bool overflow) {
    initialize(dut, overflow);
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
    for (unsigned cycle = 0U; cycle < 20U && dut.done_o == 0U; ++cycle) {
        tick(dut);
    }
    require(dut.done_o != 0U, "top did not complete within its fixed collection/decision latency");
    if (overflow) {
        require(dut.group_repairable_o == 0U, "overflow case unexpectedly repaired");
        require(dut.sa_commit_valid_o == 0U, "overflow case unexpectedly committed");
    } else {
        require(dut.group_repairable_o != 0U, "all-local top smoke case did not repair");
        require(dut.sa_commit_valid_o == 0xfU, "all-local top smoke case was not atomic");
        require(dut.selected_config_flat_o == 0U, "all-local path did not select CFG0 for all SAs");
        require(dut.release_flat_o == 0U && dut.borrow_flat_o == 0U,
                "all-local path unexpectedly used a directional edge");
    }
}

}  // namespace

int main() {
    Vrecam_dss_hyp02_static_global_top dut;
    run_case(dut, false);
    run_case(dut, true);
    std::cout << "TOP_ALL_LOCAL_SMOKE: PASS\n";
    std::cout << "TOP_OVERFLOW_REJECTION: PASS\n";
    return 0;
}
