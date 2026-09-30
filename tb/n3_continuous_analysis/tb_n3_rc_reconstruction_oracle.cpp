#include "Vdss_n3_rc_group_pivot_address_regs.h"
#include "verilated.h"

#include "n3_rc_analyzer_oracle.hpp"
#include "n3_rc_reconstruction_oracle.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void tick(Vdss_n3_rc_group_pivot_address_regs& dut) {
    dut.clk_i = 0; dut.eval();
    dut.clk_i = 1; dut.eval();
    dut.clk_i = 0; dut.eval();
}

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

uint32_t get_bits(const WData* words, unsigned bit, unsigned width) {
    uint32_t value = 0U;
    for (unsigned index = 0; index < width; ++index)
        value |= ((words[(bit + index) >> 5U] >> ((bit + index) & 31U)) & 1U) << index;
    return value;
}

n3_rc_reconstruction_oracle::RepairResult read_repairs(
    const Vdss_n3_rc_group_pivot_address_regs& dut) {
    n3_rc_reconstruction_oracle::RepairResult result{};
    for (unsigned sa = 0; sa < 4; ++sa)
        for (unsigned slot = 0; slot < 7; ++slot) {
            const unsigned flat = sa * 7U + slot;
            result[sa][slot].valid = ((dut.final_repair_line_valid_flat_o >> flat) & 1U) != 0U;
            result[sa][slot].is_row = ((dut.final_repair_is_row_flat_o >> flat) & 1U) != 0U;
            result[sa][slot].address = static_cast<uint16_t>(
                get_bits(dut.final_repair_address_flat_o, flat * 13U, 13U));
        }
    return result;
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    const auto masks = n3_rc_analyzer_oracle::load_fixed_masks(
        "dss_latency/N3_CONTINUOUS_ANALYSIS/N3_RC_GROUP_FIXED_MASK_EQUIVALENCE.csv");
    Vdss_n3_rc_group_pivot_address_regs dut;
    dut.rst_ni = 0; dut.capture_enable_i = 0; dut.capture_sa_i = 0;
    dut.pivot_rows_flat_i = 0; clear_wide(dut.pivot_cols_flat_i, 3U);
    dut.group_commit_valid_i = 0; dut.selected_config_flat_i = 0;
    dut.selected_pattern_flat_i = 0;
    tick(dut); tick(dut); dut.rst_ni = 1;

    n3_rc_reconstruction_oracle::RetainedState retained{};
    for (unsigned slot = 0; slot < 7; ++slot) {
        retained.pivot_rows[0][slot] = static_cast<uint16_t>(0x100U + slot);
        retained.pivot_cols[0][slot] = static_cast<uint16_t>(0x400U + slot);
        dut.pivot_rows_flat_i |= uint64_t{retained.pivot_rows[0][slot]} << (slot * 9U);
        put_bits(dut.pivot_cols_flat_i, slot * 13U, 13U,
                 retained.pivot_cols[0][slot]);
    }
    retained.pivot_cols[0][0] = 1U;
    retained.pivot_cols[0][1] = 257U;
    retained.pivot_cols[0][6] = 8191U;
    put_bits(dut.pivot_cols_flat_i, 0U, 13U, 1U);
    put_bits(dut.pivot_cols_flat_i, 13U, 13U, 257U);
    put_bits(dut.pivot_cols_flat_i, 78U, 13U, 8191U);
    dut.capture_enable_i = 1; tick(dut); dut.capture_enable_i = 0;

    retained.commit_valid[0] = true;
    retained.selected_config[0] = 0U;
    retained.selected_pattern[0] = 1U;
    dut.group_commit_valid_i = 1U;
    dut.selected_config_flat_i = 0U;
    dut.selected_pattern_flat_i = 1U;
    dut.eval();
    const auto local_expected = n3_rc_reconstruction_oracle::reconstruct(retained, masks);
    const bool alias_guard = n3_rc_reconstruction_oracle::equal(local_expected,
                                                                 read_repairs(dut)) &&
                             local_expected[0][0].address == 1U &&
                             local_expected[0][1].address == 257U;

    retained.selected_config[0] = 2U;
    retained.selected_pattern[0] = 1U;
    dut.selected_config_flat_i = 2U;
    dut.selected_pattern_flat_i = 1U;
    dut.eval();
    const auto seventh_expected = n3_rc_reconstruction_oracle::reconstruct(retained, masks);
    const bool seventh_pivot = n3_rc_reconstruction_oracle::equal(seventh_expected,
                                                                  read_repairs(dut)) &&
                               seventh_expected[0][6].valid &&
                               seventh_expected[0][6].address == 8191U;
    if (!alias_guard || !seventh_pivot) return EXIT_FAILURE;
    std::cout << "N3_RC_RECONSTRUCTION_ORACLE_PASS mismatches=0"
              << " physical_column_1_vs_257=PASS seventh_pivot=PASS"
              << " address_13bit_max=8191 pattern_id_width=6\n";
    return EXIT_SUCCESS;
}
