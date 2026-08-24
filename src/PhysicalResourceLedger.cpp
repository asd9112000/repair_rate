#include "../inc/PhysicalResourceLedger.hpp"

#include <algorithm>
#include <stdexcept>

namespace dynamic_spare
{
namespace
{

std::size_t dimensionValue(
    const SpareDemand &demand,
    SpareDimension dimension) noexcept
{
    return dimension == SpareDimension::Row
        ? demand.rows
        : demand.columns;
}

std::size_t &dimensionValue(
    SpareDemand &demand,
    SpareDimension dimension) noexcept
{
    return dimension == SpareDimension::Row
        ? demand.rows
        : demand.columns;
}

std::size_t configuredLocalCapacity(
    const SimulationConfig &config,
    SpareDimension dimension)
{
    if (config.topology == SharingTopology::GlobalPool)
    {
        const GlobalPoolConfiguration &pool = *config.globalPool;
        return static_cast<std::size_t>(
            dimension == SpareDimension::Row
                ? pool.localRowsPerSubarray
                : pool.localColumnsPerSubarray);
    }
    return static_cast<std::size_t>(
        dimension == SpareDimension::Row
            ? config.spareRows
            : config.spareColumns);
}

std::size_t configuredShareableCapacity(
    const SimulationConfig &config,
    std::size_t owner,
    SpareDimension dimension)
{
    if (config.topology == SharingTopology::Directional)
    {
        if (dimension == SpareDimension::Row &&
            (owner == 0 || owner == 3))
        {
            return static_cast<std::size_t>(config.sharedRows);
        }
        if (dimension == SpareDimension::Column &&
            (owner == 1 || owner == 2))
        {
            return static_cast<std::size_t>(config.sharedColumns);
        }
        return 0;
    }
    if (config.topology == SharingTopology::PairwiseEdge)
    {
        return static_cast<std::size_t>(
            dimension == SpareDimension::Row
                ? config.sharedRows
                : config.sharedColumns);
    }
    return 0;
}

} // namespace

std::size_t LedgerAllocationResult::borrowedRows() const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        transfers.begin(), transfers.end(),
        [](const BorrowTransfer &transfer)
        {
            return transfer.dimension == SpareDimension::Row;
        }));
}

std::size_t LedgerAllocationResult::borrowedColumns() const noexcept
{
    return transfers.size() - borrowedRows();
}

PhysicalResourceLedger::PhysicalResourceLedger(
    const SimulationConfig &config)
    : config_(config)
{
    config_.validate();

    std::size_t nextPhysicalId = 0;
    for (std::size_t subarray = 0;
         subarray < kSubarrayCount; ++subarray)
    {
        for (SpareDimension dimension :
             {SpareDimension::Row, SpareDimension::Column})
        {
            const std::size_t localCapacity = configuredLocalCapacity(
                config_, dimension);
            const std::size_t shareableCapacity =
                configuredShareableCapacity(config_, subarray, dimension);
            if (shareableCapacity > localCapacity)
            {
                throw std::invalid_argument(
                    "Shareable spare capacity exceeds local ownership");
            }
            for (std::size_t index = 0; index < localCapacity; ++index)
            {
                PhysicalSpareLine line;
                line.physicalId = nextPhysicalId++;
                line.ownerSubarray = static_cast<int>(subarray);
                line.dimension = dimension;
                line.ownerLineIndex = index;
                line.shareable = index >= localCapacity - shareableCapacity;
                physicalLines_.push_back(line);
            }
        }
    }

    if (config_.topology == SharingTopology::GlobalPool)
    {
        const GlobalPoolConfiguration &pool = *config_.globalPool;
        for (SpareDimension dimension :
             {SpareDimension::Row, SpareDimension::Column})
        {
            const std::size_t capacity = static_cast<std::size_t>(
                dimension == SpareDimension::Row
                    ? pool.globalRows
                    : pool.globalColumns);
            for (std::size_t index = 0; index < capacity; ++index)
            {
                PhysicalSpareLine line;
                line.physicalId = nextPhysicalId++;
                line.ownerSubarray = -1;
                line.dimension = dimension;
                line.ownerLineIndex = index;
                line.shareable = true;
                physicalLines_.push_back(line);
            }
        }
    }

    const std::size_t expectedRows =
        kSubarrayCount * static_cast<std::size_t>(config_.spareRows);
    const std::size_t expectedColumns =
        kSubarrayCount * static_cast<std::size_t>(config_.spareColumns);
    if (physicalRows() != expectedRows ||
        physicalColumns() != expectedColumns)
    {
        throw std::logic_error(
            "Physical ledger does not conserve baseline spare capacity");
    }
}

bool PhysicalResourceLedger::mayBorrow(
    int ownerSubarray,
    std::size_t borrowerSubarray,
    SpareDimension dimension) const noexcept
{
    if (config_.topology == SharingTopology::GlobalPool)
    {
        return ownerSubarray == -1;
    }
    if (ownerSubarray < 0 ||
        ownerSubarray == static_cast<int>(borrowerSubarray))
    {
        return false;
    }

    const std::size_t owner = static_cast<std::size_t>(ownerSubarray);
    if (config_.topology == SharingTopology::Directional)
    {
        return (dimension == SpareDimension::Row &&
                ((owner == 0 && borrowerSubarray == 2) ||
                 (owner == 3 && borrowerSubarray == 1))) ||
               (dimension == SpareDimension::Column &&
                ((owner == 1 && borrowerSubarray == 0) ||
                 (owner == 2 && borrowerSubarray == 3)));
    }
    if (config_.topology == SharingTopology::PairwiseEdge)
    {
        if (dimension == SpareDimension::Row)
        {
            return (owner == 0 && borrowerSubarray == 2) ||
                   (owner == 2 && borrowerSubarray == 0) ||
                   (owner == 1 && borrowerSubarray == 3) ||
                   (owner == 3 && borrowerSubarray == 1);
        }
        return (owner == 0 && borrowerSubarray == 1) ||
               (owner == 1 && borrowerSubarray == 0) ||
               (owner == 2 && borrowerSubarray == 3) ||
               (owner == 3 && borrowerSubarray == 2);
    }
    return false;
}

LedgerAllocationResult PhysicalResourceLedger::allocate(
    const std::array<SpareDemand, kSubarrayCount> &demands) const
{
    LedgerAllocationResult result;
    result.demands = demands;
    result.lines = physicalLines_;
    result.physicalRows = physicalRows();
    result.physicalColumns = physicalColumns();

    std::array<SpareDemand, kSubarrayCount> excess;
    std::array<bool, kSubarrayCount> forbiddenDualDimension{{
        false, false, false, false}};
    for (std::size_t subarray = 0;
         subarray < kSubarrayCount; ++subarray)
    {
        for (SpareDimension dimension :
             {SpareDimension::Row, SpareDimension::Column})
        {
            std::vector<std::size_t> ownedLineIndices;
            for (std::size_t lineIndex = 0;
                 lineIndex < result.lines.size(); ++lineIndex)
            {
                const PhysicalSpareLine &line = result.lines[lineIndex];
                if (line.ownerSubarray == static_cast<int>(subarray) &&
                    line.dimension == dimension)
                {
                    ownedLineIndices.push_back(lineIndex);
                }
            }
            std::stable_sort(
                ownedLineIndices.begin(), ownedLineIndices.end(),
                [&result](std::size_t left, std::size_t right)
                {
                    return result.lines[left].shareable <
                        result.lines[right].shareable;
                });

            const std::size_t demand = dimensionValue(
                demands[subarray], dimension);
            const std::size_t ownUse = std::min(
                demand, ownedLineIndices.size());
            for (std::size_t index = 0; index < ownUse; ++index)
            {
                result.lines[ownedLineIndices[index]].assignedSubarray =
                    subarray;
            }
            dimensionValue(excess[subarray], dimension) = demand - ownUse;
        }

        if (config_.modifiers.singleDimensionBorrowing &&
            excess[subarray].rows != 0 &&
            excess[subarray].columns != 0)
        {
            forbiddenDualDimension[subarray] = true;
        }
    }

    for (std::size_t borrower = 0;
         borrower < kSubarrayCount; ++borrower)
    {
        for (SpareDimension dimension :
             {SpareDimension::Row, SpareDimension::Column})
        {
            const std::size_t requestCount = dimensionValue(
                excess[borrower], dimension);
            for (std::size_t requestIndex = 0;
                 requestIndex < requestCount; ++requestIndex)
            {
                ++result.borrowRequests;
                if (forbiddenDualDimension[borrower])
                {
                    ++result.failedBorrows;
                    continue;
                }
                auto selected = result.lines.end();
                for (auto line = result.lines.begin();
                     line != result.lines.end(); ++line)
                {
                    if (line->assignedSubarray.has_value() ||
                        !line->shareable || line->dimension != dimension ||
                        !mayBorrow(
                            line->ownerSubarray, borrower, dimension))
                    {
                        continue;
                    }

                    if (line->ownerSubarray >= 0)
                    {
                        const std::size_t reserve =
                            static_cast<std::size_t>(
                                dimension == SpareDimension::Row
                                    ? config_.modifiers.minimumRowReserve
                                    : config_.modifiers.minimumColumnReserve);
                        const std::size_t ownerFree =
                            static_cast<std::size_t>(std::count_if(
                                result.lines.begin(), result.lines.end(),
                                [line, dimension](
                                    const PhysicalSpareLine &candidate)
                                {
                                    return candidate.ownerSubarray ==
                                               line->ownerSubarray &&
                                           candidate.dimension == dimension &&
                                           !candidate.assignedSubarray.has_value();
                                }));
                        if (ownerFree <= reserve)
                        {
                            continue;
                        }
                    }
                    selected = line;
                    break;
                }

                if (selected == result.lines.end())
                {
                    ++result.failedBorrows;
                    ++result.donorStarvationCount;
                    continue;
                }

                selected->assignedSubarray = borrower;
                ++result.successfulBorrows;
                dimensionValue(
                    result.borrowedBySubarray[borrower], dimension)++;
                if (selected->ownerSubarray >= 0)
                {
                    dimensionValue(
                        result.lentBySubarray[
                            static_cast<std::size_t>(
                                selected->ownerSubarray)],
                        dimension)++;
                }
                else if (dimension == SpareDimension::Row)
                {
                    ++result.globalRowsUsed;
                }
                else
                {
                    ++result.globalColumnsUsed;
                }
                result.transfers.push_back(BorrowTransfer{
                    selected->ownerSubarray,
                    borrower,
                    dimension,
                    selected->physicalId});
            }
        }
    }

    for (const PhysicalSpareLine &line : result.lines)
    {
        if (!line.assignedSubarray.has_value())
        {
            continue;
        }
        if (line.dimension == SpareDimension::Row)
        {
            ++result.usedRows;
        }
        else
        {
            ++result.usedColumns;
        }
    }
    result.unusedRows = result.physicalRows - result.usedRows;
    result.unusedColumns = result.physicalColumns - result.usedColumns;

    if (config_.topology == SharingTopology::GlobalPool)
    {
        const GlobalPoolConfiguration &pool = *config_.globalPool;
        result.remainingGlobalRows =
            static_cast<std::size_t>(pool.globalRows) -
            result.globalRowsUsed;
        result.remainingGlobalColumns =
            static_cast<std::size_t>(pool.globalColumns) -
            result.globalColumnsUsed;
    }
    result.success = result.failedBorrows == 0;
    return result;
}

std::size_t PhysicalResourceLedger::physicalRows() const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        physicalLines_.begin(), physicalLines_.end(),
        [](const PhysicalSpareLine &line)
        {
            return line.dimension == SpareDimension::Row;
        }));
}

std::size_t PhysicalResourceLedger::physicalColumns() const noexcept
{
    return physicalLines_.size() - physicalRows();
}

} // namespace dynamic_spare
