#include "Vrecam_dss_n3_rc_group_live_state_top.h"
#include "verilated.h"

#include "n3_rc_analyzer_oracle.hpp"
#include "n3_rc_group_selector_oracle.hpp"
#include "n3_rc_group_vector_generator.hpp"
#include "n3_rc_reconstruction_oracle.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {

using n3_rc_group_vector_generator::GeneratedCase;
using n3_rc_group_vector_generator::GenerationMode;
using n3_rc_reconstruction_oracle::RepairResult;

struct Options {
    uint32_t seed = 0x4e33524fU;
    unsigned cases = 100U;
    bool fixed_mode = false;
    bool formal = false;
    GenerationMode mode = GenerationMode::General;
    std::string case_csv;
};

struct EventMarkers {
    uint64_t state_update_cycle = 0U;
    std::array<uint64_t, 4> config_result_cycle{};
    uint64_t registered_solution_ready_cycle = 0U;
};

struct CaseResult {
    bool passed = false;
    unsigned latency_cycles = 0U;
    EventMarkers markers{};
    unsigned stale_generation_mix = 0U;
    unsigned earlier_sa_replay = 0U;
    unsigned partial_scan_finalization = 0U;
    unsigned old_candidate_accepted = 0U;
};

Options parse_options(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--seed" && index + 1 < argc) {
            options.seed = static_cast<uint32_t>(
                std::stoul(argv[++index], nullptr, 0));
        } else if (argument == "--cases" && index + 1 < argc) {
            options.cases = static_cast<unsigned>(
                std::stoul(argv[++index], nullptr, 0));
        } else if (argument == "--mode" && index + 1 < argc) {
            options.mode =
                n3_rc_group_vector_generator::parse_mode(argv[++index]);
            options.fixed_mode = true;
        } else if (argument == "--formal") {
            options.formal = true;
        } else if (argument == "--case-csv" && index + 1 < argc) {
            options.case_csv = argv[++index];
        } else {
            throw std::runtime_error(
                "usage: tb_n3_rc_group_latency [--seed value] [--cases count] "
                "[--mode GENERAL|REPAIRABLE|UNREPAIRABLE|HIGH_PATTERN|"
                "SEVENTH_PIVOT|HYBRID_GT_7|PHYSICAL_COLUMN_WIDE] "
                "[--formal --case-csv path]");
        }
    }
    if (options.cases == 0U)
        throw std::runtime_error("--cases must be greater than zero");
    if (options.formal &&
        (!options.fixed_mode || options.mode != GenerationMode::General ||
         options.case_csv.empty()))
        throw std::runtime_error(
            "--formal requires --mode GENERAL and --case-csv path");
    return options;
}

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
        value |= ((words[(bit + index) >> 5U] >>
                   ((bit + index) & 31U)) & 1U) << index;
    return value;
}

void drive_state(Vrecam_dss_n3_rc_group_live_state_top& dut,
                 const n3_rc_analyzer_oracle::AnalyzerState& state) {
    dut.pivot_valid_i = 0; dut.pivot_rows_flat_i = 0;
    clear_wide(dut.pivot_cols_flat_i, 3);
    dut.row_gt1_i = dut.row_gt2_i = dut.row_gt3_i = dut.row_gt4_i = 0;
    dut.col_gt1_i = dut.col_gt2_i = dut.col_gt3_i = dut.col_gt4_i = 0;
    dut.hybrid_valid_i = 0; dut.hybrid_pointer_flat_i = 0;
    dut.hybrid_descriptor_i = 0;
    clear_wide(dut.hybrid_differing_flat_i, 7);
    dut.conventional_overflow_i = state.overflow;
    for (unsigned index = 0; index < 7; ++index) {
        if (state.pivot_valid[index]) dut.pivot_valid_i |= 1U << index;
        dut.pivot_rows_flat_i |=
            uint64_t{state.pivot_rows[index]} << (index * 9U);
        put_bits(dut.pivot_cols_flat_i, index * 13U, 13U,
                 state.pivot_cols[index]);
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
        dut.hybrid_pointer_flat_i |=
            uint64_t{state.hybrid_pointer[index]} << (index * 3U);
        if (state.hybrid_descriptor[index])
            dut.hybrid_descriptor_i |= 1U << index;
        put_bits(dut.hybrid_differing_flat_i, index * 13U, 13U,
                 state.hybrid_differing[index]);
    }
}

void reset(Vrecam_dss_n3_rc_group_live_state_top& dut) {
    dut.rst_ni = 0; dut.state_update_i = 0; dut.state_sa_i = 0;
    dut.test_done_valid_i = 0; dut.test_done_sa_i = 0;
    drive_state(dut, {});
    tick(dut); tick(dut);
    dut.rst_ni = 1;
    tick(dut);
}

unsigned record_value(const Vrecam_dss_n3_rc_group_live_state_top& dut,
                      unsigned sa, unsigned slot) {
    return get_bits(dut.candidate_store_image_o,
                    (sa * 4U + slot) * 7U, 7U);
}

unsigned expected_record(const GeneratedCase& generated,
                         unsigned sa, unsigned slot) {
    const auto& candidate = generated.candidates[sa][slot];
    return (static_cast<unsigned>(candidate.pattern_id) << 1U) |
           static_cast<unsigned>(candidate.valid);
}

bool earlier_records_match(const Vrecam_dss_n3_rc_group_live_state_top& dut,
                           const GeneratedCase& generated,
                           unsigned completed_subarrays) {
    for (unsigned sa = 0; sa < completed_subarrays; ++sa)
        for (unsigned slot = 0; slot < 4; ++slot)
            if (record_value(dut, sa, slot) !=
                expected_record(generated, sa, slot))
                return false;
    return true;
}

RepairResult read_repairs(
    const Vrecam_dss_n3_rc_group_live_state_top& dut) {
    RepairResult result{};
    for (unsigned sa = 0; sa < 4; ++sa)
        for (unsigned slot = 0; slot < 7; ++slot) {
            const unsigned flat = sa * 7U + slot;
            result[sa][slot].valid =
                ((dut.final_repair_line_valid_flat_o >> flat) & 1U) != 0U;
            result[sa][slot].is_row =
                ((dut.final_repair_is_row_flat_o >> flat) & 1U) != 0U;
            result[sa][slot].address = static_cast<uint16_t>(
                get_bits(dut.final_repair_address_flat_o,
                         flat * 13U, 13U));
        }
    return result;
}

unsigned flatten_bits(const std::array<bool, 4>& values) {
    unsigned result = 0U;
    for (unsigned index = 0; index < 4; ++index)
        result |= static_cast<unsigned>(values[index]) << index;
    return result;
}

unsigned flatten_donors(const std::array<uint8_t, 4>& donors) {
    unsigned result = 0U;
    for (unsigned index = 0; index < 4; ++index)
        result |= static_cast<unsigned>(donors[index]) << (index * 2U);
    return result;
}

bool final_outputs_match(
    const Vrecam_dss_n3_rc_group_live_state_top& dut,
    const GeneratedCase& generated) {
    const auto& selection = generated.selection;
    if (dut.group_repairable_o != selection.repairable ||
        dut.sa_commit_valid_o != (selection.repairable ? 0xfU : 0U))
        return false;
    unsigned expected_config = 0U;
    unsigned expected_pattern = 0U;
    for (unsigned sa = 0; sa < 4; ++sa) {
        expected_config |=
            static_cast<unsigned>(selection.config_id[sa]) << (sa * 3U);
        expected_pattern |=
            static_cast<unsigned>(selection.pattern_id[sa]) << (sa * 6U);
    }
    if (dut.selected_config_flat_o != expected_config ||
        dut.selected_pattern_flat_o != expected_pattern)
        return false;
    const unsigned expected_borrow = flatten_bits(selection.borrow);
    const unsigned expected_release =
        static_cast<unsigned>(selection.release[0]) |
        (static_cast<unsigned>(selection.release[3]) << 1U) |
        (static_cast<unsigned>(selection.release[1]) << 2U) |
        (static_cast<unsigned>(selection.release[2]) << 3U);
    const unsigned expected_donor = selection.repairable
        ? flatten_donors(selection.donor) : 0U;
    unsigned expected_ledger = expected_release;
    if (selection.borrow[2]) expected_ledger |= 2U << 4U;
    if (selection.borrow[1]) expected_ledger |= 1U << 6U;
    if (selection.borrow[0]) expected_ledger |= 1U << 8U;
    if (selection.borrow[3]) expected_ledger |= 2U << 10U;
    if (dut.borrow_flat_o != expected_borrow ||
        dut.release_flat_o != expected_release ||
        dut.selected_donor_flat_o != expected_donor ||
        dut.ledger_released_borrower_o != expected_ledger)
        return false;
    return n3_rc_reconstruction_oracle::equal(
        generated.repairs, read_repairs(dut));
}

CaseResult run_case(Vrecam_dss_n3_rc_group_live_state_top& dut,
                    const GeneratedCase& generated) {
    CaseResult result;
    reset(dut);
    uint64_t cycle = 0U;
    for (unsigned sa = 0; sa < 4; ++sa) {
        drive_state(dut, generated.analyzer_state[sa]);
        dut.state_update_i = 1;
        dut.state_sa_i = sa;
        dut.test_done_valid_i = 1;
        dut.test_done_sa_i = sa;
        tick(dut);
        ++cycle;
        if (sa == 3U) result.markers.state_update_cycle = cycle;
        dut.state_update_i = 0;
        dut.test_done_valid_i = 0;
        if (dut.solution_ready_o) {
            ++result.partial_scan_finalization;
            return result;
        }
        if (!dut.scan_active_o || dut.active_sa_o != sa ||
            dut.scan_slot_o != 0U)
            return result;
        for (unsigned slot = 0; slot < 4; ++slot)
            if (record_value(dut, sa, slot) != 0U) {
                ++result.stale_generation_mix;
                ++result.old_candidate_accepted;
            }
        if (!earlier_records_match(dut, generated, sa))
            ++result.earlier_sa_replay;

        for (unsigned slot = 0; slot < 4; ++slot) {
            tick(dut);
            ++cycle;
            if (sa == 3U)
                result.markers.config_result_cycle[slot] = cycle;
            if (record_value(dut, sa, slot) !=
                expected_record(generated, sa, slot))
                return result;
            if (dut.solution_ready_o) {
                ++result.partial_scan_finalization;
                return result;
            }
            if (!earlier_records_match(dut, generated, sa))
                ++result.earlier_sa_replay;
        }
        if (((dut.sa_result_frozen_o >> sa) & 1U) == 0U)
            return result;
    }
    if (dut.solution_ready_o || dut.sa_result_frozen_o != 0xfU)
        return result;
    tick(dut);
    ++cycle;
    result.markers.registered_solution_ready_cycle = cycle;
    if (!dut.solution_ready_o ||
        result.markers.registered_solution_ready_cycle <=
            result.markers.state_update_cycle)
        return result;
    result.latency_cycles = static_cast<unsigned>(
        result.markers.registered_solution_ready_cycle -
        result.markers.state_update_cycle);
    result.passed =
        result.stale_generation_mix == 0U &&
        result.earlier_sa_replay == 0U &&
        final_outputs_match(dut, generated);
    return result;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Verilated::commandArgs(argc, argv);
        const Options options = parse_options(argc, argv);
        const auto masks = n3_rc_analyzer_oracle::load_fixed_masks(
            "dss_latency/N3_CONTINUOUS_ANALYSIS/"
            "N3_RC_GROUP_FIXED_MASK_EQUIVALENCE.csv");
        const GeneratedCase replay_a =
            n3_rc_group_vector_generator::generate(
                options.seed, 17U, GenerationMode::General, masks);
        const GeneratedCase replay_b =
            n3_rc_group_vector_generator::generate(
                options.seed, 17U, GenerationMode::General, masks);
        if (!n3_rc_group_vector_generator::equal(replay_a, replay_b)) {
            std::cerr << "deterministic replay mismatch\n";
            return EXIT_FAILURE;
        }

        std::ofstream case_csv;
        if (!options.case_csv.empty()) {
            case_csv.open(options.case_csv);
            if (!case_csv)
                throw std::runtime_error(
                    "cannot create per-case CSV: " + options.case_csv);
            case_csv << "case_index,mode,latency_cycles,repairable,"
                     << "config_a,config_b,config_c,config_d,"
                     << "pattern_a,pattern_b,pattern_c,pattern_d\n";
        }

        Vrecam_dss_n3_rc_group_live_state_top dut;
        unsigned functional_mismatches = 0U;
        unsigned completed_cases = 0U;
        unsigned repairable = 0U;
        unsigned unrepairable = 0U;
        unsigned stale_generation_mix = 0U;
        unsigned earlier_sa_replay = 0U;
        unsigned partial_scan_finalization = 0U;
        unsigned old_candidate_accepted = 0U;
        bool seventh_pivot = false;
        bool hybrid_above_seven = false;
        bool high_pattern = false;
        bool physical_column_wide = false;
        bool markers_pass = true;
        std::map<unsigned, unsigned> latency_histogram;
        std::array<unsigned, 7> config_distribution{};
        std::array<unsigned, 64> pattern_distribution{};

        for (unsigned index = 0; index < options.cases; ++index) {
            const GenerationMode mode = options.fixed_mode
                ? options.mode
                : n3_rc_group_vector_generator::smoke_mode(index);
            const GeneratedCase generated =
                n3_rc_group_vector_generator::generate(
                    options.seed, index, mode, masks);
            const CaseResult observed = run_case(dut, generated);
            stale_generation_mix += observed.stale_generation_mix;
            earlier_sa_replay += observed.earlier_sa_replay;
            partial_scan_finalization +=
                observed.partial_scan_finalization;
            old_candidate_accepted += observed.old_candidate_accepted;
            if (case_csv) {
                case_csv << index << ','
                         << n3_rc_group_vector_generator::mode_name(mode)
                         << ',' << observed.latency_cycles << ','
                         << static_cast<unsigned>(
                                generated.selection.repairable);
                for (unsigned sa = 0; sa < 4; ++sa)
                    case_csv << ','
                             << static_cast<unsigned>(
                                    generated.selection.config_id[sa]);
                for (unsigned sa = 0; sa < 4; ++sa)
                    case_csv << ','
                             << static_cast<unsigned>(
                                    generated.selection.pattern_id[sa]);
                case_csv << '\n';
            }
            if (!observed.passed) {
                ++functional_mismatches;
                std::cerr << "latency smoke mismatch seed=0x" << std::hex
                          << options.seed << std::dec
                          << " case=" << index
                          << " mode="
                          << n3_rc_group_vector_generator::mode_name(mode)
                          << " oracle_repairable="
                          << generated.selection.repairable
                          << " rtl_repairable="
                          << static_cast<unsigned>(dut.group_repairable_o)
                          << '\n';
                break;
            }
            ++completed_cases;
            repairable += generated.selection.repairable;
            unrepairable += !generated.selection.repairable;
            if (generated.selection.repairable)
                for (unsigned sa = 0; sa < 4; ++sa) {
                    ++config_distribution[
                        generated.selection.config_id[sa]];
                    ++pattern_distribution[
                        generated.selection.pattern_id[sa]];
                }
            seventh_pivot = seventh_pivot ||
                            generated.exercises_seventh_pivot;
            hybrid_above_seven = hybrid_above_seven ||
                                 generated.exercises_hybrid_above_seven;
            high_pattern = high_pattern ||
                           generated.exercises_high_pattern;
            physical_column_wide = physical_column_wide ||
                                   generated.exercises_wide_physical_column;
            ++latency_histogram[observed.latency_cycles];
            markers_pass = markers_pass &&
                observed.markers.config_result_cycle[0] ==
                    observed.markers.state_update_cycle + 1U &&
                observed.markers.config_result_cycle[1] ==
                    observed.markers.state_update_cycle + 2U &&
                observed.markers.config_result_cycle[2] ==
                    observed.markers.state_update_cycle + 3U &&
                observed.markers.config_result_cycle[3] ==
                    observed.markers.state_update_cycle + 4U &&
                observed.markers.registered_solution_ready_cycle ==
                    observed.markers.state_update_cycle + 5U;
        }

        const bool safety_pass = functional_mismatches == 0U &&
            completed_cases == options.cases && markers_pass &&
            stale_generation_mix == 0U && earlier_sa_replay == 0U &&
            partial_scan_finalization == 0U &&
            old_candidate_accepted == 0U;
        const bool smoke_pass = safety_pass &&
            repairable > 0U && unrepairable > 0U && seventh_pivot &&
            hybrid_above_seven && high_pattern && physical_column_wide;
        const bool run_pass = options.formal ? safety_pass : smoke_pass;
        std::cout << "N3_RC_GROUP_LATENCY_"
                  << (options.formal ? "FORMAL_" : "SMOKE_")
                  << (run_pass ? "PASS" : "FAIL")
                  << " cases=" << options.cases
                  << " seed=0x" << std::hex << options.seed << std::dec
                  << " functional_mismatches=" << functional_mismatches
                  << " repairable=" << repairable
                  << " unrepairable=" << unrepairable
                  << " deterministic_replay=PASS"
                  << " seventh_pivot=" << (seventh_pivot ? "PASS" : "FAIL")
                  << " hybrid_gt_7="
                  << (hybrid_above_seven ? "PASS" : "FAIL")
                  << " pattern_id_gt_15="
                  << (high_pattern ? "PASS" : "FAIL")
                  << " physical_column_wide="
                  << (physical_column_wide ? "PASS" : "FAIL")
                  << " latency_event_markers="
                  << (markers_pass ? "PASS" : "FAIL")
                  << " stale_generation_mix=" << stale_generation_mix
                  << " earlier_sa_replay=" << earlier_sa_replay
                  << " partial_scan_finalization="
                  << partial_scan_finalization
                  << " old_candidate_accepted="
                  << old_candidate_accepted
                  << " latency_histogram=";
        for (const auto& entry : latency_histogram)
            std::cout << entry.first << ':' << entry.second << ',';
        std::cout << " config_distribution=";
        for (unsigned config = 0; config < config_distribution.size();
             ++config)
            std::cout << config << ':' << config_distribution[config]
                      << ',';
        std::cout << " pattern_distribution=";
        for (unsigned pattern = 0;
             pattern < pattern_distribution.size(); ++pattern)
            if (pattern_distribution[pattern] != 0U)
                std::cout << pattern << ':'
                          << pattern_distribution[pattern] << ',';
        std::cout << " classification="
                  << (options.formal
                          ? "FORMAL_LATENCY_CONTROL_NOT_REPAIR_RATE"
                          : "SMOKE_ONLY_NON_FORMAL")
                  << '\n';
        return run_pass ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << "latency smoke setup error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
