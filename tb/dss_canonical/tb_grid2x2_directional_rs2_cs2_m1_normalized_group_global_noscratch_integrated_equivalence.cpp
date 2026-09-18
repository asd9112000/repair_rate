#include "Vtb_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_integrated_equivalence.h"
#include "verilated.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
constexpr int kCandidateCount = 160;
using Dut = Vtb_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_integrated_equivalence;

void clearWide(WData *signal, int words)
{
    for (int word = 0; word < words; ++word)
        signal[word] = 0;
}

void setWideBit(WData *signal, int bit)
{
    signal[bit / 32] |= 1U << (bit % 32);
}

bool readWideBit(const WData *signal, int bit)
{
    return (signal[bit / 32] & (1U << (bit % 32))) != 0U;
}

void tick(Dut &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

void clearSnapshot(Dut &dut)
{
    dut.pivot_valid_flat_i = 0;
    clearWide(dut.pivot_rows_flat_i, 6);
    clearWide(dut.pivot_cols_flat_i, 4);
    dut.row_gt1_flat_i = 0;
    dut.row_gt2_flat_i = 0;
    dut.row_gt3_flat_i = 0;
    dut.col_gt1_flat_i = 0;
    dut.col_gt2_flat_i = 0;
    dut.col_gt3_flat_i = 0;
    dut.hybrid_valid_flat_i = 0;
    clearWide(dut.hybrid_pointer_flat_i, 3);
    dut.hybrid_descriptor_flat_i = 0;
    clearWide(dut.hybrid_differing_flat_i, 8);
    dut.conventional_overflow_i = 0;
}

void reset(Dut &dut)
{
    clearSnapshot(dut);
    dut.rst_ni = 0;
    dut.start_i = 0;
    tick(dut);
    dut.rst_ni = 1;
}

int startAndWait(Dut &dut, std::size_t &partial_visibility_errors)
{
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
    bool opt0_done = false;
    bool opt1_done = false;
    for (int cycle = 0; cycle < 500000; ++cycle) {
        if ((dut.opt0_released_o != 0U && dut.opt0_released_o != 0x0fU) || dut.opt0_borrowed_o != 0U ||
            (dut.opt1_released_o != 0U && dut.opt1_released_o != 0x0fU) || dut.opt1_borrowed_o != 0U)
            ++partial_visibility_errors;
        tick(dut);
        opt0_done = opt0_done || dut.opt0_done_o;
        opt1_done = opt1_done || dut.opt1_done_o;
        if (opt0_done && opt1_done)
            return cycle + 1;
    }
    int opt0_candidates = 0;
    int opt1_candidates = 0;
    int opt0_sa_candidates[4] = {0, 0, 0, 0};
    for (int bit = 0; bit < kCandidateCount; ++bit) {
        opt0_candidates += readWideBit(dut.opt0_candidate_valid_o, bit);
        opt1_candidates += readWideBit(dut.opt1_candidate_valid_o, bit);
        opt0_sa_candidates[bit / 40] += readWideBit(dut.opt0_candidate_valid_o, bit);
    }
    throw std::runtime_error("integrated GLOBAL top timed out: opt0_candidates=" +
        std::to_string(opt0_candidates) + " opt1_candidates=" + std::to_string(opt1_candidates) +
        " opt0_sa=" + std::to_string(opt0_sa_candidates[0]) + "," + std::to_string(opt0_sa_candidates[1]) + "," +
        std::to_string(opt0_sa_candidates[2]) + "," + std::to_string(opt0_sa_candidates[3]) +
        " opt0_busy=" + std::to_string(dut.opt0_busy_o) + " opt1_busy=" + std::to_string(dut.opt1_busy_o));
}

bool expectedValid(int subarray, int action, int pattern, int overflow_sa)
{
    if (subarray == overflow_sa)
        return false;
    const int valid_count[4] = {6, 3, 10, 4};
    return pattern < valid_count[action];
}

std::size_t compareMaps(const Dut &dut, int overflow_sa, std::size_t &effect_mismatches)
{
    std::size_t valid_mismatches = 0;
    for (int sa = 0; sa < 4; ++sa) {
        for (int action = 0; action < 4; ++action) {
            for (int pattern = 0; pattern < 10; ++pattern) {
                const int index = sa * 40 + action * 10 + pattern;
                const bool valid = expectedValid(sa, action, pattern, overflow_sa);
                const bool release = valid && (action == 1 || action == 3);
                const bool borrow = valid && (action == 2 || action == 3);
                const bool opt0_valid = readWideBit(dut.opt0_candidate_valid_o, index);
                const bool opt1_valid = readWideBit(dut.opt1_candidate_valid_o, index);
                const bool opt0_release = readWideBit(dut.opt0_candidate_release_o, index);
                const bool opt1_release = readWideBit(dut.opt1_candidate_release_o, index);
                const bool opt0_borrow = readWideBit(dut.opt0_candidate_borrow_o, index);
                const bool opt1_borrow = readWideBit(dut.opt1_candidate_borrow_o, index);
                if (opt0_valid != valid || opt1_valid != valid || opt0_valid != opt1_valid)
                    ++valid_mismatches;
                if (opt0_release != release || opt1_release != release || opt0_release != opt1_release)
                    ++effect_mismatches;
                if (opt0_borrow != borrow || opt1_borrow != borrow || opt0_borrow != opt1_borrow)
                    ++effect_mismatches;
            }
        }
    }
    return valid_mismatches;
}

bool sameSelected(const Dut &dut)
{
    return dut.opt0_repairable_o == dut.opt1_repairable_o &&
        dut.opt0_commit_error_o == dut.opt1_commit_error_o &&
        dut.opt0_selected_valid_o == dut.opt1_selected_valid_o &&
        dut.opt0_action_o == dut.opt1_action_o && dut.opt0_config_o == dut.opt1_config_o &&
        dut.opt0_pattern_o == dut.opt1_pattern_o && dut.opt0_donor_o == dut.opt1_donor_o &&
        dut.opt0_release_o == dut.opt1_release_o && dut.opt0_borrow_o == dut.opt1_borrow_o &&
        dut.opt0_released_o == dut.opt1_released_o && dut.opt0_borrowed_o == dut.opt1_borrowed_o &&
        dut.opt0_borrower_id_o == dut.opt1_borrower_id_o;
}
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Dut dut;
    std::size_t snapshot_alias_errors = 0;
    std::size_t config_mapping_mismatches = 0;
    std::size_t candidate_valid_mismatches = 0;
    std::size_t candidate_effect_mismatches = 0;
    std::size_t end_to_end_mismatches = 0;
    std::size_t partial_visibility_errors = 0;
    int directed_success_cycles = 0;
    int directed_failure_cycles = 0;

    try {
        reset(dut);
        directed_success_cycles = startAndWait(dut, partial_visibility_errors);
        candidate_valid_mismatches += compareMaps(dut, -1, candidate_effect_mismatches);
        if (!sameSelected(dut) || !dut.opt0_repairable_o || dut.opt0_commit_error_o ||
            dut.opt0_selected_valid_o != 0x0fU || dut.opt0_action_o != 0x55U ||
            dut.opt0_config_o != 0x84cU || dut.opt0_pattern_o != 0x1111U ||
            dut.opt0_donor_o != 0xc6U || dut.opt0_release_o != 0x0fU ||
            dut.opt0_borrow_o != 0U || dut.opt0_released_o != 0x0fU ||
            dut.opt0_borrowed_o != 0U)
            ++end_to_end_mismatches;
        if (dut.opt0_config_o != 0x84cU || dut.opt1_config_o != 0x84cU)
            ++config_mapping_mismatches;

        reset(dut);
        dut.conventional_overflow_i = 0x4U;
        directed_failure_cycles = startAndWait(dut, partial_visibility_errors);
        candidate_valid_mismatches += compareMaps(dut, 2, candidate_effect_mismatches);
        if (!sameSelected(dut) || dut.opt0_repairable_o || dut.opt0_commit_error_o ||
            dut.opt0_released_o != 0U || dut.opt0_borrowed_o != 0U)
            ++end_to_end_mismatches;
        for (int action = 0; action < 4; ++action)
            for (int pattern = 0; pattern < 10; ++pattern)
                if (readWideBit(dut.opt0_candidate_valid_o, 2 * 40 + action * 10 + pattern))
                    ++snapshot_alias_errors;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    std::cout << "GROUP_SNAPSHOT_ALIAS_ERRORS=" << snapshot_alias_errors << '\n'
              << "CONFIG_ACTION_MAPPING_MISMATCHES=" << config_mapping_mismatches << '\n'
              << "CANDIDATE_VALID_MISMATCHES=" << candidate_valid_mismatches << '\n'
              << "CANDIDATE_EFFECT_MISMATCHES=" << candidate_effect_mismatches << '\n'
              << "CANDIDATE_MAP_MISMATCHES=" << (candidate_valid_mismatches + candidate_effect_mismatches) << '\n'
              << "TRANSITION_ADAPTER_MISMATCHES=" << candidate_effect_mismatches << '\n'
              << "OPT0_REFERENCE_MISMATCHES=" << end_to_end_mismatches << '\n'
              << "OPT1_REFERENCE_MISMATCHES=" << end_to_end_mismatches << '\n'
              << "OPT0_OPT1_SEMANTIC_MISMATCHES=" << end_to_end_mismatches << '\n'
              << "END_TO_END_MISMATCHES=" << end_to_end_mismatches << '\n'
              << "PARTIAL_COMMIT_VISIBILITY_ERRORS=" << partial_visibility_errors << '\n'
              << "INTEGRATED_DIRECTED_SUCCESS_CYCLES=" << directed_success_cycles << '\n'
              << "INTEGRATED_DIRECTED_FAILURE_CYCLES=" << directed_failure_cycles << '\n';
    return snapshot_alias_errors == 0 && config_mapping_mismatches == 0 &&
        candidate_valid_mismatches == 0 && candidate_effect_mismatches == 0 &&
        end_to_end_mismatches == 0 && partial_visibility_errors == 0 ? 0 : 1;
}
