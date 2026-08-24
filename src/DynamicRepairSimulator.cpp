#include "../inc/DynamicRepairSimulator.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace dynamic_spare
{
namespace
{

struct CapacityOption
{
    int rows = 0;
    int columns = 0;
    int extraRows = 0;
    int extraColumns = 0;

    int stage() const noexcept
    {
        return extraRows + extraColumns;
    }
};

struct ProvisioningMetrics
{
    std::size_t addressCamEntries = 0;
    std::size_t hybridCamEntries = 0;
    std::size_t matrixCells = 0;
};

struct CandidatePlan
{
    std::size_t attemptVectorIndex = 0;
    const RepairAttemptResult *attempt = nullptr;
    const CandidateRepairOption *candidate = nullptr;
};

struct GroupChoice
{
    bool hasProposal = false;
    bool success = false;
    std::array<CandidatePlan, kSubarrayCount> plans;
    LedgerAllocationResult allocation;
    std::uint64_t selectedCycles = 0;
};

std::uint64_t checkedAdd(
    std::uint64_t left,
    std::uint64_t right,
    const char *description)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
    {
        throw std::overflow_error(description);
    }
    return left + right;
}

std::uint64_t checkedMultiply(
    std::uint64_t left,
    std::uint64_t right,
    const char *description)
{
    if (left != 0 &&
        right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        throw std::overflow_error(description);
    }
    return left * right;
}

void addLatency(
    AnalysisLatencyBreakdown &destination,
    const AnalysisLatencyBreakdown &source)
{
    destination.faultInformationInsertCycles = checkedAdd(
        destination.faultInformationInsertCycles,
        source.faultInformationInsertCycles,
        "Fault-information insert cycle sum overflow");
    destination.faultInformationLookupCycles = checkedAdd(
        destination.faultInformationLookupCycles,
        source.faultInformationLookupCycles,
        "Fault-information lookup cycle sum overflow");
    destination.faultInformationReadCycles = checkedAdd(
        destination.faultInformationReadCycles,
        source.faultInformationReadCycles,
        "Fault-information read cycle sum overflow");
    destination.matrixGenerationCycles = checkedAdd(
        destination.matrixGenerationCycles,
        source.matrixGenerationCycles,
        "Matrix-generation cycle sum overflow");
    destination.solutionGenerationCycles = checkedAdd(
        destination.solutionGenerationCycles,
        source.solutionGenerationCycles,
        "Solution-generation cycle sum overflow");
    destination.solutionEvaluationCycles = checkedAdd(
        destination.solutionEvaluationCycles,
        source.solutionEvaluationCycles,
        "Solution-evaluation cycle sum overflow");
    destination.sharingAllocationCycles = checkedAdd(
        destination.sharingAllocationCycles,
        source.sharingAllocationCycles,
        "Sharing-allocation cycle sum overflow");
}

void calculateAttemptLatency(
    RepairAttemptResult &attempt,
    const SimulationConfig &config)
{
    const StorageLatencyParameters &storage =
        config.storageMode == FaultInformationStorage::CAM
            ? config.latency.cam
            : config.latency.sram;
    const std::uint64_t storageReads = checkedAdd(
        static_cast<std::uint64_t>(attempt.addressCamEntriesActive),
        checkedAdd(
            static_cast<std::uint64_t>(attempt.hybridCamEntriesActive),
            static_cast<std::uint64_t>(attempt.bufferCamEntriesActive),
            "Fault-information read operation count overflow"),
        "Fault-information read operation count overflow");

    attempt.latency.faultInformationInsertCycles = checkedMultiply(
        attempt.faultCount,
        storage.insertCyclesPerOperation,
        "Fault-information insert cycle count overflow");
    attempt.latency.faultInformationLookupCycles = checkedMultiply(
        attempt.faultCount,
        storage.lookupCyclesPerOperation,
        "Fault-information lookup cycle count overflow");
    attempt.latency.faultInformationReadCycles = checkedMultiply(
        storageReads,
        storage.readCyclesPerOperation,
        "Fault-information read cycle count overflow");
    attempt.latency.matrixGenerationCycles = checkedMultiply(
        attempt.activeMatrixCells,
        config.latency.matrixGenerationCyclesPerCell,
        "Matrix-generation cycle count overflow");
    attempt.latency.solutionGenerationCycles = checkedMultiply(
        attempt.candidateSolutions,
        config.latency.solutionGenerationCyclesPerCandidate,
        "Solution-generation cycle count overflow");
    attempt.latency.solutionEvaluationCycles = checkedMultiply(
        attempt.candidateSolutionsEvaluated,
        config.latency.solutionEvaluationCyclesPerCandidate,
        "Solution-evaluation cycle count overflow");
}

std::pair<int, int> localCapacity(
    const SimulationConfig &config,
    std::size_t)
{
    if (config.topology == SharingTopology::GlobalPool)
    {
        return {
            config.globalPool->localRowsPerSubarray,
            config.globalPool->localColumnsPerSubarray};
    }
    return {config.spareRows, config.spareColumns};
}

std::pair<int, int> maximumBorrowCapacity(
    const SimulationConfig &config,
    std::size_t subarray)
{
    switch (config.topology)
    {
        case SharingTopology::NoSharing:
            return {0, 0};
        case SharingTopology::Directional:
            if (subarray == 0 || subarray == 3)
            {
                return {0, config.sharedColumns};
            }
            return {config.sharedRows, 0};
        case SharingTopology::GlobalPool:
            return {
                config.globalPool->globalRows,
                config.globalPool->globalColumns};
        case SharingTopology::PairwiseEdge:
            return {config.sharedRows, config.sharedColumns};
    }
    return {0, 0};
}

std::pair<int, int> provisionedCapacity(
    const std::vector<CapacityOption> &options)
{
    int rows = 0;
    int columns = 0;
    for (const CapacityOption &option : options)
    {
        rows = std::max(rows, option.rows);
        columns = std::max(columns, option.columns);
    }
    return {rows, columns};
}

std::vector<CapacityOption> capacityOptions(
    const SimulationConfig &config,
    std::size_t subarray)
{
    const auto local = localCapacity(config, subarray);
    const auto maximum = maximumBorrowCapacity(config, subarray);
    const int groupLimit = config.modifiers.maximumGroupBorrowedSpares;
    const std::size_t rowLimit = std::min(
        static_cast<std::size_t>(maximum.first),
        static_cast<std::size_t>(groupLimit));
    const std::size_t columnLimit = std::min(
        static_cast<std::size_t>(maximum.second),
        static_cast<std::size_t>(groupLimit));

    std::vector<CapacityOption> options;
    for (std::size_t extraRows = 0; extraRows <= rowLimit; ++extraRows)
    {
        for (std::size_t extraColumns = 0;
             extraColumns <= columnLimit; ++extraColumns)
        {
            if (extraRows + extraColumns >
                static_cast<std::size_t>(groupLimit))
            {
                continue;
            }
            if (config.modifiers.singleDimensionBorrowing &&
                extraRows != 0 && extraColumns != 0)
            {
                continue;
            }
            const std::uint64_t activeRows =
                static_cast<std::uint64_t>(local.first) + extraRows;
            const std::uint64_t activeColumns =
                static_cast<std::uint64_t>(local.second) + extraColumns;
            if (activeRows > static_cast<std::uint64_t>(
                    std::numeric_limits<int>::max()) ||
                activeColumns > static_cast<std::uint64_t>(
                    std::numeric_limits<int>::max()))
            {
                throw std::invalid_argument(
                    "Effective spare capacity exceeds the legacy int range");
            }
            options.push_back(CapacityOption{
                static_cast<int>(activeRows),
                static_cast<int>(activeColumns),
                static_cast<int>(extraRows),
                static_cast<int>(extraColumns)});
        }
    }
    std::sort(
        options.begin(), options.end(),
        [](const CapacityOption &left, const CapacityOption &right)
        {
            return std::tie(
                       left.extraRows,
                       left.extraColumns) <
                   std::tie(
                       right.extraRows,
                       right.extraColumns);
        });
    std::stable_sort(
        options.begin(), options.end(),
        [](const CapacityOption &left, const CapacityOption &right)
        {
            return left.stage() < right.stage();
        });
    return options;
}

std::size_t hybridCapacity(int rows, int columns)
{
    if (rows == 0 || columns == 0)
    {
        return 0;
    }
    const std::uint64_t r = static_cast<std::uint64_t>(rows);
    const std::uint64_t c = static_cast<std::uint64_t>(columns);
    return static_cast<std::size_t>(r * (c - 1) + c * (r - 1));
}

ProvisioningMetrics provisioningMetrics(
    const std::vector<CapacityOption> &options)
{
    ProvisioningMetrics metrics;
    for (const CapacityOption &option : options)
    {
        const std::size_t dimension =
            static_cast<std::size_t>(option.rows) +
            static_cast<std::size_t>(option.columns);
        metrics.addressCamEntries = std::max(
            metrics.addressCamEntries, dimension);
        metrics.hybridCamEntries = std::max(
            metrics.hybridCamEntries,
            hybridCapacity(option.rows, option.columns));
        metrics.matrixCells = std::max(
            metrics.matrixCells,
            static_cast<std::size_t>(checkedMultiply(
                dimension,
                dimension,
                "Provisioned matrix cell count overflow")));
    }
    return metrics;
}

RepairAttemptResult zeroCapacityAttempt(
    const std::vector<Fault> &faults,
    const RECAMSolverRequest &request)
{
    RepairAttemptResult attempt;
    attempt.runIndex = request.runIndex;
    attempt.attemptIndex = request.attemptIndex;
    attempt.stage = request.stage;
    attempt.subarrayId = request.subarrayId;
    attempt.availableRows = 0;
    attempt.availableColumns = 0;
    attempt.faultCount = faults.size();
    attempt.addressCamEntriesProvisioned =
        static_cast<std::size_t>(request.provisionedRows) +
        static_cast<std::size_t>(request.provisionedColumns);
    attempt.addressCamEntriesPeak = 0;
    attempt.hybridCamEntriesProvisioned = hybridCapacity(
        request.provisionedRows, request.provisionedColumns);
    attempt.hybridCamEntriesPeak = 0;
    attempt.bufferCamEntriesProvisioned = static_cast<std::size_t>(
        request.bufferCamEntries);
    attempt.hybridCamEntryWidthBits = request.hybridCamEntryWidthBits;
    if (request.hybridCamEntryWidthBits.has_value())
    {
        attempt.hybridCamBitsActive = 0;
        attempt.hybridCamBitsProvisioned = checkedMultiply(
            attempt.hybridCamEntriesProvisioned,
            *request.hybridCamEntryWidthBits,
            "Zero-capacity Hybrid-CAM bit count overflow");
    }
    attempt.provisionedMatrixCells = static_cast<std::size_t>(checkedMultiply(
        attempt.addressCamEntriesProvisioned,
        attempt.addressCamEntriesProvisioned,
        "Zero-capacity provisioned matrix cell count overflow"));
    attempt.candidateSolutions = 1;
    attempt.candidateSolutionsEvaluated = 1;
    if (faults.empty())
    {
        attempt.isRepairable = true;
        attempt.repairSuccess = true;
        attempt.successfulCandidateIndex = 0;
        attempt.validCandidateIndices.push_back(0);
        attempt.validCandidateOptions.push_back(CandidateRepairOption{});
    }
    else
    {
        attempt.failedCandidates = 1;
    }
    return attempt;
}

std::vector<CandidatePlan> plansForSubarray(
    const std::vector<RepairAttemptResult> &attempts,
    std::size_t maximumStage)
{
    std::map<std::pair<std::size_t, std::size_t>, CandidatePlan> bestByDemand;
    for (std::size_t attemptIndex = 0;
         attemptIndex < attempts.size(); ++attemptIndex)
    {
        const RepairAttemptResult &attempt = attempts[attemptIndex];
        if (!attempt.repairSuccess ||
            static_cast<std::size_t>(attempt.stage) > maximumStage)
        {
            continue;
        }
        for (const CandidateRepairOption &candidate :
             attempt.validCandidateOptions)
        {
            const auto key = std::make_pair(
                candidate.usedRows, candidate.usedColumns);
            const CandidatePlan plan{
                attemptIndex, &attempt, &candidate};
            const auto existing = bestByDemand.find(key);
            const bool replace = existing == bestByDemand.end() ||
                attempt.latency.totalCycles() <
                    existing->second.attempt->latency.totalCycles() ||
                (attempt.latency.totalCycles() ==
                     existing->second.attempt->latency.totalCycles() &&
                 (candidate.candidateIndex <
                      existing->second.candidate->candidateIndex ||
                  (candidate.candidateIndex ==
                       existing->second.candidate->candidateIndex &&
                   attemptIndex <
                       existing->second.attemptVectorIndex)));
            if (replace)
            {
                bestByDemand[key] = plan;
            }
        }
    }
    std::vector<CandidatePlan> plans;
    plans.reserve(bestByDemand.size());
    for (const auto &entry : bestByDemand)
    {
        plans.push_back(entry.second);
    }
    return plans;
}

bool choiceIsBetter(
    const GroupChoice &candidate,
    const GroupChoice &current)
{
    if (!current.success)
    {
        return true;
    }
    if (candidate.allocation.transfers.size() !=
        current.allocation.transfers.size())
    {
        return candidate.allocation.transfers.size() <
            current.allocation.transfers.size();
    }
    if (candidate.selectedCycles != current.selectedCycles)
    {
        return candidate.selectedCycles < current.selectedCycles;
    }
    for (std::size_t subarray = 0;
         subarray < kSubarrayCount; ++subarray)
    {
        const std::size_t candidateIndex =
            candidate.plans[subarray].candidate->candidateIndex;
        const std::size_t currentIndex =
            current.plans[subarray].candidate->candidateIndex;
        if (candidateIndex != currentIndex)
        {
            return candidateIndex < currentIndex;
        }
    }
    for (std::size_t subarray = 0;
         subarray < kSubarrayCount; ++subarray)
    {
        if (candidate.plans[subarray].attemptVectorIndex !=
            current.plans[subarray].attemptVectorIndex)
        {
            return candidate.plans[subarray].attemptVectorIndex <
                current.plans[subarray].attemptVectorIndex;
        }
    }
    return false;
}

GroupChoice findBestChoice(
    const std::array<std::vector<RepairAttemptResult>, kSubarrayCount>
        &attempts,
    std::size_t maximumBorrowCount,
    const SimulationConfig &config,
    const PhysicalResourceLedger &ledger)
{
    std::array<std::vector<CandidatePlan>, kSubarrayCount> plans;
    for (std::size_t subarray = 0;
         subarray < kSubarrayCount; ++subarray)
    {
        plans[subarray] = plansForSubarray(
            attempts[subarray], maximumBorrowCount);
        if (plans[subarray].empty())
        {
            return {};
        }
    }

    GroupChoice best;
    GroupChoice bestFailure;
    std::array<CandidatePlan, kSubarrayCount> selectedPlans;
    const auto visit = [&](const auto &self, std::size_t subarray) -> void
    {
        if (subarray != kSubarrayCount)
        {
            for (const CandidatePlan &plan : plans[subarray])
            {
                selectedPlans[subarray] = plan;
                self(self, subarray + 1);
            }
            return;
        }

        std::array<SpareDemand, kSubarrayCount> demands;
        std::uint64_t selectedCycles = 0;
        for (std::size_t index = 0; index < kSubarrayCount; ++index)
        {
            demands[index].rows = selectedPlans[index].candidate->usedRows;
            demands[index].columns =
                selectedPlans[index].candidate->usedColumns;
            selectedCycles = checkedAdd(
                selectedCycles,
                selectedPlans[index].attempt->latency.totalCycles(),
                "Selected-attempt cycle sum overflow");
        }
        LedgerAllocationResult allocation = ledger.allocate(demands);
        if (!allocation.success ||
            allocation.transfers.size() > maximumBorrowCount)
        {
            if (allocation.success)
            {
                const std::size_t overBudget =
                    allocation.transfers.size() - maximumBorrowCount;
                allocation.success = false;
                allocation.failedBorrows += overBudget;
                allocation.successfulBorrows -= overBudget;
            }
            GroupChoice failure;
            failure.hasProposal = true;
            failure.plans = selectedPlans;
            failure.allocation = std::move(allocation);
            if (!bestFailure.hasProposal ||
                failure.allocation.failedBorrows <
                    bestFailure.allocation.failedBorrows ||
                (failure.allocation.failedBorrows ==
                     bestFailure.allocation.failedBorrows &&
                 failure.allocation.successfulBorrows >
                     bestFailure.allocation.successfulBorrows) ||
                (failure.allocation.failedBorrows ==
                     bestFailure.allocation.failedBorrows &&
                 failure.allocation.successfulBorrows ==
                     bestFailure.allocation.successfulBorrows &&
                 failure.allocation.borrowRequests <
                     bestFailure.allocation.borrowRequests))
            {
                bestFailure = std::move(failure);
            }
            return;
        }
        selectedCycles = checkedAdd(
            selectedCycles,
            checkedMultiply(
                allocation.borrowRequests,
                config.latency.sharingAllocationCyclesPerRequest,
                "Selected sharing-allocation cycle count overflow"),
            "Selected group cycle sum overflow");

        GroupChoice choice;
        choice.hasProposal = true;
        choice.success = true;
        choice.plans = selectedPlans;
        choice.allocation = std::move(allocation);
        choice.selectedCycles = selectedCycles;
        if (choiceIsBetter(choice, best))
        {
            best = std::move(choice);
        }
    };
    visit(visit, 0);
    return best.success ? best : bestFailure;
}

void applyAllocation(
    GroupRepairResult &group,
    const GroupChoice &choice,
    const SimulationConfig &config,
    bool retainSelectedRemap)
{
    group.groupRepairSuccess = choice.success;
    if (choice.hasProposal)
    {
        group.sharing.borrowRequests = choice.allocation.borrowRequests;
        group.sharing.successfulBorrows = choice.allocation.successfulBorrows;
        group.sharing.failedBorrows = choice.allocation.failedBorrows;
        group.sharing.donorStarvationCount =
            choice.allocation.donorStarvationCount;
        group.sharing.globalRowsUsed = choice.allocation.globalRowsUsed;
        group.sharing.globalColumnsUsed = choice.allocation.globalColumnsUsed;
        group.sharing.remainingGlobalRows =
            choice.allocation.remainingGlobalRows;
        group.sharing.remainingGlobalColumns =
            choice.allocation.remainingGlobalColumns;
        group.latency.sharingAllocationCycles = checkedMultiply(
            choice.allocation.borrowRequests,
            config.latency.sharingAllocationCyclesPerRequest,
            "Sharing-allocation cycle count overflow");
    }
    if (!choice.success)
    {
        return;
    }

    group.successfulGroupBorrowCount = choice.allocation.transfers.size();
    group.usedRows = choice.allocation.usedRows;
    group.usedColumns = choice.allocation.usedColumns;
    group.unusedPhysicalRows = choice.allocation.unusedRows;
    group.unusedPhysicalColumns = choice.allocation.unusedColumns;
    group.sharing.borrowedRows = choice.allocation.borrowedRows();
    group.sharing.borrowedColumns = choice.allocation.borrowedColumns();
    for (const SpareDemand &lent : choice.allocation.lentBySubarray)
    {
        group.sharing.lentRows += lent.rows;
        group.sharing.lentColumns += lent.columns;
    }
    for (std::size_t subarray = 0;
         subarray < kSubarrayCount; ++subarray)
    {
        const CandidatePlan &plan = choice.plans[subarray];
        group.selectedAttemptIndices[subarray] =
            plan.attemptVectorIndex;
        group.selectedCandidateIndices[subarray] =
            plan.candidate->candidateIndex;
        if (retainSelectedRemap)
        {
            group.selectedCandidateOptions[subarray] = *plan.candidate;
        }
        group.repairSuccess[subarray] = true;
    }

}

} // namespace

GroupRepairResult DynamicRepairSimulator::run(
    const FaultGroup &faults,
    const SimulationConfig &config,
    std::size_t runIndex,
    bool retainSelectedRemap) const
{
    config.validate();
    PhysicalResourceLedger ledger(config);

    GroupRepairResult group;
    group.seed = config.randomSeed;
    group.runIndex = runIndex;
    group.faultCountModel = config.faultCountModel;
    group.topology = config.topology;

    std::array<std::vector<CapacityOption>, kSubarrayCount> options;
    std::array<std::pair<int, int>, kSubarrayCount> provisioned;
    std::array<ProvisioningMetrics, kSubarrayCount> hardwareProvisioning;
    for (std::size_t subarray = 0;
         subarray < kSubarrayCount; ++subarray)
    {
        options[subarray] = capacityOptions(config, subarray);
        provisioned[subarray] = provisionedCapacity(options[subarray]);
        hardwareProvisioning[subarray] = provisioningMetrics(
            options[subarray]);
        if (options[subarray].empty() || options[subarray].front().stage() != 0)
        {
            throw std::logic_error(
                "Every topology must expose a zero-borrow local attempt");
        }
    }

    const int activeBufferEntries = config.usePaperCamReuseCapacity
        ? config.spareRows + config.spareColumns
        : config.bufferCamEntries;
    const auto runAttempt = [&](std::size_t subarray,
                                const CapacityOption &capacity)
    {
        RECAMSolverRequest request;
        request.runIndex = runIndex;
        request.subarrayId = static_cast<int>(subarray);
        request.availableRows = capacity.rows;
        request.availableColumns = capacity.columns;
        request.bufferCamEntries = activeBufferEntries;
        request.provisionedRows = provisioned[subarray].first;
        request.provisionedColumns = provisioned[subarray].second;
        request.stage = capacity.stage();
        request.attemptIndex = group.attemptsBySubarray[subarray].size();
        request.hybridCamEntryWidthBits =
            config.hybridCamEntryWidthBits;

        RepairAttemptResult attempt =
            capacity.rows == 0 && capacity.columns == 0
                ? zeroCapacityAttempt(faults[subarray], request)
                : solver_.solve(faults[subarray], request);
        attempt.addressCamEntriesProvisioned =
            hardwareProvisioning[subarray].addressCamEntries;
        attempt.hybridCamEntriesProvisioned =
            hardwareProvisioning[subarray].hybridCamEntries;
        attempt.provisionedMatrixCells =
            hardwareProvisioning[subarray].matrixCells;
        if (config.hybridCamEntryWidthBits.has_value())
        {
            attempt.hybridCamBitsProvisioned = checkedMultiply(
                attempt.hybridCamEntriesProvisioned,
                *config.hybridCamEntryWidthBits,
                "Provisioned Hybrid-CAM bit count overflow");
        }
        calculateAttemptLatency(attempt, config);
        group.attemptsBySubarray[subarray].push_back(std::move(attempt));
    };

    for (std::size_t subarray = 0;
         subarray < kSubarrayCount; ++subarray)
    {
        runAttempt(subarray, options[subarray].front());
        const RepairAttemptResult &localAttempt =
            group.attemptsBySubarray[subarray].front();
        group.faultCounts[subarray] = localAttempt.faultCount;
        group.localRepairSuccess[subarray] = localAttempt.repairSuccess;
        group.repairSuccess[subarray] = localAttempt.repairSuccess;
    }
    group.baselineGroupRepairSuccess = std::all_of(
        group.localRepairSuccess.begin(),
        group.localRepairSuccess.end(),
        [](bool success) { return success; });

    GroupChoice best = findBestChoice(
        group.attemptsBySubarray, 0, config, ledger);
    const std::size_t maximumBorrowCount = static_cast<std::size_t>(
        config.modifiers.maximumGroupBorrowedSpares);

    if (config.topology != SharingTopology::NoSharing)
    {
        if (config.modifiers.localFirst)
        {
            if (!best.success)
            {
                for (std::size_t stage = 1;
                     stage <= maximumBorrowCount && !best.success; ++stage)
                {
                    for (std::size_t subarray = 0;
                         subarray < kSubarrayCount; ++subarray)
                    {
                        if (group.localRepairSuccess[subarray])
                        {
                            continue;
                        }
                        for (const CapacityOption &capacity : options[subarray])
                        {
                            if (capacity.stage() == static_cast<int>(stage))
                            {
                                runAttempt(subarray, capacity);
                            }
                        }
                    }
                    best = findBestChoice(
                        group.attemptsBySubarray, stage, config, ledger);
                }
            }
        }
        else
        {
            for (std::size_t subarray = 0;
                 subarray < kSubarrayCount; ++subarray)
            {
                for (std::size_t option = 1;
                     option < options[subarray].size(); ++option)
                {
                    runAttempt(subarray, options[subarray][option]);
                }
            }
            best = findBestChoice(
                group.attemptsBySubarray,
                maximumBorrowCount,
                config,
                ledger);
        }
    }

    applyAllocation(group, best, config, retainSelectedRemap);
    std::size_t totalAttempts = 0;
    for (const auto &subarrayAttempts : group.attemptsBySubarray)
    {
        totalAttempts += subarrayAttempts.size();
        for (const RepairAttemptResult &attempt : subarrayAttempts)
        {
            addLatency(group.latency, attempt.latency);
        }
    }
    group.extraAnalysisAttempts = totalAttempts - kSubarrayCount;
    group.sharing.repairSuccessDueToSharing =
        group.groupRepairSuccess && !group.baselineGroupRepairSuccess;
    group.sharing.repairFailureEvenAfterSharing =
        !group.groupRepairSuccess &&
        config.topology != SharingTopology::NoSharing;
    group.sharingGain =
        static_cast<int>(group.groupRepairSuccess) -
        static_cast<int>(group.baselineGroupRepairSuccess);
    return group;
}

} // namespace dynamic_spare
