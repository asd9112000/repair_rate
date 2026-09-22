#include "../inc/SharedCollectorRecam.hpp"

#include "../inc/SolGenerator.hpp"
#include "../inc/RECAM_PE.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>

namespace dynamic_spare
{
namespace
{

// The retained Directional V2 collector has five pivot positions and eleven
// reachable Hybrid records.  Seven is the legacy analyzer input width, not a
// collector capacity, so it must not reject the shared state here.

struct HybridRecord
{
    Fault fault;
    std::size_t pivot = 0;
    bool rowIsDifferent = false;
};

struct SharedFaultState
{
    std::vector<Fault> pivots;
    std::vector<HybridRecord> hybrids;
    std::vector<Fault> bufferFaults;
    std::map<int, int> rowCounts;
    std::map<int, int> columnCounts;
    std::vector<int> pivotRowCounts;
    std::vector<int> pivotColumnCounts;
    bool overflow = false;
};

MatrixRepairAddress matrixAddress(const Fault &fault)
{
    return {fault.HBMID, fault.ChannelID, fault.BankID, fault.SubarrayGroupID,
            fault.SubarrayID, fault.r, fault.c};
}

SharedFaultState collect(const std::vector<Fault> &faults,
                         int bufferCapacity,
                         int collectorRows,
                         int collectorColumns)
{
    SharedFaultState state;
    for (const Fault &fault : faults)
    {
        ++state.rowCounts[fault.r];
        ++state.columnCounts[fault.c];
        std::optional<std::size_t> relation;
        bool rowIsDifferent = false;
        for (std::size_t pivot = 0; pivot < state.pivots.size(); ++pivot)
        {
            if (fault.r == state.pivots[pivot].r)
            {
                relation = pivot;
                break;
            }
            if (fault.c == state.pivots[pivot].c)
            {
                relation = pivot;
                rowIsDifferent = true;
                break;
            }
        }
        if (!relation.has_value())
        {
            if (state.pivots.size() < kDirectionalSharedCollectorPivots)
            {
                state.pivots.push_back(fault);
                state.pivotRowCounts.push_back(1);
                state.pivotColumnCounts.push_back(1);
            }
            else if (state.bufferFaults.size() <
                     static_cast<std::size_t>(bufferCapacity))
            {
                state.bufferFaults.push_back(fault);
            }
            else
            {
                state.overflow = true;
            }
            continue;
        }

        const std::size_t pivot = *relation;
        const int maximum = rowIsDifferent ? collectorRows : collectorColumns;
        int &relatedCount = rowIsDifferent
            ? state.pivotColumnCounts[pivot]
            : state.pivotRowCounts[pivot];
        ++relatedCount;
        if (relatedCount > maximum)
        {
            state.hybrids.erase(
                std::remove_if(state.hybrids.begin(), state.hybrids.end(),
                    [pivot, rowIsDifferent](const HybridRecord &entry)
                    {
                        return entry.pivot == pivot &&
                            entry.rowIsDifferent == rowIsDifferent;
                    }),
                state.hybrids.end());
            continue;
        }
        if (state.hybrids.size() == kDirectionalSharedCollectorHybridEntries)
        {
            state.overflow = true;
            continue;
        }
        state.hybrids.push_back({fault, pivot, rowIsDifferent});
    }
    return state;
}

RepairLineMapping mappingFor(const MatrixRepairAddress &address,
                             SpareDimension dimension, int replacement)
{
    RepairLineMapping mapping;
    mapping.HBMID = address.HBMID;
    mapping.ChannelID = address.ChannelID;
    mapping.BankID = address.BankID;
    mapping.SubarrayGroupID = address.SubarrayGroupID;
    mapping.SubarrayID = address.SubarrayID;
    mapping.sourceRow = address.row;
    mapping.sourceColumn = address.column;
    mapping.dimension = dimension;
    mapping.originalAddress = dimension == SpareDimension::Row
        ? address.row : address.column;
    mapping.replacementAddress = replacement;
    mapping.latency = RemapTable::kDefaultRemapLatency;
    return mapping;
}

RepairAttemptResult analyze(const SharedFaultState &shared,
                            const RECAMSolverRequest &request)
{
    const int rows = request.availableRows;
    const int columns = request.availableColumns;
    const int dimension = rows + columns;
    if (rows < 1 || columns < 1 ||
        dimension > static_cast<int>(kDirectionalSharedCollectorPivots))
        throw std::invalid_argument("Model-B2 configuration is outside the frozen shared analyzer envelope");

    RepairAttemptResult result;
    result.runIndex = request.runIndex;
    result.attemptIndex = request.attemptIndex;
    result.stage = request.stage;
    result.subarrayId = request.subarrayId;
    result.availableRows = rows;
    result.availableColumns = columns;
    result.provisionedRows = request.provisionedRows;
    result.provisionedColumns = request.provisionedColumns;
    result.faultCount = 0;
    for (const auto &count : shared.rowCounts)
        result.faultCount += static_cast<std::size_t>(count.second);
    result.pivotFaultCount = shared.pivots.size();
    result.nonpivotFaultCount = shared.hybrids.size();
    result.bufferedPivotFaultCount = shared.bufferFaults.size();
    result.addressCamEntriesActive = shared.pivots.size();
    result.addressCamEntriesPeak = shared.pivots.size();
    result.hybridCamEntriesActive = shared.hybrids.size();
    result.hybridCamEntriesPeak = shared.hybrids.size();
    result.hybridCamWriteOperations = shared.hybrids.size();
    result.hybridRecords.reserve(shared.hybrids.size());
    for (const HybridRecord &hybrid : shared.hybrids)
    {
        result.hybridRecords.push_back(HybridRecordSnapshot{
            hybrid.fault.r, hybrid.fault.c,
            static_cast<int>(hybrid.pivot), hybrid.rowIsDifferent});
    }
    result.bufferCamEntriesActive = shared.bufferFaults.size();
    result.bufferCamEntriesProvisioned = static_cast<std::size_t>(
        request.bufferCamEntries);
    result.hybridCamEntryWidthBits = request.hybridCamEntryWidthBits;
    result.camStorageOverflow = shared.overflow;
    result.isRepairable = !shared.overflow;
    result.matrixDimension = static_cast<std::size_t>(dimension);
    result.activeMatrixCells = static_cast<std::size_t>(dimension * dimension);

    std::vector<std::optional<MatrixRepairAddress>> rowAddresses(dimension);
    std::vector<std::optional<MatrixRepairAddress>> columnAddresses(dimension);
    std::vector<std::vector<bool>> matrix(dimension, std::vector<bool>(dimension, false));
    std::vector<int> rowDictionary;
    std::vector<int> columnDictionary;
    const int pivotCount = std::min<int>(dimension, shared.pivots.size());
    std::vector<bool> rowMust(pivotCount, false);
    std::vector<bool> columnMust(pivotCount, false);
    for (int index = 0; index < pivotCount; ++index)
    {
        const Fault &pivot = shared.pivots[static_cast<std::size_t>(index)];
        rowDictionary.push_back(pivot.r);
        columnDictionary.push_back(pivot.c);
        rowAddresses[index] = matrixAddress(pivot);
        columnAddresses[index] = matrixAddress(pivot);
        rowMust[index] = shared.rowCounts.at(pivot.r) > columns;
        columnMust[index] = shared.columnCounts.at(pivot.c) > rows;
    }
    for (int row = 0; row < dimension; ++row)
        for (int column = 0; column < dimension; ++column)
            matrix[row][column] = (row == column && row < pivotCount) ||
                (row < pivotCount && rowMust[row]) ||
                (column < pivotCount && columnMust[column]);

    for (const HybridRecord &hybrid : shared.hybrids)
    {
        if (hybrid.pivot >= static_cast<std::size_t>(pivotCount))
            continue;
        if ((hybrid.rowIsDifferent && columnMust[hybrid.pivot]) ||
            (!hybrid.rowIsDifferent && rowMust[hybrid.pivot]))
            continue;
        if (hybrid.rowIsDifferent)
        {
            const int differing = hybrid.fault.r;
            auto found = std::find(rowDictionary.begin(), rowDictionary.end(), differing);
            if (found != rowDictionary.end())
            {
                matrix[static_cast<std::size_t>(found - rowDictionary.begin())][hybrid.pivot] = true;
            }
            else if (rowDictionary.size() < static_cast<std::size_t>(dimension))
            {
                const std::size_t slot = rowDictionary.size();
                rowDictionary.push_back(differing);
                rowAddresses[slot] = matrixAddress(hybrid.fault);
                matrix[slot][hybrid.pivot] = true;
            }
            else
            {
                for (int row = 0; row < dimension; ++row)
                    matrix[row][hybrid.pivot] = true;
            }
        }
        else
        {
            const int differing = hybrid.fault.c;
            auto found = std::find(columnDictionary.begin(), columnDictionary.end(), differing);
            if (found != columnDictionary.end())
            {
                matrix[hybrid.pivot][static_cast<std::size_t>(found - columnDictionary.begin())] = true;
            }
            else if (columnDictionary.size() < static_cast<std::size_t>(dimension))
            {
                const std::size_t slot = columnDictionary.size();
                columnDictionary.push_back(differing);
                columnAddresses[slot] = matrixAddress(hybrid.fault);
                matrix[hybrid.pivot][slot] = true;
            }
            else
            {
                for (int column = 0; column < dimension; ++column)
                    matrix[hybrid.pivot][column] = true;
            }
        }
    }

    result.sharedCollectorMatrixBits.reserve(
        static_cast<std::size_t>(dimension * dimension));
    for (int row = 0; row < dimension; ++row)
        for (int column = 0; column < dimension; ++column)
            result.sharedCollectorMatrixBits.push_back(matrix[row][column]);

    SolGenerator solutions(rows, columns);
    result.candidateSolutions = solutions.allSolVectorsType.size();
    result.candidateSolutionsEvaluated = result.candidateSolutions;
    TileSolutionState state;
    state.subarrayId = request.subarrayId;
    state.spareRows = rows;
    state.spareColumns = columns;
    state.matrixRowAddresses = rowAddresses;
    state.matrixColumnAddresses = columnAddresses;
    state.validSolutionBitmap.assign(result.candidateSolutions, false);
    for (std::size_t candidate = 0; candidate < solutions.allSolVectorsType.size(); ++candidate)
    {
        const solVector &orientation = solutions.allSolVectorsType[candidate];
        bool valid = !shared.overflow;
        for (int row = 0; row < dimension; ++row)
            for (int column = 0; column < dimension; ++column)
                if (matrix[row][column] && orientation[row] && !orientation[column])
                    valid = false;
        if (!valid)
            continue;
        state.validSolutionBitmap[candidate] = true;
        result.validCandidateIndices.push_back(candidate);
        CandidateRepairOption option;
        option.candidateIndex = candidate;
        int nextRow = 999000;
        int nextColumn = 999000;
        for (int index = 0; index < dimension; ++index)
        {
            const SpareDimension line = orientation[index]
                ? SpareDimension::Column : SpareDimension::Row;
            const auto &address = orientation[index]
                ? columnAddresses[index] : rowAddresses[index];
            if (!address.has_value())
                continue;
            option.mappings.push_back(mappingFor(*address, line,
                line == SpareDimension::Row ? nextRow++ : nextColumn++));
        }
        for (std::size_t index = static_cast<std::size_t>(dimension);
             index < shared.pivots.size(); ++index)
        {
            const MatrixRepairAddress address = matrixAddress(shared.pivots[index]);
            option.bufferMappings.push_back({address.HBMID, address.ChannelID,
                address.BankID, address.SubarrayGroupID, address.SubarrayID,
                address.row, address.column, RemapTable::kDefaultCamReuseLatency});
        }
        for (const Fault &fault : shared.bufferFaults)
            option.bufferMappings.push_back({fault.HBMID, fault.ChannelID,
                fault.BankID, fault.SubarrayGroupID, fault.SubarrayID, fault.r,
                fault.c, RemapTable::kDefaultCamReuseLatency});
        std::set<int> usedRows;
        std::set<int> usedColumns;
        for (const RepairLineMapping &mapping : option.mappings)
            (mapping.dimension == SpareDimension::Row ? usedRows : usedColumns)
                .insert(mapping.originalAddress);
        option.usedRows = usedRows.size();
        option.usedColumns = usedColumns.size();
        option.bufferCamRemapCount = option.bufferMappings.size();
        result.validCandidateOptions.push_back(std::move(option));
    }
    result.failedCandidates = result.candidateSolutions - result.validCandidateIndices.size();
    state.compressedStorageBits = static_cast<std::uint64_t>(dimension) *
        (request.rowAddressWidthBits + request.columnAddressWidthBits + 2U) +
        result.candidateSolutions;
    if (!result.validCandidateOptions.empty())
        state.camReuseMappings = result.validCandidateOptions.front().bufferMappings;
    result.tileSolutionState = std::move(state);
    result.repairSuccess = result.isRepairable && !result.validCandidateOptions.empty();
    if (result.repairSuccess)
    {
        const CandidateRepairOption &selected = result.validCandidateOptions.front();
        result.successfulCandidateIndex = selected.candidateIndex;
        result.usedRows = selected.usedRows;
        result.usedColumns = selected.usedColumns;
        result.selectedMappings = selected.mappings;
    }
    result.unusedAccessibleRows = static_cast<std::size_t>(rows) - result.usedRows;
    result.unusedAccessibleColumns = static_cast<std::size_t>(columns) - result.usedColumns;
    return result;
}
} // namespace

std::vector<RepairAttemptResult> solveSharedCollectorConfigSet(
    const std::vector<Fault> &faults,
    const std::vector<RECAMSolverRequest> &requests)
{
    if (requests.empty())
        return {};
    const int bufferCapacity = requests.front().bufferCamEntries;
    if (bufferCapacity < 0)
        throw std::invalid_argument("Model-B2 buffer capacity cannot be negative");
    int collectorRows = 0;
    int collectorColumns = 0;
    for (const RECAMSolverRequest &request : requests)
    {
        if (request.bufferCamEntries != bufferCapacity)
            throw std::invalid_argument(
                "All Model-B2 Config requests must share one buffer capacity");
        collectorRows = std::max(collectorRows, request.availableRows);
        collectorColumns = std::max(collectorColumns, request.availableColumns);
    }
    const SharedFaultState shared = collect(
        faults, bufferCapacity, collectorRows, collectorColumns);
    std::vector<RepairAttemptResult> results;
    results.reserve(requests.size());
    for (const RECAMSolverRequest &request : requests)
        results.push_back(analyze(shared, request));
    return results;
}

} // namespace dynamic_spare
