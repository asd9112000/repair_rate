#include "Vrecam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top.h"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void tick(Vrecam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

void initialize(Vrecam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top &dut,
                bool overflow)
{
    dut.start_i = 0;
    dut.pivot_valid_i = 0;
    dut.pivot_rows_flat_i = 0;
    for (auto &word : dut.pivot_cols_flat_i) word = 0;
    dut.row_gt1_i = 0;
    dut.row_gt2_i = 0;
    dut.row_gt3_i = 0;
    dut.col_gt1_i = 0;
    dut.col_gt2_i = 0;
    dut.col_gt3_i = 0;
    dut.hybrid_valid_i = 0;
    dut.hybrid_pointer_flat_i = 0;
    dut.hybrid_descriptor_i = 0;
    for (auto &word : dut.hybrid_differing_flat_i) word = 0;
    dut.conventional_overflow_i = overflow ? 1U : 0U;
    dut.rst_ni = 0;
    tick(dut);
    dut.rst_ni = 1;
}

void runCase(bool overflow)
{
    Vrecam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top dut;
    initialize(dut, overflow);
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
    unsigned streamed_commit_mask = 0;
    unsigned streamed_config_mask = 0;
    for (unsigned cycle = 0; cycle < 16U && dut.done_o == 0U; ++cycle) {
        dut.eval();
        if (dut.solution_commit_valid_o != 0U) {
            streamed_commit_mask |= 1U << dut.solution_sa_o;
            streamed_config_mask |= static_cast<unsigned>(dut.solution_config_o) <<
                (3U * dut.solution_sa_o);
        }
        tick(dut);
    }
    require(dut.done_o != 0U, "top completes within four ranks per SA");
    if (overflow) {
        require(dut.group_repairable_o == 0U, "overflow input must fail");
        require(streamed_commit_mask == 0U, "overflow input must not commit");
    } else {
        require(dut.group_repairable_o != 0U, "all-local analyzer case repairs");
        require(streamed_commit_mask == 0xfU, "all-local case streams all SAs");
        require(streamed_config_mask == 0x84cU,
                "all-valid case follows frozen HYP02 EARLY slot priority");
    }
}

} // namespace

int main()
{
    runCase(false);
    runCase(true);
    std::cout << "HYP02_SHARED_ANALYZER_ALL_LOCAL=PASS\n";
    std::cout << "HYP02_SHARED_ANALYZER_OVERFLOW_REJECTION=PASS\n";
    return 0;
}
