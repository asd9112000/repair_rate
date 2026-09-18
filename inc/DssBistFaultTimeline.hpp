#ifndef DSS_BIST_FAULT_TIMELINE_HPP
#define DSS_BIST_FAULT_TIMELINE_HPP

#include "DynamicRepairSimulator.hpp"
#include "FaultAddress.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace dynamic_spare
{

// This reporting-only trace does not participate in candidate selection,
// ledger state, or repairability.  A fault list is accepted by the model at
// the completed serial-BIST word event selected by SerialBistSchedule.
enum class FaultAcceptSource
{
    None,
    ModelDerived
};

const char *toString(FaultAcceptSource source) noexcept;

struct AcceptedFaultEvent
{
    std::size_t subarrayId = 0;
    std::size_t faultIndex = 0;
    Fault fault{};
    std::uint64_t acceptCycle = 0;
};

struct SaBistFaultTimeline
{
    std::uint64_t testStartCycle = 0;
    std::uint64_t testDoneCycle = 0;
    bool hasAcceptedFault = false;
    std::optional<std::uint64_t> lastFaultAcceptCycle;
    FaultAcceptSource faultAcceptSource = FaultAcceptSource::None;
};

struct GroupBistFaultTimeline
{
    std::uint64_t testDoneCycle = 0;
    bool hasAcceptedFault = false;
    std::optional<std::uint64_t> lastFaultAcceptCycle;
};

struct DssBistFaultTimeline
{
    std::array<SaBistFaultTimeline, kSubarrayCount> subarrays{};
    GroupBistFaultTimeline group;
    std::vector<AcceptedFaultEvent> acceptedFaults;
};

// Materializes serial schedule boundaries and model-derived fault acceptance
// for one fixed A->B->C->D group.  It neither advances a device scheduler nor
// launches a DSS decision; callers correlate this trace with those endpoints.
DssBistFaultTimeline materializeDssBistFaultTimeline(
    const FaultGroup &faults,
    const SerialBistSchedule &schedule);

} // namespace dynamic_spare

#endif
