#include "Vrecam_dss_line1x4_rs2_cs2_m1_normalized_global_atomic_group_commit.h"
#include "verilated.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {
using Dut = Vrecam_dss_line1x4_rs2_cs2_m1_normalized_global_atomic_group_commit;
constexpr std::uint32_t kUnassigned = 0x924924U;

void tick(Dut &dut) {
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
    dut.clk_i = 0;
    dut.eval();
}

std::uint32_t ownerRows() {
    std::uint32_t result = 0;
    for (unsigned line = 0; line < 8; ++line)
        result |= (line / 2) << (line * 3);
    return result;
}

void reset(Dut &dut) {
    dut.rst_ni = 0;
    dut.start_i = 0;
    dut.selected_valid_i = 0;
    dut.selected_used_rows_flat_i = 0;
    dut.selected_used_cols_flat_i = 0;
    dut.selected_donor_flat_i = 0xffff;
    dut.selected_row_assignment_flat_i = 0;
    tick(dut);
    dut.rst_ni = 1;
}

void driveLocalTuple(Dut &dut) {
    dut.selected_valid_i = 0xf;
    dut.selected_used_rows_flat_i = 0x492;
    dut.selected_used_cols_flat_i = 0;
    dut.selected_donor_flat_i = 0xffff;
    dut.selected_row_assignment_flat_i = ownerRows();
}
} // namespace

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Dut dut;
    unsigned atomicity_violations = 0;
    unsigned partial_visibility_errors = 0;
    unsigned commit_error_state_corruption = 0;
    try {
        reset(dut);
        driveLocalTuple(dut);
        dut.start_i = 1;
        tick(dut);
        dut.start_i = 0;
        for (int cycle = 0; cycle < 5; ++cycle) {
            if (dut.persistent_row_assignment_flat_o != kUnassigned)
                ++partial_visibility_errors;
            tick(dut);
        }
        if (!dut.commit_accepted_o || dut.persistent_row_assignment_flat_o != ownerRows())
            ++atomicity_violations;

        reset(dut);
        driveLocalTuple(dut);
        dut.selected_used_rows_flat_i = 0x0cb;
        dut.selected_donor_flat_i = 0xfff3;
        dut.selected_row_assignment_flat_i = ownerRows();
        dut.start_i = 1;
        tick(dut);
        dut.start_i = 0;
        for (int cycle = 0; cycle < 8 && !dut.commit_error_o; ++cycle)
            tick(dut);
        if (!dut.commit_error_o || dut.persistent_row_assignment_flat_o != kUnassigned)
            ++commit_error_state_corruption;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "ATOMICITY_VIOLATIONS=" << atomicity_violations << '\n'
              << "PARTIAL_COMMIT_VISIBILITY_ERRORS=" << partial_visibility_errors << '\n'
              << "COMMIT_ERROR_STATE_CORRUPTION=" << commit_error_state_corruption << '\n';
    return atomicity_violations || partial_visibility_errors || commit_error_state_corruption;
}
