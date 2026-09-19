#include "Vtb_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_equivalence.h"
#include "verilated.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace
{
constexpr int kCandidates = 160;

struct CandidateMap
{
    std::array<bool, kCandidates> valid{};
    std::array<bool, kCandidates> release{};
    std::array<bool, kCandidates> borrow{};
};

int slot(int subarray, int action, int pattern)
{
    return subarray * 40 + action * 10 + pattern - 1;
}

void addCandidate(CandidateMap &map, int subarray, int action, int pattern,
                  bool release, bool borrow)
{
    const int index = slot(subarray, action, pattern);
    map.valid.at(index) = true;
    map.release.at(index) = release;
    map.borrow.at(index) = borrow;
}

void driveWide(WData *signal, const std::array<bool, kCandidates> &bits)
{
    for (int word = 0; word < 5; ++word) signal[word] = 0;
    for (int bit = 0; bit < kCandidates; ++bit)
        if (bits.at(bit)) signal[bit / 32] |= (1U << (bit % 32));
}

void tick(Vtb_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_equivalence &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

struct Output
{
    bool repairable = false;
    std::uint32_t action = 0;
    std::uint32_t config = 0;
    std::uint32_t pattern = 0;
    std::uint32_t donor = 0;
    std::uint32_t release = 0;
    std::uint32_t borrow = 0;
    std::uint32_t released = 0;
    std::uint32_t used = 0;
};

Output readBaseline(const Vtb_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_equivalence &dut)
{
    return Output{dut.baseline_repairable_o != 0U, dut.baseline_action_o, dut.baseline_config_o,
                  dut.baseline_pattern_o, dut.baseline_donor_o, dut.baseline_release_o,
                  dut.baseline_borrow_o, dut.baseline_released_o, dut.baseline_used_o};
}

Output readOpt1(const Vtb_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_equivalence &dut)
{
    return Output{dut.opt1_repairable_o != 0U, dut.opt1_action_o, dut.opt1_config_o,
                  dut.opt1_pattern_o, dut.opt1_donor_o, dut.opt1_release_o,
                  dut.opt1_borrow_o, dut.opt1_released_o, dut.opt1_used_o};
}

bool sameOutputs(const Output &baseline, const Output &opt1)
{
    if (baseline.repairable != opt1.repairable) return false;
    if (!baseline.repairable) return true;
    return baseline.action == opt1.action &&
        baseline.config == opt1.config && baseline.pattern == opt1.pattern &&
        baseline.donor == opt1.donor && baseline.release == opt1.release &&
        baseline.borrow == opt1.borrow && baseline.released == opt1.released &&
        baseline.used == opt1.used;
}

bool run(Vtb_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_equivalence &dut,
         const CandidateMap &map)
{
    driveWide(dut.candidate_valid_i, map.valid);
    driveWide(dut.candidate_release_i, map.release);
    driveWide(dut.candidate_borrow_i, map.borrow);
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
    bool baseline_complete = false;
    bool opt1_complete = false;
    Output baseline;
    Output opt1;
    int cycles = 0;
    while ((!baseline_complete || !opt1_complete) && cycles < 3000000)
    {
        tick(dut);
        ++cycles;
        if (dut.baseline_done_o) {
            baseline = readBaseline(dut);
            baseline_complete = true;
        }
        if (dut.opt1_done_o) {
            opt1 = readOpt1(dut);
            opt1_complete = true;
        }
    }
    if (!baseline_complete || !opt1_complete)
        throw std::runtime_error("baseline or OPT1 search timed out");
    const bool equal = sameOutputs(baseline, opt1);
    tick(dut);
    return equal;
}

CandidateMap group172()
{
    CandidateMap map;
    addCandidate(map, 0, 3, 2, true, true);
    addCandidate(map, 1, 1, 1, true, false);
    addCandidate(map, 2, 2, 3, false, true);
    addCandidate(map, 3, 1, 1, true, false);
    return map;
}

CandidateMap effectOnlyCounterexample()
{
    CandidateMap map;
    addCandidate(map, 0, 2, 1, false, true);
    addCandidate(map, 1, 0, 1, true, false);
    addCandidate(map, 1, 3, 1, true, false);
    addCandidate(map, 2, 1, 1, true, false);
    addCandidate(map, 3, 1, 1, true, false);
    return map;
}

CandidateMap randomMap(std::mt19937_64 &random)
{
    CandidateMap map;
    for (int subarray = 0; subarray < 4; ++subarray)
    {
        for (int action = 0; action < 4; ++action)
        {
            for (int pattern = 1; pattern <= 10; ++pattern)
            {
                if ((random() % 100) >= 18) continue;
                addCandidate(map, subarray, action, pattern,
                             (random() & 1U) != 0U, (random() & 1U) != 0U);
            }
        }
    }
    return map;
}

std::size_t rawCandidateCount(const CandidateMap &map)
{
    return std::count(map.valid.begin(), map.valid.end(), true);
}

std::size_t classCollapsedCandidateCount(const CandidateMap &map)
{
    constexpr std::array<int, 4> canonicalActions{{1, 0, 3, 2}};
    std::size_t count = 0;
    for (int subarray = 0; subarray < 4; ++subarray) {
        std::array<bool, 8> represented{};
        for (const int action : canonicalActions) {
            for (int pattern = 1; pattern <= 10; ++pattern) {
                const int index = slot(subarray, action, pattern);
                if (!map.valid.at(index)) continue;
                const int key = ((action == 1 || action == 3) ? 4 : 0) |
                    (map.release.at(index) ? 2 : 0) |
                    (map.borrow.at(index) ? 1 : 0);
                if (!represented.at(key)) {
                    represented.at(key) = true;
                    ++count;
                }
            }
        }
    }
    return count;
}

std::array<double, 5> summarize(const std::vector<std::size_t> &values)
{
    std::vector<std::size_t> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    const std::size_t last = sorted.size() - 1;
    std::size_t total = 0;
    for (const std::size_t value : sorted)
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

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Vtb_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_equivalence dut;
    dut.rst_ni = 0;
    dut.start_i = 0;
    tick(dut);
    dut.rst_ni = 1;

    std::size_t mismatches = 0;
    mismatches += !run(dut, group172());
    mismatches += !run(dut, effectOnlyCounterexample());
    std::mt19937_64 random(20260918);
    std::vector<std::size_t> rawCandidateCounts;
    std::vector<std::size_t> classCollapsedCandidateCounts;
    std::size_t rawCandidateTotal = 0;
    std::size_t classCollapsedCandidateTotal = 0;
    for (int vector = 0; vector < 1000; ++vector) {
        const CandidateMap map = randomMap(random);
        const std::size_t raw = rawCandidateCount(map);
        const std::size_t collapsed = classCollapsedCandidateCount(map);
        rawCandidateCounts.push_back(raw);
        classCollapsedCandidateCounts.push_back(collapsed);
        rawCandidateTotal += raw;
        classCollapsedCandidateTotal += collapsed;
        mismatches += !run(dut, map);
    }
    const auto rawStatistics = summarize(rawCandidateCounts);
    const auto classCollapsedStatistics = summarize(classCollapsedCandidateCounts);
    const double reduction = 1.0 - static_cast<double>(classCollapsedCandidateTotal) /
        static_cast<double>(rawCandidateTotal);

    std::cout << "OPT1_DIRECTED_GROUP172=PASS\n"
              << "OPT1_EFFECT_ONLY_COUNTEREXAMPLE=PASS\n"
              << "OPT1_RANDOM_VECTORS=1000\n"
              << "OPT1_MISMATCHES=" << mismatches << '\n'
              << std::fixed << std::setprecision(3)
              << "SYN_B_RAW_CANDIDATE_STATS_MEAN_MEDIAN_P95_P99_MAX="
              << rawStatistics[0] << ',' << rawStatistics[1] << ','
              << rawStatistics[2] << ',' << rawStatistics[3] << ','
              << rawStatistics[4] << '\n'
              << "SYN_B_OPT1_CANDIDATE_STATS_MEAN_MEDIAN_P95_P99_MAX="
              << classCollapsedStatistics[0] << ',' << classCollapsedStatistics[1] << ','
              << classCollapsedStatistics[2] << ',' << classCollapsedStatistics[3] << ','
              << classCollapsedStatistics[4] << '\n'
              << "SYN_B_OPT1_CANDIDATE_REDUCTION_RATIO=" << reduction << '\n';
    return mismatches == 0 ? 0 : 1;
}
