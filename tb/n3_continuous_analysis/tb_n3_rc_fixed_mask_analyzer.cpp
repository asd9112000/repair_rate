#include "Vrecam_n3_rc_fixed_mask_analyzer.h"
#include "verilated.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void clear_wide(WData* words, unsigned word_count) {
    for (unsigned index = 0; index < word_count; ++index) words[index] = 0U;
}

void set_wide_bits(WData* words, unsigned bit, unsigned width, uint32_t value) {
    for (unsigned index = 0; index < width; ++index) {
        const unsigned target = bit + index;
        const WData mask = static_cast<WData>(1U) << (target & 31U);
        if ((value >> index) & 1U) words[target >> 5U] |= mask;
        else words[target >> 5U] &= ~mask;
    }
}

void clear_inputs(Vrecam_n3_rc_fixed_mask_analyzer& dut) {
    dut.row_count_i = 0;
    dut.col_count_i = 0;
    dut.transpose_i = 0;
    dut.pivot_valid_i = 0;
    dut.pivot_rows_flat_i = 0;
    clear_wide(dut.pivot_cols_flat_i, 3);
    dut.row_gt1_i = 0; dut.row_gt2_i = 0; dut.row_gt3_i = 0; dut.row_gt4_i = 0;
    dut.col_gt1_i = 0; dut.col_gt2_i = 0; dut.col_gt3_i = 0; dut.col_gt4_i = 0;
    dut.hybrid_valid_i = 0;
    dut.hybrid_pointer_flat_i = 0;
    dut.hybrid_descriptor_i = 0;
    clear_wide(dut.hybrid_differing_flat_i, 7);
    dut.conventional_overflow_i = 0;
}

bool expect_first(Vrecam_n3_rc_fixed_mask_analyzer& dut, unsigned rows,
                  unsigned cols, bool transpose, unsigned& cases) {
    clear_inputs(dut);
    dut.row_count_i = rows;
    dut.col_count_i = cols;
    dut.transpose_i = transpose;
    dut.pivot_valid_i = (1U << (rows + cols)) - 1U;
    dut.eval();
    ++cases;
    if (!dut.repairable_o || !dut.solution_valid_o || dut.pattern_id_o != 1U) {
        std::cerr << "first-pattern failure rows=" << rows << " cols=" << cols
                  << " transpose=" << transpose << " id=" << unsigned(dut.pattern_id_o) << '\n';
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vrecam_n3_rc_fixed_mask_analyzer dut;
    unsigned cases = 0;

    if (!expect_first(dut, 3, 3, false, cases) || !expect_first(dut, 3, 2, false, cases) ||
        !expect_first(dut, 2, 3, true, cases) || !expect_first(dut, 4, 3, false, cases) ||
        !expect_first(dut, 3, 4, true, cases) || !expect_first(dut, 4, 2, false, cases) ||
        !expect_first(dut, 2, 4, true, cases)) return EXIT_FAILURE;

    clear_inputs(dut);
    dut.row_count_i = 3; dut.col_count_i = 3; dut.pivot_valid_i = 0x3fU;
    dut.conventional_overflow_i = 1;
    dut.eval();
    ++cases;
    if (dut.repairable_o || dut.solution_valid_o) {
        std::cerr << "overflow did not suppress candidates\n";
        return EXIT_FAILURE;
    }

    clear_inputs(dut);
    dut.row_count_i = 3; dut.col_count_i = 3; dut.pivot_valid_i = 0x3fU;
    set_wide_bits(dut.pivot_cols_flat_i, 0U, 13U, 1U);
    set_wide_bits(dut.pivot_cols_flat_i, 4U * 13U, 13U, 257U);
    dut.hybrid_valid_i = 1U;
    dut.hybrid_pointer_flat_i = 3U;
    set_wide_bits(dut.hybrid_differing_flat_i, 0U, 13U, 257U);
    dut.eval();
    if (!dut.repairable_o || dut.pattern_id_o != 1U) {
        std::cerr << "full-width 257 path lost distinction id=" << unsigned(dut.pattern_id_o) << '\n';
        return EXIT_FAILURE;
    }
    set_wide_bits(dut.hybrid_differing_flat_i, 0U, 13U, 1U);
    dut.eval();
    ++cases;
    if (!dut.repairable_o || dut.pattern_id_o == 1U) {
        std::cerr << "physical-column alias did not change decision id=" << unsigned(dut.pattern_id_o) << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "N3_RC_FIXED_MASK_ANALYZER_PASS cases=" << cases
              << " mismatches=0 physical_column_alias=PASS\n";
    return EXIT_SUCCESS;
}
