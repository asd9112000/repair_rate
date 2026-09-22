#include "DynamicFaultGenerator.hpp"
#include "DynamicRepairSimulator.hpp"
#include "PhysicalResourceLedger.hpp"
#include "SimulationConfig.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace
{

using namespace dynamic_spare;

constexpr std::uint64_t kSeed = 20260910;
constexpr std::size_t kGroupsPerPoint = 300;
constexpr std::array<std::uint64_t, 4> kFaultLoads{{16, 20, 24, 28}};
constexpr std::array<std::size_t, 4> kOldPriority{{0, 1, 2, 3}};
constexpr std::array<std::size_t, 4> kNewPriority{{1, 0, 3, 2}};

void require(bool condition, const std::string &message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct Evaluation
{
    bool repairable = false;
    int failureSubarray = -1;
    std::array<int, 4> selectedSlots{{-1, -1, -1, -1}};
    std::array<std::size_t, 4> transferCounts{};
};

struct Counts
{
    std::size_t bothPass = 0;
    std::size_t bothFail = 0;
    std::size_t newOnly = 0;
    std::size_t oldOnly = 0;
};

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

// Test-only replay over the production-created candidate universe. The literal
// rank makes the retained historical path independent of production priority.
Evaluation evaluatePriority(const GroupRepairResult &source,
                            const SimulationConfig &config,
                            const std::array<std::size_t, 4> &priority)
{
    PhysicalResourceLedger ledger(config);
    std::array<SpareDemand, kSubarrayCount> demands{};
    Evaluation result;
    for (std::size_t sa = 0; sa < kSubarrayCount; ++sa)
    {
        bool selected = false;
        for (const std::size_t slot : priority)
        {
            const auto &attempts = source.attemptsBySubarray[sa];
            if (slot >= attempts.size())
                continue;
            const RepairAttemptResult &attempt = attempts[slot];
            if (!attempt.repairSuccess || !attempt.tileSolutionState.has_value() ||
                attempt.validCandidateOptions.empty())
                continue;
            const CandidateRepairOption *candidate = &attempt.validCandidateOptions.front();
            for (const CandidateRepairOption &option : attempt.validCandidateOptions)
            {
                if (option.candidateIndex < candidate->candidateIndex)
                    candidate = &option;
            }
            auto proposed = demands;
            proposed[sa] = {candidate->usedRows, candidate->usedColumns};
            const LedgerAllocationResult allocation =
                ledger.allocateSequential(proposed, sa + 1);
            if (!allocation.success ||
                allocation.transfers.size() >
                    static_cast<std::size_t>(config.modifiers.maximumGroupBorrowedSpares))
                continue;
            demands = proposed;
            result.selectedSlots[sa] = static_cast<int>(slot);
            result.transferCounts[sa] = allocation.transfers.size();
            selected = true;
            break;
        }
        if (!selected)
        {
            result.failureSubarray = static_cast<int>(sa);
            return result;
        }
    }
    result.repairable = true;
    return result;
}

std::array<int, 4> productionSlots(const GroupRepairResult &result)
{
    std::array<int, 4> slots{{-1, -1, -1, -1}};
    for (const auto &trace : result.v2DecisionTrace)
    {
        if (trace.selected)
            slots[trace.subarray] = static_cast<int>(trace.roleSlot);
    }
    return slots;
}

const char *outcomeName(const Evaluation &oldResult, const Evaluation &newResult)
{
    if (oldResult.repairable && newResult.repairable) return "BOTH_PASS";
    if (!oldResult.repairable && !newResult.repairable) return "BOTH_FAIL";
    return newResult.repairable ? "NEW_ONLY" : "OLD_ONLY";
}

void add(Counts &counts, const Evaluation &oldResult, const Evaluation &newResult)
{
    if (oldResult.repairable && newResult.repairable) ++counts.bothPass;
    else if (!oldResult.repairable && !newResult.repairable) ++counts.bothFail;
    else if (newResult.repairable) ++counts.newOnly;
    else ++counts.oldOnly;
}

std::string slots(const std::array<int, 4> &value)
{
    return std::to_string(value[0]) + ':' + std::to_string(value[1]) + ':' +
        std::to_string(value[2]) + ':' + std::to_string(value[3]);
}

void writeSummary(std::ofstream &output, std::uint64_t faultLoad,
                  const Counts &counts)
{
    const std::size_t oldRepairable = counts.bothPass + counts.oldOnly;
    const std::size_t newRepairable = counts.bothPass + counts.newOnly;
    require(counts.bothPass + counts.bothFail + counts.newOnly + counts.oldOnly ==
                kGroupsPerPoint,
            "B2 paired outcome accounting does not conserve groups");
    const double oldRate = 100.0 * static_cast<double>(oldRepairable) / kGroupsPerPoint;
    const double newRate = 100.0 * static_cast<double>(newRepairable) / kGroupsPerPoint;
    output << faultLoad << ',' << kGroupsPerPoint << ',' << counts.bothPass << ','
           << counts.bothFail << ',' << counts.newOnly << ',' << counts.oldOnly << ','
           << oldRepairable << ',' << newRepairable << ',' << std::fixed
           << std::setprecision(6) << oldRate << ',' << newRate << ','
           << newRate - oldRate << '\n';
}

} // namespace

int main()
{
    try
    {
        const std::filesystem::path outputRoot =
            "tmp/date2026/final_early_b2_priority_ab";
        std::filesystem::create_directories(outputRoot);
        std::ofstream summary(outputRoot / "summary.csv");
        std::ofstream examples(outputRoot / "divergence_examples.csv");
        std::ofstream manifest(outputRoot / "manifest.txt");
        require(summary.good() && examples.good() && manifest.good(),
                "Unable to create B2 output files");
        const auto timestamp = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        manifest << "phase=FINAL-EARLY-B2\n"
                 << "source_checkpoint=939a4044ea0634d836ce723f636c2079a4626959\n"
                 << "timestamp_unix=" << timestamp << '\n'
                 << "corpus_identity=deterministic_dynamic_fault_generator_moderate_imbalance_mixed\n"
                 << "seed=" << kSeed << '\n'
                 << "rs=2\ncs=2\nm=1\ntopology=directional\n"
                 << "groups_per_f_group=" << kGroupsPerPoint << '\n'
                 << "f_group=16,20,24,28\n"
                 << "old_policy_id=directional_v2_early_l_r_b_rb\n"
                 << "old_slots=0,1,2,3\n"
                 << "new_policy_id=directional_v2_early_r_l_rb_b\n"
                 << "new_slots=1,0,3,2\n";
        summary << "f_group,total_groups,both_pass,both_fail,new_only,old_only,"
                   "old_repairable,new_repairable,old_repair_rate_percent,"
                   "new_repair_rate_percent,delta_pp\n";
        examples << "f_group,group_id,outcome,first_different_sa,old_slots,new_slots,"
                    "old_failure_sa,new_failure_sa,old_transfer_count,new_transfer_count\n";

        Counts aggregate;
        for (const std::uint64_t faultLoad : kFaultLoads)
        {
            const SimulationConfig config = configFor(faultLoad);
            DynamicFaultGenerator generator(config);
            DynamicRepairSimulator simulator;
            Counts counts;
            bool savedNewOnly = false;
            bool savedOldOnly = false;
            for (std::size_t groupId = 0; groupId < kGroupsPerPoint; ++groupId)
            {
                const FaultGroup faults = generator.generate(groupId);
                const GroupRepairResult production = simulator.run(
                    faults, config, groupId, true);
                const Evaluation oldResult = evaluatePriority(
                    production, config, kOldPriority);
                const Evaluation newResult = evaluatePriority(
                    production, config, kNewPriority);
                require(production.groupRepairSuccess == newResult.repairable &&
                            productionSlots(production) == newResult.selectedSlots,
                        "Literal new-priority replay differs from production DirectionalV2Early");
                add(counts, oldResult, newResult);
                add(aggregate, oldResult, newResult);
                const char *outcome = outcomeName(oldResult, newResult);
                const bool save = (std::string(outcome) == "NEW_ONLY" && !savedNewOnly) ||
                    (std::string(outcome) == "OLD_ONLY" && !savedOldOnly);
                if (save)
                {
                    std::size_t firstDifferent = 4;
                    for (std::size_t sa = 0; sa < kSubarrayCount; ++sa)
                    {
                        if (oldResult.selectedSlots[sa] != newResult.selectedSlots[sa])
                        {
                            firstDifferent = sa;
                            break;
                        }
                    }
                    examples << faultLoad << ',' << groupId << ',' << outcome << ','
                             << (firstDifferent == 4 ? -1 : static_cast<int>(firstDifferent))
                             << ',' << '"' << slots(oldResult.selectedSlots) << '"' << ','
                             << '"' << slots(newResult.selectedSlots) << '"' << ','
                             << oldResult.failureSubarray << ',' << newResult.failureSubarray
                             << ',' << oldResult.transferCounts[std::min(firstDifferent, std::size_t{3})]
                             << ',' << newResult.transferCounts[std::min(firstDifferent, std::size_t{3})]
                             << '\n';
                    savedNewOnly = savedNewOnly || std::string(outcome) == "NEW_ONLY";
                    savedOldOnly = savedOldOnly || std::string(outcome) == "OLD_ONLY";
                }
            }
            writeSummary(summary, faultLoad, counts);
            std::cout << "FINAL_EARLY_B2_POINT F_GROUP=" << faultLoad
                      << " BOTH_PASS=" << counts.bothPass
                      << " BOTH_FAIL=" << counts.bothFail
                      << " NEW_ONLY=" << counts.newOnly
                      << " OLD_ONLY=" << counts.oldOnly << '\n';
        }
        const std::size_t total = aggregate.bothPass + aggregate.bothFail +
            aggregate.newOnly + aggregate.oldOnly;
        require(total == kFaultLoads.size() * kGroupsPerPoint,
                "B2 aggregate paired outcome accounting does not conserve groups");
        std::cout << "FINAL_EARLY_B2_TOTAL GROUPS=" << total
                  << " BOTH_PASS=" << aggregate.bothPass
                  << " BOTH_FAIL=" << aggregate.bothFail
                  << " NEW_ONLY=" << aggregate.newOnly
                  << " OLD_ONLY=" << aggregate.oldOnly << '\n';
    }
    catch (const std::exception &error)
    {
        std::cerr << "FINAL_EARLY_B2_PRIORITY_AB FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
