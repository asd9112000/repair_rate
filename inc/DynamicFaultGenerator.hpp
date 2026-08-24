#ifndef DYNAMIC_SPARE_SHARING_DYNAMIC_FAULT_GENERATOR_HPP
#define DYNAMIC_SPARE_SHARING_DYNAMIC_FAULT_GENERATOR_HPP

#include "DynamicRepairSimulator.hpp"
#include "SimulationConfig.hpp"

#include <cstddef>
#include <vector>

namespace dynamic_spare
{

class DynamicFaultGenerator
{
public:
    explicit DynamicFaultGenerator(const SimulationConfig &config);

    FaultGroup generate(std::size_t runIndex);
    std::vector<FaultGroup> generateRuns(std::size_t runCount);

private:
    class StableRandom;

    SimulationConfig config_;
    std::vector<std::size_t> countsForGroup() const;
    std::vector<Fault> faultsForSubarray(
        std::size_t count,
        std::size_t runIndex,
        std::size_t subarray,
        StableRandom &random) const;
};

} // namespace dynamic_spare

#endif
