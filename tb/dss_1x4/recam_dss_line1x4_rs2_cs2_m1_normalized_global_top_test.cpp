#if defined(GLOBAL_OPT1_CLASSCOLLAPSED_TEST)
#include "Vrecam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_top.h"
#else
#include "Vrecam_dss_line1x4_rs2_cs2_m1_normalized_global_top.h"
#endif
#include "verilated.h"

#include <iostream>
#include <stdexcept>

namespace {
#if defined(GLOBAL_OPT1_CLASSCOLLAPSED_TEST)
using Dut = Vrecam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_top;
#else
using Dut = Vrecam_dss_line1x4_rs2_cs2_m1_normalized_global_top;
#endif

void tick(Dut &dut) {
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
    dut.clk_i = 0;
    dut.eval();
}

void clear(Dut &dut) {
    dut.pivot_valid_flat_i = 0;
    for (int index = 0; index < 7; ++index)
        dut.pivot_rows_flat_i[index] = 0;
    for (int index = 0; index < 4; ++index)
        dut.pivot_cols_flat_i[index] = 0;
    dut.row_gt1_flat_i = 0;
    dut.row_gt2_flat_i = 0;
    dut.row_gt3_flat_i = 0;
    dut.row_gt4_flat_i = 0;
    dut.col_gt1_flat_i = 0;
    dut.col_gt2_flat_i = 0;
    dut.col_gt3_flat_i = 0;
    dut.col_gt4_flat_i = 0;
    dut.hybrid_valid_flat_i = 0;
    dut.hybrid_descriptor_flat_i = 0;
    for (int index = 0; index < 4; ++index)
        dut.hybrid_pointer_flat_i[index] = 0;
    for (int index = 0; index < 12; ++index)
        dut.hybrid_differing_flat_i[index] = 0;
    dut.conventional_overflow_i = 0;
}

void reset(Dut &dut) {
    clear(dut);
    dut.rst_ni = 0;
    dut.start_i = 0;
    tick(dut);
    dut.rst_ni = 1;
}

int run(Dut &dut) {
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
    for (int cycle = 0; cycle < 2000000; ++cycle) {
        tick(dut);
        if (dut.done_o)
            return cycle + 1;
    }
    throw std::runtime_error("GLOBAL integrated top timed out");
}

bool bit(const WData *data, int index) {
    return (data[index / 32] >> (index % 32)) & 1U;
}

int candidateCount(const WData *data) {
    int total = 0;
    for (int index = 0; index < 180; ++index)
        total += bit(data, index);
    return total;
}
} // namespace

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Dut dut;
    unsigned candidate_table_mismatches = 0;
    unsigned end_to_end_mismatches = 0;
    unsigned atomic_visibility_errors = 0;
    int success_cycles = 0;
    int failure_cycles = 0;
    try {
        reset(dut);
        success_cycles = run(dut);
        if (!dut.group_repairable_o || dut.selected_valid_o != 15 ||
            dut.selected_attempt_flat_o != 0 || dut.selected_pattern_flat_o != 0x1111 ||
            dut.selected_used_rows_flat_o != 0 || dut.selected_used_cols_flat_o != 0 ||
            dut.row_assignment_flat_o != 0x924924U || candidateCount(dut.candidate_valid_debug_o) != 94)
            ++end_to_end_mismatches;

        reset(dut);
        dut.conventional_overflow_i = 4;
        failure_cycles = run(dut);
        if (dut.group_repairable_o || dut.selected_valid_o != 0 ||
            dut.row_assignment_flat_o != 0x924924U || candidateCount(dut.candidate_valid_debug_o) != 63)
            ++end_to_end_mismatches;

        reset(dut);
        dut.conventional_overflow_i = 8;
        dut.start_i = 1;
        tick(dut);
        dut.start_i = 0;
        for (int cycle = 0; cycle < 2000000 && !dut.done_o; ++cycle) {
            if (dut.row_assignment_flat_o != 0x924924U)
                ++atomic_visibility_errors;
            tick(dut);
        }
        if (!dut.done_o || dut.group_repairable_o)
            ++candidate_table_mismatches;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "CANDIDATE_TABLE_MISMATCHES=" << candidate_table_mismatches << '\n'
              << "END_TO_END_MISMATCHES=" << end_to_end_mismatches << '\n'
              << "PARTIAL_COMMIT_VISIBILITY_ERRORS=" << atomic_visibility_errors << '\n'
              << "INTEGRATED_GLOBAL_SUCCESS_CYCLES=" << success_cycles << '\n'
              << "INTEGRATED_GLOBAL_FAILURE_CYCLES=" << failure_cycles << '\n';
    return candidate_table_mismatches || end_to_end_mismatches || atomic_visibility_errors;
}
