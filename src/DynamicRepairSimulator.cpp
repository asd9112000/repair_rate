#include "../inc/DynamicRepairSimulator.hpp"
#include "../inc/CamRecamModel.hpp"
#include "../inc/SharedCollectorRecam.hpp"
#include "../inc/V2GroupNoScratchPolicy.hpp"

#include <algorithm>
#include <array>
#include <chrono>
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
        return std::max(0, extraRows) + std::max(0, extraColumns);
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
    std::optional<GlobalSearchMetrics> globalSearchMetrics;
    std::array<std::optional<RemainingSpareResources>, kSubarrayCount>
        remainingResourcesAfterTile;
    std::array<std::optional<int>, kSubarrayCount> configIds;
    std::array<std::optional<V2GroupAction>, kSubarrayCount> actions;
    std::optional<std::size_t> failureSubarray;
    std::vector<GroupRepairResult::V2DecisionTrace> trace;
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

std::vector<CapacityOption> v2CapacityOptions(
    const SimulationConfig &config,
    std::size_t subarray)
{
    const auto local = localCapacity(config, subarray);
    if (local.first < 1 || local.second < 1)
        throw std::invalid_argument("GROUP_NO_SCRATCH_V2 requires positive local row/column capacity");
    if (subarray == 0 || subarray == 3)
    {
        return {{local.first, local.second, 0, 0},
                {local.first - 1, local.second, -1, 0},
                {local.first, local.second + 1, 0, 1},
                {local.first - 1, local.second + 1, -1, 1}};
    }
    return {{local.first, local.second, 0, 0},
            {local.first, local.second - 1, 0, -1},
            {local.first + 1, local.second, 1, 0},
            {local.first + 1, local.second - 1, 1, -1}};
}

std::vector<CapacityOption> rowCycleCapacityOptions(
    const SimulationConfig &config,
    std::size_t subarray)
{
    const auto local = localCapacity(config, subarray);
    if (local.first < 1 || local.second < 1)
        throw std::invalid_argument("G2X2_R static policy requires positive local capacity");
    return {{local.first, local.second, 0, 0},
            {local.first - 1, local.second, -1, 0},
            {local.first + 1, local.second, 1, 0}};
}

std::vector<CapacityOption> lineRowCapacityOptions(
    const SimulationConfig &config,
    std::size_t subarray)
{
    const auto local = localCapacity(config, subarray);
    if (local.first < 1 || local.second < 1)
        throw std::invalid_argument("L1X4_R static policy requires positive local capacity");
    if (subarray == 0)
        return {{local.first, local.second, 0, 0},
                {local.first - 1, local.second, -1, 0}};
    if (subarray == 3)
        return {{local.first, local.second, 0, 0},
                {local.first + 1, local.second, 1, 0}};
    return {{local.first, local.second, 0, 0},
            {local.first - 1, local.second, -1, 0},
            {local.first + 1, local.second, 1, 0}};
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

// Ledger allocation depends only on a candidate's row/column demand.  For a
// fixed demand, the frozen GLOBAL ranking always prefers the lowest PatternID
// and then lowest attempt index, so all other candidates are dominated without
// changing repairability, final ledger ownership, or selected reconstruction.
std::vector<CandidatePlan> globalPlansForSubarray(
    const std::vector<RepairAttemptResult> &attempts)
{
    std::map<std::pair<std::size_t, std::size_t>, CandidatePlan> bestByDemand;
    for (const CandidatePlan &plan : compressedPlansForSubarray(attempts))
    {
        const auto key = std::make_pair(plan.usedRows, plan.usedColumns);
        const auto existing = bestByDemand.find(key);
        if (existing == bestByDemand.end() ||
            std::tie(plan.solutionId, plan.attemptVectorIndex) <
                std::tie(existing->second.solutionId,
                         existing->second.attemptVectorIndex))
        {
            bestByDemand[key] = plan;
        }
    }
    std::vector<CandidatePlan> plans;
    plans.reserve(bestByDemand.size());
    for (const auto &entry : bestByDemand)
        plans.push_back(entry.second);
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
    const auto started = std::chrono::steady_clock::now();
    std::array<std::vector<CandidatePlan>, kSubarrayCount> plans;
    GlobalSearchMetrics metrics;
    metrics.searchNodesVisited = 1; // root
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        plans[subarray] = globalPlansForSubarray(attempts[subarray]);
        metrics.candidateCounts[subarray] = plans[subarray].size();
        if (plans[subarray].empty())
            return {};
        metrics.rawCartesianProductSize = subarray == 0
            ? plans[subarray].size()
            : checkedMultiply(metrics.rawCartesianProductSize,
                              plans[subarray].size(),
                              "GLOBAL Cartesian-product size overflow");
    }

    GroupChoice best;
    std::array<CandidatePlan, kSubarrayCount> selected;
    std::uint64_t checked = 0;
    std::uint64_t feasible = 0;
    std::array<SpareDemand, kSubarrayCount> demands;
    const auto visit = [&](const auto &self, std::size_t subarray) -> void
    {
        if (subarray == kSubarrayCount)
        {
            return;
        }
        for (const CandidatePlan &plan : plans[subarray])
        {
            ++metrics.searchNodesVisited;
            selected[subarray] = plan;
            demands[subarray] = {plan.usedRows, plan.usedColumns};
            // GLOBAL must test legality with the same A->B->C->D ledger
            // commit contract used by the corresponding EARLY policies.
            // allocate() first reserves every SA's owned lines, whereas an
            // EARLY commitment may already have lent a later SA's shareable
            // line.  Mixing those allocation orders could make GLOBAL reject
            // an EARLY-feasible demand tuple, so it is not a valid superset
            // check.  The committed prefix is monotonic and can safely prune
            // this exact DFS without changing its candidate objective.
            LedgerAllocationResult allocation = ledger.allocateSequential(
                demands, subarray + 1);
            if (!allocation.success ||
                allocation.transfers.size() > maximumBorrowCount)
            {
                ++metrics.partialAssignmentsPruned;
                demands[subarray] = {};
                continue;
            }
            if (subarray + 1 < kSubarrayCount)
            {
                self(self, subarray + 1);
                demands[subarray] = {};
                continue;
            }

            ++checked;
            ++metrics.completeAssignmentsChecked;
            ++feasible;
            ++metrics.legalCompleteAssignments;
            if (!metrics.firstFeasibleNodeIndex.has_value())
                metrics.firstFeasibleNodeIndex = checked;
            bool middleBranch = false;
            for (const BorrowTransfer &transfer : allocation.transfers)
            {
                if (transfer.dimension != SpareDimension::Row)
                    continue;
                if (transfer.donorSubarray < static_cast<int>(transfer.borrowerSubarray))
                    ++metrics.leftDonorTransfers;
                else
                    ++metrics.rightDonorTransfers;
                middleBranch = middleBranch ||
                    transfer.borrowerSubarray == 1 || transfer.borrowerSubarray == 2;
            }
            if (middleBranch)
                ++metrics.middleSaDonorChoiceBranches;
            GroupChoice choice;
            choice.hasProposal = true;
            choice.success = true;
            choice.plans = selected;
            choice.allocation = std::move(allocation);
            if (compressedChoiceIsBetter(choice, best, rowOnly))
                best = std::move(choice);
            demands[subarray] = {};
            return;
        }
    };
    visit(visit, 0);
    best.candidatesChecked = checked;
    best.feasibleCombinations = feasible;
    metrics.runtimeMicroseconds = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - started).count());
    best.globalSearchMetrics = metrics;
    return best;
}

bool pairChoiceIsBetter(
    const GroupChoice &candidate,
    const GroupChoice &current,
    const std::array<std::size_t, 2> &pair)
{
    if (!current.success)
        return true;
    if (candidate.allocation.borrowedRows() != current.allocation.borrowedRows())
    {
        return candidate.allocation.borrowedRows() <
            current.allocation.borrowedRows();
    }
    if (candidate.allocation.usedRows != current.allocation.usedRows)
        return candidate.allocation.usedRows < current.allocation.usedRows;
    for (std::size_t subarray : pair)
    {
        if (candidate.plans[subarray].solutionId !=
            current.plans[subarray].solutionId)
        {
            return candidate.plans[subarray].solutionId <
                current.plans[subarray].solutionId;
        }
    }
    for (std::size_t subarray : pair)
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

GroupChoice findPairGlobalChoice(
    const std::array<std::vector<RepairAttemptResult>, kSubarrayCount>
        &attempts,
    std::size_t maximumBorrowCount,
    const PhysicalResourceLedger &ledger)
{
    std::array<std::vector<CandidatePlan>, kSubarrayCount> plans;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        plans[subarray] = compressedPlansForSubarray(attempts[subarray]);
        if (plans[subarray].empty())
            return {};
    }

    GroupChoice result;
    result.hasProposal = true;
    for (const std::array<std::size_t, 2> pair :
         {std::array<std::size_t, 2>{{0, 1}},
          std::array<std::size_t, 2>{{2, 3}}})
    {
        GroupChoice pairBest;
        for (const CandidatePlan &first : plans[pair[0]])
        {
            for (const CandidatePlan &second : plans[pair[1]])
            {
                ++result.candidatesChecked;
                std::array<SpareDemand, kSubarrayCount> demands{};
                demands[pair[0]] = {first.usedRows, first.usedColumns};
                demands[pair[1]] = {second.usedRows, second.usedColumns};
                const std::size_t committedTiles = pair[1] + 1;
                LedgerAllocationResult allocation = ledger.allocateSequential(
                    demands, committedTiles);
                if (!allocation.success ||
                    allocation.transfers.size() > maximumBorrowCount)
                {
                    continue;
                }
                ++result.feasibleCombinations;
                GroupChoice candidate;
                candidate.hasProposal = true;
                candidate.success = true;
                candidate.plans[pair[0]] = first;
                candidate.plans[pair[1]] = second;
                candidate.allocation = std::move(allocation);
                if (pairChoiceIsBetter(candidate, pairBest, pair))
                    pairBest = std::move(candidate);
            }
        }
        if (!pairBest.success)
            return {};
        result.plans[pair[0]] = pairBest.plans[pair[0]];
        result.plans[pair[1]] = pairBest.plans[pair[1]];
    }

    std::array<SpareDemand, kSubarrayCount> demands;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        demands[subarray] = {result.plans[subarray].usedRows,
                             result.plans[subarray].usedColumns};
    }
    result.allocation = ledger.allocateSequential(demands, kSubarrayCount);
    result.success = result.allocation.success &&
        result.allocation.transfers.size() <= maximumBorrowCount;
    return result;
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

// R1B v1 priority: capacity attempts are local first, then increasing row
// borrow; within an attempt the retained PatternIDs are ascending.  The first
// ledger-legal plan commits immediately. For neighbor sharing the ledger's
// physical owner order resolves a middle SA's left neighbor before right.
GroupChoice findOneByFourEarlyChoice(
    const std::array<std::vector<RepairAttemptResult>, kSubarrayCount>
        &attempts,
    std::size_t maximumBorrowCount,
    const PhysicalResourceLedger &ledger)
{
    GroupChoice result;
    result.hasProposal = true;
    std::array<SpareDemand, kSubarrayCount> committedDemands;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const std::vector<CandidatePlan> plans =
            compressedPlansForSubarray(attempts[subarray]);
        bool selected = false;
        for (const CandidatePlan &plan : plans)
        {
            ++result.candidatesChecked;
            auto demands = committedDemands;
            demands[subarray] = {plan.usedRows, plan.usedColumns};
            LedgerAllocationResult allocation = ledger.allocateSequential(
                demands, subarray + 1);
            if (!allocation.success ||
                allocation.transfers.size() > maximumBorrowCount)
            {
                continue;
            }
            ++result.feasibleCombinations;
            result.plans[subarray] = plan;
            committedDemands[subarray] = demands[subarray];
            result.allocation = std::move(allocation);
            result.remainingResourcesAfterTile[subarray] =
                RemainingSpareResources{result.allocation.unusedRows,
                                        result.allocation.unusedColumns};
            selected = true;
            break;
        }
        if (!selected)
        {
            result.failureSubarray = subarray;
            return result;
        }
    }
    result.success = true;
    return result;
}

std::vector<int> ledgerOwners(const LedgerAllocationResult &allocation)
{
    std::vector<int> owners;
    owners.reserve(allocation.lines.size());
    for (const PhysicalSpareLine &line : allocation.lines)
    {
        owners.push_back(line.assignedSubarray.has_value()
            ? static_cast<int>(*line.assignedSubarray) : -1);
    }
    return owners;
}

GroupChoice findV2GroupNoScratchChoice(
    const std::array<std::vector<RepairAttemptResult>, kSubarrayCount> &attempts,
    std::size_t maximumBorrowCount,
    const PhysicalResourceLedger &ledger,
    bool rtlCanonicalPriority,
    ConfigContractVersion configContract)
{
    GroupChoice result;
    result.hasProposal = true;
    std::array<SpareDemand, kSubarrayCount> committedDemands;

    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const auto mappings = rtlCanonicalPriority
            ? v2RtlGroupRoleSlotMappings(configContract, subarray)
            : v2RoleSlotMappings(configContract, subarray);
        const auto plans = compressedPlansForSubarray(attempts[subarray]);
        const LedgerAllocationResult before =
            ledger.allocateSequential(committedDemands, subarray);
        bool selected = false;
        for (const V2RoleSlotMapping &mapping : mappings)
        {
            GroupRepairResult::V2DecisionTrace trace;
            trace.role = mapping.role;
            trace.subarray = subarray;
            trace.roleSlot = mapping.roleSlot;
            trace.configId = mapping.configId;
            trace.action = mapping.action;
            trace.ledgerOwnersBefore = ledgerOwners(before);
            trace.ledgerOwnersAfter = trace.ledgerOwnersBefore;

            const auto plan = std::find_if(
                plans.begin(), plans.end(), [&mapping](const CandidatePlan &candidate)
                {
                    return candidate.attemptVectorIndex == mapping.roleSlot;
                });
            trace.configFeasible = plan != plans.end();
            if (plan != plans.end())
            {
                trace.smallestPatternId = plan->solutionId + 1;
                trace.requiredRows = plan->usedRows;
                trace.requiredColumns = plan->usedColumns;
                auto demands = committedDemands;
                demands[subarray] = {plan->usedRows, plan->usedColumns};
                LedgerAllocationResult allocation =
                    ledger.allocateSequential(demands, subarray + 1);
                trace.ledgerValid = allocation.success &&
                    allocation.transfers.size() <= maximumBorrowCount;
                if (trace.ledgerValid)
                {
                    trace.selected = true;
                    trace.ledgerOwnersAfter = ledgerOwners(allocation);
                    result.plans[subarray] = *plan;
                    result.configIds[subarray] = mapping.configId;
                    result.actions[subarray] = mapping.action;
                    committedDemands[subarray] = demands[subarray];
                    result.allocation = std::move(allocation);
                    result.remainingResourcesAfterTile[subarray] =
                        RemainingSpareResources{result.allocation.unusedRows,
                                                result.allocation.unusedColumns};
                    selected = true;
                }
            }
            ++result.candidatesChecked;
            result.trace.push_back(std::move(trace));
            if (selected)
                break;
        }
        if (!selected)
        {
            result.failureSubarray = subarray;
            result.success = false;
            return result;
        }
    }
    result.success = true;
    return result;
}

// Exact group-global search for the frozen directional V2 contract.  Unlike
// findCompressedGroupChoice(), this deliberately retains every V2 attempt /
// PatternID candidate: V2 release-slot identity is part of the contract and
// candidates are not collapsed by their row/column demand.
GroupChoice findDirectionalV2GroupGlobalChoice(
    const std::array<std::vector<RepairAttemptResult>, kSubarrayCount> &attempts,
    std::size_t maximumBorrowCount,
    const PhysicalResourceLedger &ledger,
    ConfigContractVersion configContract)
{
    const auto started = std::chrono::steady_clock::now();
    std::array<std::vector<CandidatePlan>, kSubarrayCount> plans;
    GlobalSearchMetrics metrics;
    metrics.searchNodesVisited = 1; // root
    metrics.exhaustiveEnumeration = false;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        plans[subarray] = compressedPlansForSubarray(attempts[subarray]);
        metrics.candidateCounts[subarray] = plans[subarray].size();
        if (plans[subarray].empty())
        {
            metrics.terminationReason = "EMPTY_V2_LOCAL_CANDIDATE_SET";
            GroupChoice failure;
            failure.hasProposal = true;
            failure.globalSearchMetrics = metrics;
            failure.failureSubarray = subarray;
            return failure;
        }
        metrics.rawCartesianProductSize = subarray == 0
            ? plans[subarray].size()
            : checkedMultiply(metrics.rawCartesianProductSize,
                              plans[subarray].size(),
                              "V2 GLOBAL Cartesian-product size overflow");
    }

    GroupChoice result;
    result.hasProposal = true;
    std::array<CandidatePlan, kSubarrayCount> selected;
    std::array<SpareDemand, kSubarrayCount> demands{};
    bool stop = false;
    const auto visit = [&](const auto &self, std::size_t subarray) -> void
    {
        if (stop || subarray == kSubarrayCount)
            return;
        for (const CandidatePlan &plan : plans[subarray])
        {
            if (stop)
                return;
            ++metrics.searchNodesVisited;
            selected[subarray] = plan;
            demands[subarray] = {plan.usedRows, plan.usedColumns};
            const LedgerAllocationResult allocation = ledger.allocateSequential(
                demands, subarray + 1);
            if (!allocation.success ||
                allocation.transfers.size() > maximumBorrowCount)
            {
                ++metrics.partialAssignmentsPruned;
                demands[subarray] = {};
                continue;
            }
            if (subarray + 1 != kSubarrayCount)
            {
                self(self, subarray + 1);
                demands[subarray] = {};
                continue;
            }

            ++metrics.completeAssignmentsChecked;
            ++metrics.legalCompleteAssignments;
            if (!metrics.firstFeasibleNodeIndex.has_value())
                metrics.firstFeasibleNodeIndex = metrics.completeAssignmentsChecked;
            result.success = true;
            result.plans = selected;
            result.allocation = allocation;
            result.candidatesChecked = metrics.searchNodesVisited - 1;
            result.feasibleCombinations = metrics.legalCompleteAssignments;
            metrics.stoppedAtFirstLegal = true;
            metrics.terminationReason = "FIRST_LEGAL_COMPLETE_TUPLE";
            stop = true;
            demands[subarray] = {};
            return;
        }
    };
    visit(visit, 0);

    if (!result.success && metrics.terminationReason.empty())
        metrics.terminationReason = "NO_LEGAL_COMPLETE_TUPLE";
    metrics.runtimeMicroseconds = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - started).count());
    result.globalSearchMetrics = metrics;

    if (!result.success)
        return result;

    // Capture selected V2 slot/configuration identity and exact sequential
    // ledger states for replay/debugging.  Slot order is the stable V2 slot
    // order (0,1,2,3), not numeric ConfigID order.
    std::array<SpareDemand, kSubarrayCount> committedDemands{};
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const auto mappings = v2RoleSlotMappings(configContract, subarray);
        const CandidatePlan &plan = result.plans[subarray];
        if (plan.attemptVectorIndex >= mappings.size())
            throw std::logic_error("V2 GLOBAL candidate has an invalid role slot");
        const V2RoleSlotMapping &mapping = mappings[plan.attemptVectorIndex];
        const LedgerAllocationResult before = ledger.allocateSequential(
            committedDemands, subarray);
        committedDemands[subarray] = {plan.usedRows, plan.usedColumns};
        const LedgerAllocationResult after = ledger.allocateSequential(
            committedDemands, subarray + 1);
        if (!after.success || after.transfers.size() > maximumBorrowCount)
            throw std::logic_error("Selected V2 GLOBAL tuple lost ledger legality");

        GroupRepairResult::V2DecisionTrace trace;
        trace.role = mapping.role;
        trace.subarray = subarray;
        trace.roleSlot = mapping.roleSlot;
        trace.configId = mapping.configId;
        trace.action = mapping.action;
        trace.configFeasible = true;
        trace.smallestPatternId = plan.solutionId + 1;
        trace.requiredRows = plan.usedRows;
        trace.requiredColumns = plan.usedColumns;
        trace.ledgerValid = true;
        trace.selected = true;
        trace.ledgerOwnersBefore = ledgerOwners(before);
        trace.ledgerOwnersAfter = ledgerOwners(after);
        result.trace.push_back(std::move(trace));
        result.configIds[subarray] = mapping.configId;
        result.actions[subarray] = mapping.action;
        result.remainingResourcesAfterTile[subarray] =
            RemainingSpareResources{after.unusedRows, after.unusedColumns};
    }
    return result;
}

struct CanonicalDirectionalState
{
    std::uint8_t releasedMask = 0;
    std::uint8_t usedMask = 0;
    std::uint8_t releaseRequirementMask = 0;
    std::size_t borrowCount = 0;
};

constexpr bool releasesResource(V2GroupAction action) noexcept
{
    return action == V2GroupAction::ReleaseOnly ||
        action == V2GroupAction::ReleaseAndBorrow;
}

// Resource IDs are the frozen PhysicalResourceLedger directional lines:
// A_ROW=0, D_ROW=1, B_COL=2, C_COL=3.  The corresponding legal borrower is
// C, B, A, D.  This preserves the simulator's physical topology authority;
// the array order is also the deterministic donor order for this topology.
constexpr std::array<std::size_t, kSubarrayCount> kReleaseResource{{0, 2, 3, 1}};
constexpr std::array<std::size_t, kSubarrayCount> kBorrowResource{{2, 1, 0, 3}};
constexpr std::array<std::size_t, kSubarrayCount> kResourceOwner{{0, 3, 1, 2}};

bool advanceCanonicalDirectionalState(
    CanonicalDirectionalState &state,
    std::size_t subarray,
    V2GroupAction action,
    bool candidateActuallyReleases,
    bool candidateActuallyBorrows,
    std::size_t maximumBorrowCount)
{
    const std::size_t releaseResource = kReleaseResource[subarray];
    const std::uint8_t releaseBit = static_cast<std::uint8_t>(1U << releaseResource);
    const bool explicitRelease = releasesResource(action);
    if ((state.releaseRequirementMask & releaseBit) != 0 && !explicitRelease)
        return false;
    if (candidateActuallyReleases)
    {
        state.releasedMask |= releaseBit;
        if (explicitRelease)
            state.releaseRequirementMask &= static_cast<std::uint8_t>(~releaseBit);
    }

    if (!candidateActuallyBorrows)
        return true;
    if (state.borrowCount == maximumBorrowCount)
        return false;
    const std::size_t donorResource = kBorrowResource[subarray];
    const std::uint8_t donorBit = static_cast<std::uint8_t>(1U << donorResource);
    if ((state.usedMask & donorBit) != 0)
        return false;
    if ((state.releasedMask & donorBit) == 0)
    {
        if (kResourceOwner[donorResource] <= subarray)
            return false;
        state.releaseRequirementMask |= donorBit;
    }
    state.usedMask |= donorBit;
    ++state.borrowCount;
    return true;
}

std::vector<CandidatePlan> canonicalDirectionalPlansForSubarray(
    const std::vector<RepairAttemptResult> &attempts,
    ConfigContractVersion contract,
    std::size_t subarray)
{
    const std::vector<CandidatePlan> raw = compressedPlansForSubarray(attempts);
    std::vector<CandidatePlan> ordered;
    for (const V2RoleSlotMapping &mapping :
         v2RtlGroupRoleSlotMappings(contract, subarray))
    {
        for (const CandidatePlan &plan : raw)
        {
            if (plan.attemptVectorIndex == mapping.roleSlot)
                ordered.push_back(plan);
        }
    }
    return ordered;
}

// Canonical GLOBAL keeps the historical oracle for provenance and implements
// the hardware-facing tuple contract separately: R,L,RB,B, PatternID ascending,
// bounded A->B->C->D DFS, and exact future-owner release obligations.
GroupChoice findDirectionalV2GroupGlobalCanonicalChoice(
    const std::array<std::vector<RepairAttemptResult>, kSubarrayCount> &attempts,
    std::size_t maximumBorrowCount,
    const PhysicalResourceLedger &ledger,
    ConfigContractVersion configContract)
{
    const auto started = std::chrono::steady_clock::now();
    std::array<std::vector<CandidatePlan>, kSubarrayCount> plans;
    GlobalSearchMetrics metrics;
    metrics.searchNodesVisited = 1;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        plans[subarray] = canonicalDirectionalPlansForSubarray(
            attempts[subarray], configContract, subarray);
        metrics.candidateCounts[subarray] = plans[subarray].size();
        if (plans[subarray].empty())
        {
            metrics.terminationReason = "EMPTY_V2_LOCAL_CANDIDATE_SET";
            GroupChoice failure;
            failure.hasProposal = true;
            failure.failureSubarray = subarray;
            failure.globalSearchMetrics = metrics;
            return failure;
        }
        metrics.rawCartesianProductSize = subarray == 0
            ? plans[subarray].size()
            : checkedMultiply(metrics.rawCartesianProductSize,
                              plans[subarray].size(),
                              "Canonical V2 GLOBAL Cartesian-product overflow");
    }

    GroupChoice result;
    result.hasProposal = true;
    std::array<CandidatePlan, kSubarrayCount> selected;
    std::array<SpareDemand, kSubarrayCount> demands{};
    bool stop = false;
    const auto visit = [&](const auto &self,
                           std::size_t subarray,
                           CanonicalDirectionalState state) -> void
    {
        if (stop || subarray == kSubarrayCount)
            return;
        const auto mappings = v2RoleSlotMappings(configContract, subarray);
        for (const CandidatePlan &plan : plans[subarray])
        {
            if (stop)
                return;
            ++metrics.searchNodesVisited;
            if (plan.attemptVectorIndex >= mappings.size())
                throw std::logic_error("Canonical GLOBAL role slot is invalid");
            CanonicalDirectionalState next = state;
            const V2GroupAction action = mappings[plan.attemptVectorIndex].action;
            const std::size_t localCapacity =
                configContract == ConfigContractVersion::FrozenDate2x2M1 ? 2 : 3;
            const bool actualRelease = subarray == 0 || subarray == 3
                ? plan.usedRows < localCapacity
                : plan.usedColumns < localCapacity;
            const bool actualBorrow = subarray == 0 || subarray == 3
                ? plan.usedColumns > localCapacity
                : plan.usedRows > localCapacity;
            if (!advanceCanonicalDirectionalState(
                    next, subarray, action, actualRelease, actualBorrow,
                    maximumBorrowCount))
            {
                ++metrics.partialAssignmentsPruned;
                continue;
            }
            selected[subarray] = plan;
            demands[subarray] = {plan.usedRows, plan.usedColumns};
            if (subarray + 1 != kSubarrayCount)
            {
                self(self, subarray + 1, next);
                demands[subarray] = {};
                continue;
            }
            ++metrics.completeAssignmentsChecked;
            if (next.releaseRequirementMask != 0)
            {
                ++metrics.partialAssignmentsPruned;
                demands[subarray] = {};
                continue;
            }
            const LedgerAllocationResult allocation = ledger.allocateSequential(
                demands, kSubarrayCount);
            if (!allocation.success ||
                allocation.transfers.size() > maximumBorrowCount)
            {
                ++metrics.partialAssignmentsPruned;
                demands[subarray] = {};
                continue;
            }
            ++metrics.legalCompleteAssignments;
            metrics.firstFeasibleNodeIndex = metrics.completeAssignmentsChecked;
            result.success = true;
            result.plans = selected;
            result.allocation = allocation;
            result.candidatesChecked = metrics.searchNodesVisited - 1;
            result.feasibleCombinations = metrics.legalCompleteAssignments;
            metrics.stoppedAtFirstLegal = true;
            metrics.terminationReason =
                "FIRST_LEGAL_COMPLETE_TUPLE_WITH_RELEASE_OBLIGATIONS";
            stop = true;
            demands[subarray] = {};
            return;
        }
    };
    visit(visit, 0, CanonicalDirectionalState{});

    if (!result.success && metrics.terminationReason.empty())
        metrics.terminationReason = "NO_LEGAL_COMPLETE_TUPLE";
    metrics.runtimeMicroseconds = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - started).count());
    result.globalSearchMetrics = metrics;
    if (!result.success)
        return result;

    std::array<SpareDemand, kSubarrayCount> committedDemands{};
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const auto mappings = v2RoleSlotMappings(configContract, subarray);
        const CandidatePlan &plan = result.plans[subarray];
        const V2RoleSlotMapping &mapping = mappings[plan.attemptVectorIndex];
        const LedgerAllocationResult before = ledger.allocateSequential(
            committedDemands, subarray);
        committedDemands[subarray] = {plan.usedRows, plan.usedColumns};
        const LedgerAllocationResult after = ledger.allocateSequential(
            committedDemands, subarray + 1);
        GroupRepairResult::V2DecisionTrace trace;
        trace.role = mapping.role;
        trace.subarray = subarray;
        trace.roleSlot = mapping.roleSlot;
        trace.configId = mapping.configId;
        trace.action = mapping.action;
        trace.configFeasible = true;
        trace.smallestPatternId = plan.solutionId + 1;
        trace.requiredRows = plan.usedRows;
        trace.requiredColumns = plan.usedColumns;
        trace.ledgerValid = after.success;
        trace.selected = true;
        trace.ledgerOwnersBefore = ledgerOwners(before);
        trace.ledgerOwnersAfter = ledgerOwners(after);
        result.trace.push_back(std::move(trace));
        result.configIds[subarray] = mapping.configId;
        result.actions[subarray] = mapping.action;
        result.remainingResourcesAfterTile[subarray] =
            RemainingSpareResources{after.unusedRows, after.unusedColumns};
    }
    return result;
}

bool isHyp02StaticPolicy(SolutionTakePolicy policy) noexcept
{
    return policy == SolutionTakePolicy::Hyp02StaticEarly ||
        policy == SolutionTakePolicy::Hyp02StaticGlobal;
}

bool isGrid2x2RowStaticPolicy(SolutionTakePolicy policy) noexcept
{
    return policy == SolutionTakePolicy::Grid2x2RowStaticEarly ||
        policy == SolutionTakePolicy::Grid2x2RowStaticGlobal;
}

bool isLine1x4RowStaticPolicy(SolutionTakePolicy policy) noexcept
{
    return policy == SolutionTakePolicy::Line1x4RowStaticEarly ||
        policy == SolutionTakePolicy::Line1x4RowStaticGlobal;
}

bool hyp02StaticTupleLegal(const std::array<std::size_t, kSubarrayCount> &slots)
{
    const bool aRelease = slots[0] == 1 || slots[0] == 3;
    const bool aBorrow = slots[0] == 2 || slots[0] == 3;
    const bool bRelease = slots[1] == 1 || slots[1] == 3;
    const bool bBorrow = slots[1] == 2 || slots[1] == 3;
    const bool cRelease = slots[2] == 1 || slots[2] == 3;
    const bool cBorrow = slots[2] == 2 || slots[2] == 3;
    const bool dRelease = slots[3] == 1 || slots[3] == 3;
    const bool dBorrow = slots[3] == 2 || slots[3] == 3;
    return (!cBorrow || aRelease) && (!bBorrow || dRelease) &&
        (!aBorrow || bRelease) && (!dBorrow || cRelease);
}

bool hyp02PrefixCompatible(
    const std::array<std::size_t, kSubarrayCount> &selected,
    std::size_t prefixLength,
    std::size_t candidateSlot)
{
    for (std::size_t tuple = 0; tuple < 256; ++tuple)
    {
        const std::array<std::size_t, kSubarrayCount> slots{{
            tuple & 0x3U, (tuple >> 2U) & 0x3U,
            (tuple >> 4U) & 0x3U, (tuple >> 6U) & 0x3U}};
        if (!hyp02StaticTupleLegal(slots) || slots[prefixLength] != candidateSlot)
            continue;
        bool matches = true;
        for (std::size_t subarray = 0; subarray < prefixLength; ++subarray)
            matches = matches && slots[subarray] == selected[subarray];
        if (matches)
            return true;
    }
    return false;
}

using StaticActionConfigMap =
    std::array<std::optional<std::size_t>, 4>;

const StaticActionConfigMap kHyp02ActionToConfig{{0, 1, 2, 3}};
const StaticActionConfigMap kRowCycleActionToConfig{{0, 1, 2, 0}};
const std::array<StaticActionConfigMap, kSubarrayCount> kLineActionToConfig{{
    StaticActionConfigMap{{0, 1, std::nullopt, std::nullopt}},
    StaticActionConfigMap{{0, 1, 2, 0}},
    StaticActionConfigMap{{0, 1, 2, 0}},
    StaticActionConfigMap{{0, std::nullopt, 1, std::nullopt}}}};

const CandidatePlan *hyp02PlanForSlot(
    const std::vector<CandidatePlan> &plans,
    std::size_t slot,
    const StaticActionConfigMap &actionToConfig)
{
    if (!actionToConfig[slot].has_value())
        return nullptr;
    const std::size_t configIndex = *actionToConfig[slot];
    const CandidatePlan *selected = nullptr;
    for (const CandidatePlan &plan : plans)
    {
        if (plan.attemptVectorIndex != configIndex ||
            (selected != nullptr &&
             std::tie(plan.solutionId, plan.attemptVectorIndex) >=
                 std::tie(selected->solutionId, selected->attemptVectorIndex)))
            continue;
        selected = &plan;
    }
    return selected;
}

void captureHyp02Selection(
    GroupChoice &choice,
    const std::array<std::size_t, kSubarrayCount> &slots,
    ConfigContractVersion contract,
    bool rowOnlyCycle)
{
    constexpr std::array<V2GroupAction, 4> actions{{
        V2GroupAction::Local, V2GroupAction::ReleaseOnly,
        V2GroupAction::BorrowOnly, V2GroupAction::ReleaseAndBorrow}};
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const V2RoleSlotMapping fallback =
            v2RoleSlotMappings(contract, subarray).at(slots[subarray]);
        const char role = subarray == 0 ? 'A' :
            (subarray == 1 ? 'B' : (subarray == 2 ? 'C' : 'D'));
        const V2RoleSlotMapping rowMapping{
            role, slots[subarray], slots[subarray] == 1 ? 1 :
                (slots[subarray] == 2 ? 2 : 0), actions[slots[subarray]]};
        const V2RoleSlotMapping &mapping = rowOnlyCycle ? rowMapping : fallback;
        choice.configIds[subarray] = mapping.configId;
        choice.actions[subarray] = mapping.action;
        GroupRepairResult::V2DecisionTrace trace;
        trace.role = mapping.role;
        trace.subarray = subarray;
        trace.roleSlot = mapping.roleSlot;
        trace.configId = mapping.configId;
        trace.action = mapping.action;
        trace.configFeasible = true;
        trace.smallestPatternId = choice.plans[subarray].solutionId + 1;
        trace.requiredRows = choice.plans[subarray].usedRows;
        trace.requiredColumns = choice.plans[subarray].usedColumns;
        trace.ledgerValid = true;
        trace.selected = true;
        choice.trace.push_back(std::move(trace));
    }
}

GroupChoice findHyp02StaticEarlyChoice(
    const std::array<std::vector<RepairAttemptResult>, kSubarrayCount> &attempts,
    ConfigContractVersion contract,
    bool rowOnlyCycle,
    const StaticActionConfigMap &actionToConfig)
{
    constexpr std::array<std::size_t, 4> priority{{1, 0, 3, 2}};
    GroupChoice result;
    result.hasProposal = true;
    std::array<std::size_t, kSubarrayCount> slots{};
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const std::vector<CandidatePlan> plans =
            compressedPlansForSubarray(attempts[subarray]);
        bool selected = false;
        for (const std::size_t slot : priority)
        {
            ++result.candidatesChecked;
            const CandidatePlan *plan = hyp02PlanForSlot(plans, slot, actionToConfig);
            if (plan == nullptr || !hyp02PrefixCompatible(slots, subarray, slot))
                continue;
            slots[subarray] = slot;
            result.plans[subarray] = *plan;
            selected = true;
            break;
        }
        if (!selected)
        {
            result.failureSubarray = subarray;
            return result;
        }
    }
    result.success = true;
    result.feasibleCombinations = 1;
    captureHyp02Selection(result, slots, contract, rowOnlyCycle);
    return result;
}

GroupChoice findHyp02StaticGlobalChoice(
    const std::array<std::vector<RepairAttemptResult>, kSubarrayCount> &attempts,
    ConfigContractVersion contract,
    bool rowOnlyCycle,
    const StaticActionConfigMap &actionToConfig)
{
    GroupChoice result;
    result.hasProposal = true;
    std::array<std::vector<CandidatePlan>, kSubarrayCount> plans;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
        plans[subarray] = compressedPlansForSubarray(attempts[subarray]);
    for (std::size_t tuple = 0; tuple < 256; ++tuple)
    {
        const std::array<std::size_t, kSubarrayCount> slots{{
            tuple & 0x3U, (tuple >> 2U) & 0x3U,
            (tuple >> 4U) & 0x3U, (tuple >> 6U) & 0x3U}};
        if (!hyp02StaticTupleLegal(slots))
            continue;
        ++result.candidatesChecked;
        bool allValid = true;
        for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
        {
            const CandidatePlan *plan = hyp02PlanForSlot(plans[subarray], slots[subarray], actionToConfig);
            if (plan == nullptr)
            {
                allValid = false;
                break;
            }
            result.plans[subarray] = *plan;
        }
        if (!allValid)
            continue;
        result.success = true;
        result.feasibleCombinations = 1;
        captureHyp02Selection(result, slots, contract, rowOnlyCycle);
        return result;
    }
    return result;
}

bool lineSlotsLegal(const std::array<std::size_t, kSubarrayCount> &slots)
{
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
        if (!kLineActionToConfig[subarray][slots[subarray]].has_value())
            return false;
    const bool aRelease = slots[0] == 1;
    const bool bRelease = slots[1] == 1 || slots[1] == 3;
    const bool bBorrow = slots[1] == 2 || slots[1] == 3;
    const bool cRelease = slots[2] == 1 || slots[2] == 3;
    const bool cBorrow = slots[2] == 2 || slots[2] == 3;
    const bool dBorrow = slots[3] == 2;
    return (!bBorrow || aRelease) && (!cBorrow || bRelease) &&
        (!dBorrow || cRelease);
}

bool linePrefixCompatible(
    const std::array<std::size_t, kSubarrayCount> &selected,
    std::size_t prefixLength, std::size_t candidateSlot)
{
    for (std::size_t tuple = 0; tuple < 256; ++tuple)
    {
        const std::array<std::size_t, kSubarrayCount> slots{{
            tuple & 0x3U, (tuple >> 2U) & 0x3U,
            (tuple >> 4U) & 0x3U, (tuple >> 6U) & 0x3U}};
        if (!lineSlotsLegal(slots) || slots[prefixLength] != candidateSlot)
            continue;
        bool matches = true;
        for (std::size_t subarray = 0; subarray < prefixLength; ++subarray)
            matches = matches && slots[subarray] == selected[subarray];
        if (matches) return true;
    }
    return false;
}

void captureLineSelection(GroupChoice &choice,
                          const std::array<std::size_t, kSubarrayCount> &slots)
{
    constexpr std::array<V2GroupAction, 4> actions{{
        V2GroupAction::Local, V2GroupAction::ReleaseOnly,
        V2GroupAction::BorrowOnly, V2GroupAction::ReleaseAndBorrow}};
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const char role = subarray == 0 ? 'A' :
            (subarray == 1 ? 'B' : (subarray == 2 ? 'C' : 'D'));
        const std::size_t slot = slots[subarray];
        const int configId = slot == 1 ? 1 : (slot == 2 ? 2 : 0);
        choice.configIds[subarray] = configId;
        choice.actions[subarray] = actions[slot];
        GroupRepairResult::V2DecisionTrace trace;
        trace.role = role; trace.subarray = subarray; trace.roleSlot = slot;
        trace.configId = configId; trace.action = actions[slot];
        trace.configFeasible = true; trace.ledgerValid = true; trace.selected = true;
        trace.smallestPatternId = choice.plans[subarray].solutionId + 1;
        trace.requiredRows = choice.plans[subarray].usedRows;
        trace.requiredColumns = choice.plans[subarray].usedColumns;
        choice.trace.push_back(std::move(trace));
    }
}

GroupChoice findLineStaticEarlyChoice(
    const std::array<std::vector<RepairAttemptResult>, kSubarrayCount> &attempts)
{
    constexpr std::array<std::size_t, 4> priority{{1, 0, 3, 2}};
    GroupChoice result; result.hasProposal = true;
    std::array<std::size_t, kSubarrayCount> slots{};
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const auto plans = compressedPlansForSubarray(attempts[subarray]);
        bool selected = false;
        for (const std::size_t slot : priority)
        {
            ++result.candidatesChecked;
            const CandidatePlan *plan = hyp02PlanForSlot(
                plans, slot, kLineActionToConfig[subarray]);
            if (plan == nullptr || !linePrefixCompatible(slots, subarray, slot))
                continue;
            slots[subarray] = slot; result.plans[subarray] = *plan;
            selected = true; break;
        }
        if (!selected) { result.failureSubarray = subarray; return result; }
    }
    result.success = true; result.feasibleCombinations = 1;
    captureLineSelection(result, slots); return result;
}

GroupChoice findLineStaticGlobalChoice(
    const std::array<std::vector<RepairAttemptResult>, kSubarrayCount> &attempts)
{
    GroupChoice result; result.hasProposal = true;
    std::array<std::vector<CandidatePlan>, kSubarrayCount> plans;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
        plans[subarray] = compressedPlansForSubarray(attempts[subarray]);
    for (std::size_t tuple = 0; tuple < 256; ++tuple)
    {
        const std::array<std::size_t, kSubarrayCount> slots{{
            tuple & 0x3U, (tuple >> 2U) & 0x3U,
            (tuple >> 4U) & 0x3U, (tuple >> 6U) & 0x3U}};
        if (!lineSlotsLegal(slots)) continue;
        ++result.candidatesChecked; bool allValid = true;
        for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
        {
            const CandidatePlan *plan = hyp02PlanForSlot(
                plans[subarray], slots[subarray], kLineActionToConfig[subarray]);
            if (plan == nullptr) { allValid = false; break; }
            result.plans[subarray] = *plan;
        }
        if (!allValid) continue;
        result.success = true; result.feasibleCombinations = 1;
        captureLineSelection(result, slots); return result;
    }
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
    group.selectedConfigIds = choice.configIds;
    group.selectedV2Actions = choice.actions;
    group.firstFailureSubarray = choice.failureSubarray;
    group.v2DecisionTrace = choice.trace;
    group.globalSearchMetrics = choice.globalSearchMetrics;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        if (choice.configIds[subarray].has_value())
            group.selectedPatternIds[subarray] =
                choice.plans[subarray].solutionId + 1;
    }
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
        if (config.solutionTakePolicy == SolutionTakePolicy::Early ||
            config.solutionTakePolicy == SolutionTakePolicy::LocalFirst ||
            config.solutionTakePolicy == SolutionTakePolicy::DirectionalV2Early ||
            config.solutionTakePolicy == SolutionTakePolicy::Hyp02StaticEarly ||
            config.solutionTakePolicy == SolutionTakePolicy::Line1x4RowStaticEarly ||
            config.solutionTakePolicy == SolutionTakePolicy::GroupNoScratchV2 ||
            config.solutionTakePolicy ==
                SolutionTakePolicy::GroupGreedyRtlCanonical ||
            config.solutionTakePolicy ==
                SolutionTakePolicy::OneByFourTwoPairwiseEarlyV1 ||
            config.solutionTakePolicy ==
                SolutionTakePolicy::OneByFourTwoPairwiseReleaseAwareEarlyV1 ||
            config.solutionTakePolicy ==
                SolutionTakePolicy::OneByFourSingleHopEarlyV1 ||
            config.solutionTakePolicy ==
                SolutionTakePolicy::OneByFourSingleHopReleaseAwareEarlyV1)
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
    group.finalLedgerOwners = ledgerOwners(choice.allocation);
    for (const BorrowTransfer &transfer : choice.allocation.transfers)
    {
        group.selectedBorrowTransfers.push_back({
            transfer.donorSubarray,
            transfer.borrowerSubarray,
            transfer.dimension,
            transfer.physicalLineId});
    }
    for (const PhysicalSpareLine &line : choice.allocation.lines)
    {
        if (line.dimension == SpareDimension::Column)
        {
            if (line.assignedSubarray.has_value())
                ++group.localColumnUsed;
            continue;
        }
        if (!line.shareable)
        {
            if (line.assignedSubarray.has_value())
                ++group.localRowUsed;
        }
        else if (!line.assignedSubarray.has_value())
        {
            ++group.remainingShareableRows;
        }
        else if (line.ownerSubarray == static_cast<int>(*line.assignedSubarray))
        {
            ++group.ownShareableRowUsed;
        }
        else
        {
            ++group.borrowedShareableRowUsed;
        }
    }
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

bool isTwoPairwisePolicy(SolutionTakePolicy policy) noexcept
{
    return policy == SolutionTakePolicy::OneByFourTwoPairwiseEarlyV1 ||
        policy == SolutionTakePolicy::OneByFourTwoPairwisePairGlobalV1 ||
        policy == SolutionTakePolicy::OneByFourTwoPairwiseReleaseAwareEarlyV1;
}

bool isSingleHopPolicy(SolutionTakePolicy policy) noexcept
{
    return policy == SolutionTakePolicy::OneByFourSingleHopEarlyV1 ||
        policy == SolutionTakePolicy::OneByFourSingleHopGlobalV1 ||
        policy == SolutionTakePolicy::OneByFourSingleHopReleaseAwareEarlyV1;
}

void validateOneByFourPolicy(const SimulationConfig &config)
{
    if (isTwoPairwisePolicy(config.solutionTakePolicy) &&
        (config.layout != GroupLayout::Line1x4 ||
         (config.topology != SharingTopology::PairSharing &&
          config.topology != SharingTopology::NoSharing) ||
         config.sharedColumns != 0))
    {
        throw std::invalid_argument(
            "Two-Pairwise policies require layout=1x4, topology=pair, "
            "and shared_columns=0");
    }
    if (isSingleHopPolicy(config.solutionTakePolicy) &&
        (config.layout != GroupLayout::Line1x4 ||
         (config.topology != SharingTopology::NeighborSharing &&
          config.topology != SharingTopology::NoSharing) ||
         config.sharedColumns != 0))
    {
        throw std::invalid_argument(
            "Single-Hop policies require layout=1x4, topology=neighbor, "
            "and shared_columns=0");
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
    validateOneByFourPolicy(config);
    if (isGrid2x2RowStaticPolicy(config.solutionTakePolicy) &&
        (config.layout != GroupLayout::Grid2x2 ||
         config.topology != SharingTopology::Directional ||
         config.sharedRows != 1 || config.sharedColumns != 0))
    {
        throw std::invalid_argument(
            "G2X2_R static policies require 2x2 directional row-only m=1; got layout=" +
            std::string(toString(config.layout)) + ", topology=" +
            std::string(toString(config.topology)) + ", shared_rows=" +
            std::to_string(config.sharedRows) + ", shared_columns=" +
            std::to_string(config.sharedColumns));
    }
    if (isLine1x4RowStaticPolicy(config.solutionTakePolicy) &&
        (config.layout != GroupLayout::Line1x4 ||
         config.topology != SharingTopology::NeighborSharing ||
         config.sharedRows != 1 || config.sharedColumns != 0))
    {
        throw std::invalid_argument(
            "L1X4_R static policies require 1x4 neighbor row-only m=1");
    }
    PhysicalResourceLedger ledger(config);

    GroupRepairResult group;
    group.seed = config.randomSeed;
    group.runIndex = runIndex;
    group.faultCountModel = config.faultCountModel;
    group.layout = config.layout;
    group.topology = config.topology;
    group.solutionTakePolicy = config.solutionTakePolicy;
    group.privateRowCountPerSubarray = static_cast<std::size_t>(
        config.layout == GroupLayout::Line1x4
            ? config.spareRows - config.sharedRows : config.spareRows);
    group.privateColumnCountPerSubarray = static_cast<std::size_t>(
        config.spareColumns);
    group.shareableRowCountPerSubarray = static_cast<std::size_t>(
        config.layout == GroupLayout::Line1x4 ? config.sharedRows : 0);
    group.totalRowSpareLinesGroup = kSubarrayCount *
        static_cast<std::size_t>(config.spareRows);
    group.totalColumnSpareLinesGroup = kSubarrayCount *
        static_cast<std::size_t>(config.spareColumns);
    group.totalPhysicalSpareLinesGroup = group.totalRowSpareLinesGroup +
        group.totalColumnSpareLinesGroup;
    if (config.solutionTakePolicy == SolutionTakePolicy::DirectionalV2Early ||
        config.solutionTakePolicy == SolutionTakePolicy::GroupGreedyRtlCanonical ||
        config.solutionTakePolicy == SolutionTakePolicy::DirectionalV2GroupGlobal ||
        config.solutionTakePolicy ==
            SolutionTakePolicy::DirectionalV2GroupGlobalCanonical ||
        isHyp02StaticPolicy(config.solutionTakePolicy))
    {
        group.configContractVersion = canonicalV2ConfigContract(config);
    }
    else if (config.solutionTakePolicy == SolutionTakePolicy::GroupNoScratchV2)
    {
        group.configContractVersion =
            ConfigContractVersion::HistoricalCppV2SlotMapV1;
    }

    std::array<std::vector<CapacityOption>, kSubarrayCount> options;
    std::array<std::pair<int, int>, kSubarrayCount> provisioned;
    std::array<ProvisioningMetrics, kSubarrayCount> hardwareProvisioning;
    for (std::size_t subarray = 0;
         subarray < kSubarrayCount; ++subarray)
    {
        options[subarray] = isLine1x4RowStaticPolicy(config.solutionTakePolicy)
            ? lineRowCapacityOptions(config, subarray)
            : (isGrid2x2RowStaticPolicy(config.solutionTakePolicy)
                ? rowCycleCapacityOptions(config, subarray)
                : ((config.solutionTakePolicy == SolutionTakePolicy::GroupNoScratchV2 ||
                config.solutionTakePolicy == SolutionTakePolicy::DirectionalV2Early ||
                config.solutionTakePolicy ==
                    SolutionTakePolicy::GroupGreedyRtlCanonical ||
                config.solutionTakePolicy ==
                    SolutionTakePolicy::DirectionalV2GroupGlobal ||
                config.solutionTakePolicy ==
                    SolutionTakePolicy::DirectionalV2GroupGlobalCanonical ||
                isHyp02StaticPolicy(config.solutionTakePolicy))
                ? v2CapacityOptions(config, subarray)
                : capacityOptions(config, subarray)));
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
        const bool useSharedCollectorModel =
            config.solutionTakePolicy == SolutionTakePolicy::DirectionalV2Early;
        if (useSharedCollectorModel &&
            !group.attemptsBySubarray[subarray].empty())
        {
            // The first call for this SA produces all four Config projections.
            // Later scheduling loops retain their legacy shape but must not
            // recollect the fault stream for an individual Config.
            return;
        }

        const auto finalizeAttempt = [&](RepairAttemptResult attempt)
        {
            attempt.addressCamEntriesProvisioned = useSharedCollectorModel
                ? kDirectionalSharedCollectorPivots
                : hardwareProvisioning[subarray].addressCamEntries;
            attempt.hybridCamEntriesProvisioned = useSharedCollectorModel
                ? kDirectionalSharedCollectorHybridEntries
                : hardwareProvisioning[subarray].hybridCamEntries;
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

        if (useSharedCollectorModel)
        {
            std::vector<RECAMSolverRequest> requests;
            requests.reserve(options[subarray].size());
            for (std::size_t optionIndex = 0;
                 optionIndex < options[subarray].size(); ++optionIndex)
            {
                const CapacityOption &option = options[subarray][optionIndex];
                RECAMSolverRequest request;
                request.runIndex = runIndex;
                request.subarrayId = static_cast<int>(subarray);
                request.availableRows = option.rows;
                request.availableColumns = option.columns;
                request.bufferCamEntries = activeBufferEntries;
                request.provisionedRows = provisioned[subarray].first;
                request.provisionedColumns = provisioned[subarray].second;
                request.stage = option.stage();
                request.attemptIndex = optionIndex;
                request.hybridCamEntryWidthBits = config.hybridCamEntryWidthBits;
                request.rowAddressWidthBits = config.rowAddressWidthBits;
                request.columnAddressWidthBits = config.columnAddressWidthBits;
                requests.push_back(request);
            }
            for (RepairAttemptResult attempt :
                 solveSharedCollectorConfigSet(faults[subarray], requests))
            {
                finalizeAttempt(std::move(attempt));
            }
            return;
        }

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
        finalizeAttempt(std::move(attempt));
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
        const GroupChoice oneByFourEarly =
            (isTwoPairwisePolicy(config.solutionTakePolicy) ||
             isSingleHopPolicy(config.solutionTakePolicy))
                ? findOneByFourEarlyChoice(
                      group.attemptsBySubarray, maximumBorrowCount, ledger)
                : GroupChoice{};
        const bool compressedChoiceRequired =
            !isTwoPairwisePolicy(config.solutionTakePolicy) &&
            !(config.solutionTakePolicy ==
                  SolutionTakePolicy::OneByFourSingleHopEarlyV1);
        // Two-Pairwise has a pair-scoped global policy, and Single-Hop EARLY
        // commits without global search.  Computing the generic four-SA
        // compressed tuple in either case cannot affect the selected policy;
        // avoid turning an EARLY preflight into an unreported global sweep.
        const GroupChoice compressed = compressedChoiceRequired
            ? findCompressedGroupChoice(
                  group.attemptsBySubarray, maximumBorrowCount, rowOnly,
                  ledger)
            : GroupChoice{};
        const GroupChoice pairGlobal =
            config.solutionTakePolicy ==
                    SolutionTakePolicy::OneByFourTwoPairwisePairGlobalV1
                ? findPairGlobalChoice(
                      group.attemptsBySubarray, maximumBorrowCount, ledger)
                : GroupChoice{};
        // This dominance assertion belongs to the generic 2x2 C1/C3
        // comparison.  The 1x4 policies have their own versioned EARLY
        // priorities and pair/single-hop global scopes; comparing them to
        // the generic selector would reject a legal 1x4 run before its
        // selected policy is applied.
        if (config.layout == GroupLayout::Grid2x2 &&
            early.success && !compressed.success)
        {
            throw std::logic_error(
                "GROUP_COMPRESSED rejected a feasible EARLY selection");
        }
        group.earlySuccess = early.success;
        group.groupCompressedSuccess = compressed.success;
        group.greedyLoss = !early.success && compressed.success;
        const GroupChoice v2 =
            (config.solutionTakePolicy == SolutionTakePolicy::GroupNoScratchV2 ||
             config.solutionTakePolicy == SolutionTakePolicy::DirectionalV2Early ||
             config.solutionTakePolicy ==
                 SolutionTakePolicy::GroupGreedyRtlCanonical)
            ? findV2GroupNoScratchChoice(
                  group.attemptsBySubarray, maximumBorrowCount, ledger,
                  config.solutionTakePolicy ==
                          SolutionTakePolicy::GroupGreedyRtlCanonical ||
                      config.solutionTakePolicy ==
                          SolutionTakePolicy::DirectionalV2Early,
                  group.configContractVersion)
            : GroupChoice{};
        const GroupChoice v2Global =
            config.solutionTakePolicy ==
                    SolutionTakePolicy::DirectionalV2GroupGlobal
                ? findDirectionalV2GroupGlobalChoice(
                      group.attemptsBySubarray, maximumBorrowCount, ledger,
                      group.configContractVersion)
                : GroupChoice{};
        const GroupChoice canonicalV2Global =
            config.solutionTakePolicy ==
                    SolutionTakePolicy::DirectionalV2GroupGlobalCanonical
                ? findDirectionalV2GroupGlobalCanonicalChoice(
                      group.attemptsBySubarray, maximumBorrowCount, ledger,
                      group.configContractVersion)
                : GroupChoice{};
        const GroupChoice hyp02StaticEarly =
            config.solutionTakePolicy == SolutionTakePolicy::Hyp02StaticEarly
                ? findHyp02StaticEarlyChoice(
                      group.attemptsBySubarray, group.configContractVersion, false, kHyp02ActionToConfig)
                : GroupChoice{};
        const GroupChoice hyp02StaticGlobal =
            config.solutionTakePolicy == SolutionTakePolicy::Hyp02StaticGlobal
                ? findHyp02StaticGlobalChoice(
                      group.attemptsBySubarray, group.configContractVersion, false, kHyp02ActionToConfig)
                : GroupChoice{};
        const GroupChoice grid2x2RowStaticEarly =
            config.solutionTakePolicy == SolutionTakePolicy::Grid2x2RowStaticEarly
                ? findHyp02StaticEarlyChoice(
                      group.attemptsBySubarray, group.configContractVersion, true, kRowCycleActionToConfig)
                : GroupChoice{};
        const GroupChoice grid2x2RowStaticGlobal =
            config.solutionTakePolicy == SolutionTakePolicy::Grid2x2RowStaticGlobal
                ? findHyp02StaticGlobalChoice(
                      group.attemptsBySubarray, group.configContractVersion, true, kRowCycleActionToConfig)
                : GroupChoice{};
        const GroupChoice line1x4RowStaticEarly =
            config.solutionTakePolicy == SolutionTakePolicy::Line1x4RowStaticEarly
                ? findLineStaticEarlyChoice(group.attemptsBySubarray)
                : GroupChoice{};
        const GroupChoice line1x4RowStaticGlobal =
            config.solutionTakePolicy == SolutionTakePolicy::Line1x4RowStaticGlobal
                ? findLineStaticGlobalChoice(group.attemptsBySubarray)
                : GroupChoice{};
        if (config.solutionTakePolicy == SolutionTakePolicy::Early)
            best = early;
        else if (config.solutionTakePolicy == SolutionTakePolicy::LocalFirst)
            best = findOneByFourEarlyChoice(
                group.attemptsBySubarray, maximumBorrowCount, ledger);
        else if (config.solutionTakePolicy == SolutionTakePolicy::GroupNoScratchV2 ||
                 config.solutionTakePolicy == SolutionTakePolicy::DirectionalV2Early ||
                 config.solutionTakePolicy ==
                 SolutionTakePolicy::GroupGreedyRtlCanonical)
            best = v2;
        else if (config.solutionTakePolicy ==
                 SolutionTakePolicy::DirectionalV2GroupGlobal)
            best = v2Global;
        else if (config.solutionTakePolicy ==
                 SolutionTakePolicy::DirectionalV2GroupGlobalCanonical)
            best = canonicalV2Global;
        else if (config.solutionTakePolicy == SolutionTakePolicy::Hyp02StaticEarly)
            best = hyp02StaticEarly;
        else if (config.solutionTakePolicy == SolutionTakePolicy::Hyp02StaticGlobal)
            best = hyp02StaticGlobal;
        else if (config.solutionTakePolicy == SolutionTakePolicy::Grid2x2RowStaticEarly)
            best = grid2x2RowStaticEarly;
        else if (config.solutionTakePolicy == SolutionTakePolicy::Grid2x2RowStaticGlobal)
            best = grid2x2RowStaticGlobal;
        else if (config.solutionTakePolicy == SolutionTakePolicy::Line1x4RowStaticEarly)
            best = line1x4RowStaticEarly;
        else if (config.solutionTakePolicy == SolutionTakePolicy::Line1x4RowStaticGlobal)
            best = line1x4RowStaticGlobal;
        else if (config.solutionTakePolicy ==
                 SolutionTakePolicy::OneByFourTwoPairwiseEarlyV1 ||
                 config.solutionTakePolicy ==
                     SolutionTakePolicy::OneByFourTwoPairwiseReleaseAwareEarlyV1 ||
                 config.solutionTakePolicy ==
                     SolutionTakePolicy::OneByFourSingleHopEarlyV1 ||
                 config.solutionTakePolicy ==
                     SolutionTakePolicy::OneByFourSingleHopReleaseAwareEarlyV1)
            best = (config.solutionTakePolicy ==
                        SolutionTakePolicy::OneByFourTwoPairwiseReleaseAwareEarlyV1 ||
                    config.solutionTakePolicy ==
                        SolutionTakePolicy::OneByFourSingleHopReleaseAwareEarlyV1)
                ? early
                : oneByFourEarly;
        else if (config.solutionTakePolicy ==
                 SolutionTakePolicy::OneByFourTwoPairwisePairGlobalV1)
            best = pairGlobal;
        else
            best = compressed;
        if (!best.success)
        {
            group.solutionSelectionFailureReason =
                (config.solutionTakePolicy == SolutionTakePolicy::GroupCompressed ||
                 config.solutionTakePolicy == SolutionTakePolicy::GroupGlobal ||
                 config.solutionTakePolicy ==
                     SolutionTakePolicy::DirectionalV2GroupGlobal ||
                 config.solutionTakePolicy ==
                     SolutionTakePolicy::DirectionalV2GroupGlobalCanonical ||
                 config.solutionTakePolicy == SolutionTakePolicy::Hyp02StaticGlobal ||
                 config.solutionTakePolicy == SolutionTakePolicy::Grid2x2RowStaticGlobal ||
                 config.solutionTakePolicy == SolutionTakePolicy::Line1x4RowStaticGlobal ||
                 config.solutionTakePolicy ==
                     SolutionTakePolicy::OneByFourTwoPairwisePairGlobalV1 ||
                 config.solutionTakePolicy ==
                     SolutionTakePolicy::OneByFourSingleHopGlobalV1)
                    ? "NO_FEASIBLE_GROUP_COMBINATION"
                    : "NO_FEASIBLE_SOLUTION_AFTER_PRIOR_COMMIT";
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
