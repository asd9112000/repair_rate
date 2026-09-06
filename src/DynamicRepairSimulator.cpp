#include "../inc/DynamicRepairSimulator.hpp"
#include "../inc/CamRecamModel.hpp"

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
    std::size_t solutionId = 0;
    std::size_t usedRows = 0;
    std::size_t usedColumns = 0;
};

struct GroupChoice
{
    bool hasProposal = false;
    bool success = false;
    std::array<CandidatePlan, kSubarrayCount> plans;
    LedgerAllocationResult allocation;
    std::uint64_t selectedCycles = 0;
    std::uint64_t candidatesChecked = 0;
    std::uint64_t feasibleCombinations = 0;
    std::array<std::optional<RemainingSpareResources>, kSubarrayCount>
        remainingResourcesAfterTile;
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

    if (attempt.sramRecam.has_value())
    {
        attempt.biraLatency.storageTechnology =
            BiraStorageTechnology::Sram;
        attempt.biraLatency.faultCollectionWorkCycles =
            attempt.sramRecam->bira.faultCollectionCycles;
        attempt.biraLatency.repairAnalysisWorkCycles =
            attempt.sramRecam->bira.repairAnalysisCycles;
        attempt.hardwareMetrics = attempt.sramRecam->hardware;
    }
    else
    {
        attempt.biraLatency.storageTechnology =
            config.storageMode == FaultInformationStorage::SRAM
                ? BiraStorageTechnology::Sram
                : BiraStorageTechnology::Cam;
        if (config.storageMode == FaultInformationStorage::CAM)
        {
            CamLatencyParameters parameters;
            parameters.insertCyclesPerFault =
                config.latency.cam.insertCyclesPerOperation;
            parameters.lookupCyclesPerFault =
                config.latency.cam.lookupCyclesPerOperation;
            parameters.readCyclesPerEntry =
                config.latency.cam.readCyclesPerOperation;
            parameters.matrixCyclesPerCell =
                config.latency.matrixGenerationCyclesPerCell;
            parameters.solutionGenerationCyclesPerCandidate =
                config.latency.solutionGenerationCyclesPerCandidate;
            parameters.solutionEvaluationCyclesPerCandidate =
                config.latency.solutionEvaluationCyclesPerCandidate;
            CamBiraWorkload workload;
            workload.faultsDetected = attempt.faultCount;
            workload.storageEntriesRead = storageReads;
            workload.activeMatrixCells = attempt.activeMatrixCells;
            workload.candidatesGenerated = attempt.candidateSolutions;
            workload.candidatesEvaluated =
                attempt.candidateSolutionsEvaluated;
            attempt.biraLatency = modelCamBiraLatency(
                parameters, workload);

            if (attempt.availableRows != 0 ||
                attempt.availableColumns != 0)
            {
                RecamGeometryConfig geometry;
                geometry.rows = config.memoryRows;
                geometry.columns = config.memoryColumns;
                geometry.spareRows = static_cast<std::uint32_t>(
                    attempt.availableRows);
                geometry.spareColumns = static_cast<std::uint32_t>(
                    attempt.availableColumns);
                geometry.channels = 1;
                geometry.dataWordBits = config.dataWidthBits;
                geometry.onlineReuseEntries = static_cast<std::uint32_t>(
                    attempt.bufferCamEntriesProvisioned);
                attempt.hardwareMetrics = deriveCamHardwareMetrics(geometry);
            }
        }
        else
        {
            // A custom SRAM solver should attach detailed SRAM_RECAM metrics.
            // Keep the legacy generic latency path for external solvers that
            // have not migrated to the common backend contract yet.
            attempt.biraLatency.faultCollectionWorkCycles = checkedAdd(
                attempt.latency.faultInformationInsertCycles,
                attempt.latency.faultInformationLookupCycles,
                "BIRA fault-collection cycle count overflow");
            attempt.biraLatency.repairAnalysisWorkCycles = checkedAdd(
                attempt.latency.faultInformationReadCycles,
                checkedAdd(
                    attempt.latency.matrixGenerationCycles,
                    checkedAdd(
                        attempt.latency.solutionGenerationCycles,
                        attempt.latency.solutionEvaluationCycles,
                        "BIRA repair-analysis cycle count overflow"),
                    "BIRA repair-analysis cycle count overflow"),
                "BIRA repair-analysis cycle count overflow");
        }
    }
    finalizeBiraWork(attempt.biraLatency);
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
        case SharingTopology::PairSharing:
            return {config.sharedRows, 0};
        case SharingTopology::NeighborSharing:
            return {
                config.sharedRows *
                    (subarray == 0 || subarray == 3 ? 1 : 2),
                0};
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
    attempt.provisionedRows = request.provisionedRows;
    attempt.provisionedColumns = request.provisionedColumns;
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
    TileSolutionState state;
    state.subarrayId = request.subarrayId;
    state.spareRows = 0;
    state.spareColumns = 0;
    state.validSolutionBitmap = {faults.empty()};
    state.compressedStorageBits = 1;
    attempt.tileSolutionState = std::move(state);
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
                attemptIndex, &attempt, &candidate,
                candidate.candidateIndex,
                candidate.usedRows,
                candidate.usedColumns};
            const auto existing = bestByDemand.find(key);
            const bool replace = existing == bestByDemand.end() ||
                attempt.latency.totalCycles() <
                    existing->second.attempt->latency.totalCycles() ||
                (attempt.latency.totalCycles() ==
                     existing->second.attempt->latency.totalCycles() &&
                 (candidate.candidateIndex <
                      existing->second.solutionId ||
                  (candidate.candidateIndex ==
                       existing->second.solutionId &&
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

std::vector<CandidatePlan> compressedPlansForSubarray(
    const std::vector<RepairAttemptResult> &attempts)
{
    std::vector<CandidatePlan> plans;
    for (std::size_t attemptIndex = 0;
         attemptIndex < attempts.size(); ++attemptIndex)
    {
        const RepairAttemptResult &attempt = attempts[attemptIndex];
        if (!attempt.repairSuccess || !attempt.tileSolutionState.has_value())
            continue;
        const TileSolutionState &state = *attempt.tileSolutionState;
        if (state.validSolutionBitmap.size() != attempt.candidateSolutions)
        {
            throw std::logic_error(
                "Compressed valid-solution bitmap has the wrong size");
        }
        std::map<std::size_t, const CandidateRepairOption *> optionsById;
        for (const CandidateRepairOption &option :
             attempt.validCandidateOptions)
        {
            optionsById.emplace(option.candidateIndex, &option);
        }
        for (std::size_t solutionId = 0;
             solutionId < state.validSolutionBitmap.size(); ++solutionId)
        {
            const bool listed = optionsById.find(solutionId) !=
                optionsById.end();
            if (state.validSolutionBitmap[solutionId] != listed)
            {
                throw std::logic_error(
                    "Compressed bitmap differs from RECAM validSolList");
            }
            if (listed)
            {
                const DecodedSolution decoded = decodeSolution(
                    state, solutionId);
                const CandidateRepairOption *option =
                    optionsById.at(solutionId);
                if (decoded.sourceRows.size() != option->usedRows ||
                    decoded.sourceColumns.size() != option->usedColumns)
                {
                    throw std::logic_error(
                        "Compressed matrix-address decode differs from the "
                        "retained RECAM remap");
                }
                plans.push_back(CandidatePlan{
                    attemptIndex, &attempt, option,
                    solutionId,
                    decoded.sourceRows.size(),
                    decoded.sourceColumns.size()});
            }
        }
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
            candidate.plans[subarray].solutionId;
        const std::size_t currentIndex =
            current.plans[subarray].solutionId;
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
            demands[index].rows = selectedPlans[index].usedRows;
            demands[index].columns =
                selectedPlans[index].usedColumns;
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

bool compressedChoiceIsBetter(
    const GroupChoice &candidate,
    const GroupChoice &current,
    bool rowOnly)
{
    if (!current.success)
        return true;
    const std::size_t candidateBorrowed = rowOnly
        ? candidate.allocation.borrowedRows()
        : candidate.allocation.transfers.size();
    const std::size_t currentBorrowed = rowOnly
        ? current.allocation.borrowedRows()
        : current.allocation.transfers.size();
    if (candidateBorrowed != currentBorrowed)
    {
        return candidateBorrowed < currentBorrowed;
    }
    const std::size_t candidateLines = rowOnly
        ? candidate.allocation.usedRows
        : candidate.allocation.usedRows + candidate.allocation.usedColumns;
    const std::size_t currentLines = rowOnly
        ? current.allocation.usedRows
        : current.allocation.usedRows + current.allocation.usedColumns;
    if (candidateLines != currentLines)
        return candidateLines < currentLines;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const std::size_t left =
            candidate.plans[subarray].solutionId;
        const std::size_t right =
            current.plans[subarray].solutionId;
        if (left != right)
            return left < right;
    }
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const std::size_t left =
            candidate.plans[subarray].attemptVectorIndex;
        const std::size_t right =
            current.plans[subarray].attemptVectorIndex;
        if (left != right)
            return left < right;
    }
    return false;
}

GroupChoice findCompressedGroupChoice(
    const std::array<std::vector<RepairAttemptResult>, kSubarrayCount>
        &attempts,
    std::size_t maximumBorrowCount,
    bool rowOnly,
    const PhysicalResourceLedger &ledger)
{
    std::array<std::vector<CandidatePlan>, kSubarrayCount> plans;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        plans[subarray] = compressedPlansForSubarray(attempts[subarray]);
        if (plans[subarray].empty())
            return {};
    }

    GroupChoice best;
    std::array<CandidatePlan, kSubarrayCount> selected;
    std::uint64_t checked = 0;
    std::uint64_t feasible = 0;
    const auto visit = [&](const auto &self, std::size_t subarray) -> void
    {
        if (subarray < kSubarrayCount)
        {
            for (const CandidatePlan &plan : plans[subarray])
            {
                selected[subarray] = plan;
                self(self, subarray + 1);
            }
            return;
        }
        ++checked;
        std::array<SpareDemand, kSubarrayCount> demands;
        for (std::size_t index = 0; index < kSubarrayCount; ++index)
        {
            demands[index] = {
                selected[index].usedRows,
                selected[index].usedColumns};
        }
        LedgerAllocationResult allocation = ledger.allocate(demands);
        if (!allocation.success ||
            allocation.transfers.size() > maximumBorrowCount)
            return;
        ++feasible;
        GroupChoice choice;
        choice.hasProposal = true;
        choice.success = true;
        choice.plans = selected;
        choice.allocation = std::move(allocation);
        if (compressedChoiceIsBetter(choice, best, rowOnly))
            best = std::move(choice);
    };
    visit(visit, 0);
    best.candidatesChecked = checked;
    best.feasibleCombinations = feasible;
    return best;
}

GroupChoice findEarlyChoice(
    const std::array<std::vector<RepairAttemptResult>, kSubarrayCount>
        &attempts,
    std::size_t maximumBorrowCount,
    bool rowOnly,
    const PhysicalResourceLedger &ledger)
{
    GroupChoice result;
    result.hasProposal = true;
    std::array<SpareDemand, kSubarrayCount> committedDemands;

    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const std::vector<CandidatePlan> plans =
            compressedPlansForSubarray(attempts[subarray]);
        bool found = false;
        CandidatePlan bestPlan;
        LedgerAllocationResult bestAllocation;
        for (const CandidatePlan &plan : plans)
        {
            ++result.candidatesChecked;
            auto demands = committedDemands;
            demands[subarray] = {
                plan.usedRows,
                plan.usedColumns};
            LedgerAllocationResult allocation = ledger.allocateSequential(
                demands, subarray + 1);
            if (!allocation.success ||
                allocation.transfers.size() > maximumBorrowCount)
                continue;
            ++result.feasibleCombinations;
            const auto rank = std::make_tuple(
                rowOnly ? allocation.borrowedRows()
                        : allocation.transfers.size(),
                rowOnly ? allocation.usedRows
                        : allocation.usedRows + allocation.usedColumns,
                plan.solutionId,
                plan.attemptVectorIndex);
            const auto bestRank = std::make_tuple(
                rowOnly ? bestAllocation.borrowedRows()
                        : bestAllocation.transfers.size(),
                rowOnly ? bestAllocation.usedRows
                        : bestAllocation.usedRows +
                              bestAllocation.usedColumns,
                found ? bestPlan.solutionId : 0,
                found ? bestPlan.attemptVectorIndex : 0);
            if (!found || rank < bestRank)
            {
                found = true;
                bestPlan = plan;
                bestAllocation = std::move(allocation);
            }
        }
        if (!found)
        {
            result.success = false;
            return result;
        }
        result.plans[subarray] = bestPlan;
        committedDemands[subarray] = {
            bestPlan.usedRows,
            bestPlan.usedColumns};
        result.allocation = std::move(bestAllocation);
        result.remainingResourcesAfterTile[subarray] =
            RemainingSpareResources{
                result.allocation.unusedRows,
                result.allocation.unusedColumns};
    }
    result.success = true;
    return result;
}

void captureCompressedSelection(
    GroupRepairResult &group,
    const GroupChoice &choice)
{
    group.solutionSelectionWork = choice.candidatesChecked;
    group.feasibleCombinationCount = choice.feasibleCombinations;
    group.remainingResourcesAfterTile =
        choice.remainingResourcesAfterTile;
    if (!choice.success)
    {
        for (std::size_t subarray = 0;
             subarray < kSubarrayCount; ++subarray)
        {
            const TileSolutionState *state = nullptr;
            if (choice.plans[subarray].attempt != nullptr &&
                choice.plans[subarray].attempt->tileSolutionState.has_value())
            {
                state = &*choice.plans[subarray].attempt->tileSolutionState;
            }
            else
            {
                for (const RepairAttemptResult &attempt :
                     group.attemptsBySubarray[subarray])
                {
                    if (attempt.tileSolutionState.has_value() &&
                        attempt.repairSuccess)
                    {
                        state = &*attempt.tileSolutionState;
                        break;
                    }
                }
            }
            if (state == nullptr)
                continue;
            group.validSolutionBitmaps[subarray] =
                state->validSolutionBitmap;
            group.compressedStateBits = checkedAdd(
                group.compressedStateBits,
                state->compressedStorageBits,
                "Compressed group solution-state bit count overflow");
        }
        return;
    }
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const TileSolutionState &state =
            *choice.plans[subarray].attempt->tileSolutionState;
        group.validSolutionBitmaps[subarray] = state.validSolutionBitmap;
        group.compressedStateBits = checkedAdd(
            group.compressedStateBits,
            state.compressedStorageBits,
            "Compressed group solution-state bit count overflow");
    }
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
        if (config.solutionTakePolicy == SolutionTakePolicy::Early)
        {
            for (std::size_t subarray = 0;
                 subarray < kSubarrayCount; ++subarray)
            {
                const CandidatePlan &plan = choice.plans[subarray];
                if (plan.attempt == nullptr || plan.candidate == nullptr)
                    continue;
                group.selectedAttemptIndices[subarray] =
                    plan.attemptVectorIndex;
                group.selectedCandidateIndices[subarray] = plan.solutionId;
                if (retainSelectedRemap)
                    group.selectedCandidateOptions[subarray] = *plan.candidate;
                group.repairSuccess[subarray] = true;
            }
        }
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
            plan.solutionId;
        if (retainSelectedRemap)
        {
            group.selectedCandidateOptions[subarray] = *plan.candidate;
        }
        group.repairSuccess[subarray] = true;
    }

}

} // namespace

DynamicRepairSimulator::DynamicRepairSimulator()
    : solver_(std::make_shared<RECAMSolverAdapter>())
{
}

DynamicRepairSimulator::DynamicRepairSimulator(
    std::shared_ptr<const RepairAttemptSolver> solver)
    : solver_(std::move(solver))
{
    if (!solver_)
    {
        throw std::invalid_argument(
            "DynamicRepairSimulator requires a non-null attempt solver");
    }
}

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
    group.layout = config.layout;
    group.topology = config.topology;
    group.solutionTakePolicy = config.solutionTakePolicy;

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
        request.rowAddressWidthBits = config.rowAddressWidthBits;
        request.columnAddressWidthBits = config.columnAddressWidthBits;

        RepairAttemptResult attempt =
            capacity.rows == 0 && capacity.columns == 0
                ? zeroCapacityAttempt(faults[subarray], request)
                : solver_->solve(faults[subarray], request);
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

    if (config.solutionTakePolicy == SolutionTakePolicy::Legacy &&
        config.topology != SharingTopology::NoSharing)
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

    if (config.solutionTakePolicy != SolutionTakePolicy::Legacy)
    {
        // Explicit solution-take policies require the complete compressed
        // candidate set.  The legacy/local-first attempt schedule above stays
        // untouched when the new flag is absent.
        for (std::size_t subarray = 0;
             subarray < kSubarrayCount; ++subarray)
        {
            for (std::size_t option = 1;
                 option < options[subarray].size(); ++option)
            {
                runAttempt(subarray, options[subarray][option]);
            }
        }
        const bool rowOnly = config.layout == GroupLayout::Line1x4;
        const GroupChoice early = findEarlyChoice(
            group.attemptsBySubarray, maximumBorrowCount, rowOnly, ledger);
        const GroupChoice compressed = findCompressedGroupChoice(
            group.attemptsBySubarray, maximumBorrowCount, rowOnly, ledger);
        if (early.success && !compressed.success)
        {
            throw std::logic_error(
                "GROUP_COMPRESSED rejected a feasible EARLY selection");
        }
        group.earlySuccess = early.success;
        group.groupCompressedSuccess = compressed.success;
        group.greedyLoss = !early.success && compressed.success;
        best = config.solutionTakePolicy == SolutionTakePolicy::Early
            ? early
            : compressed;
        if (!best.success)
        {
            group.solutionSelectionFailureReason =
                config.solutionTakePolicy == SolutionTakePolicy::Early
                    ? "NO_FEASIBLE_SOLUTION_AFTER_PRIOR_COMMIT"
                    : "NO_FEASIBLE_GROUP_COMBINATION";
        }
        captureCompressedSelection(group, best);
    }

    applyAllocation(group, best, config, retainSelectedRemap);
    std::size_t totalAttempts = 0;
    for (const auto &subarrayAttempts : group.attemptsBySubarray)
    {
        totalAttempts += subarrayAttempts.size();
        for (const RepairAttemptResult &attempt : subarrayAttempts)
        {
            addLatency(group.latency, attempt.latency);
            accumulateBiraWork(group.biraLatency, attempt.biraLatency);
        }
    }
    group.biraLatency.sharingAllocationWorkCycles =
        group.latency.sharingAllocationCycles;
    finalizeBiraWork(group.biraLatency);
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
