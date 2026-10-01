#include "Vrecam_n3_rc_fixed_mask_analyzer.h"
#include "n3_rc_analyzer_oracle.hpp"
#include "verilated.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {
using n3_rc_analyzer_oracle::AnalyzerResult;
using n3_rc_analyzer_oracle::AnalyzerState;

void put(WData* words, unsigned bit, unsigned width, uint32_t value) {
    for (unsigned n = 0; n < width; ++n)
        if ((value >> n) & 1U) words[(bit + n) >> 5U] |= WData{1U} << ((bit + n) & 31U);
}

void drive(Vrecam_n3_rc_fixed_mask_analyzer& dut, const AnalyzerState& s) {
    dut.row_count_i = 4U; dut.col_count_i = 3U; dut.transpose_i = 0;
    dut.pivot_valid_i = 0; dut.pivot_rows_flat_i = 0;
    for (unsigned n = 0; n < 3; ++n) dut.pivot_cols_flat_i[n] = 0;
    dut.row_gt1_i = dut.row_gt2_i = dut.row_gt3_i = dut.row_gt4_i = 0;
    dut.col_gt1_i = dut.col_gt2_i = dut.col_gt3_i = dut.col_gt4_i = 0;
    dut.hybrid_valid_i = 0; dut.hybrid_pointer_flat_i = 0; dut.hybrid_descriptor_i = 0;
    for (unsigned n = 0; n < 7; ++n) dut.hybrid_differing_flat_i[n] = 0;
    dut.conventional_overflow_i = s.overflow;
    for (unsigned n = 0; n < 7; ++n) {
        if (s.pivot_valid[n]) dut.pivot_valid_i |= 1U << n;
        dut.pivot_rows_flat_i |= uint64_t{s.pivot_rows[n]} << (n * 9U);
        put(dut.pivot_cols_flat_i, n * 13U, 13U, s.pivot_cols[n]);
        if (s.row_gt[0][n]) dut.row_gt1_i |= 1U << n;
        if (s.row_gt[1][n]) dut.row_gt2_i |= 1U << n;
        if (s.row_gt[2][n]) dut.row_gt3_i |= 1U << n;
        if (s.row_gt[3][n]) dut.row_gt4_i |= 1U << n;
        if (s.col_gt[0][n]) dut.col_gt1_i |= 1U << n;
        if (s.col_gt[1][n]) dut.col_gt2_i |= 1U << n;
        if (s.col_gt[2][n]) dut.col_gt3_i |= 1U << n;
        if (s.col_gt[3][n]) dut.col_gt4_i |= 1U << n;
    }
    for (unsigned n = 0; n < 17; ++n) {
        if (s.hybrid_valid[n]) dut.hybrid_valid_i |= 1U << n;
        dut.hybrid_pointer_flat_i |= uint64_t{s.hybrid_pointer[n]} << (n * 3U);
        if (s.hybrid_descriptor[n]) dut.hybrid_descriptor_i |= 1U << n;
        put(dut.hybrid_differing_flat_i, n * 13U, 13U, s.hybrid_differing[n]);
    }
    dut.eval();
}

AnalyzerState distinction_state(uint32_t& seed) {
    using n3_rc_analyzer_oracle::next_random;
    AnalyzerState s;
    for (unsigned n = 0; n < 7; ++n) {
        s.pivot_valid[n] = true;
        s.pivot_rows[n] = next_random(seed) & 0x1ffU;
        s.pivot_cols[n] = next_random(seed) & 0x1fffU;
        for (unsigned level = 0; level < 4; ++level) {
            s.row_gt[level][n] = (next_random(seed) & 7U) == 0U;
            s.col_gt[level][n] = (next_random(seed) & 7U) == 0U;
        }
    }
    for (unsigned n = 0; n < 17; ++n) {
        s.hybrid_valid[n] = (next_random(seed) & 3U) == 0U;
        s.hybrid_pointer[n] = next_random(seed) % 7U;
        s.hybrid_descriptor[n] = (next_random(seed) & 1U) != 0U;
        s.hybrid_differing[n] = next_random(seed) & 0x1fffU;
    }
    return s;
}

AnalyzerResult rtl_result(Vrecam_n3_rc_fixed_mask_analyzer& dut, const AnalyzerState& s) {
    drive(dut, s); return {bool(dut.repairable_o), uint8_t(dut.pattern_id_o)};
}
bool differs(AnalyzerResult a, AnalyzerResult b) {
    return a.repairable != b.repairable || a.pattern != b.pattern;
}
}  // namespace

int main(int argc, char** argv) {
    using namespace n3_rc_analyzer_oracle;
    Verilated::commandArgs(argc, argv);
    Vrecam_n3_rc_fixed_mask_analyzer dut;
    const FixedMaskMap masks = load_fixed_masks(
        "dss_latency/N3_CONTINUOUS_ANALYSIS/N3_RC_GROUP_FIXED_MASK_EQUIVALENCE.csv");
    uint32_t seed = 0x4e334449U;
    bool seventh = false, hybrid_nine = false, high_pattern = false;
    for (unsigned attempt = 0; attempt < 250000U; ++attempt) {
        const AnalyzerState state = distinction_state(seed);
        const AnalyzerResult full = rtl_result(dut, state);
        const AnalyzerResult reference = evaluate(state, {4U, 3U, false}, masks);
        if (differs(full, reference)) return EXIT_FAILURE;
        if (full.repairable && full.pattern > 15U) high_pattern = true;
        if (!seventh) { AnalyzerState without = state; without.pivot_valid[6] = false;
            seventh = differs(full, rtl_result(dut, without)); }
        if (!hybrid_nine) { AnalyzerState without = state; without.hybrid_valid[8] = false;
            hybrid_nine = state.hybrid_valid[8] && differs(full, rtl_result(dut, without)); }
        if (seventh && hybrid_nine && high_pattern) break;
    }
    std::cout << "SEVENTH_PIVOT_DISTINCTION:" << (seventh ? "PASS" : "FAIL") << '\n'
              << "HYBRID_GT_7_DISTINCTION:" << (hybrid_nine ? "PASS" : "FAIL") << '\n'
              << "PATTERN_ID_GT_15_ANALYZER_WITNESS:" << (high_pattern ? "PASS" : "FAIL") << '\n';
    return (seventh && hybrid_nine && high_pattern) ? EXIT_SUCCESS : EXIT_FAILURE;
}
