#ifndef DYNAMIC_SPARE_SHARING_REPAIR_ATTEMPT_SOLVER_HPP
#define DYNAMIC_SPARE_SHARING_REPAIR_ATTEMPT_SOLVER_HPP

#include "Fault.hpp"
#include "RepairResult.hpp"

#include <vector>

namespace dynamic_spare
{

struct RECAMSolverRequest;

// Storage-independent boundary between the dynamic spare-sharing allocator
// and one subarray repair attempt.  Implementations may change storage/search
// timing, but must preserve the RECAM repair semantics exposed in the result.
class RepairAttemptSolver
{
public:
    virtual ~RepairAttemptSolver() = default;

    virtual RepairAttemptResult solve(
        const std::vector<Fault> &faults,
        const RECAMSolverRequest &request) const = 0;
};

} // namespace dynamic_spare

#endif
