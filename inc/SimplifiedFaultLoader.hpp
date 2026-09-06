#ifndef DYNAMIC_SPARE_SHARING_SIMPLIFIED_FAULT_LOADER_HPP
#define DYNAMIC_SPARE_SHARING_SIMPLIFIED_FAULT_LOADER_HPP

#include "DynamicRepairSimulator.hpp"
#include "SimulationConfig.hpp"

#include <filesystem>
#include <vector>

namespace dynamic_spare
{

// Load the flat integration format:
// HBMID ChannelID BankID SubarrayGroupID SubarrayID Row Col
//
// Physical groups are returned in lexicographic hardware-address order, while
// fault order within each subarray is preserved exactly as it appears in the
// input file.
std::vector<FaultGroup> loadSimplifiedFaultGroups(
    const std::filesystem::path &path,
    const SimulationConfig &config);

// Load the legacy count-framed fault_generator/faults.faults grammar:
// list_count, then for each A/B/C/D list: fault_count + seven-field records.
std::vector<FaultGroup> loadCountFramedFaultGroups(
    const std::filesystem::path &path,
    const SimulationConfig &config);

} // namespace dynamic_spare

#endif
