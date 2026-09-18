#include "Vrecam_dss_v2_retained_collector_bank.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {
void tick(Vrecam_dss_v2_retained_collector_bank& dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void fault(Vrecam_dss_v2_retained_collector_bank& dut, unsigned row, unsigned col)
{
    require(dut.fault_ready_o, "collector became not-ready before twelve faults");
    dut.fault_valid_i = 1;
    dut.fault_row_i = row;
    dut.fault_col_i = col;
    tick(dut);
    dut.fault_valid_i = 0;
}
} // namespace

int main()
{
    try {
        Vrecam_dss_v2_retained_collector_bank dut;
        dut.rst_ni = 0;
        dut.clear_i = 0;
        dut.fault_valid_i = 0;
        dut.fault_row_i = 0;
        dut.fault_col_i = 0;
        dut.config_id_i = 0;
        dut.analysis_complete_i = 0;
        dut.analysis_generation_i = 0;
        tick(dut);
        dut.rst_ni = 1;

        // One Pivot followed by eleven same-row faults is the mandatory
        // membership-visible-11 boundary.  H10 is valid, H11-H13 are not.
        fault(dut, 7, 3);
        for (unsigned col = 4; col <= 14; ++col)
            fault(dut, 7, col);
        require(dut.fault_count_o == 12, "fault count reconstruction mismatch");
        require(!dut.fault_ready_o, "fault-ready remains high at MAX_FAULTS");
        require((dut.hybrid_valid_o & 0x07ffU) == 0x07ffU,
                "reachable Hybrid prefix was not preserved");
        require((dut.hybrid_valid_o & 0x3800U) == 0,
                "unreachable H11-H13 became valid");
        require((dut.reuse_valid_o & 0x0800U) == 0,
                "unreachable reuse R11 became valid");
        require((dut.hybrid_cfg_valid_flat_o[0] & 0x7fU) == 0x7fU,
                "Hybrid cfg-valid reconstruction mismatch for pointer zero");

        // A stale completion may not establish validity; a matching completion may.
        dut.analysis_complete_i = 1;
        dut.analysis_generation_i = 0;
        tick(dut);
        require(!dut.analysis_valid_o, "stale analysis completion became valid");
        dut.analysis_generation_i = dut.fault_generation_o;
        tick(dut);
        require(dut.analysis_valid_o, "matching analysis completion was rejected");
        dut.clear_i = 1;
        tick(dut);
        dut.clear_i = 0;
        require(!dut.analysis_valid_o, "new collector generation did not invalidate analysis");

        // Update wins over completion for a normal (first-Pivot) generation.
        dut.analysis_complete_i = 1;
        dut.analysis_generation_i = 0;
        dut.fault_valid_i = 1;
        dut.fault_row_i = 21;
        dut.fault_col_i = 31;
        tick(dut);
        dut.fault_valid_i = 0;
        dut.analysis_complete_i = 0;
        require(dut.fault_generation_o == 1 && !dut.analysis_valid_o,
                "normal same-cycle update did not win over completion");

        // The same priority holds after a Hybrid-producing relation.
        dut.analysis_complete_i = 1;
        dut.analysis_generation_i = dut.fault_generation_o;
        dut.fault_valid_i = 1;
        dut.fault_row_i = 21;
        dut.fault_col_i = 32;
        tick(dut);
        dut.fault_valid_i = 0;
        dut.analysis_complete_i = 0;
        require(dut.fault_generation_o == 2 && !dut.analysis_valid_o &&
                    (dut.hybrid_valid_o & 1U),
                "Hybrid same-cycle update did not win over completion");

        // Five Pivots followed by an unrelated fault exercises the retained
        // CAM-reuse append rule under the same update-vs-completion race.
        dut.clear_i = 1;
        tick(dut);
        dut.clear_i = 0;
        for (unsigned index = 0; index < 5; ++index)
            fault(dut, 40 + index, 60 + index);
        dut.analysis_complete_i = 1;
        dut.analysis_generation_i = dut.fault_generation_o;
        dut.fault_valid_i = 1;
        dut.fault_row_i = 99;
        dut.fault_col_i = 199;
        tick(dut);
        dut.fault_valid_i = 0;
        dut.analysis_complete_i = 0;
        require(dut.fault_generation_o == 6 && !dut.analysis_valid_o &&
                    (dut.reuse_valid_o & 1U),
                "CAM-reuse same-cycle update did not win over completion");

        std::cout << "S1GA2G_RETAINED_BANK_TEST PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "S1GA2G_RETAINED_BANK_TEST FAIL: " << error.what() << '\n';
        return 1;
    }
}
