#include "DssPostBistLatency.hpp"

#include <limits>
#include <stdexcept>

namespace dynamic_spare
{
namespace
{

std::uint64_t checkedAdd(
    std::uint64_t left,
    std::uint64_t right,
    const char *description)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
    {
        throw std::overflow_error(description);
    }
    return left + right;
}

std::size_t activeSaCount(const DssDecisionWorkPlan &plan)
{
    if (plan.groupRepairable)
    {
        if (plan.failurePosition != -1)
        {
            throw std::invalid_argument(
                "A repairable DSS plan cannot have a failure position");
        }
        return 4;
    }

    if (plan.failurePosition < 0 || plan.failurePosition >= 4)
    {
        throw std::invalid_argument(
            "A failed DSS plan requires failure position A through D");
    }
    return static_cast<std::size_t>(plan.failurePosition) + 1;
}

std::uint64_t decisionExecutionCycles(const DssDecisionWorkPlan &plan)
{
    const std::size_t activeCount = activeSaCount(plan);
    std::uint64_t allocationCycles = 0;
    for (std::size_t sa = 0; sa < plan.candidateEvaluations.size(); ++sa)
    {
        const unsigned int evaluations = plan.candidateEvaluations[sa];
        if (sa < activeCount)
        {
            if (evaluations == 0 || evaluations > 4)
            {
                throw std::invalid_argument(
                    "Each active DSS SA requires one through four evaluations");
            }
            allocationCycles = checkedAdd(
                allocationCycles,
                evaluations,
                "DSS allocation-cycle overflow");
        }
        else if (evaluations != 0)
        {
            throw std::invalid_argument(
                "A DSS plan contains work after first failure");
        }
    }

    if (plan.policy == DssPostBistPolicy::GroupNoScratch)
    {
        // Frozen V2 GROUP-NoScratch behavior: four canonical slots for each
        // of A/B/C/D are collected before sequential allocation begins.
        return checkedAdd(16, allocationCycles,
                          "DSS GROUP execution-cycle overflow");
    }
    return allocationCycles;
}

} // namespace

const char *toString(DssPostBistPolicy policy) noexcept
{
    switch (policy)
    {
        case DssPostBistPolicy::Early: return "EARLY";
        case DssPostBistPolicy::GroupNoScratch: return "GROUP_NO_SCRATCH";
    }
    return "UNKNOWN";
}

DssPostBistLatencyResult modelDssPostBistLatency(
    const DssPostBistRequest &request)
{
    if (request.faultEvents.empty())
    {
        throw std::invalid_argument(
            "A DSS post-BIST request requires at least one fault event");
    }

    const std::uint64_t executionCycles =
        decisionExecutionCycles(request.finalDecisionPlan);
    std::uint64_t previousCycle = 0;
    bool havePreviousCycle = false;
    bool haveFaultSetChange = false;
    std::uint64_t lastFaultCycle = 0;
    std::size_t changeCount = 0;

    // Every changing event launches a concurrent decision attempt.  Its work
    // is superseded by the next changing event, so only the last launch can
    // reach the observable frozen DSS decision boundary.
    for (const DssFaultSetEvent &event : request.faultEvents)
    {
        if (havePreviousCycle && event.cycle < previousCycle)
        {
            throw std::invalid_argument(
                "DSS fault events must be in nondecreasing BIST-cycle order");
        }
        if (event.cycle > request.bistEndCycle)
        {
            throw std::invalid_argument(
                "A DSS fault event cannot occur after BIST completion");
        }
        previousCycle = event.cycle;
        havePreviousCycle = true;
        if (event.changesFaultSet)
        {
            lastFaultCycle = event.cycle;
            haveFaultSetChange = true;
            ++changeCount;
        }
    }
    if (!haveFaultSetChange)
    {
        throw std::invalid_argument(
            "A DSS post-BIST request requires a fault-set-changing event");
    }

    DssPostBistLatencyResult result;
    result.policy = request.finalDecisionPlan.policy;
    result.faultCount = request.faultEvents.size();
    result.faultSetChangeCount = changeCount;
    result.lastFaultCycle = lastFaultCycle;
    result.bistEndCycle = request.bistEndCycle;
    result.gapCycles = request.bistEndCycle - lastFaultCycle;
    result.decisionExecutionCycles = executionCycles;
    result.decisionReadyCycle = checkedAdd(
        lastFaultCycle,
        executionCycles,
        "DSS decision-ready cycle overflow");
    result.latencyFromLastFaultCycles = executionCycles;
    result.latencyPostBistCycles = result.decisionReadyCycle >
        request.bistEndCycle
            ? result.decisionReadyCycle - request.bistEndCycle
            : 0;
    result.invalidatedDecisionCount = changeCount - 1;
    result.groupRepairable = request.finalDecisionPlan.groupRepairable;
    result.failurePosition = request.finalDecisionPlan.failurePosition;
    return result;
}

} // namespace dynamic_spare
