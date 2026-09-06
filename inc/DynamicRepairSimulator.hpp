#ifndef DYNAMIC_SPARE_SHARING_DYNAMIC_REPAIR_SIMULATOR_HPP
#define DYNAMIC_SPARE_SHARING_DYNAMIC_REPAIR_SIMULATOR_HPP

#include "PhysicalResourceLedger.hpp"
#include "RepairAttemptSolver.hpp"
#include "RECAMSolverAdapter.hpp"
#include "RepairResult.hpp"
#include "SimulationConfig.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace dynamic_spare
{

using FaultGroup = std::array<std::vector<Fault>, kSubarrayCount>;

class DynamicRepairSimulator
{
public:
    DynamicRepairSimulator();
    explicit DynamicRepairSimulator(
        std::shared_ptr<const RepairAttemptSolver> solver);

    GroupRepairResult run(
        const FaultGroup &faults,
        const SimulationConfig &config,
        std::size_t runIndex,
        bool retainSelectedRemap = false) const;

private:
    std::shared_ptr<const RepairAttemptSolver> solver_;
};

} // namespace dynamic_spare

#endif
