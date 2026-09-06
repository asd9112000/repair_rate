#ifndef DYNAMIC_SPARE_SHARING_REPAIR_RESULT_HPP
#define DYNAMIC_SPARE_SHARING_REPAIR_RESULT_HPP

#include "BiraLatency.hpp"
#include "HardwareMetrics.hpp"
#include "SimulationConfig.hpp"
#include "SramRecamModel.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace dynamic_spare
{

enum class SpareDimension
{
    Row,
    Column
};

struct RepairLineMapping
{
    int HBMID = -1;
    int ChannelID = -1;
    int BankID = -1;
    int SubarrayGroupID = -1;
    int SubarrayID = -1;
    int sourceRow = -1;
    int sourceColumn = -1;
    SpareDimension dimension = SpareDimension::Row;
    int originalAddress = -1;
    int replacementAddress = -1;
    int latency = 0;
};

// Exact-address repair served by the additional-pivot CAM-reuse buffer.
// Unlike RepairLineMapping, this entry consumes no spare row or column.
struct BufferRepairMapping
{
    int HBMID = -1;
    int ChannelID = -1;
    int BankID = -1;
    int SubarrayGroupID = -1;
    int SubarrayID = -1;
    int sourceRow = -1;
    int sourceColumn = -1;
    int latency = 0;
};

// Pointer-free physical-cell address retained after RECAM scratch is cleared.
// An absent optional slot preserves the matrix's unused-address semantics.
struct MatrixRepairAddress
{
    int HBMID = -1;
    int ChannelID = -1;
    int BankID = -1;
    int SubarrayGroupID = -1;
    int SubarrayID = -1;
    int row = -1;
    int column = -1;
};

struct TileSolutionState
{
    int subarrayId = -1;
    int spareRows = 0;
    int spareColumns = 0;
    std::vector<std::optional<MatrixRepairAddress>> matrixRowAddresses;
    std::vector<std::optional<MatrixRepairAddress>> matrixColumnAddresses;
    std::vector<bool> validSolutionBitmap;
    std::vector<BufferRepairMapping> camReuseMappings;
    std::uint64_t compressedStorageBits = 0;
};

struct DecodedSolution
{
    std::size_t solutionId = 0;
    std::vector<MatrixRepairAddress> sourceRows;
    std::vector<MatrixRepairAddress> sourceColumns;
};

struct RemainingSpareResources
{
    std::size_t rows = 0;
    std::size_t columns = 0;
};

// Value-only snapshot for one matrix-valid paper candidate.  Keeping every
// option is necessary because a later group allocator may prefer a candidate
// that uses fewer borrowed physical lines than validSolList.front().
struct CandidateRepairOption
{
    std::size_t candidateIndex = 0;
    std::size_t usedRows = 0;
    std::size_t usedColumns = 0;
    std::size_t bufferCamRemapCount = 0;
    std::vector<RepairLineMapping> mappings;
    std::vector<BufferRepairMapping> bufferMappings;
};

struct AnalysisLatencyBreakdown
{
    std::uint64_t faultInformationInsertCycles = 0;
    std::uint64_t faultInformationLookupCycles = 0;
    std::uint64_t faultInformationReadCycles = 0;
    std::uint64_t matrixGenerationCycles = 0;
    std::uint64_t solutionGenerationCycles = 0;
    std::uint64_t solutionEvaluationCycles = 0;
    std::uint64_t sharingAllocationCycles = 0;

    std::uint64_t totalCycles() const noexcept
    {
        return faultInformationInsertCycles +
               faultInformationLookupCycles +
               faultInformationReadCycles +
               matrixGenerationCycles +
               solutionGenerationCycles +
               solutionEvaluationCycles +
               sharingAllocationCycles;
    }
};

struct RepairAttemptResult
{
    std::size_t runIndex = 0;
    std::size_t attemptIndex = 0;
    int stage = 0;
    int subarrayId = -1;
    int availableRows = 0;
    int availableColumns = 0;
    int provisionedRows = 0;
    int provisionedColumns = 0;

    bool isRepairable = false;
    bool repairSuccess = false;
    bool camStorageOverflow = false;

    std::size_t faultCount = 0;
    std::size_t pivotFaultCount = 0;
    std::size_t nonpivotFaultCount = 0;
    std::size_t bufferedPivotFaultCount = 0;
    std::size_t overflowPivotFaultCount = 0;

    std::size_t usedRows = 0;
    std::size_t usedColumns = 0;
    std::size_t unusedAccessibleRows = 0;
    std::size_t unusedAccessibleColumns = 0;
    std::vector<RepairLineMapping> selectedMappings;

    std::size_t addressCamEntriesActive = 0;
    std::optional<std::size_t> addressCamEntriesPeak;
    std::size_t addressCamEntriesProvisioned = 0;
    std::size_t hybridCamEntriesActive = 0;
    std::optional<std::size_t> hybridCamEntriesPeak;
    std::size_t hybridCamWriteOperations = 0;
    std::size_t hybridCamEntriesProvisioned = 0;
    std::size_t bufferCamEntriesActive = 0;
    std::size_t bufferCamEntriesProvisioned = 0;
    std::optional<std::uint32_t> hybridCamEntryWidthBits;
    std::optional<std::uint64_t> hybridCamBitsActive;
    std::optional<std::uint64_t> hybridCamBitsProvisioned;

    std::size_t matrixDimension = 0;
    std::size_t activeMatrixCells = 0;
    std::size_t provisionedMatrixCells = 0;

    std::uint64_t candidateSolutions = 0;
    std::uint64_t candidateSolutionsEvaluated = 0;
    std::uint64_t failedCandidates = 0;
    std::optional<std::size_t> successfulCandidateIndex;
    std::vector<std::size_t> validCandidateIndices;
    std::vector<CandidateRepairOption> validCandidateOptions;
    std::optional<TileSolutionState> tileSolutionState;

    std::uint64_t analysisAttempts = 1;
    AnalysisLatencyBreakdown latency;
    BiraLatencyResult biraLatency;
    std::optional<HardwareMetrics> hardwareMetrics;
    // Populated only by the SRAM_RECAM solver wrapper.  Existing CAM fields
    // remain unchanged for CSV/golden compatibility.
    std::optional<sram_recam::SramRecamAttemptMetrics> sramRecam;
};

struct SharingMetrics
{
    std::size_t borrowedRows = 0;
    std::size_t borrowedColumns = 0;
    std::size_t lentRows = 0;
    std::size_t lentColumns = 0;
    std::size_t borrowRequests = 0;
    std::size_t successfulBorrows = 0;
    std::size_t failedBorrows = 0;
    std::size_t donorStarvationCount = 0;
    std::size_t globalRowsUsed = 0;
    std::size_t globalColumnsUsed = 0;
    std::size_t remainingGlobalRows = 0;
    std::size_t remainingGlobalColumns = 0;
    bool repairSuccessDueToSharing = false;
    bool repairFailureEvenAfterSharing = false;
};

struct GroupRepairResult
{
    std::uint64_t seed = 0;
    std::size_t runIndex = 0;
    FaultCountModel faultCountModel = FaultCountModel::Uniform;
    GroupLayout layout = GroupLayout::Grid2x2;
    SharingTopology topology = SharingTopology::NoSharing;

    std::array<std::size_t, kSubarrayCount> faultCounts{{0, 0, 0, 0}};
    std::array<bool, kSubarrayCount> localRepairSuccess{{false, false, false, false}};
    std::array<bool, kSubarrayCount> repairSuccess{{false, false, false, false}};
    std::array<std::vector<RepairAttemptResult>, kSubarrayCount>
        attemptsBySubarray;
    std::array<std::optional<std::size_t>, kSubarrayCount>
        selectedAttemptIndices;
    std::array<std::optional<std::size_t>, kSubarrayCount>
        selectedCandidateIndices;
    // Populated only when the caller asks the simulator to retain the exact
    // four-subarray remap selected by the group allocator.  This keeps normal
    // statistical sweeps from retaining large remap snapshots.
    std::array<std::optional<CandidateRepairOption>, kSubarrayCount>
        selectedCandidateOptions;

    SolutionTakePolicy solutionTakePolicy = SolutionTakePolicy::Legacy;
    std::array<std::vector<bool>, kSubarrayCount> validSolutionBitmaps;
    std::uint64_t solutionSelectionWork = 0;
    std::uint64_t feasibleCombinationCount = 0;
    std::uint64_t compressedStateBits = 0;
    std::array<std::optional<RemainingSpareResources>, kSubarrayCount>
        remainingResourcesAfterTile;
    std::optional<bool> earlySuccess;
    std::optional<bool> groupCompressedSuccess;
    bool greedyLoss = false;
    std::string solutionSelectionFailureReason;

    bool groupRepairSuccess = false;
    bool baselineGroupRepairSuccess = false;
    int sharingGain = 0;
    std::uint64_t baselineAnalysisCycles = 0;
    // Canonical group-wide borrow stage.  repair_after_1/2/3 CSV fields are
    // derived from this value so they cannot contradict one another.
    std::optional<std::size_t> successfulGroupBorrowCount;
    std::uint64_t extraAnalysisAttempts = 0;

    SharingMetrics sharing;
    std::size_t usedRows = 0;
    std::size_t usedColumns = 0;
    std::size_t unusedPhysicalRows = 0;
    std::size_t unusedPhysicalColumns = 0;
    AnalysisLatencyBreakdown latency;
    BiraLatencyResult biraLatency;

    bool repairedAfterBorrowCount(std::size_t borrowCount) const noexcept
    {
        return groupRepairSuccess &&
               successfulGroupBorrowCount.has_value() &&
               *successfulGroupBorrowCount == borrowCount;
    }
};

} // namespace dynamic_spare

#endif
