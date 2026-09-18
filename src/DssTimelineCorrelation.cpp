#include "DssTimelineCorrelation.hpp"

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

std::uint64_t checkedSubtract(
    std::uint64_t later,
    std::uint64_t earlier,
    const char *description)
{
    if (later < earlier)
    {
        throw std::invalid_argument(description);
    }
    return later - earlier;
}

std::array<std::size_t, kSubarrayCount> faultCounts(
    const FaultGroup &faults)
{
    std::array<std::size_t, kSubarrayCount> counts{};
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        counts[subarray] = faults[subarray].size();
    }
    return counts;
}

} // namespace

const char *toString(DssTimelinePolicy policy) noexcept
{
    switch (policy)
    {
        case DssTimelinePolicy::Early: return "EARLY";
        case DssTimelinePolicy::GroupNoScratchV2:
            return "GROUP_NO_SCRATCH_V2";
    }
    return "UNKNOWN";
}

DssTimelineCorrelationTrace correlateDssTimeline(
    const DssTimelineCorrelationRequest &request)
{
    if (request.transactionId.empty() || request.faultCorpusId.empty() ||
        request.decision.candidateContractId.empty())
    {
        throw std::invalid_argument(
            "Timeline correlation requires transaction, corpus, and candidate IDs");
    }
    if (request.decision.saFaultCounts != faultCounts(request.faults))
    {
        throw std::invalid_argument(
            "Decision trace fault counts do not match its FaultGroup corpus");
    }
    if (request.decision.groupDecisionCycle == 0)
    {
        throw std::invalid_argument(
            "Timeline correlation requires a terminal DSS decision edge");
    }
    if (request.decision.repairable !=
        !request.decision.failurePosition.has_value())
    {
        throw std::invalid_argument(
            "DSS repairability and failure position disagree");
    }

    DssTimelineCorrelationTrace trace;
    trace.transactionId = request.transactionId;
    trace.faultCorpusId = request.faultCorpusId;
    trace.seed = request.seed;
    trace.policy = request.decision.policy;
    trace.candidateContractId = request.decision.candidateContractId;
    trace.saFaultCounts = request.decision.saFaultCounts;
    trace.selectedConfigIds = request.decision.selectedConfigIds;
    trace.selectedPatternIds = request.decision.selectedPatternIds;
    trace.bist = materializeDssBistFaultTimeline(
        request.faults, request.schedule);
    trace.dssStartCycle = checkedAdd(
        trace.bist.group.testDoneCycle, request.harnessStartOffsetCycles,
        "DSS correlation start-cycle overflow");
    trace.repairable = request.decision.repairable;
    trace.failurePosition = request.decision.failurePosition;

    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const auto relativeCycle = request.decision.saDecisionCycles[subarray];
        if (!relativeCycle)
        {
            continue;
        }
        trace.saDecisionCycles[subarray] = checkedAdd(
            trace.dssStartCycle, *relativeCycle,
            "DSS correlation SA decision-cycle overflow");
        if (*trace.saDecisionCycles[subarray] >
            checkedAdd(trace.dssStartCycle,
                       request.decision.groupDecisionCycle,
                       "DSS correlation group decision-cycle overflow"))
        {
            throw std::invalid_argument(
                "SA decision edge occurs after group decision edge");
        }
    }
    trace.groupDecisionCycle = checkedAdd(
        trace.dssStartCycle, request.decision.groupDecisionCycle,
        "DSS correlation group decision-cycle overflow");
    trace.deltaTestDoneToDssStart = checkedSubtract(
        trace.dssStartCycle, trace.bist.group.testDoneCycle,
        "DSS start precedes GROUP_TEST_DONE");
    trace.deltaTestDoneToGroupDone = checkedSubtract(
        trace.groupDecisionCycle, trace.bist.group.testDoneCycle,
        "DSS group done precedes GROUP_TEST_DONE");

    if (trace.bist.group.lastFaultAcceptCycle)
    {
        trace.bistTailCycles = checkedSubtract(
            trace.bist.group.testDoneCycle,
            *trace.bist.group.lastFaultAcceptCycle,
            "GROUP_LAST_FAULT_ACCEPT exceeds GROUP_TEST_DONE");
        trace.deltaLastFaultToGroupDone = checkedSubtract(
            trace.groupDecisionCycle, *trace.bist.group.lastFaultAcceptCycle,
            "DSS group done precedes GROUP_LAST_FAULT_ACCEPT");
        trace.faultToGroupDoneCycles = *trace.deltaLastFaultToGroupDone;
        const std::uint64_t composed = checkedAdd(
            checkedAdd(*trace.bistTailCycles, trace.deltaTestDoneToDssStart,
                       "Timeline decomposition overflow"),
            request.decision.groupDecisionCycle,
            "Timeline decomposition overflow");
        trace.decompositionIdentityHolds =
            composed == *trace.deltaLastFaultToGroupDone;
    }
    return trace;
}

} // namespace dynamic_spare
