#ifndef DYNAMIC_SPARE_SHARING_DYNAMIC_REMAP_REPORTER_HPP
#define DYNAMIC_SPARE_SHARING_DYNAMIC_REMAP_REPORTER_HPP

#include "DynamicRepairSimulator.hpp"
#include "RepairResult.hpp"
#include "SimulationConfig.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace dynamic_spare
{

// Runtime latency is separate from offline BIRA work.  BUFFMAP keeps the
// read-hit response latency for compatibility; the companion manifest carries
// the write-hit latency and deterministic SRAM placement.
enum class RuntimeRepairStorage
{
    Cam,
    SramSerial,
    SramChunked,
    SramWide
};

const char *toString(RuntimeRepairStorage storage) noexcept;

struct RuntimeRepairLatencyConfig
{
    RuntimeRepairStorage storage = RuntimeRepairStorage::Cam;
    std::uint32_t sramParallelism = 1;
    bool registeredSearch = false;
    std::uint64_t sramDataReadCycles = 0;
    std::uint64_t sramDataWriteCycles = 1;
    std::uint64_t sramMuxCycles = 1;
    // Frozen legacy CAM read endpoint remains three cycles.  A CAM write is
    // modeled as associative lookup plus replacement-data write.
    std::uint64_t camReadHitCycles = 3;
    std::uint64_t camWriteHitCycles = 2;

    void validate() const;
};

struct RemapWriteSummary
{
    std::size_t successfulGroups = 0;
    std::size_t failedGroups = 0;
    std::size_t lineMappings = 0;
    std::size_t bufferMappings = 0;
    std::size_t runtimeRepairEntries = 0;
};

class DynamicRemapReporter
{
public:
    static RemapWriteSummary write(
        const std::filesystem::path &fullPath,
        const std::filesystem::path &simplifiedPath,
        const SimulationConfig &config,
        const std::vector<FaultGroup> &faultGroups,
        const std::vector<GroupRepairResult> &results,
        const RuntimeRepairLatencyConfig &runtimeConfig = {});
};

} // namespace dynamic_spare

#endif
