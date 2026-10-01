#include "Vrecam_n3_rc_fixed_mask_analyzer.h"
#include "n3_rc_analyzer_oracle.hpp"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {
using n3_rc_analyzer_oracle::AnalyzerState;

void clear_wide(WData* words, unsigned count) {
    for (unsigned index = 0; index < count; ++index) words[index] = 0U;
}

void put_bits(WData* words, unsigned bit, unsigned width, uint32_t value) {
    for (unsigned index = 0; index < width; ++index) {
        const unsigned target = bit + index;
        const WData mask = static_cast<WData>(1U) << (target & 31U);
        if ((value >> index) & 1U) words[target >> 5U] |= mask;
        else words[target >> 5U] &= ~mask;
    }
}

void drive(Vrecam_n3_rc_fixed_mask_analyzer& dut, const AnalyzerState& state,
           unsigned rows, unsigned cols, bool transpose) {
    dut.row_count_i = rows; dut.col_count_i = cols; dut.transpose_i = transpose;
    dut.pivot_valid_i = 0; dut.pivot_rows_flat_i = 0; clear_wide(dut.pivot_cols_flat_i, 3);
    dut.row_gt1_i = 0; dut.row_gt2_i = 0; dut.row_gt3_i = 0; dut.row_gt4_i = 0;
    dut.col_gt1_i = 0; dut.col_gt2_i = 0; dut.col_gt3_i = 0; dut.col_gt4_i = 0;
    dut.hybrid_valid_i = 0; dut.hybrid_pointer_flat_i = 0; dut.hybrid_descriptor_i = 0;
    clear_wide(dut.hybrid_differing_flat_i, 7); dut.conventional_overflow_i = state.overflow;
    for (unsigned index = 0; index < 7U; ++index) {
        if (state.pivot_valid[index]) dut.pivot_valid_i |= 1U << index;
        dut.pivot_rows_flat_i |= static_cast<uint64_t>(state.pivot_rows[index]) << (index * 9U);
        put_bits(dut.pivot_cols_flat_i, index * 13U, 13U, state.pivot_cols[index]);
        if (state.row_gt[0][index]) dut.row_gt1_i |= 1U << index;
        if (state.row_gt[1][index]) dut.row_gt2_i |= 1U << index;
        if (state.row_gt[2][index]) dut.row_gt3_i |= 1U << index;
        if (state.row_gt[3][index]) dut.row_gt4_i |= 1U << index;
        if (state.col_gt[0][index]) dut.col_gt1_i |= 1U << index;
        if (state.col_gt[1][index]) dut.col_gt2_i |= 1U << index;
        if (state.col_gt[2][index]) dut.col_gt3_i |= 1U << index;
        if (state.col_gt[3][index]) dut.col_gt4_i |= 1U << index;
    }
    for (unsigned index = 0; index < 17U; ++index) {
        if (state.hybrid_valid[index]) dut.hybrid_valid_i |= 1U << index;
        dut.hybrid_pointer_flat_i |= static_cast<uint64_t>(state.hybrid_pointer[index]) << (index * 3U);
        if (state.hybrid_descriptor[index]) dut.hybrid_descriptor_i |= 1U << index;
        put_bits(dut.hybrid_differing_flat_i, index * 13U, 13U, state.hybrid_differing[index]);
    }
}
}  // namespace

int main(int argc, char** argv) {
    using namespace n3_rc_analyzer_oracle;
    constexpr std::array<unsigned, 7> kRows = {3, 3, 2, 4, 3, 4, 2};
    constexpr std::array<unsigned, 7> kCols = {3, 2, 3, 3, 4, 2, 4};
    constexpr std::array<bool, 7> kTranspose = {false, false, true, false, true, false, true};
    const unsigned vectors = argc == 2 ? std::strtoul(argv[1], nullptr, 10) : 10000U;
    const FixedMaskMap masks = load_fixed_masks(
        "dss_latency/N3_CONTINUOUS_ANALYSIS/N3_RC_GROUP_FIXED_MASK_EQUIVALENCE.csv");
    uint32_t seed = 0x4e33524fU;
    std::array<unsigned, 7> capacity_counts{};
    std::array<unsigned, 36> pattern_counts{};
    unsigned feasible = 0, unrepairable = 0;
    Verilated::commandArgs(argc, argv);
    Vrecam_n3_rc_fixed_mask_analyzer dut;
    for (unsigned vector = 0; vector < vectors; ++vector) {
        const unsigned which = vector % kRows.size();
        const unsigned canonical_rows = kTranspose[which] ? kCols[which] : kRows[which];
        const unsigned canonical_cols = kTranspose[which] ? kRows[which] : kCols[which];
        const AnalyzerState state = generate_random_state(seed, canonical_rows + canonical_cols);
        const AnalyzerResult expected = evaluate(state, {kRows[which], kCols[which], kTranspose[which]}, masks);
        drive(dut, state, kRows[which], kCols[which], kTranspose[which]); dut.eval();
        if (dut.repairable_o != expected.repairable || dut.solution_valid_o != expected.repairable ||
            dut.pattern_id_o != expected.pattern) return EXIT_FAILURE;
        ++capacity_counts[which];
        if (expected.repairable) { ++feasible; ++pattern_counts[expected.pattern]; } else ++unrepairable;
    }
    std::cout << "N3_RC_ANALYZER_RANDOM_PASS vectors=" << vectors
              << " seed=0x4e33524f mismatches=0 feasible=" << feasible
              << " unrepairable=" << unrepairable << " capacities=";
    for (unsigned count : capacity_counts) std::cout << count << ',';
    std::cout << " patterns=";
    for (unsigned id = 1; id < pattern_counts.size(); ++id)
        if (pattern_counts[id] != 0U) std::cout << id << ':' << pattern_counts[id] << ',';
    std::cout << '\n';
    return (feasible == 16U && unrepairable == 9984U) ? EXIT_SUCCESS : EXIT_FAILURE;
}
