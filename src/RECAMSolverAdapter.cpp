#include "../inc/RECAMSolverAdapter.hpp"

#include "../inc/RECAM_PE.hpp"
#include "../inc/SolGenerator.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>

namespace dynamic_spare
{
namespace
{

std::size_t checkedSizeAdd(
    std::size_t left,
    std::size_t right,
    const char *description)
{
    if (right > std::numeric_limits<std::size_t>::max() - left)
    {
        throw std::overflow_error(
            std::string(description) + " exceeds the platform size range");
    }
    return left + right;
}

std::size_t checkedSizeMultiply(
    std::size_t left,
    std::size_t right,
    const char *description)
{
    if (left != 0 &&
        right > std::numeric_limits<std::size_t>::max() / left)
    {
        throw std::overflow_error(
            std::string(description) + " exceeds the platform size range");
    }
    return left * right;
}

std::uint64_t checkedUint64Multiply(
    std::uint64_t left,
    std::uint64_t right,
    const char *description)
{
    if (left != 0 &&
        right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        throw std::overflow_error(
            std::string(description) + " exceeds the uint64 range");
    }
    return left * right;
}

std::size_t matrixDimension(
    int rows,
    int columns,
    const char *description)
{
    return checkedSizeAdd(
        static_cast<std::size_t>(rows),
        static_cast<std::size_t>(columns),
        description);
}

std::size_t hybridCamCapacity(
    int rows,
    int columns,
    const char *description)
{
    if (rows == 0 || columns == 0)
    {
        return 0;
    }
    const std::uint64_t r = static_cast<std::uint64_t>(rows);
    const std::uint64_t c = static_cast<std::uint64_t>(columns);
    const std::uint64_t rowDescriptors = r * (c - 1);
    const std::uint64_t columnDescriptors = c * (r - 1);
    const std::uint64_t capacity = rowDescriptors + columnDescriptors;
    if (capacity > std::numeric_limits<std::size_t>::max())
    {
        throw std::overflow_error(
            std::string(description) + " exceeds the platform size range");
    }
    return static_cast<std::size_t>(capacity);
}

void validateLegacyCoreCapacity(int rows, int columns)
{
    const std::size_t dimension = matrixDimension(
        rows, columns, "Active RECAM matrix dimension");
    const std::size_t hybridCapacity = hybridCamCapacity(
        rows, columns, "Active Hybrid-CAM capacity");
    if (dimension == 0 ||
        dimension > static_cast<std::size_t>(
            std::numeric_limits<int>::max()) ||
        hybridCapacity > static_cast<std::size_t>(
            std::numeric_limits<int>::max()))
    {
        throw std::invalid_argument(
            "Active RECAM capacity exceeds the legacy core's safe int range");
    }
}

using FaultDomain = std::tuple<int, int, int, int>;

FaultDomain faultDomain(const Fault &fault)
{
    return std::make_tuple(
        fault.HBMID,
        fault.ChannelID,
        fault.BankID,
        fault.SubarrayGroupID);
}

void validateFaultIdentity(
    const std::vector<Fault> &faults,
    int requestedSubarrayId)
{
    if (requestedSubarrayId < 0 ||
        requestedSubarrayId >= static_cast<int>(kSubarrayCount))
    {
        throw std::invalid_argument(
            "RECAM request subarray ID must identify A, B, C, or D");
    }

    if (faults.empty())
    {
        return;
    }

    const FaultDomain expectedDomain = faultDomain(faults.front());
    for (std::size_t index = 0; index < faults.size(); ++index)
    {
        const Fault &fault = faults[index];
        if (fault.SubarrayID != requestedSubarrayId)
        {
            throw std::invalid_argument(
                "Fault at index " + std::to_string(index) +
                " does not match the requested subarray ID");
        }
        if (faultDomain(fault) != expectedDomain)
        {
            throw std::invalid_argument(
                "Fault at index " + std::to_string(index) +
                " belongs to a different HBM/channel/bank/subarray group");
        }
    }
}

using PhysicalLineKey = std::tuple<int, int, int, int, int, int>;

PhysicalLineKey lineKey(
    const RemapTable::RemapEntry &entry,
    SpareDimension dimension)
{
    const RemapTable::AddressEntry &address = entry.addressEntry;
    return std::make_tuple(
        address.HBMID,
        address.ChannelID,
        address.BankID,
        address.SubarrayGroupID,
        address.SubarrayID,
        dimension == SpareDimension::Row ? address.r : address.c);
}

CandidateRepairOption snapshotCandidateOption(
    std::size_t candidateIndex,
    const RemapTable &table)
{
    CandidateRepairOption option;
    option.candidateIndex = candidateIndex;
    option.bufferCamRemapCount = table.BufferRemapEntries.size();

    std::set<PhysicalLineKey> usedRows;
    std::set<PhysicalLineKey> usedColumns;
    for (const RemapTable::RemapEntry &entry : table.RemapEntries)
    {
        const SpareDimension dimension = entry.isSpareRow
            ? SpareDimension::Row
            : SpareDimension::Column;
        RepairLineMapping mapping;
        mapping.HBMID = entry.addressEntry.HBMID;
        mapping.ChannelID = entry.addressEntry.ChannelID;
        mapping.BankID = entry.addressEntry.BankID;
        mapping.SubarrayGroupID = entry.addressEntry.SubarrayGroupID;
        mapping.SubarrayID = entry.addressEntry.SubarrayID;
        mapping.sourceRow = entry.addressEntry.r;
        mapping.sourceColumn = entry.addressEntry.c;
        mapping.dimension = dimension;
        mapping.originalAddress = entry.isSpareRow
            ? entry.addressEntry.r
            : entry.addressEntry.c;
        mapping.replacementAddress = entry.newRowColAddr;
        mapping.latency = entry.Latency;
        option.mappings.push_back(mapping);
        if (dimension == SpareDimension::Row)
        {
            usedRows.insert(lineKey(entry, dimension));
        }
        else
        {
            usedColumns.insert(lineKey(entry, dimension));
        }
    }
    option.bufferMappings.reserve(table.BufferRemapEntries.size());
    for (const RemapTable::BufferRemapEntry &entry :
         table.BufferRemapEntries)
    {
        BufferRepairMapping mapping;
        mapping.HBMID = entry.addressEntry.HBMID;
        mapping.ChannelID = entry.addressEntry.ChannelID;
        mapping.BankID = entry.addressEntry.BankID;
        mapping.SubarrayGroupID = entry.addressEntry.SubarrayGroupID;
        mapping.SubarrayID = entry.addressEntry.SubarrayID;
        mapping.sourceRow = entry.addressEntry.r;
        mapping.sourceColumn = entry.addressEntry.c;
        mapping.latency = entry.Latency;
        option.bufferMappings.push_back(mapping);
    }
    option.usedRows = usedRows.size();
    option.usedColumns = usedColumns.size();
    return option;
}

MatrixRepairAddress snapshotMatrixAddress(
    const RemapTable::AddressEntry &address)
{
    return MatrixRepairAddress{
        address.HBMID,
        address.ChannelID,
        address.BankID,
        address.SubarrayGroupID,
        address.SubarrayID,
        address.r,
        address.c};
}

} // namespace

DecodedSolution decodeSolution(
    const TileSolutionState &state,
    std::size_t solutionId)
{
    const std::size_t dimension = static_cast<std::size_t>(
        state.spareRows + state.spareColumns);
    SolGenerator solutions(state.spareRows, state.spareColumns);
    if (state.matrixRowAddresses.size() != dimension ||
        state.matrixColumnAddresses.size() != dimension ||
        state.validSolutionBitmap.size() !=
            solutions.allSolVectorsType.size())
    {
        throw std::invalid_argument(
            "TileSolutionState violates its RECAM size contract");
    }
    if (solutionId >= state.validSolutionBitmap.size())
    {
        throw std::out_of_range("RECAM solution ID is out of range");
    }

    DecodedSolution decoded;
    decoded.solutionId = solutionId;
    const solVector &orientation = solutions.allSolVectorsType[solutionId];
    for (std::size_t index = 0; index < dimension; ++index)
    {
        if (orientation[index])
        {
            if (state.matrixColumnAddresses[index].has_value())
                decoded.sourceColumns.push_back(
                    *state.matrixColumnAddresses[index]);
        }
        else if (state.matrixRowAddresses[index].has_value())
        {
            decoded.sourceRows.push_back(*state.matrixRowAddresses[index]);
        }
    }
    return decoded;
}

RepairAttemptResult RECAMSolverAdapter::solve(
    const std::vector<Fault> &faults,
    const RECAMSolverRequest &request) const
{
    // The legacy Hybrid-CAM sizing expression is only defined safely for
    // A single zero axis is a valid sharedLine=2 boundary.  The legacy core
    // has an explicit zero-descriptor Hybrid-CAM rule for it.  With both axes
    // zero there is no RECAM solution space to analyze.
    if (request.availableRows < 0 || request.availableColumns < 0 ||
        (request.availableRows == 0 && request.availableColumns == 0))
    {
        throw std::invalid_argument(
            "RECAM capacities must be nonnegative and not both zero");
    }
    if (request.bufferCamEntries < 0)
    {
        throw std::invalid_argument("Buffer CAM entries cannot be negative");
    }
    if (request.hybridCamEntryWidthBits.has_value() &&
        *request.hybridCamEntryWidthBits == 0)
    {
        throw std::invalid_argument(
            "Hybrid CAM entry width must be positive when configured");
    }
    validateFaultIdentity(faults, request.subarrayId);
    validateLegacyCoreCapacity(
        request.availableRows, request.availableColumns);

    const int provisionedRows = request.provisionedRows < 0
        ? request.availableRows
        : request.provisionedRows;
    const int provisionedColumns = request.provisionedColumns < 0
        ? request.availableColumns
        : request.provisionedColumns;
    if (provisionedRows < 0 || provisionedColumns < 0 ||
        (provisionedRows == 0 && provisionedColumns == 0) ||
        provisionedRows < request.availableRows ||
        provisionedColumns < request.availableColumns)
    {
        throw std::invalid_argument(
            "Provisioned RECAM capacity cannot be below active capacity");
    }

    const std::size_t activeDimension = matrixDimension(
        request.availableRows,
        request.availableColumns,
        "Active RECAM matrix dimension");
    const std::size_t provisionedDimension = matrixDimension(
        provisionedRows,
        provisionedColumns,
        "Provisioned RECAM matrix dimension");
    const std::size_t provisionedHybridCapacity = hybridCamCapacity(
        provisionedRows,
        provisionedColumns,
        "Provisioned Hybrid-CAM capacity");

    FaultList faultList(
        request.availableRows,
        request.availableColumns,
        request.bufferCamEntries);
    for (const Fault &fault : faults)
    {
        Fault unclassifiedFault = fault;
        unclassifiedFault.isPivot = false;
        unclassifiedFault.isBuffer = false;
        unclassifiedFault.isNonPivot = false;
        unclassifiedFault.isOverflowPivot = false;
        faultList.addFault(unclassifiedFault);
    }
    faultList.classifyFaults();

    RECAM_PE pe(
        request.availableRows,
        request.availableColumns,
        request.bufferCamEntries);
    SolGenerator solutions(
        request.availableRows,
        request.availableColumns);
    pe.loadFaultsToCAMs(faultList);
    pe.genFaultAnalyzeMatrix();
    pe.genValidSolList(solutions.allSolMatrixsType);

    RepairAttemptResult result;
    result.runIndex = request.runIndex;
    result.attemptIndex = request.attemptIndex;
    result.stage = request.stage;
    result.subarrayId = request.subarrayId;
    result.availableRows = request.availableRows;
    result.availableColumns = request.availableColumns;
    result.provisionedRows = provisionedRows;
    result.provisionedColumns = provisionedColumns;
    result.isRepairable = pe.isRepairable;
    result.repairSuccess = pe.RepairSuccess;
    result.camStorageOverflow = pe.camStorageOverflow;

    result.faultCount = faultList.PEFaults.size();
    result.pivotFaultCount = faultList.pivotFaults.size();
    result.nonpivotFaultCount = faultList.nonPivotFaults.size();
    result.bufferedPivotFaultCount = faultList.bufferFaults.size();
    result.overflowPivotFaultCount = faultList.overflowPivotFaults.size();

    result.addressCamEntriesActive = pe.addressCAM->addressCAMEntries.size();
    result.addressCamEntriesPeak = result.addressCamEntriesActive;
    result.addressCamEntriesProvisioned = provisionedDimension;
    result.hybridCamEntriesActive = pe.hybridCAM->hybridCAMEntries.size();
    result.hybridCamEntriesPeak = pe.hybridCAM->peakEntries;
    result.hybridCamWriteOperations = pe.hybridCAM->writeOperations;
    result.hybridCamEntriesProvisioned = provisionedHybridCapacity;
    result.bufferCamEntriesActive = pe.bufferCAM->bufferFaults.size();
    result.bufferCamEntriesProvisioned = static_cast<std::size_t>(
        request.bufferCamEntries);
    result.hybridCamEntryWidthBits = request.hybridCamEntryWidthBits;
    if (request.hybridCamEntryWidthBits.has_value())
    {
        result.hybridCamBitsActive = checkedUint64Multiply(
            static_cast<std::uint64_t>(result.hybridCamEntriesActive),
            *request.hybridCamEntryWidthBits,
            "Active Hybrid-CAM bit capacity");
        result.hybridCamBitsProvisioned = checkedUint64Multiply(
            static_cast<std::uint64_t>(result.hybridCamEntriesProvisioned),
            *request.hybridCamEntryWidthBits,
            "Provisioned Hybrid-CAM bit capacity");
    }

    if (pe.matrixSize < 0 ||
        static_cast<std::size_t>(pe.matrixSize) != activeDimension)
    {
        throw std::logic_error(
            "Legacy RECAM matrix dimension does not match the request");
    }
    result.matrixDimension = activeDimension;
    result.activeMatrixCells = checkedSizeMultiply(
        activeDimension,
        activeDimension,
        "Active RECAM matrix cell count");
    result.provisionedMatrixCells = checkedSizeMultiply(
        provisionedDimension,
        provisionedDimension,
        "Provisioned RECAM matrix cell count");

    result.candidateSolutions = solutions.allSolMatrixsType.size();
    // The legacy RECAM_PE API deliberately collects every valid solution.
    // Early-stop accounting will be added as a separate, explicit mode; this
    // foundation adapter records the exact current behavior.
    result.candidateSolutionsEvaluated = result.candidateSolutions;
    if (pe.validSolList.size() != pe.remapTableList.size())
    {
        throw std::logic_error(
            "RECAM valid-solution and remap-table lists are misaligned");
    }
    result.validCandidateIndices.reserve(pe.validSolList.size());
    result.validCandidateOptions.reserve(pe.validSolList.size());
    for (std::size_t validIndex = 0;
         validIndex < pe.validSolList.size(); ++validIndex)
    {
        const int candidateIndex = pe.validSolList[validIndex];
        result.validCandidateIndices.push_back(
            static_cast<std::size_t>(candidateIndex));
        result.validCandidateOptions.push_back(snapshotCandidateOption(
            static_cast<std::size_t>(candidateIndex),
            pe.remapTableList[validIndex]));
    }
    result.failedCandidates = result.candidateSolutions -
        result.validCandidateIndices.size();

    TileSolutionState state;
    state.subarrayId = request.subarrayId;
    state.spareRows = request.availableRows;
    state.spareColumns = request.availableColumns;
    state.matrixRowAddresses.reserve(pe.matrixRowAddresses.size());
    state.matrixColumnAddresses.reserve(pe.matrixColumnAddresses.size());
    for (const auto &address : pe.matrixRowAddresses)
    {
        state.matrixRowAddresses.push_back(address.has_value()
            ? std::optional<MatrixRepairAddress>(
                  snapshotMatrixAddress(*address))
            : std::nullopt);
    }
    for (const auto &address : pe.matrixColumnAddresses)
    {
        state.matrixColumnAddresses.push_back(address.has_value()
            ? std::optional<MatrixRepairAddress>(
                  snapshotMatrixAddress(*address))
            : std::nullopt);
    }
    state.validSolutionBitmap.assign(result.candidateSolutions, false);
    for (std::size_t candidateIndex : result.validCandidateIndices)
    {
        state.validSolutionBitmap[candidateIndex] = true;
    }
    if (!result.validCandidateOptions.empty())
    {
        state.camReuseMappings =
            result.validCandidateOptions.front().bufferMappings;
    }
    const std::uint64_t addressBits = checkedUint64Multiply(
        static_cast<std::uint64_t>(activeDimension),
        static_cast<std::uint64_t>(request.rowAddressWidthBits) +
            request.columnAddressWidthBits,
        "Compressed matrix-address dictionary bit count");
    const std::uint64_t validityBits = checkedUint64Multiply(
        static_cast<std::uint64_t>(activeDimension), 2,
        "Compressed matrix-address validity bit count");
    state.compressedStorageBits = addressBits + validityBits +
        result.candidateSolutions;
    result.tileSolutionState = std::move(state);

    if (result.repairSuccess && !pe.validSolList.empty())
    {
        result.successfulCandidateIndex = static_cast<std::size_t>(
            pe.validSolList.front());

        const CandidateRepairOption &selected =
            result.validCandidateOptions.front();
        result.usedRows = selected.usedRows;
        result.usedColumns = selected.usedColumns;
        result.selectedMappings = selected.mappings;
    }

    if (result.usedRows > static_cast<std::size_t>(request.availableRows) ||
        result.usedColumns >
            static_cast<std::size_t>(request.availableColumns))
    {
        throw std::logic_error(
            "Selected RECAM mapping exceeds its accessible spare capacity");
    }
    result.unusedAccessibleRows =
        static_cast<std::size_t>(request.availableRows) -
        result.usedRows;
    result.unusedAccessibleColumns =
        static_cast<std::size_t>(request.availableColumns) -
        result.usedColumns;
    return result;
}

} // namespace dynamic_spare
