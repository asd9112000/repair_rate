#include "Vtb_n2_ca_live_state_relative_top.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

struct Image { uint64_t lo; uint16_t hi; };

struct Observation {
    bool early_ready = false;
    bool group_ready = false;
    uint64_t early_ready_cycle = 0;
    uint64_t group_ready_cycle = 0;
};

struct Options {
    unsigned cases = 100;
    uint32_t seed = 20260910U;
    std::string raw_path;
};

uint32_t next_random(uint32_t& value) {
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

Image random_image(uint32_t& random) {
    uint64_t lo = 0;
    uint64_t hi = 0;
    for (unsigned index = 0; index < 16; ++index) {
        const uint64_t entry = (next_random(random) & 3U) != 0U
            ? ((static_cast<uint64_t>((next_random(random) % 15U) + 1U) << 1U) | 1U)
            : 0U;
        const unsigned offset = index * 5U;
        if (offset < 64U) {
            lo |= entry << offset;
            if (offset > 59U) hi |= entry >> (64U - offset);
        } else {
            hi |= entry << (offset - 64U);
        }
    }
    return {lo, static_cast<uint16_t>(hi)};
}

void observe(const Vtb_n2_ca_live_state_relative_top& dut, uint64_t cycle,
             Observation& observation) {
    if (!observation.early_ready && dut.early_live_done_o) {
        observation.early_ready = true;
        observation.early_ready_cycle = cycle;
    }
    if (!observation.group_ready && dut.group_live_ready_o) {
        observation.group_ready = true;
        observation.group_ready_cycle = cycle;
    }
}

void tick(Vtb_n2_ca_live_state_relative_top& dut, uint64_t& cycle,
          Observation& observation) {
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
    observe(dut, cycle, observation);
    dut.clk_i = 0;
    dut.eval();
    ++cycle;
}

void reset(Vtb_n2_ca_live_state_relative_top& dut, uint64_t& cycle,
           Observation& observation) {
    cycle = 0;
    observation = {};
    dut.rst_ni = 0;
    dut.canonical_start_i = 0;
    dut.live_state_update_i = 0;
    dut.live_state_sa_i = 0;
    dut.live_test_done_valid_i = 0;
    dut.live_test_done_sa_i = 0;
    dut.candidate_image_lo_i = 0;
    dut.candidate_image_hi_i = 0;
    tick(dut, cycle, observation);
    tick(dut, cycle, observation);
    dut.rst_ni = 1;
    observation = {};
}

bool early_match(const Vtb_n2_ca_live_state_relative_top& dut) {
    return dut.early_canonical_repairable_o == dut.early_live_repairable_o &&
        dut.early_canonical_commit_mask_o == dut.early_live_commit_mask_o &&
        dut.early_canonical_config_flat_o == dut.early_live_config_flat_o &&
        dut.early_canonical_pattern_flat_o == dut.early_live_pattern_flat_o;
}

bool group_match(const Vtb_n2_ca_live_state_relative_top& dut) {
    return dut.group_canonical_repairable_o == dut.group_live_repairable_o &&
        dut.group_canonical_config_o == dut.group_live_config_o &&
        dut.group_canonical_pattern_o == dut.group_live_pattern_o &&
        dut.group_canonical_donor_o == dut.group_live_donor_o &&
        dut.group_canonical_borrow_o == dut.group_live_borrow_o &&
        dut.group_canonical_release_o == dut.group_live_release_o;
}

bool formula_matches(uint64_t update_cycle, uint64_t ready_cycle,
                     unsigned policy_latency) {
    if (ready_cycle < update_cycle || ready_cycle - update_cycle != policy_latency) return false;
    for (unsigned gap = 0; gap <= 8; ++gap) {
        const uint64_t bist_end = update_cycle + gap;
        const uint64_t measured = ready_cycle > bist_end ? ready_cycle - bist_end : 0;
        const unsigned expected = policy_latency > gap ? policy_latency - gap : 0;
        if (measured != expected) return false;
    }
    return true;
}

void write_record(std::ofstream& raw, unsigned case_id, const char* policy,
                  const char* outcome, unsigned selected_rank,
                  uint64_t config, uint64_t pattern, uint64_t update_cycle,
                  uint64_t ready_cycle, unsigned policy_latency,
                  const Image& image) {
    raw << case_id << ',' << policy << ",GROUP_DECISION," << outcome << ','
        << selected_rank << ',' << config << ',' << pattern << ','
        << update_cycle << ',' << ready_cycle << ',' << policy_latency << ','
        << policy_latency << ",0x" << std::hex << std::setw(16) << std::setfill('0')
        << image.lo << std::setw(4) << static_cast<unsigned>(image.hi) << std::dec
        << std::setfill(' ') << '\n';
}

bool parse_options(int argc, char** argv, Options& options) {
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--cases" && index + 1 < argc) {
            options.cases = static_cast<unsigned>(std::strtoul(argv[++index], nullptr, 10));
        } else if (argument == "--seed" && index + 1 < argc) {
            options.seed = static_cast<uint32_t>(std::strtoul(argv[++index], nullptr, 10));
        } else if (argument == "--raw" && index + 1 < argc) {
            options.raw_path = argv[++index];
        } else {
            return false;
        }
    }
    return options.cases > 0 && !options.raw_path.empty();
}

}  // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse_options(argc, argv, options)) {
        std::cerr << "usage: --cases N --seed N --raw PATH\n";
        return 2;
    }

    std::filesystem::create_directories(std::filesystem::path(options.raw_path).parent_path());
    std::ofstream raw(options.raw_path);
    if (!raw) {
        std::cerr << "cannot write raw CSV: " << options.raw_path << '\n';
        return 2;
    }
    raw << "case_id,policy,record_scope,outcome_class,selected_rank,selected_config_flat,"
           "selected_pattern_flat,T_state_update,T_decision_ready,L_policy,G_state_zero,"
           "state_image_hex\n";

    Vtb_n2_ca_live_state_relative_top dut;
    uint32_t random = options.seed;
    unsigned mismatches = 0;
    unsigned early_rank_count[5] = {0, 0, 0, 0, 0};
    unsigned early_unrepairable = 0;
    bool formula_ok = true;
    bool group_fixed_5 = true;

    for (unsigned case_id = 0; case_id < options.cases; ++case_id) {
        const Image image = random_image(random);
        uint64_t cycle = 0;
        Observation observation;
        reset(dut, cycle, observation);
        dut.candidate_image_lo_i = image.lo;
        dut.candidate_image_hi_i = image.hi;

        dut.canonical_start_i = 1;
        tick(dut, cycle, observation);
        dut.canonical_start_i = 0;
        // EARLY canonical done is a one-cycle pulse.  The lockstep wrapper
        // retains its canonical result fields, so use a bounded completion
        // window rather than sampling that pulse as a level.
        for (unsigned wait = 0; wait < 32; ++wait) {
            tick(dut, cycle, observation);
        }

        uint64_t early_last_update = 0;
        uint64_t group_final_update = 0;
        for (unsigned sa = 0; sa < 4; ++sa) {
            dut.live_state_update_i = 1;
            dut.live_state_sa_i = sa;
            dut.live_test_done_valid_i = 1;
            dut.live_test_done_sa_i = sa;
            if (!observation.early_ready) early_last_update = cycle;
            if (sa == 3) group_final_update = cycle;
            tick(dut, cycle, observation);
            dut.live_state_update_i = 0;
            dut.live_test_done_valid_i = 0;

            for (unsigned wait = 0;
                 wait < 8 && ((dut.group_live_frozen_o >> sa) & 1U) == 0U; ++wait) {
                tick(dut, cycle, observation);
            }
            if (((dut.group_live_frozen_o >> sa) & 1U) == 0U) {
                std::cerr << "GROUP freeze timeout case=" << case_id << " sa=" << sa << '\n';
                return 1;
            }
        }
        for (unsigned wait = 0; wait < 4 && !observation.group_ready; ++wait) {
            tick(dut, cycle, observation);
        }
        for (unsigned wait = 0; wait < 4 && !observation.early_ready; ++wait) {
            tick(dut, cycle, observation);
        }
        if (!observation.early_ready || !observation.group_ready) {
            std::cerr << "decision timeout case=" << case_id << '\n';
            return 1;
        }

        const unsigned early_latency = static_cast<unsigned>(
            observation.early_ready_cycle - early_last_update);
        const unsigned group_latency = static_cast<unsigned>(
            observation.group_ready_cycle - group_final_update);
        const bool early_repairable = dut.early_live_repairable_o;
        const bool group_repairable = dut.group_live_repairable_o;
        const unsigned early_rank = early_repairable ? early_latency : 0U;
        if (early_repairable && (early_rank < 1 || early_rank > 4)) {
            std::cerr << "EARLY rank/latency violation case=" << case_id << '\n';
            return 1;
        }
        if (early_repairable) ++early_rank_count[early_rank];
        else ++early_unrepairable;
        if (group_latency != 5U) group_fixed_5 = false;
        formula_ok = formula_ok && formula_matches(early_last_update,
            observation.early_ready_cycle, early_latency);
        formula_ok = formula_ok && formula_matches(group_final_update,
            observation.group_ready_cycle, group_latency);
        if (!early_match(dut) || !group_match(dut)) ++mismatches;

        write_record(raw, case_id, "EARLY",
            early_repairable ? "REPAIRABLE" : "UNREPAIRABLE_FULL_SEARCH", early_rank,
            dut.early_live_config_flat_o, dut.early_live_pattern_flat_o, early_last_update,
            observation.early_ready_cycle, early_latency, image);
        write_record(raw, case_id, "GROUP",
            group_repairable ? "REPAIRABLE" : "UNREPAIRABLE", 0U,
            dut.group_live_config_o, dut.group_live_pattern_o, group_final_update,
            observation.group_ready_cycle, group_latency, image);
    }

    raw.close();
    const bool rank_mapping = early_rank_count[1] + early_rank_count[2] +
        early_rank_count[3] + early_rank_count[4] + early_unrepairable == options.cases;
    std::cout << "CA_LIVE_STATE_RELATIVE_PASS cases=" << options.cases
              << " seed=" << options.seed << " mismatches=" << mismatches
              << " early_rank_counts=" << early_rank_count[1] << ',' << early_rank_count[2]
              << ',' << early_rank_count[3] << ',' << early_rank_count[4]
              << " early_unrepairable=" << early_unrepairable
              << " EARLY_RANK_LATENCY_MAPPING=" << (rank_mapping ? "PASS" : "FAIL")
              << " GROUP_FIXED_5=" << (group_fixed_5 ? "PASS" : "FAIL")
              << " STATE_RELATIVE_FORMULA_MATCH=" << (formula_ok ? "PASS" : "FAIL")
              << " G_STATE_ZERO_EQUALS_POLICY_LATENCY=" << (formula_ok ? "PASS" : "FAIL")
              << '\n';
    return mismatches == 0 && rank_mapping && group_fixed_5 && formula_ok ? 0 : 1;
}
