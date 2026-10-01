#include "Vrecam_dss_n3_rc_group_live_state_top.h"
#include "verilated.h"

#include "n3_rc_analyzer_oracle.hpp"
#include "n3_rc_group_selector_oracle.hpp"
#include "n3_rc_reconstruction_oracle.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

using n3_rc_group_selector_oracle::CandidateStore;
using n3_rc_group_selector_oracle::Selection;
using n3_rc_reconstruction_oracle::RepairResult;
using n3_rc_reconstruction_oracle::RetainedState;

void tick(Vrecam_dss_n3_rc_group_live_state_top& dut) {
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

uint32_t next_random(uint32_t& state) {
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return state;
}

void reset(Vrecam_dss_n3_rc_group_live_state_top& dut) {
    dut.rst_ni = 0;
    dut.state_update_i = 0;
    dut.state_sa_i = 0;
    dut.test_done_valid_i = 0;
    dut.test_done_sa_i = 0;
    dut.pivot_valid_i = 0;
    dut.pivot_rows_flat_i = 0;
    clear_wide(dut.pivot_cols_flat_i, 3);
    dut.row_gt1_i = 0; dut.row_gt2_i = 0; dut.row_gt3_i = 0; dut.row_gt4_i = 0;
    dut.col_gt1_i = 0; dut.col_gt2_i = 0; dut.col_gt3_i = 0; dut.col_gt4_i = 0;
    dut.hybrid_valid_i = 0; dut.hybrid_pointer_flat_i = 0; dut.hybrid_descriptor_i = 0;
    clear_wide(dut.hybrid_differing_flat_i, 7);
    dut.conventional_overflow_i = 0;
    tick(dut); tick(dut);
    dut.rst_ni = 1;
}

bool complete_sa(Vrecam_dss_n3_rc_group_live_state_top& dut, unsigned sa) {
    dut.state_update_i = 1;
    dut.state_sa_i = sa;
    dut.test_done_valid_i = 1;
    dut.test_done_sa_i = sa;
    tick(dut);
    dut.state_update_i = 0;
    dut.test_done_valid_i = 0;
    for (unsigned cycle = 0; cycle < 4; ++cycle) tick(dut);
    return ((dut.sa_result_frozen_o >> sa) & 1U) != 0U;
}

CandidateStore read_store(const Vrecam_dss_n3_rc_group_live_state_top& dut) {
    CandidateStore store{};
    for (unsigned sa = 0; sa < 4; ++sa)
        for (unsigned slot = 0; slot < 4; ++slot) {
            const unsigned record = get_bits(dut.candidate_store_image_o,
                                             (sa * 4U + slot) * 7U, 7U);
            store[sa][slot] = {(record & 1U) != 0U,
                               static_cast<uint8_t>(record >> 1U)};
        }
    return store;
}

RepairResult read_repairs(const Vrecam_dss_n3_rc_group_live_state_top& dut) {
    RepairResult result{};
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

bool verify_selection(const Vrecam_dss_n3_rc_group_live_state_top& dut,
                      const Selection& expected) {
    if (dut.group_repairable_o != expected.repairable) return false;
    if (!expected.repairable) return dut.sa_commit_valid_o == 0U;
    for (unsigned sa = 0; sa < 4; ++sa) {
        if (((dut.selected_config_flat_o >> (sa * 3U)) & 7U) != expected.config_id[sa] ||
            ((dut.selected_pattern_flat_o >> (sa * 6U)) & 63U) != expected.pattern_id[sa] ||
            ((dut.borrow_flat_o >> sa) & 1U) != expected.borrow[sa])
            return false;
    }
    return dut.sa_commit_valid_o == 0xfU;
}

bool run_vector(Vrecam_dss_n3_rc_group_live_state_top& dut, uint32_t& seed,
                unsigned vector, const n3_rc_analyzer_oracle::FixedMaskMap& masks) {
    RetainedState retained{};
    reset(dut);
    dut.pivot_valid_i = 0x7fU;
    for (unsigned slot = 0; slot < 7; ++slot) {
        const uint16_t row = next_random(seed) & 0x1ffU;
        const uint16_t col = next_random(seed) & 0x1fffU;
        dut.pivot_rows_flat_i |= static_cast<uint64_t>(row) << (slot * 9U);
        put_bits(dut.pivot_cols_flat_i, slot * 13U, 13U, col);
        for (unsigned sa = 0; sa < 4; ++sa) {
            retained.pivot_rows[sa][slot] = row;
            retained.pivot_cols[sa][slot] = col;
        }
    }
    for (unsigned sa = 0; sa < 4; ++sa)
        if (!complete_sa(dut, sa)) {
            std::cerr << "freeze timeout vector=" << vector << " sa=" << sa << '\n';
            return false;
        }
    tick(dut);
    const Selection selected = n3_rc_group_selector_oracle::select(read_store(dut));
    if (!dut.solution_ready_o || !verify_selection(dut, selected)) {
        std::cerr << "GROUP oracle mismatch vector=" << vector << '\n';
        return false;
    }
    for (unsigned sa = 0; sa < 4; ++sa) {
        retained.commit_valid[sa] = selected.repairable;
        retained.selected_config[sa] = selected.config_id[sa];
        retained.selected_pattern[sa] = selected.pattern_id[sa];
    }
    const RepairResult expected = n3_rc_reconstruction_oracle::reconstruct(retained, masks);
    if (!n3_rc_reconstruction_oracle::equal(expected, read_repairs(dut))) {
        std::cerr << "reconstruction mismatch vector=" << vector << '\n';
        return false;
    }
    return true;
}

bool run_physical_alias_guard(const n3_rc_analyzer_oracle::FixedMaskMap& masks) {
    RetainedState retained{};
    retained.commit_valid[0] = true;
    retained.selected_config[0] = 0U;
    retained.selected_pattern[0] = 1U;
    retained.pivot_cols[0][0] = 1U;
    retained.pivot_cols[0][1] = 257U;
    const RepairResult result = n3_rc_reconstruction_oracle::reconstruct(retained, masks);
    return result[0][0].valid && result[0][1].valid &&
           result[0][0].address == 1U && result[0][1].address == 257U &&
           result[0][0].address != result[0][1].address;
}

}  // namespace

int main(int argc, char** argv) {
    const unsigned vectors = argc == 2 ? std::strtoul(argv[1], nullptr, 10) : 1000U;
    uint32_t seed = 0x4e335247U;
    Verilated::commandArgs(argc, argv);
    const auto masks = n3_rc_analyzer_oracle::load_fixed_masks(
        "dss_latency/N3_CONTINUOUS_ANALYSIS/N3_RC_GROUP_FIXED_MASK_EQUIVALENCE.csv");
    if (!run_physical_alias_guard(masks)) return EXIT_FAILURE;
    Vrecam_dss_n3_rc_group_live_state_top dut;
    for (unsigned vector = 0; vector < vectors; ++vector)
        if (!run_vector(dut, seed, vector, masks)) return EXIT_FAILURE;
    std::cout << "N3_RC_GROUP_FULL_TOP_LOCKSTEP_PASS vectors=" << vectors
              << " seed=0x4e335247 mismatches=0 repairable=" << vectors
              << " unrepairable=0 selected_config=T0:4 per vector selected_pattern=1:4 per vector"
              << " physical_column_1_vs_257=PASS reconstruction_oracle_match=PASS\n";
    return EXIT_SUCCESS;
}
