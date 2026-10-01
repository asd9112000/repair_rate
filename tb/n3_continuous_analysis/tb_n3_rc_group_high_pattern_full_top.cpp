#include "Vrecam_dss_n3_rc_group_live_state_top.h"
#include "verilated.h"

#include "n3_rc_analyzer_oracle.hpp"
#include "n3_rc_group_selector_oracle.hpp"
#include "n3_rc_reconstruction_oracle.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

using n3_rc_analyzer_oracle::AnalyzerState;
using n3_rc_group_selector_oracle::CandidateStore;
using n3_rc_reconstruction_oracle::RepairResult;
using n3_rc_reconstruction_oracle::RetainedState;

void tick_top(Vrecam_dss_n3_rc_group_live_state_top& dut) {
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

void drive_top_state(Vrecam_dss_n3_rc_group_live_state_top& dut,
                     const AnalyzerState& state) {
    dut.pivot_valid_i = 0; dut.pivot_rows_flat_i = 0; clear_wide(dut.pivot_cols_flat_i, 3);
    dut.row_gt1_i = dut.row_gt2_i = dut.row_gt3_i = dut.row_gt4_i = 0;
    dut.col_gt1_i = dut.col_gt2_i = dut.col_gt3_i = dut.col_gt4_i = 0;
    dut.hybrid_valid_i = 0; dut.hybrid_pointer_flat_i = 0; dut.hybrid_descriptor_i = 0;
    clear_wide(dut.hybrid_differing_flat_i, 7);
    dut.conventional_overflow_i = state.overflow;
    for (unsigned index = 0; index < 7; ++index) {
        if (state.pivot_valid[index]) dut.pivot_valid_i |= 1U << index;
        dut.pivot_rows_flat_i |= uint64_t{state.pivot_rows[index]} << (index * 9U);
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
    for (unsigned index = 0; index < 17; ++index) {
        if (state.hybrid_valid[index]) dut.hybrid_valid_i |= 1U << index;
        dut.hybrid_pointer_flat_i |= uint64_t{state.hybrid_pointer[index]} << (index * 3U);
        if (state.hybrid_descriptor[index]) dut.hybrid_descriptor_i |= 1U << index;
        put_bits(dut.hybrid_differing_flat_i, index * 13U, 13U,
                 state.hybrid_differing[index]);
    }
}

void reset_top(Vrecam_dss_n3_rc_group_live_state_top& dut) {
    dut.rst_ni = 0; dut.state_update_i = 0; dut.state_sa_i = 0;
    dut.test_done_valid_i = 0; dut.test_done_sa_i = 0;
    AnalyzerState zero{}; drive_top_state(dut, zero);
    tick_top(dut); tick_top(dut); dut.rst_ni = 1; tick_top(dut);
}

bool complete_sa(Vrecam_dss_n3_rc_group_live_state_top& dut, unsigned sa) {
    dut.state_update_i = 1; dut.state_sa_i = sa;
    dut.test_done_valid_i = 1; dut.test_done_sa_i = sa;
    tick_top(dut);
    dut.state_update_i = 0; dut.test_done_valid_i = 0;
    for (unsigned cycle = 0; cycle < 4; ++cycle) tick_top(dut);
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

RetainedState retained_from(const AnalyzerState& witness,
                            const n3_rc_group_selector_oracle::Selection& selected) {
    RetainedState retained{};
    for (unsigned sa = 0; sa < 4; ++sa) {
        retained.commit_valid[sa] = selected.repairable;
        retained.selected_config[sa] = selected.config_id[sa];
        retained.selected_pattern[sa] = selected.pattern_id[sa];
        for (unsigned slot = 0; slot < 7; ++slot) {
            retained.pivot_rows[sa][slot] = witness.pivot_rows[slot];
            retained.pivot_cols[sa][slot] = witness.pivot_cols[slot];
        }
    }
    return retained;
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    const auto masks = n3_rc_analyzer_oracle::load_fixed_masks(
        "dss_latency/N3_CONTINUOUS_ANALYSIS/N3_RC_GROUP_FIXED_MASK_EQUIVALENCE.csv");
    uint32_t seed = 0x4e33524fU;
    AnalyzerState witness{};
    n3_rc_analyzer_oracle::AnalyzerResult expected{false, 0U};
    unsigned witness_index = 0U;
    for (; witness_index < 250000U; ++witness_index) {
        AnalyzerState candidate = n3_rc_analyzer_oracle::generate_random_state(seed, 6U);
        const auto result = n3_rc_analyzer_oracle::evaluate(
            candidate, {3U, 3U, false}, masks);
        if (result.repairable && result.pattern > 15U) {
            witness = candidate;
            expected = result;
            break;
        }
    }
    if (!expected.repairable) {
        std::cerr << "no deterministic high-pattern witness\n";
        return EXIT_FAILURE;
    }

    Vrecam_dss_n3_rc_group_live_state_top dut;
    reset_top(dut); drive_top_state(dut, witness);
    for (unsigned sa = 0; sa < 4; ++sa)
        if (!complete_sa(dut, sa)) return EXIT_FAILURE;
    tick_top(dut); tick_top(dut);

    const CandidateStore store = read_store(dut);
    const auto selected = n3_rc_group_selector_oracle::select(store);
    bool trace_ok = dut.solution_ready_o && selected.repairable && dut.group_repairable_o;
    for (unsigned sa = 0; sa < 4; ++sa) {
        trace_ok = trace_ok && selected.config_id[sa] == 0U &&
                   selected.pattern_id[sa] == expected.pattern &&
                   ((dut.selected_config_flat_o >> (sa * 3U)) & 7U) == selected.config_id[sa] &&
                   ((dut.selected_pattern_flat_o >> (sa * 6U)) & 63U) == selected.pattern_id[sa];
    }

    RetainedState retained = retained_from(witness, selected);
    const RepairResult full_result =
        n3_rc_reconstruction_oracle::reconstruct(retained, masks);
    const bool reconstruction_ok =
        n3_rc_reconstruction_oracle::equal(full_result, read_repairs(dut));
    RetainedState aliased = retained;
    for (unsigned sa = 0; sa < 4; ++sa)
        aliased.selected_pattern[sa] &= 0x0fU;
    const RepairResult alias_result =
        n3_rc_reconstruction_oracle::reconstruct(aliased, masks);
    const bool alias_rejected =
        !n3_rc_reconstruction_oracle::equal(full_result, alias_result);
    if (!trace_ok || !reconstruction_ok || !alias_rejected) return EXIT_FAILURE;

    std::cout << "PATTERN_ID_GT_15_FULL_TOP_PASS witness_index=" << witness_index
              << " capacity=3R3C config=0 pattern=" << unsigned(expected.pattern)
              << " binary=0b" << ((expected.pattern >> 5U) & 1U)
              << ((expected.pattern >> 4U) & 1U)
              << ((expected.pattern >> 3U) & 1U)
              << ((expected.pattern >> 2U) & 1U)
              << ((expected.pattern >> 1U) & 1U) << (expected.pattern & 1U)
              << " analyzer=" << unsigned(expected.pattern)
              << " store_write=" << unsigned(expected.pattern)
              << " store_read=" << unsigned(expected.pattern)
              << " selector=" << unsigned(expected.pattern)
              << " registered=" << unsigned(expected.pattern)
              << " reconstruction=" << unsigned(expected.pattern)
              << " high_bits_preserved=PASS reconstruction_oracle_match=PASS"
              << " truncated_alias_behavior_rejected=PASS\n";
    return EXIT_SUCCESS;
}
