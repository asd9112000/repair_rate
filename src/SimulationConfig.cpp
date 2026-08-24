#include "../inc/SimulationConfig.hpp"

#include <limits>
#include <numeric>
#include <stdexcept>

namespace dynamic_spare
{
namespace
{

std::uint64_t legacyHybridCapacity(int rows, int columns)
{
    const std::uint64_t r = static_cast<std::uint64_t>(rows);
    const std::uint64_t c = static_cast<std::uint64_t>(columns);
    return r * (c - 1) + c * (r - 1);
}

void validateLegacyRecamCapacity(int rows, int columns)
{
    const std::uint64_t dimension =
        static_cast<std::uint64_t>(rows) +
        static_cast<std::uint64_t>(columns);
    if (dimension > static_cast<std::uint64_t>(
            std::numeric_limits<int>::max()) ||
        legacyHybridCapacity(rows, columns) >
            static_cast<std::uint64_t>(std::numeric_limits<int>::max()))
    {
        throw std::invalid_argument(
            "RECAM capacity exceeds the legacy core's safe int range");
    }
}

} // namespace

void SimulationConfig::validate() const
{
    if (numSubarrays != kSubarrayCount)
    {
        throw std::invalid_argument(
            "DynamicSpareSharing currently requires exactly four subarrays");
    }
    if (spareRows <= 0 || spareColumns <= 0)
    {
        throw std::invalid_argument("Rs and Cs must be positive");
    }
    validateLegacyRecamCapacity(spareRows, spareColumns);
    if (sharedRows < 0 || sharedRows > spareRows ||
        sharedColumns < 0 || sharedColumns > spareColumns)
    {
        throw std::invalid_argument(
            "shared rows/columns must be within the physical local capacity");
    }
    if (bufferCamEntries < 0)
    {
        throw std::invalid_argument("Buffer CAM entries cannot be negative");
    }
    if (dataWidthBits == 0 || rowAddressWidthBits == 0 ||
        columnAddressWidthBits == 0)
    {
        throw std::invalid_argument("Data and address widths must be positive");
    }
    if (hybridCamEntryWidthBits.has_value() &&
        *hybridCamEntryWidthBits == 0)
    {
        throw std::invalid_argument(
            "Hybrid CAM entry width must be positive when configured");
    }
    if (simulationRuns == 0)
    {
        throw std::invalid_argument("simulation_runs must be positive");
    }
    if (memoryRows == 0 || memoryColumns == 0)
    {
        throw std::invalid_argument(
            "Memory row/column dimensions must be positive");
    }
    if (memoryRows > static_cast<std::uint32_t>(
            std::numeric_limits<int>::max()) ||
        memoryColumns > static_cast<std::uint32_t>(
            std::numeric_limits<int>::max()))
    {
        throw std::invalid_argument(
            "Memory addresses exceed the legacy signed-int range");
    }
    const std::uint64_t memoryCells =
        static_cast<std::uint64_t>(memoryRows) * memoryColumns;
    if (faultCount > memoryCells * kSubarrayCount)
    {
        throw std::invalid_argument(
            "fault_count exceeds the four-subarray unique cell capacity");
    }
    if (modifiers.minimumRowReserve < 0 ||
        modifiers.minimumRowReserve > spareRows ||
        modifiers.minimumColumnReserve < 0 ||
        modifiers.minimumColumnReserve > spareColumns)
    {
        throw std::invalid_argument(
            "Minimum local reserve must be within the local spare capacity");
    }
    if (modifiers.maximumGroupBorrowedSpares < 0)
    {
        throw std::invalid_argument(
            "Maximum group borrowed spare count cannot be negative");
    }

    if (faultCountModel == FaultCountModel::UserDefined)
    {
        for (int count : userDefinedFaultCounts)
        {
            if (count < 0)
            {
                throw std::invalid_argument(
                    "User-defined fault counts cannot be negative");
            }
            if (static_cast<std::uint64_t>(count) > memoryCells)
            {
                throw std::invalid_argument(
                    "A user-defined fault count exceeds unique memory cells");
            }
        }
        const std::uint64_t userTotal = std::accumulate(
            userDefinedFaultCounts.begin(), userDefinedFaultCounts.end(),
            std::uint64_t{0});
        if (userTotal != faultCount)
        {
            throw std::invalid_argument(
                "User-defined A/B/C/D fault counts must sum to fault_count");
        }
    }

    if (topology == SharingTopology::GlobalPool)
    {
        if (!globalPool.has_value())
        {
            throw std::invalid_argument(
                "Global-pool topology requires a pool configuration");
        }
        const GlobalPoolConfiguration &pool = *globalPool;
        if (pool.localRowsPerSubarray < 0 ||
            pool.localColumnsPerSubarray < 0 ||
            pool.globalRows < 0 || pool.globalColumns < 0)
        {
            throw std::invalid_argument(
                "Global-pool capacities cannot be negative");
        }
        const std::uint64_t subarrayCount =
            static_cast<std::uint64_t>(numSubarrays);
        const std::uint64_t physicalRows =
            subarrayCount * static_cast<std::uint64_t>(spareRows);
        const std::uint64_t physicalColumns =
            subarrayCount * static_cast<std::uint64_t>(spareColumns);
        const std::uint64_t redistributedRows =
            subarrayCount *
                static_cast<std::uint64_t>(pool.localRowsPerSubarray) +
            static_cast<std::uint64_t>(pool.globalRows);
        const std::uint64_t redistributedColumns =
            subarrayCount *
                static_cast<std::uint64_t>(pool.localColumnsPerSubarray) +
            static_cast<std::uint64_t>(pool.globalColumns);
        if (redistributedRows != physicalRows ||
            redistributedColumns != physicalColumns)
        {
            throw std::invalid_argument(
                "Global-pool redistribution must conserve physical spares");
        }
    }

    const std::uint64_t physicalRows =
        static_cast<std::uint64_t>(numSubarrays) *
        static_cast<std::uint64_t>(spareRows);
    const std::uint64_t physicalColumns =
        static_cast<std::uint64_t>(numSubarrays) *
        static_cast<std::uint64_t>(spareColumns);
    if (physicalRows > std::numeric_limits<std::size_t>::max() ||
        physicalColumns > std::numeric_limits<std::size_t>::max())
    {
        throw std::invalid_argument(
            "Physical spare totals exceed the platform size range");
    }
}

const char *toString(FaultCountModel model) noexcept
{
    switch (model)
    {
        case FaultCountModel::FileProvided: return "file_provided";
        case FaultCountModel::Uniform: return "uniform";
        case FaultCountModel::ModerateImbalance: return "moderate_imbalance";
        case FaultCountModel::StrongImbalance: return "strong_imbalance";
        case FaultCountModel::Hotspot: return "hotspot";
        case FaultCountModel::UserDefined: return "user_defined";
    }
    return "unknown";
}

const char *toString(FaultSpatialModel model) noexcept
{
    switch (model)
    {
        case FaultSpatialModel::FileProvided: return "file_provided";
        case FaultSpatialModel::Uniform: return "uniform";
        case FaultSpatialModel::Mixed: return "mixed";
        case FaultSpatialModel::Clustered: return "clustered";
    }
    return "unknown";
}

const char *toString(SharingTopology topology) noexcept
{
    switch (topology)
    {
        case SharingTopology::NoSharing: return "no_sharing";
        case SharingTopology::Directional: return "directional";
        case SharingTopology::GlobalPool: return "global_pool";
        case SharingTopology::PairwiseEdge: return "pairwise_edge";
    }
    return "unknown";
}

const char *toString(FaultInformationStorage storage) noexcept
{
    switch (storage)
    {
        case FaultInformationStorage::CAM: return "cam";
        case FaultInformationStorage::SRAM: return "sram";
    }
    return "unknown";
}

} // namespace dynamic_spare
