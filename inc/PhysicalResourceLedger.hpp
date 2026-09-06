#ifndef DYNAMIC_SPARE_SHARING_PHYSICAL_RESOURCE_LEDGER_HPP
#define DYNAMIC_SPARE_SHARING_PHYSICAL_RESOURCE_LEDGER_HPP

#include "RepairResult.hpp"
#include "SimulationConfig.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace dynamic_spare
{

struct SpareDemand
{
    std::size_t rows = 0;
    std::size_t columns = 0;
};

struct PhysicalSpareLine
{
    std::size_t physicalId = 0;
    // -1 denotes a group-owned line in the global pool.
    int ownerSubarray = -1;
    SpareDimension dimension = SpareDimension::Row;
    std::size_t ownerLineIndex = 0;
    bool shareable = false;
    std::optional<std::size_t> assignedSubarray;
};

struct BorrowTransfer
{
    int donorSubarray = -1;
    std::size_t borrowerSubarray = 0;
    SpareDimension dimension = SpareDimension::Row;
    std::size_t physicalLineId = 0;
};

struct LedgerAllocationResult
{
    bool success = false;
    std::array<SpareDemand, kSubarrayCount> demands;
    std::array<SpareDemand, kSubarrayCount> borrowedBySubarray;
    std::array<SpareDemand, kSubarrayCount> lentBySubarray;
    std::vector<PhysicalSpareLine> lines;
    std::vector<BorrowTransfer> transfers;

    std::size_t physicalRows = 0;
    std::size_t physicalColumns = 0;
    std::size_t usedRows = 0;
    std::size_t usedColumns = 0;
    std::size_t unusedRows = 0;
    std::size_t unusedColumns = 0;

    std::size_t borrowRequests = 0;
    std::size_t successfulBorrows = 0;
    std::size_t failedBorrows = 0;
    std::size_t donorStarvationCount = 0;
    std::size_t globalRowsUsed = 0;
    std::size_t globalColumnsUsed = 0;
    std::size_t remainingGlobalRows = 0;
    std::size_t remainingGlobalColumns = 0;

    std::size_t borrowedRows() const noexcept;
    std::size_t borrowedColumns() const noexcept;
};

class PhysicalResourceLedger
{
public:
    explicit PhysicalResourceLedger(const SimulationConfig &config);

    LedgerAllocationResult allocate(
        const std::array<SpareDemand, kSubarrayCount> &demands) const;

    // Allocate tiles A..committedTileCount-1 in order.  Each tile borrows
    // before the next tile receives local resources, so earlier assignments
    // cannot be reclaimed.  Used by the non-backtracking EARLY policy.
    LedgerAllocationResult allocateSequential(
        const std::array<SpareDemand, kSubarrayCount> &demands,
        std::size_t committedTileCount) const;

    std::size_t physicalRows() const noexcept;
    std::size_t physicalColumns() const noexcept;

    bool canBorrow(
        int ownerSubarray,
        std::size_t borrowerSubarray,
        SpareDimension dimension) const noexcept;

private:
    SimulationConfig config_;
    std::vector<PhysicalSpareLine> physicalLines_;

    bool mayBorrow(
        int ownerSubarray,
        std::size_t borrowerSubarray,
        SpareDimension dimension) const noexcept;
};

} // namespace dynamic_spare

#endif
