#include "Vrecam_dss_v2_early_overlap_equivalence_test_top.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>

namespace {
constexpr unsigned kSaCount = 4;
constexpr unsigned kPayloadWords = 7;
constexpr unsigned kRandomVectors = 1000;
constexpr unsigned kTimingRandomVectors = 1000;
using Payload = std::array<std::uint32_t, kPayloadWords>;
using Group = std::array<Payload, kSaCount>;

struct Counters {
    unsigned directed = 0;
    unsigned random = 0;
    unsigned mismatches = 0;
    unsigned action_mismatches = 0;
    unsigned ledger_mismatches = 0;
    unsigned failure_mismatches = 0;
    unsigned release_cases = 0;
    unsigned borrow_cases = 0;
    unsigned release_borrow_cases = 0;
};

void tick(Vrecam_dss_v2_early_overlap_equivalence_test_top& dut) {
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

void clear_inputs(Vrecam_dss_v2_early_overlap_equivalence_test_top& dut) {
    dut.overlap_start_i = 0;
    dut.baseline_start_i = 0;
    dut.summary_update_valid_i = 0;
    dut.summary_update_sa_i = 0;
    dut.test_done_valid_i = 0;
    dut.test_done_sa_i = 0;
    for (unsigned word = 0; word < kPayloadWords; ++word)
        dut.summary_payload_i[word] = 0;
}

void set_bits(Payload& payload, unsigned lsb, unsigned width, std::uint32_t value) {
    for (unsigned bit = 0; bit < width; ++bit) {
        const unsigned index = lsb + bit;
        const std::uint32_t mask = std::uint32_t{1} << (index % 32);
        if ((value >> bit) & 1U)
            payload[index / 32] |= mask;
        else
            payload[index / 32] &= ~mask;
    }
}

Payload make_summary(std::mt19937& rng) {
    Payload payload{};
    const unsigned pivots = rng() % 6;
    const unsigned hybrids = pivots == 0 ? 0 : rng() % 8;
    for (unsigned index = 0; index < pivots; ++index) {
        set_bits(payload, index, 1, 1);
        set_bits(payload, 5 + index * 9, 9, rng() & 0x1ff);
        set_bits(payload, 50 + index * 5, 5, rng() & 0x1f);
    }
    set_bits(payload, 75, 5, rng() & 0x1f);
    set_bits(payload, 80, 5, rng() & 0x1f);
    set_bits(payload, 85, 5, rng() & 0x1f);
    set_bits(payload, 90, 5, rng() & 0x1f);
    set_bits(payload, 95, 5, rng() & 0x1f);
    set_bits(payload, 100, 5, rng() & 0x1f);
    for (unsigned index = 0; index < hybrids; ++index) {
        set_bits(payload, 105 + index, 1, 1);
        set_bits(payload, 112 + index * 3, 3, rng() % pivots);
        set_bits(payload, 133 + index, 1, rng() & 1U);
        set_bits(payload, 140 + index * 9, 9, rng() & 0x1ff);
    }
    set_bits(payload, 203, 1, (rng() % 20) == 0);
    return payload;
}

Payload overflow_summary() {
    Payload payload{};
    set_bits(payload, 203, 1, 1);
    return payload;
}

void drive_summary(Vrecam_dss_v2_early_overlap_equivalence_test_top& dut,
                   unsigned sa, const Payload& payload) {
    dut.summary_update_valid_i = 1;
    dut.summary_update_sa_i = sa;
    for (unsigned word = 0; word < kPayloadWords; ++word)
        dut.summary_payload_i[word] = payload[word];
    tick(dut);
    dut.summary_update_valid_i = 0;
}

void drive_test_done(Vrecam_dss_v2_early_overlap_equivalence_test_top& dut,
                     unsigned sa) {
    dut.test_done_valid_i = 1;
    dut.test_done_sa_i = sa;
    tick(dut);
    dut.test_done_valid_i = 0;
}

void begin_overlap(Vrecam_dss_v2_early_overlap_equivalence_test_top& dut) {
    clear_inputs(dut);
    dut.rst_ni = 0;
    tick(dut);
    dut.rst_ni = 1;
    dut.overlap_start_i = 1;
    tick(dut);
    dut.overlap_start_i = 0;
}

bool compare_final(const Vrecam_dss_v2_early_overlap_equivalence_test_top& dut,
                   Counters& counters) {
    bool pass = true;
    if (dut.overlap_group_repairable_o != dut.baseline_group_repairable_o ||
        dut.overlap_sa_commit_valid_o != dut.baseline_sa_commit_valid_o ||
        dut.overlap_selected_config_flat_o != dut.baseline_selected_config_flat_o ||
        dut.overlap_selected_pattern_flat_o != dut.baseline_selected_pattern_flat_o) {
        ++counters.mismatches;
        pass = false;
    }
    if (dut.overlap_borrow_flat_o != dut.baseline_borrow_flat_o ||
        dut.overlap_release_flat_o != dut.baseline_release_flat_o ||
        dut.overlap_selected_donor_flat_o != dut.baseline_selected_donor_flat_o) {
        ++counters.action_mismatches;
        pass = false;
    }
    if (dut.overlap_ledger_o != dut.baseline_ledger_o) {
        ++counters.ledger_mismatches;
        pass = false;
    }
    if (!dut.overlap_group_repairable_o &&
        dut.overlap_failure_position_o != dut.baseline_failure_position_o) {
        ++counters.failure_mismatches;
        pass = false;
    }
    return pass;
}

bool run_group(Vrecam_dss_v2_early_overlap_equivalence_test_top& dut,
               const Group& final_group, std::mt19937& rng, Counters& counters,
               bool timing_randomized) {
    begin_overlap(dut);

    // A/B/C/D updates are intentionally not owner ordered.
    const std::array<unsigned, 4> order = timing_randomized
        ? std::array<unsigned, 4>{{2, 0, 3, 1}}
        : std::array<unsigned, 4>{{0, 1, 2, 3}};
    for (unsigned sa : order) {
        if (timing_randomized && (rng() & 1U))
            tick(dut);
        drive_summary(dut, sa, final_group[sa]);
    }

    // Make all final summaries available to the untouched baseline oracle.
    for (unsigned sa = 0; sa < kSaCount; ++sa) {
        if (timing_randomized)
            for (unsigned delay = 0; delay < (rng() % 3); ++delay)
                tick(dut);
        drive_test_done(dut, sa);
    }

    dut.baseline_start_i = 1;
    tick(dut);
    dut.baseline_start_i = 0;

    bool overlap_terminal = !dut.overlap_busy_o;
    bool baseline_terminal = false;
    for (unsigned cycle = 0; cycle < 100; ++cycle) {
        tick(dut);
        overlap_terminal = overlap_terminal || dut.overlap_done_o;
        baseline_terminal = baseline_terminal || dut.baseline_done_o;
        if (overlap_terminal && baseline_terminal)
            break;
    }
    if (!overlap_terminal || !baseline_terminal) {
        std::cerr << "timeout overlap_busy=" << unsigned(dut.overlap_busy_o)
                  << " overlap_commits=0x" << std::hex
                  << unsigned(dut.overlap_sa_commit_valid_o)
                  << " overlap_valid=0x" << unsigned(dut.overlap_analysis_valid_o)
                  << " overlap_gen=0x" << unsigned(dut.overlap_fault_generation_o)
                  << " baseline_busy=" << std::dec << unsigned(dut.baseline_busy_o)
                  << " baseline_commits=0x" << std::hex
                  << unsigned(dut.baseline_sa_commit_valid_o) << std::dec << '\n';
        throw std::runtime_error("overlap or baseline terminal timeout");
    }

    const bool pass = compare_final(dut, counters);
    for (unsigned sa = 0; sa < kSaCount; ++sa) {
        if ((dut.overlap_release_flat_o >> sa) & 1U) ++counters.release_cases;
        if ((dut.overlap_borrow_flat_o >> sa) & 1U) ++counters.borrow_cases;
        if (((dut.overlap_release_flat_o >> sa) & 1U) &&
            ((dut.overlap_borrow_flat_o >> sa) & 1U))
            ++counters.release_borrow_cases;
    }
    return pass;
}

void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

void directed_race_tests(Vrecam_dss_v2_early_overlap_equivalence_test_top& dut,
                         Counters& counters) {
    std::mt19937 rng(20260915);
    Group zeros{};
    require(run_group(dut, zeros, rng, counters, false),
            "all-local baseline equivalence failed");
    ++counters.directed;

    for (unsigned failed_sa = 0; failed_sa < kSaCount; ++failed_sa) {
        Group group{};
        group[failed_sa] = overflow_summary();
        require(run_group(dut, group, rng, counters, true),
                "terminal failure equivalence failed");
        ++counters.directed;
    }

    // An owner update on the edge that would otherwise complete slot 3 wins.
    // Repeated updates exercise generations 1, 2, and 3; only the final
    // overflow snapshot may determine the A result.
    begin_overlap(dut);
    drive_summary(dut, 0, Payload{});
    drive_summary(dut, 1, Payload{});
    drive_summary(dut, 2, Payload{});
    drive_summary(dut, 3, Payload{});
    drive_summary(dut, 0, Payload{});
    require((dut.overlap_latest_candidate_ready_o & 1U) == 0,
            "same-edge update incorrectly accepted old completion");
    tick(dut);
    drive_summary(dut, 0, overflow_summary());
    for (unsigned sa = 0; sa < kSaCount; ++sa) drive_test_done(dut, sa);
    dut.baseline_start_i = 1;
    tick(dut);
    dut.baseline_start_i = 0;
    bool overlap_terminal = dut.overlap_done_o || !dut.overlap_busy_o;
    bool baseline_terminal = dut.baseline_done_o;
    for (unsigned cycle = 0; cycle < 100 && !(overlap_terminal && baseline_terminal);
         ++cycle) {
        tick(dut);
        overlap_terminal = overlap_terminal || dut.overlap_done_o;
        baseline_terminal = baseline_terminal || dut.baseline_done_o;
    }
    require(overlap_terminal && baseline_terminal,
            "same-cycle update/done terminal timeout");
    require((dut.overlap_fault_generation_o & 0xfU) >= 3,
            "restart stress did not advance A through three generations");
    require(compare_final(dut, counters),
            "same-cycle update/done equivalence failed");
    ++counters.directed;

    // Test done before analysis completion: no commit is legal on the first
    // three subsequent candidate-sweep edges.
    begin_overlap(dut);
    drive_summary(dut, 0, Payload{});
    drive_test_done(dut, 0);
    for (unsigned cycle = 0; cycle < 3; ++cycle) {
        tick(dut);
        require((dut.overlap_sa_commit_valid_o & 1U) == 0,
                "commit occurred before complete candidate sweep");
    }
    ++counters.directed;

    // Analysis may complete before test_done, but a final ledger decision is
    // still prohibited until the producer declares the snapshot final.
    begin_overlap(dut);
    drive_summary(dut, 0, Payload{});
    drive_summary(dut, 1, Payload{});
    drive_summary(dut, 2, Payload{});
    drive_summary(dut, 3, Payload{});
    tick(dut);
    require((dut.overlap_latest_candidate_ready_o & 1U) != 0,
            "analysis did not complete before deferred test_done");
    require((dut.overlap_sa_commit_valid_o & 1U) == 0,
            "analysis completion committed before test_done");
    ++counters.directed;
}

} // namespace

int main() {
    try {
        Vrecam_dss_v2_early_overlap_equivalence_test_top dut;
        Counters counters;
        std::mt19937 rng(20260915);

        directed_race_tests(dut, counters);

        for (unsigned vector = 0; vector < kRandomVectors; ++vector) {
            Group group{};
            for (Payload& summary : group)
                summary = make_summary(rng);
            require(run_group(dut, group, rng, counters, false),
                    "random final-output equivalence failed");
            ++counters.random;
        }

        Counters timing_counters;
        std::mt19937 timing_rng(20260916);
        for (unsigned vector = 0; vector < kTimingRandomVectors; ++vector) {
            Group group{};
            for (Payload& summary : group)
                summary = make_summary(timing_rng);
            require(run_group(dut, group, timing_rng, timing_counters, true),
                    "timing-randomized final-output equivalence failed");
            ++timing_counters.random;
        }

        // Find directed action witnesses from final complete summaries.  Each
        // witness remains checked against EARLY_BASELINE; this is not a new
        // optimization or a provisional-trace comparison.
        for (unsigned vector = 0;
             vector < 10000 &&
             (counters.release_cases == 0 || counters.borrow_cases == 0 ||
              counters.release_borrow_cases == 0);
             ++vector) {
            Group group{};
            for (Payload& summary : group)
                summary = make_summary(rng);
            require(run_group(dut, group, rng, counters, true),
                    "directed action-witness equivalence failed");
        }
        require(counters.release_cases != 0, "release witness not found");
        require(counters.borrow_cases != 0, "borrow witness not found");
        require(counters.release_borrow_cases != 0,
                "release+borrow witness not found");

        std::cout << "S1GA_DIRECTED total=" << counters.directed
                  << " pass=" << counters.directed << " fail=0\n";
        std::cout << "S1GA_RANDOM vectors=" << counters.random
                  << " seed=20260915 repairable_mismatches=" << counters.mismatches
                  << " ConfigID_mismatches=" << counters.mismatches
                  << " PatternID_mismatches=" << counters.mismatches
                  << " action_mismatches=" << counters.action_mismatches
                  << " ledger_mismatches=" << counters.ledger_mismatches
                  << " failure_position_mismatches=" << counters.failure_mismatches << "\n";
        std::cout << "S1GA_ACTION_COVERAGE release=" << counters.release_cases
                  << " borrow=" << counters.borrow_cases
                  << " release_borrow=" << counters.release_borrow_cases << "\n";
        std::cout << "S1GA_TIMING_RANDOM vectors=" << timing_counters.random
                  << " seed=20260916 final_result_mismatches="
                  << (timing_counters.mismatches + timing_counters.action_mismatches +
                      timing_counters.ledger_mismatches + timing_counters.failure_mismatches)
                  << "\n";
        std::cout << "S1GA_RESTART_STRESS PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "S1GA_FAIL: " << error.what() << '\n';
        return 1;
    }
}
