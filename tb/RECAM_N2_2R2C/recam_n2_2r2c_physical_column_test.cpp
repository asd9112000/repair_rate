#include "Vrecam_n2_2r2c_comparison_top.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {
constexpr std::array<std::uint16_t, 8> kBoundaryColumns = {0, 31, 32, 255, 256, 257, 4095, 8191};

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void put_bits(WData* words, int offset, int width, std::uint32_t value) {
    for (int bit = 0; bit < width; ++bit) {
        const int absolute_bit = offset + bit;
        const WData mask = WData{1} << (absolute_bit % 32);
        if ((value >> bit) & 1U) {
            words[absolute_bit / 32] |= mask;
        } else {
            words[absolute_bit / 32] &= ~mask;
        }
    }
}

void clear_inputs(Vrecam_n2_2r2c_comparison_top& dut) {
    dut.pivot_valid_i = 0;
    dut.pivot_rows_flat_i = 0;
    dut.pivot_cols_flat_i = 0;
    dut.row_must_i = 0;
    dut.col_must_i = 0;
    dut.hybrid_valid_i = 0;
    dut.hybrid_rows_flat_i = 0;
    for (auto& word : dut.hybrid_cols_flat_i) {
        word = 0;
    }
    dut.cam_overflow_i = 0;
}

void set_pivot(Vrecam_n2_2r2c_comparison_top& dut, int slot, std::uint16_t row, std::uint16_t column) {
    dut.pivot_rows_flat_i |= static_cast<QData>(row) << (slot * 9);
    dut.pivot_cols_flat_i |= static_cast<QData>(column) << (slot * 13);
}

void set_hybrid(Vrecam_n2_2r2c_comparison_top& dut, int slot, std::uint16_t row, std::uint16_t column) {
    dut.hybrid_rows_flat_i |= static_cast<QData>(row) << (slot * 9);
    put_bits(dut.hybrid_cols_flat_i, slot * 13, 13, column);
}

void require_equal_decision(const Vrecam_n2_2r2c_comparison_top& dut) {
    require(dut.corrected_matrix_flat_o == dut.historical_matrix_flat_o, "low-column matrix mismatch");
    require(dut.corrected_candidate_valid_o == dut.historical_candidate_valid_o, "low-column candidate vector mismatch");
    require(dut.corrected_repairable_o == dut.historical_repairable_o, "low-column repairable mismatch");
    require(dut.corrected_pattern_id_o == dut.historical_pattern_id_o, "low-column PatternID mismatch");
}

void apply_low_column_vector(Vrecam_n2_2r2c_comparison_top& dut, std::uint32_t& state) {
    auto next = [&state]() {
        state = state * 1664525U + 1013904223U;
        return state;
    };
    clear_inputs(dut);
    const int pivot_count = static_cast<int>(next() % 5U);
    dut.pivot_valid_i = pivot_count == 0 ? 0U : static_cast<std::uint8_t>((1U << pivot_count) - 1U);
    for (int slot = 0; slot < pivot_count; ++slot) {
        set_pivot(dut, slot, static_cast<std::uint16_t>(next() & 0x1ffU), static_cast<std::uint16_t>(next() & 0x1fU));
    }
    dut.row_must_i = static_cast<std::uint8_t>(next() & 0x0fU);
    dut.col_must_i = static_cast<std::uint8_t>(next() & 0x0fU);
    dut.hybrid_valid_i = static_cast<std::uint8_t>(next() & 0x7fU);
    for (int slot = 0; slot < 7; ++slot) {
        set_hybrid(dut, slot, static_cast<std::uint16_t>(next() & 0x1ffU), static_cast<std::uint16_t>(next() & 0x1fU));
    }
    dut.cam_overflow_i = next() & 1U;
}
}

int main() {
    Vrecam_n2_2r2c_comparison_top dut;

    clear_inputs(dut);
    set_pivot(dut, 0, 17, 1);
    set_pivot(dut, 1, 23, 257);
    dut.pivot_valid_i = 0x3;
    dut.hybrid_valid_i = 0x1;
    set_hybrid(dut, 0, 17, 257);
    dut.eval();
    require((dut.corrected_matrix_flat_o & 0x0002U) != 0U, "physical 1 and 257 must remain distinct");
    require((dut.historical_matrix_flat_o & 0x0002U) == 0U, "historical five-bit control must alias 1 and 257");
    std::cout << "PHYSICAL_COLUMN_ALIAS_TEST: PASS\n";

    for (const std::uint16_t boundary : kBoundaryColumns) {
        clear_inputs(dut);
        set_pivot(dut, 0, 17, 1);
        set_pivot(dut, 1, 23, boundary);
        dut.pivot_valid_i = 0x3;
        dut.hybrid_valid_i = 0x1;
        set_hybrid(dut, 0, 17, boundary);
        dut.eval();
        if (boundary != 1) {
            require((dut.corrected_matrix_flat_o & 0x0002U) != 0U, "boundary column lost physical identity");
        }
    }
    std::cout << "BOUNDARY_TEST: PASS\n";

    std::uint32_t state = 0x20260924U;
    for (int vector = 0; vector < 1000; ++vector) {
        apply_low_column_vector(dut, state);
        dut.eval();
        require_equal_decision(dut);
    }
    std::cout << "LOW_COLUMN_EQUIVALENCE: 1000 / 1000 PASS\n";
    std::cout << "LOW_COLUMN_EQUIVALENCE: 0 mismatches\n";

    clear_inputs(dut);
    dut.pivot_valid_i = 0x1;
    set_pivot(dut, 0, 12, 4095);
    dut.eval();
    require(dut.corrected_repairable_o, "all-local repair must remain repairable");
    dut.row_must_i = 0x1;
    dut.eval();
    require(dut.corrected_matrix_flat_o != 0, "row-only must condition missing");
    dut.row_must_i = 0;
    dut.col_must_i = 0x1;
    dut.eval();
    require(dut.corrected_matrix_flat_o != 0, "column-only must condition missing");
    dut.cam_overflow_i = 1;
    dut.eval();
    require(!dut.corrected_repairable_o && dut.corrected_pattern_id_o == 0, "overflow capacity failure missing");
    std::cout << "RECAM_REGRESSION: PASS\n";
}
