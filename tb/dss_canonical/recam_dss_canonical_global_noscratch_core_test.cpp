#include "Vrecam_dss_canonical_global_noscratch_core.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

namespace
{
constexpr int kCandidates = 160;
struct Map
{
    std::array<bool, kCandidates> valid{};
    std::array<bool, kCandidates> release{};
    std::array<bool, kCandidates> borrow{};
};
struct Result
{
    bool repairable = false;
    std::array<int, 4> action{{0, 0, 0, 0}};
    std::array<int, 4> config{{0, 0, 0, 0}};
    std::array<int, 4> pattern{{0, 0, 0, 0}};
    std::array<int, 4> donor{{0, 0, 0, 0}};
    std::array<bool, 4> release{{false, false, false, false}};
    std::array<bool, 4> borrow{{false, false, false, false}};
    int releasedMask = 0;
    int usedMask = 0;
};

int index(int sa, int action, int patternZero)
{
    return sa * 40 + action * 10 + patternZero;
}

void candidate(Map &map, int sa, int action, int pattern,
               bool release, bool borrow)
{
    const int i = index(sa, action, pattern - 1);
    map.valid[i] = true;
    map.release[i] = release;
    map.borrow[i] = borrow;
}

Result oracle(const Map &map)
{
    constexpr std::array<int, 4> order{{1, 0, 3, 2}};
    constexpr std::array<int, 4> releaseResource{{0, 2, 3, 1}};
    constexpr std::array<int, 4> borrowResource{{2, 1, 0, 3}};
    constexpr std::array<int, 4> owner{{0, 3, 1, 2}};
    Result result;
    std::array<int, 4> selectedAction{};
    std::array<int, 4> selectedPattern{};
    std::array<bool, 4> selectedRelease{};
    std::array<bool, 4> selectedBorrow{};
    const auto visit = [&](const auto &self, int depth, int released, int used,
                           int req, int borrowCount) -> bool
    {
        for (int action : order)
        {
            for (int pattern = 1; pattern <= 10; ++pattern)
            {
                const int i = index(depth, action, pattern - 1);
                if (!map.valid[i]) continue;
                int nextReleased = released;
                int nextUsed = used;
                int nextReq = req;
                int nextCount = borrowCount;
                const int releaseBit = 1 << releaseResource[depth];
                const bool explicitRelease = action == 1 || action == 3;
                if ((req & releaseBit) && !(explicitRelease && map.release[i]))
                    continue;
                if (map.release[i])
                {
                    nextReleased |= releaseBit;
                    if (explicitRelease) nextReq &= ~releaseBit;
                }
                if (map.borrow[i])
                {
                    const int donor = borrowResource[depth];
                    const int bit = 1 << donor;
                    if (nextCount == 3 || (used & bit)) continue;
                    if (!(released & bit) && owner[donor] <= depth) continue;
                    nextUsed |= bit;
                    ++nextCount;
                    if (!(released & bit)) nextReq |= bit;
                }
                selectedAction[depth] = action;
                selectedPattern[depth] = pattern;
                selectedRelease[depth] = map.release[i];
                selectedBorrow[depth] = map.borrow[i];
                if (depth != 3)
                {
                    if (self(self, depth + 1, nextReleased, nextUsed,
                             nextReq, nextCount)) return true;
                }
                else if (nextReq == 0)
                {
                    result.repairable = true;
                    result.action = selectedAction;
                    result.pattern = selectedPattern;
                    result.release = selectedRelease;
                    result.borrow = selectedBorrow;
                    result.releasedMask = nextReleased;
                    result.usedMask = nextUsed;
                    for (int sa = 0; sa < 4; ++sa)
                    {
                        result.donor[sa] = borrowResource[sa];
                        const int selected = result.action[sa];
                        result.config[sa] = (sa == 0 || sa == 3)
                            ? std::array<int, 4>{{0, 4, 5, 6}}[selected]
                            : selected;
                    }
                    return true;
                }
            }
        }
        return false;
    };
    visit(visit, 0, 0, 0, 0, 0);
    return result;
}

void driveWide(WData *signal, const std::array<bool, kCandidates> &bits)
{
    for (int word = 0; word < 5; ++word) signal[word] = 0;
    for (int bit = 0; bit < kCandidates; ++bit)
        if (bits[bit]) signal[bit / 32] |= (1U << (bit % 32));
}

void tick(Vrecam_dss_canonical_global_noscratch_core &dut)
{
    dut.clk_i = 0; dut.eval();
    dut.clk_i = 1; dut.eval();
}

Result run(Vrecam_dss_canonical_global_noscratch_core &dut, const Map &map)
{
    driveWide(dut.candidate_valid_i, map.valid);
    driveWide(dut.candidate_release_i, map.release);
    driveWide(dut.candidate_borrow_i, map.borrow);
    dut.start_i = 1; tick(dut); dut.start_i = 0;
    int cycles = 0;
    while (!dut.done_o && cycles < 3000000) { tick(dut); ++cycles; }
    if (!dut.done_o) throw std::runtime_error("RTL search timeout");
    Result result;
    result.repairable = dut.group_repairable_o;
    result.releasedMask = dut.final_released_mask_o;
    result.usedMask = dut.final_used_mask_o;
    for (int sa = 0; sa < 4; ++sa)
    {
        result.action[sa] = (dut.selected_action_flat_o >> (sa * 2)) & 3;
        result.config[sa] = (dut.selected_config_flat_o >> (sa * 3)) & 7;
        result.pattern[sa] = (dut.selected_pattern_flat_o >> (sa * 4)) & 15;
        result.donor[sa] = (dut.selected_donor_flat_o >> (sa * 2)) & 3;
        result.release[sa] = (dut.selected_release_o >> sa) & 1;
        result.borrow[sa] = (dut.selected_borrow_o >> sa) & 1;
    }
    tick(dut); // consume one-cycle done pulse
    return result;
}

bool equal(const Result &a, const Result &b)
{
    if (a.repairable != b.repairable) return false;
    if (!a.repairable) return true;
    return a.action == b.action && a.config == b.config && a.pattern == b.pattern &&
        a.donor == b.donor && a.release == b.release &&
        a.borrow == b.borrow && a.releasedMask == b.releasedMask &&
        a.usedMask == b.usedMask;
}

Map allRelease()
{
    Map map;
    for (int sa = 0; sa < 4; ++sa) candidate(map, sa, 1, 1, true, false);
    return map;
}
} // namespace

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Vrecam_dss_canonical_global_noscratch_core dut;
    dut.rst_ni = 0; dut.start_i = 0; tick(dut); dut.rst_ni = 1;
    std::array<Map, 6> directed;
    directed[0] = allRelease();
    candidate(directed[1], 0, 2, 1, false, true);
    candidate(directed[1], 1, 1, 1, true, false);
    candidate(directed[1], 2, 1, 1, true, false);
    candidate(directed[1], 3, 1, 1, true, false);
    candidate(directed[2], 0, 2, 1, false, true);
    candidate(directed[2], 1, 0, 1, false, false);
    candidate(directed[2], 2, 1, 1, true, false);
    candidate(directed[2], 3, 1, 1, true, false);
    directed[3] = directed[2];
    candidate(directed[3], 0, 1, 2, true, false); // A backtrack succeeds
    directed[4] = Map{}; // exhaustion
    // Reconstructed N2/F16/group172 canonical candidate map.
    candidate(directed[5], 0, 3, 2, true, true);
    candidate(directed[5], 1, 1, 1, true, false);
    candidate(directed[5], 2, 2, 3, false, true);
    candidate(directed[5], 3, 1, 1, true, false);

    std::size_t mismatches = 0;
    for (const Map &map : directed)
        mismatches += !equal(oracle(map), run(dut, map));

    std::mt19937_64 random(20260918);
    for (int vector = 0; vector < 1000; ++vector)
    {
        Map map;
        for (int sa = 0; sa < 4; ++sa)
        {
            for (int action = 0; action < 4; ++action)
            {
                for (int pattern = 1; pattern <= 10; ++pattern)
                {
                    if ((random() % 100) >= 18) continue;
                    const bool explicitRelease = action == 1 || action == 3;
                    const bool borrowEnvelope = action == 2 || action == 3;
                    candidate(map, sa, action, pattern,
                              explicitRelease || (random() & 1),
                              borrowEnvelope && (random() & 1));
                }
            }
            // Bound runtime and guarantee at least one complete tuple.
            candidate(map, sa, 1, 10, true, false);
        }
        mismatches += !equal(oracle(map), run(dut, map));
    }
    std::cout << "DIRECTED_VECTORS=" << directed.size()
              << " RANDOM_VECTORS=1000 MISMATCHES=" << mismatches << '\n';
    return mismatches == 0 ? 0 : 1;
}
