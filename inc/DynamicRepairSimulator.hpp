#ifndef DYNAMIC_SPARE_SHARING_DYNAMIC_REPAIR_SIMULATOR_HPP
#define DYNAMIC_SPARE_SHARING_DYNAMIC_REPAIR_SIMULATOR_HPP

#include "PhysicalResourceLedger.hpp"
#include "RECAMSolverAdapter.hpp"
#include "RepairResult.hpp"
#include "SimulationConfig.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace dynamic_spare
{

using FaultGroup = std::array<std::vector<Fault>, kSubarrayCount>;

class DynamicRepairSimulator
{
public:
    GroupRepairResult run(
        const FaultGroup &faults,
        const SimulationConfig &config,
        std::size_t runIndex,
        bool retainSelectedRemap = false) const;

private:
    RECAMSolverAdapter solver_;
};

} // namespace dynamic_spare

#endif
