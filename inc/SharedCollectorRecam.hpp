#ifndef DYNAMIC_SPARE_SHARING_SHARED_COLLECTOR_RECAM_HPP
#define DYNAMIC_SPARE_SHARING_SHARED_COLLECTOR_RECAM_HPP

#include "RECAMSolverAdapter.hpp"

#include <cstddef>
#include <vector>

namespace dynamic_spare
{

constexpr std::size_t kDirectionalSharedCollectorPivots = 5;
constexpr std::size_t kDirectionalSharedCollectorHybridEntries = 11;

// Model-B2: collect one maximum logical fault state, then evaluate each
// configuration against that same retained state.  This intentionally does
// not alter the standalone RECAMSolverAdapter contract.
std::vector<RepairAttemptResult> solveSharedCollectorConfigSet(
    const std::vector<Fault> &faults,
    const std::vector<RECAMSolverRequest> &requests);

} // namespace dynamic_spare

#endif
