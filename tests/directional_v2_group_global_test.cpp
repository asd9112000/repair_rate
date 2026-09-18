#include "DynamicRepairSimulator.hpp"
#include "PhysicalResourceLedger.hpp"
#include "SimulationConfig.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using namespace dynamic_spare;

void require(bool condition, const std::string &message)
{
    if (!condition)
        throw std::runtime_error(message);
}

SimulationConfig directionalV2Config(int spares, SolutionTakePolicy policy)
{
    SimulationConfig config;
    config.spareRows = spares;
    config.spareColumns = spares;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    config.layout = GroupLayout::Grid2x2;
    config.topology = SharingTopology::Directional;
    config.solutionTakePolicy = policy;
    // Keep the historical R3 directional policy's default group-borrow
    // budget.  `sharedRows/sharedColumns=1` constrains each shareable line
    // class; it is not a request to redefine the separate group budget.
    config.usePaperCamReuseCapacity = true;
    return config;
}

Fault makeFault(int subarray, int row, int column)
{
    Fault fault;
    fault.HBMID = 0;
    fault.ChannelID = 0;
    fault.BankID = 51;
    fault.SubarrayGroupID = 0;
    fault.SubarrayID = subarray;
    fault.r = row;
    fault.c = column;
    return fault;
}

FaultGroup knownWitness()
{
    FaultGroup group;
    group[0] = {makeFault(0, 161, 944), makeFault(0, 1015, 944),
                makeFault(0, 117, 822), makeFault(0, 161, 943),
                makeFault(0, 1015, 1000), makeFault(0, 636, 100),
                makeFault(0, 726, 944)};
    group[1] = {makeFault(1, 580, 175), makeFault(1, 136, 430),
                makeFault(1, 112, 263), makeFault(1, 7, 706),
                makeFault(1, 580, 63)};
    group[2] = {makeFault(2, 21, 191)};
    group[3] = {makeFault(3, 237, 213), makeFault(3, 235, 214),
                makeFault(3, 1003, 130)};
    return group;
}

struct Candidate
{
    std::size_t usedRows = 0;
    std::size_t usedColumns = 0;
};

std::array<std::vector<Candidate>, kSubarrayCount> candidates(
    const GroupRepairResult &result)
{
    std::array<std::vector<Candidate>, kSubarrayCount> all;
    for (std::size_t sa = 0; sa < kSubarrayCount; ++sa)
    {
        for (const RepairAttemptResult &attempt : result.attemptsBySubarray[sa])
        {
            if (!attempt.repairSuccess || !attempt.tileSolutionState.has_value())
                continue;
            for (const CandidateRepairOption &option : attempt.validCandidateOptions)
                all[sa].push_back({option.usedRows, option.usedColumns});
        }
    }
    return all;
}

struct BruteForceResult
{
    bool success = false;
    std::uint64_t complete = 0;
    std::uint64_t legal = 0;
};

BruteForceResult bruteForce(
    const GroupRepairResult &v2CandidateSource,
    const SimulationConfig &config,
    bool countAllLegal)
{
    const auto all = candidates(v2CandidateSource);
    for (const auto &perSa : all)
    {
        if (perSa.empty())
            return {};
    }
    PhysicalResourceLedger ledger(config);
    BruteForceResult result;
    std::array<SpareDemand, kSubarrayCount> demands{};
    bool stop = false;
    const auto visit = [&](const auto &self, std::size_t sa) -> void
    {
        if (stop)
            return;
        if (sa == kSubarrayCount)
        {
            ++result.complete;
            const LedgerAllocationResult allocation =
                ledger.allocateSequential(demands, kSubarrayCount);
            if (allocation.success &&
                allocation.transfers.size() <=
                    static_cast<std::size_t>(config.modifiers.maximumGroupBorrowedSpares))
            {
                result.success = true;
                ++result.legal;
                stop = !countAllLegal;
            }
            return;
        }
        for (const Candidate &candidate : all[sa])
        {
            demands[sa] = {candidate.usedRows, candidate.usedColumns};
            self(self, sa + 1);
            if (stop)
                return;
        }
    };
    visit(visit, 0);
    return result;
}

FaultGroup randomEasyGroup(std::uint64_t seed)
{
    FaultGroup group;
    for (std::size_t sa = 0; sa < kSubarrayCount; ++sa)
    {
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        const int row = static_cast<int>((seed >> 20) % 1024);
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        const int column = static_cast<int>((seed >> 20) % 1024);
        group[sa].push_back(makeFault(static_cast<int>(sa), row, column));
    }
    return group;
}

void testDirectionalV2GlobalKnownWitness()
{
    DynamicRepairSimulator simulator;
    const FaultGroup faults = knownWitness();
    const auto greedy = simulator.run(
        faults, directionalV2Config(2, SolutionTakePolicy::GroupGreedyRtlCanonical),
        51, true);
    const auto generic = simulator.run(
        faults, directionalV2Config(2, SolutionTakePolicy::GroupGlobal), 51, true);
    const auto v2Global = simulator.run(
        faults, directionalV2Config(2, SolutionTakePolicy::DirectionalV2GroupGlobal),
        51, true);
    const BruteForceResult oracle = bruteForce(
        v2Global,
        directionalV2Config(2, SolutionTakePolicy::DirectionalV2GroupGlobal),
        true);

    require(greedy.groupRepairSuccess, "known witness GREEDY must pass");
    require(!generic.groupRepairSuccess, "known witness generic GLOBAL must fail");
    require(v2Global.groupRepairSuccess, "known witness V2 GLOBAL must pass");
    require(oracle.success && oracle.complete == 23805 && oracle.legal == 6693,
            "known witness V2 brute-force result changed");
    require(v2Global.globalSearchMetrics.has_value() &&
                v2Global.globalSearchMetrics->candidateCounts ==
                    std::array<std::size_t, 4>{{3, 15, 23, 23}} &&
                v2Global.globalSearchMetrics->stoppedAtFirstLegal,
            "V2 GLOBAL witness instrumentation or candidate contract changed");
    std::cout << "test_directional_v2_global_known_witness PASS\n";
}

void testDirectionalV2GlobalContainsGreedy()
{
    DynamicRepairSimulator simulator;
    std::size_t checked = 0;
    for (const int n : {2, 3})
    {
        for (std::uint64_t seed = 1; seed <= 32; ++seed)
        {
            const FaultGroup faults = randomEasyGroup(seed + static_cast<std::uint64_t>(n) * 1000);
            const auto greedy = simulator.run(
                faults, directionalV2Config(n, SolutionTakePolicy::GroupGreedyRtlCanonical),
                seed, false);
            const auto global = simulator.run(
                faults, directionalV2Config(n, SolutionTakePolicy::DirectionalV2GroupGlobal),
                seed, false);
            require(!greedy.groupRepairSuccess || global.groupRepairSuccess,
                    "V2 GLOBAL omitted a passing GREEDY candidate");
            ++checked;
        }
    }
    std::cout << "test_directional_v2_global_contains_greedy PASS vectors="
              << checked << '\n';
}

void testDirectionalV2EarlyUsesFrozenContract()
{
    DynamicRepairSimulator simulator;
    const FaultGroup faults = knownWitness();
    const auto early = simulator.run(
        faults, directionalV2Config(2, SolutionTakePolicy::DirectionalV2Early),
        51, true);
    const auto global = simulator.run(
        faults, directionalV2Config(2, SolutionTakePolicy::DirectionalV2GroupGlobal),
        51, true);
    require(early.configContractVersion == ConfigContractVersion::FrozenDate2x2M1,
            "V2 EARLY did not select the frozen directional V2 contract");
    require(!early.v2DecisionTrace.empty() &&
                early.v2DecisionTrace.front().roleSlot == 0,
            "V2 EARLY no longer starts with V2 slot 0");
    require(!early.groupRepairSuccess || global.groupRepairSuccess,
            "V2 GLOBAL omitted a passing V2 EARLY candidate");
    std::cout << "test_directional_v2_early_uses_frozen_contract PASS\n";
}

void testDirectionalV2GlobalVsBruteforce()
{
    DynamicRepairSimulator simulator;
    std::size_t mismatches = 0;
    for (const int n : {2, 3})
    {
        for (std::uint64_t seed = 101; seed < 109; ++seed)
        {
            const FaultGroup faults = randomEasyGroup(seed + static_cast<std::uint64_t>(n) * 1000);
            const SimulationConfig config = directionalV2Config(
                n, SolutionTakePolicy::DirectionalV2GroupGlobal);
            const auto production = simulator.run(faults, config, seed, false);
            const BruteForceResult oracle = bruteForce(production, config, false);
            if (production.groupRepairSuccess != oracle.success)
                ++mismatches;
        }
    }
    require(mismatches == 0, "V2 GLOBAL differs from independent brute force");
    std::cout << "test_directional_v2_global_vs_bruteforce PASS mismatches=0\n";
}

} // namespace

int main()
{
    testDirectionalV2GlobalKnownWitness();
    testDirectionalV2GlobalContainsGreedy();
    testDirectionalV2EarlyUsesFrozenContract();
    testDirectionalV2GlobalVsBruteforce();
}
