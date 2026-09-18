#include "DssBistFaultTimeline.hpp"

#include <algorithm>
#include <stdexcept>

namespace dynamic_spare
{

const char *toString(FaultAcceptSource source) noexcept
{
    switch (source)
    {
        case FaultAcceptSource::None: return "NONE";
        case FaultAcceptSource::ModelDerived: return "MODEL_DERIVED";
    }
    return "UNKNOWN";
}

DssBistFaultTimeline materializeDssBistFaultTimeline(
    const FaultGroup &faults,
    const SerialBistSchedule &schedule)
{
    schedule.validate();
    if (schedule.subarrayCount != kSubarrayCount)
    {
        throw std::invalid_argument(
            "DSS BIST timeline requires exactly four serial subarrays");
    }

    DssBistFaultTimeline timeline;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        SaBistFaultTimeline &saTimeline = timeline.subarrays[subarray];
        saTimeline.testStartCycle = schedule.subarrayStartCycle(subarray);
        saTimeline.testDoneCycle = schedule.subarrayCompletionCycle(subarray);

        for (std::size_t faultIndex = 0;
             faultIndex < faults[subarray].size(); ++faultIndex)
        {
            const Fault &fault = faults[subarray][faultIndex];
            if (fault.SubarrayID != static_cast<int>(subarray))
            {
                throw std::invalid_argument(
                    "Fault group index and physical SubarrayID differ");
            }

            const std::uint64_t acceptCycle = schedule.arrivalCycle(fault);
            if (acceptCycle > saTimeline.testDoneCycle)
            {
                throw std::logic_error(
                    "A serial BIST fault accept occurs after SA test completion");
            }
            saTimeline.hasAcceptedFault = true;
            saTimeline.faultAcceptSource = FaultAcceptSource::ModelDerived;
            if (!saTimeline.lastFaultAcceptCycle ||
                acceptCycle > *saTimeline.lastFaultAcceptCycle)
            {
                saTimeline.lastFaultAcceptCycle = acceptCycle;
            }
            timeline.acceptedFaults.push_back(
                {subarray, faultIndex, fault, acceptCycle});
        }
    }

    std::sort(timeline.acceptedFaults.begin(), timeline.acceptedFaults.end(),
              [](const AcceptedFaultEvent &left,
                 const AcceptedFaultEvent &right) {
                  if (left.acceptCycle != right.acceptCycle)
                  {
                      return left.acceptCycle < right.acceptCycle;
                  }
                  if (left.subarrayId != right.subarrayId)
                  {
                      return left.subarrayId < right.subarrayId;
                  }
                  return left.faultIndex < right.faultIndex;
              });

    timeline.group.testDoneCycle = schedule.completionCycle();
    for (const SaBistFaultTimeline &saTimeline : timeline.subarrays)
    {
        if (!saTimeline.hasAcceptedFault)
        {
            continue;
        }
        timeline.group.hasAcceptedFault = true;
        if (!timeline.group.lastFaultAcceptCycle ||
            *saTimeline.lastFaultAcceptCycle >
                *timeline.group.lastFaultAcceptCycle)
        {
            timeline.group.lastFaultAcceptCycle =
                saTimeline.lastFaultAcceptCycle;
        }
    }
    return timeline;
}

} // namespace dynamic_spare
