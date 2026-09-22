#include "DynamicFaultGenerator.hpp"
#include "DynamicRepairSimulator.hpp"
#include "SimulationConfig.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
using namespace dynamic_spare;

constexpr std::uint64_t kSeed = 20260922;
constexpr std::size_t kGroupsPerPoint = 300;
constexpr std::array<std::uint64_t, 4> kFaultLoads{{16, 20, 24, 28}};
constexpr std::array<std::size_t, 4> kPriorityP0{{1, 0, 3, 2}};
constexpr std::array<std::size_t, 4> kPriorityP1{{1, 3, 0, 2}};

void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}

SimulationConfig configFor(std::uint64_t faultCount)
{
    SimulationConfig config;
    config.spareRows = 2;
    config.spareColumns = 2;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    config.layout = GroupLayout::Grid2x2;
    config.topology = SharingTopology::Directional;
    config.solutionTakePolicy = SolutionTakePolicy::DirectionalV2Early;
    config.modifiers.maximumGroupBorrowedSpares = 1;
    config.usePaperCamReuseCapacity = true;
    config.faultCount = faultCount;
    config.simulationRuns = kGroupsPerPoint;
    config.randomSeed = kSeed;
    config.memoryRows = 1024;
    config.memoryColumns = 1024;
    config.faultCountModel = FaultCountModel::ModerateImbalance;
    config.faultSpatialModel = FaultSpatialModel::Mixed;
    return config;
}

constexpr std::array<std::uint8_t, 4> kOwner{{0x1, 0x4, 0x8, 0x2}};
constexpr std::array<std::uint8_t, 4> kBorrower{{0x4, 0x2, 0x1, 0x8}};

// Independent theorem table: it deliberately does not call V2RoleSlotMapping
// or any production action/claim helper.
constexpr std::uint8_t fixedClaimMask(std::size_t sa, std::size_t slot)
{
    return slot == 0 ? kOwner[sa] :
           slot == 1 ? 0 :
           slot == 2 ? static_cast<std::uint8_t>(kOwner[sa] | kBorrower[sa]) :
           kBorrower[sa];
}

std::uint8_t observedClaimMask(std::size_t sa,
                               const CandidateRepairOption &candidate)
{
    const bool owner = (sa == 0 || sa == 3)
        ? candidate.usedRows >= 2
        : candidate.usedColumns >= 2;
    const bool borrower = (sa == 0 || sa == 3)
        ? candidate.usedColumns > 2
        : candidate.usedRows > 2;
    return static_cast<std::uint8_t>((owner ? kOwner[sa] : 0) |
                                     (borrower ? kBorrower[sa] : 0));
}

const CandidateRepairOption *firstCandidate(const GroupRepairResult &group,
                                             std::size_t sa,
                                             std::size_t slot)
{
    const RepairAttemptResult &attempt = group.attemptsBySubarray.at(sa).at(slot);
    if (!attempt.repairSuccess || attempt.validCandidateOptions.empty())
        return nullptr;
    return &*std::min_element(attempt.validCandidateOptions.begin(),
        attempt.validCandidateOptions.end(),
        [](const CandidateRepairOption &left, const CandidateRepairOption &right)
        { return left.candidateIndex < right.candidateIndex; });
}

struct PolicyResult
{
    bool success = false;
    std::array<int, 4> slots{{-1, -1, -1, -1}};
};

// Test-only P0/P1 replay used only to identify the four C1R3 loss coordinates.
PolicyResult replayPolicy(const GroupRepairResult &group,
                          const std::array<std::size_t, 4> &priority)
{
    PolicyResult result;
    std::uint8_t available = 0x0f;
    std::size_t borrows = 0;
    for (std::size_t sa = 0; sa < 4; ++sa)
    {
        for (std::size_t slot : priority)
        {
            const CandidateRepairOption *candidate = firstCandidate(group, sa, slot);
            if (candidate == nullptr)
                continue;
            const std::uint8_t mask = observedClaimMask(sa, *candidate);
            const bool borrowsNow = (mask & kBorrower[sa]) != 0;
            if ((available & mask) != mask || (borrowsNow && borrows == 1))
                continue;
            available = static_cast<std::uint8_t>(available & ~mask);
            if (borrowsNow)
                ++borrows;
            result.slots[sa] = static_cast<int>(slot);
            break;
        }
        if (result.slots[sa] < 0)
            return result;
    }
    result.success = true;
    return result;
}

void run()
{
    const std::filesystem::path root = "tmp/date2026/final_early_c1r4";
    std::filesystem::create_directories(root);
    std::ofstream table(root / "static_action_table.csv");
    std::ofstream losses(root / "p1_loss_cases.csv");
    require(table.good() && losses.good(), "unable to write C1R4 temporary evidence");
    table << "sa,slot,config_id,claim_mask,reachable,observed_matches\n";
    losses << "f_group,group,p0_slots,p1_slots\n";

    std::array<std::array<std::size_t, 4>, 4> reachable{};
    std::array<std::array<std::size_t, 4>, 4> matches{};
    std::size_t theoremMismatches = 0;
    std::size_t p1Losses = 0;
    DynamicRepairSimulator simulator;
    for (std::uint64_t faultLoad : kFaultLoads)
    {
        const SimulationConfig config = configFor(faultLoad);
        DynamicFaultGenerator generator(config);
        for (std::size_t groupId = 0; groupId < kGroupsPerPoint; ++groupId)
        {
            const GroupRepairResult group = simulator.run(
                generator.generate(groupId), config, groupId, true);
            const PolicyResult p0 = replayPolicy(group, kPriorityP0);
            const PolicyResult p1 = replayPolicy(group, kPriorityP1);
            require(p0.success == group.groupRepairSuccess,
                    "test-only P0 replay differs from production");
            if (p0.success && !p1.success)
            {
                ++p1Losses;
                losses << faultLoad << ',' << groupId << ','
                       << p0.slots[0] << ':' << p0.slots[1] << ':'
                       << p0.slots[2] << ':' << p0.slots[3] << ','
                       << p1.slots[0] << ':' << p1.slots[1] << ':'
                       << p1.slots[2] << ':' << p1.slots[3] << '\n';
            }
            for (std::size_t sa = 0; sa < 4; ++sa)
            {
                if (!group.selectedCandidateOptions[sa].has_value() ||
                    !group.selectedAttemptIndices[sa].has_value())
                    continue;
                const std::size_t slot = *group.selectedAttemptIndices[sa];
                ++reachable[sa][slot];
                const bool match = observedClaimMask(
                    sa, *group.selectedCandidateOptions[sa]) ==
                    fixedClaimMask(sa, slot);
                if (match)
                    ++matches[sa][slot];
                else
                    ++theoremMismatches;
            }
        }
    }
    require(p1Losses == 4, "C1R3 P1 loss replay did not reproduce four cases");

    const std::array<std::array<int, 4>, 4> configIds{{
        {{0, 4, 5, 6}}, {{0, 1, 2, 3}},
        {{0, 1, 2, 3}}, {{0, 4, 5, 6}}}};
    for (std::size_t sa = 0; sa < 4; ++sa)
        for (std::size_t slot = 0; slot < 4; ++slot)
            table << static_cast<char>('A' + sa) << ',' << slot << ','
                  << configIds[sa][slot] << ','
                  << static_cast<unsigned>(fixedClaimMask(sa, slot)) << ','
                  << reachable[sa][slot] << ',' << matches[sa][slot] << '\n';

    // The C1R reference set contains 16 known R-overflow coordinates.  R has
    // static claim mask zero, so every repaired R must agree with the theorem.
    struct Replay { int sa; std::uint64_t load; std::size_t group; };
    constexpr std::array<Replay, 16> known{{
        {2,16,7},{2,16,155},{2,16,171},{2,16,196},{2,16,216},
         {2,20,9},{1,20,108},{1,20,171},{2,20,177},{2,20,190},
         {1,20,216},{1,20,219},{1,20,220},{1,20,290},{1,24,219},{1,28,35}}};
    std::size_t knownMatches = 0;
    for (const Replay item : known)
    {
        const SimulationConfig config = configFor(item.load);
        DynamicFaultGenerator generator(config);
        const GroupRepairResult group = simulator.run(
            generator.generate(item.group), config, item.group, true);
        const CandidateRepairOption *candidate = firstCandidate(group, item.sa, 1);
        if (candidate != nullptr && observedClaimMask(item.sa, *candidate) == 0)
            ++knownMatches;
    }
    require(knownMatches == known.size(), "C1R analogous static R action mismatch");

    // Vector216 is the original C1 witness: C's local action is invalid and
    // its R action must be selected.  Keep it explicit rather than relying on
    // the aggregate 16-coordinate regression set above.
    {
        const SimulationConfig config = configFor(16);
        DynamicFaultGenerator generator(config);
        const GroupRepairResult vector216 = simulator.run(
            generator.generate(216), config, 216, true);
        require(vector216.groupRepairSuccess,
                "Vector216 must remain repairable by the P0 policy");
        require(vector216.selectedAttemptIndices[2].has_value() &&
                    *vector216.selectedAttemptIndices[2] == 1,
                "Vector216 C must select R after local L is invalid");
        require(vector216.selectedCandidateOptions[2].has_value() &&
                    observedClaimMask(2, *vector216.selectedCandidateOptions[2]) ==
                        fixedClaimMask(2, 1),
                "Vector216 C:R static action mismatch");
    }
    require(theoremMismatches == 0, "fixed static claim theorem counterexample");
    std::cout << "FINAL_EARLY_C1R4 THEOREM=PASS REACHABLE_SLOTS=";
    for (const auto &bySlot : reachable)
        for (std::size_t count : bySlot)
            std::cout << count << ',';
    std::cout << " P1_LOSSES=" << p1Losses << " KNOWN16=" << knownMatches << '\n';
}
} // namespace

int main()
{
    try { run(); }
    catch (const std::exception &error)
    {
        std::cerr << "FINAL_EARLY_C1R4 FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
