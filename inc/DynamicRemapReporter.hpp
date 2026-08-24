#ifndef DYNAMIC_SPARE_SHARING_DYNAMIC_REMAP_REPORTER_HPP
#define DYNAMIC_SPARE_SHARING_DYNAMIC_REMAP_REPORTER_HPP

#include "DynamicRepairSimulator.hpp"
#include "RepairResult.hpp"
#include "SimulationConfig.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace dynamic_spare
{

struct RemapWriteSummary
{
    std::size_t successfulGroups = 0;
    std::size_t failedGroups = 0;
    std::size_t lineMappings = 0;
    std::size_t bufferMappings = 0;
};

class DynamicRemapReporter
{
public:
    static RemapWriteSummary write(
        const std::filesystem::path &fullPath,
        const std::filesystem::path &simplifiedPath,
        const SimulationConfig &config,
        const std::vector<FaultGroup> &faultGroups,
        const std::vector<GroupRepairResult> &results);
};

} // namespace dynamic_spare

#endif
