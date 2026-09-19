#if defined(GLOBAL_OPT1_CLASSCOLLAPSED_TEST)
#include "Vrecam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_core.h"
#else
#include "Vrecam_dss_line1x4_rs2_cs2_m1_normalized_global_core.h"
#endif
#include "verilated.h"

#include <array>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
#if defined(GLOBAL_OPT1_CLASSCOLLAPSED_TEST)
using Dut = Vrecam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_core;
constexpr const char *kOptLevel = "OPT1";
#else
using Dut = Vrecam_dss_line1x4_rs2_cs2_m1_normalized_global_core;
constexpr const char *kOptLevel = "OPT0";
#endif
constexpr int kSubarrays = 4;
constexpr int kAttempts = 3;
constexpr int kPatterns = 15;
using CandidateMap = std::array<std::array<unsigned, 2>, 180>;

int index(int subarray, int attempt, int pattern) {
    return subarray * 45 + attempt * 15 + pattern;
}

void tick(Dut &dut) {
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
    dut.clk_i = 0;
    dut.eval();
}

void clear(Dut &dut) {
    for (int word = 0; word < 6; ++word)
        dut.candidate_valid_i[word] = 0;
    for (int word = 0; word < 17; ++word)
        dut.candidate_used_rows_flat_i[word] = 0;
    for (int word = 0; word < 12; ++word)
        dut.candidate_used_cols_flat_i[word] = 0;
}

void setBits(WData *target, int offset, unsigned value, int width) {
    target[offset / 32] |= value << (offset % 32);
    if (offset % 32 + width > 32)
        target[offset / 32 + 1] |= value >> (32 - offset % 32);
}

void drive(Dut &dut, const CandidateMap &map) {
    clear(dut);
    for (int candidate = 0; candidate < 180; ++candidate) {
        const unsigned valid = map[candidate][0];
        const unsigned demand = map[candidate][1];
        if (valid)
            dut.candidate_valid_i[candidate / 32] |= 1U << (candidate % 32);
        setBits(dut.candidate_used_rows_flat_i, candidate * 3, demand >> 2, 3);
        setBits(dut.candidate_used_cols_flat_i, candidate * 2, demand & 3U, 2);
    }
}

void add(CandidateMap &map, int subarray, int attempt, int pattern, unsigned rows, unsigned columns) {
    map[index(subarray, attempt, pattern)] = {{1, (rows << 2) | columns}};
}

unsigned rowAssignment(const Dut &dut, int line) {
    return (dut.selected_row_assignment_flat_o >> (line * 3)) & 7U;
}

unsigned donor(const Dut &dut, int subarray) {
    return (dut.selected_donor_flat_o >> (subarray * 4)) & 15U;
}

int run(Dut &dut, const CandidateMap &map) {
    dut.rst_ni = 0;
    dut.start_i = 0;
    drive(dut, map);
    tick(dut);
    dut.rst_ni = 1;
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
    for (int cycle = 0; cycle < 200000; ++cycle) {
        drive(dut, map);
        tick(dut);
        if (dut.search_done_o)
            return cycle + 1;
    }
    throw std::runtime_error("GLOBAL core timed out");
}

std::array<double, 5> summarize(const std::vector<std::uint64_t> &values) {
    std::vector<std::uint64_t> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    const std::size_t last = sorted.size() - 1;
    std::uint64_t total = 0;
    for (const std::uint64_t value : sorted)
        total += value;
    return {{
        static_cast<double>(total) / sorted.size(),
        static_cast<double>(sorted[last * 50 / 100]),
        static_cast<double>(sorted[last * 95 / 100]),
        static_cast<double>(sorted[last * 99 / 100]),
        static_cast<double>(sorted.back())
    }};
}
} // namespace

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Dut dut;
    std::size_t repairability_mismatches = 0;
    std::size_t objective_mismatches = 0;
    std::size_t selected_tuple_mismatches = 0;
    std::size_t final_owner_mismatches = 0;
    std::size_t adjacency_mismatches = 0;
    std::size_t forwarding_mismatches = 0;
    std::size_t pattern_tie_mismatches = 0;
    std::size_t attempt_tie_mismatches = 0;
    std::size_t canonical_equivalence_mismatches = 0;
    std::size_t oracle_mismatches = 0;
    std::uint64_t total_cycles = 0;
    std::uint64_t total_candidate_visits = 0;
    std::vector<std::uint64_t> candidate_visits_by_case;
    try {
        CandidateMap early_global_witness{};
        add(early_global_witness, 0, 0, 0, 2, 0);
        add(early_global_witness, 0, 0, 1, 1, 0);
        add(early_global_witness, 1, 2, 0, 4, 0);
        add(early_global_witness, 2, 0, 0, 1, 0);
        add(early_global_witness, 3, 0, 0, 1, 0);
        run(dut, early_global_witness);
        if (!dut.group_repairable_o || (dut.selected_pattern_flat_o & 15U) != 2U)
            ++objective_mismatches;

        CandidateMap d_to_a{};
        add(d_to_a, 0, 0, 0, 1, 0);
        add(d_to_a, 1, 0, 0, 1, 0);
        add(d_to_a, 2, 0, 0, 2, 0);
        add(d_to_a, 3, 1, 0, 3, 0);
        run(dut, d_to_a);
        if (dut.group_repairable_o)
            ++adjacency_mismatches;

        CandidateMap no_forward{};
        add(no_forward, 0, 0, 0, 1, 0);
        add(no_forward, 1, 1, 0, 3, 0);
        add(no_forward, 2, 1, 0, 3, 0);
        add(no_forward, 3, 0, 0, 1, 0);
        run(dut, no_forward);
        if (!dut.group_repairable_o || (donor(dut, 1) & 3U) != 0U ||
            (donor(dut, 2) & 3U) != 3U || rowAssignment(dut, 1) != 1U)
            ++forwarding_mismatches;

        CandidateMap pattern_tie{};
        add(pattern_tie, 0, 0, 0, 0, 0);
        add(pattern_tie, 0, 0, 1, 0, 0);
        add(pattern_tie, 1, 0, 0, 0, 0);
        add(pattern_tie, 2, 0, 0, 0, 0);
        add(pattern_tie, 3, 0, 0, 0, 0);
        run(dut, pattern_tie);
        if (!dut.group_repairable_o || (dut.selected_pattern_flat_o & 15U) != 1U) {
            ++pattern_tie_mismatches;
            ++canonical_equivalence_mismatches;
        }

        CandidateMap attempt_tie{};
        add(attempt_tie, 0, 0, 0, 0, 0);
        add(attempt_tie, 1, 0, 0, 0, 0);
        add(attempt_tie, 1, 1, 0, 0, 0);
        add(attempt_tie, 2, 0, 0, 0, 0);
        add(attempt_tie, 3, 0, 0, 0, 0);
        run(dut, attempt_tie);
        if (!dut.group_repairable_o || ((dut.selected_attempt_flat_o >> 2) & 3U) != 0U) {
            ++attempt_tie_mismatches;
            ++canonical_equivalence_mismatches;
        }

        if (argc < 2)
            throw std::runtime_error("shared C++ GLOBAL oracle corpus path is required");
        std::ifstream corpus(argv[1]);
        unsigned cases = 0;
        unsigned seed = 0;
        if (!(corpus >> cases >> seed) || cases != 1000)
            throw std::runtime_error("invalid shared GLOBAL oracle corpus header");
        for (unsigned vector = 0; vector < cases; ++vector) {
            unsigned expected_success = 0;
            unsigned expected_borrows = 0;
            unsigned expected_used_rows = 0;
            std::array<unsigned, 4> attempts{};
            std::array<unsigned, 4> patterns{};
            std::array<unsigned, 4> rows{};
            std::array<unsigned, 4> columns{};
            std::array<unsigned, 4> donors{};
            std::array<unsigned, 8> owners{};
            CandidateMap map{};
            corpus >> expected_success >> expected_borrows >> expected_used_rows;
            for (int subarray = 0; subarray < kSubarrays; ++subarray)
                corpus >> attempts[subarray] >> patterns[subarray] >> rows[subarray] >> columns[subarray];
            for (unsigned &entry : donors)
                corpus >> entry;
            for (unsigned &entry : owners)
                corpus >> entry;
            for (int candidate = 0; candidate < 180; ++candidate) {
                unsigned valid = 0;
                unsigned candidate_rows = 0;
                unsigned candidate_columns = 0;
                corpus >> valid >> candidate_rows >> candidate_columns;
                map[candidate] = {{valid, (candidate_rows << 2) | candidate_columns}};
            }
            if (!corpus)
                throw std::runtime_error("truncated shared GLOBAL oracle corpus");
            total_cycles += run(dut, map);
            total_candidate_visits += dut.candidate_evaluations_o;
            candidate_visits_by_case.push_back(dut.candidate_evaluations_o);
            bool mismatch = unsigned(dut.group_repairable_o) != expected_success;
            if (unsigned(dut.group_repairable_o) != expected_success)
                ++repairability_mismatches;
            if (expected_success) {
                if (unsigned(dut.selected_borrow_count_o) != expected_borrows ||
                    unsigned(dut.selected_used_rows_total_o) != expected_used_rows) {
                    ++objective_mismatches;
                    mismatch = true;
                }
                for (int subarray = 0; subarray < kSubarrays; ++subarray) {
                    if (((dut.selected_attempt_flat_o >> (subarray * 2)) & 3U) != attempts[subarray] ||
                        ((dut.selected_pattern_flat_o >> (subarray * 4)) & 15U) != patterns[subarray] ||
                        ((dut.selected_used_rows_flat_o >> (subarray * 3)) & 7U) != rows[subarray] ||
                        ((dut.selected_used_cols_flat_o >> (subarray * 2)) & 3U) != columns[subarray] ||
                        donor(dut, subarray) != donors[subarray]) {
                        ++selected_tuple_mismatches;
                        mismatch = true;
                    }
                }
                for (int line = 0; line < 8; ++line) {
                    if (rowAssignment(dut, line) != owners[line]) {
                        ++final_owner_mismatches;
                        mismatch = true;
                    }
                }
            }
            if (mismatch) {
                if (oracle_mismatches == 0) {
                    std::cout << "FIRST_GLOBAL_ORACLE_MISMATCH_CASE=" << vector << '\n';
                    for (int subarray = 0; subarray < kSubarrays; ++subarray) {
                        std::cout << "  SA" << subarray << " expected="
                                  << attempts[subarray] << ',' << patterns[subarray] << ','
                                  << rows[subarray] << ',' << columns[subarray] << ',' << donors[subarray]
                                  << " observed="
                                  << ((dut.selected_attempt_flat_o >> (subarray * 2)) & 3U) << ','
                                  << ((dut.selected_pattern_flat_o >> (subarray * 4)) & 15U) << ','
                                  << ((dut.selected_used_rows_flat_o >> (subarray * 3)) & 7U) << ','
                                  << ((dut.selected_used_cols_flat_o >> (subarray * 2)) & 3U) << ','
                                  << donor(dut, subarray) << '\n';
                    }
                }
                ++oracle_mismatches;
            }
        }
        const auto visitStatistics = summarize(candidate_visits_by_case);
        std::cout << "CPP_RTL_GLOBAL_SHARED_CORPUS=YES\n"
                  << "CPP_ORACLE_RANDOM_CASES=" << cases << '\n'
                  << "CPP_ORACLE_SEED=" << seed << '\n'
                  << "REPAIRABILITY_MISMATCHES=" << repairability_mismatches << '\n'
                  << "SELECTED_TUPLE_MISMATCHES=" << selected_tuple_mismatches << '\n'
                  << "OBJECTIVE_MISMATCHES=" << objective_mismatches << '\n'
                  << "FINAL_OWNER_MISMATCHES=" << final_owner_mismatches << '\n'
                  << "GLOBAL_ADJACENCY_MISMATCHES=" << adjacency_mismatches << '\n'
                  << "BORROWED_LINE_FORWARDING_VIOLATIONS=" << forwarding_mismatches << '\n'
                  << "PATTERNID_EQUAL_DEMAND_TIE_MISMATCHES=" << pattern_tie_mismatches << '\n'
                  << "ATTEMPT_INDEX_EQUAL_DEMAND_TIE_MISMATCHES=" << attempt_tie_mismatches << '\n'
                  << "CANONICAL_EQUIVALENCE_MISMATCHES=" << canonical_equivalence_mismatches << '\n'
                  << "CPP_ORACLE_MISMATCHES=" << oracle_mismatches << '\n'
                  << "GLOBAL_CANDIDATE_VISITS=" << total_candidate_visits << '\n'
                  << std::fixed << std::setprecision(3)
                  << "SYN_D_" << kOptLevel
                  << "_VISIT_STATS_AGGREGATE_MEAN_MEDIAN_P95_P99_MAX="
                  << total_candidate_visits << ',' << visitStatistics[0] << ','
                  << visitStatistics[1] << ',' << visitStatistics[2] << ','
                  << visitStatistics[3] << ',' << visitStatistics[4] << '\n'
                  << "GLOBAL_DFS_TOTAL_CYCLES=" << total_cycles << '\n';
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return repairability_mismatches || objective_mismatches || selected_tuple_mismatches || final_owner_mismatches ||
        adjacency_mismatches || forwarding_mismatches ||
        canonical_equivalence_mismatches || oracle_mismatches;
}
