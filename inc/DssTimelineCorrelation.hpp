#ifndef DSS_TIMELINE_CORRELATION_HPP
#define DSS_TIMELINE_CORRELATION_HPP

#include "DssBistFaultTimeline.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace dynamic_spare
{

enum class DssTimelinePolicy
{
    Early,
    GroupNoScratchV2
};

const char *toString(DssTimelinePolicy policy) noexcept;

// These are observed controller-relative edges from the frozen S1B-1 core
// harnesses.  They are inputs to correlation, not a replacement for RTL.
struct DssDecisionRelativeTrace
{
    DssTimelinePolicy policy = DssTimelinePolicy::Early;
    std::string candidateContractId;
    std::array<std::size_t, kSubarrayCount> saFaultCounts{};
    std::array<unsigned int, kSubarrayCount> selectedConfigIds{};
    std::array<unsigned int, kSubarrayCount> selectedPatternIds{};
    std::array<std::optional<std::uint64_t>, kSubarrayCount>
        saDecisionCycles;
    std::uint64_t groupDecisionCycle = 0;
    bool repairable = false;
    std::optional<std::size_t> failurePosition;
};

struct DssTimelineCorrelationRequest
{
    std::string transactionId;
    std::string faultCorpusId;
    std::optional<std::uint64_t> seed;
    FaultGroup faults;
    SerialBistSchedule schedule;
    DssDecisionRelativeTrace decision;

    // The correlation harness waits this many transaction-cycle edges after
    // GROUP_TEST_DONE before presenting start_i to the unmodified DSS core.
    // It is reporting glue, not an architectural wait state.
    std::uint64_t harnessStartOffsetCycles = 1;
};

struct DssTimelineCorrelationTrace
{
    std::string transactionId;
    std::string faultCorpusId;
    std::optional<std::uint64_t> seed;
    DssTimelinePolicy policy = DssTimelinePolicy::Early;
    std::string candidateContractId;
    std::array<std::size_t, kSubarrayCount> saFaultCounts{};
    std::array<unsigned int, kSubarrayCount> selectedConfigIds{};
    std::array<unsigned int, kSubarrayCount> selectedPatternIds{};
    DssBistFaultTimeline bist;
    std::uint64_t dssStartCycle = 0;
    std::array<std::optional<std::uint64_t>, kSubarrayCount>
        saDecisionCycles;
    std::uint64_t groupDecisionCycle = 0;
    bool repairable = false;
    std::optional<std::size_t> failurePosition;
    std::uint64_t deltaTestDoneToDssStart = 0;
    std::uint64_t deltaTestDoneToGroupDone = 0;
    std::optional<std::uint64_t> deltaLastFaultToGroupDone;
    std::optional<std::uint64_t> bistTailCycles;
    std::optional<std::uint64_t> faultToGroupDoneCycles;
    bool decompositionIdentityHolds = false;
};

DssTimelineCorrelationTrace correlateDssTimeline(
    const DssTimelineCorrelationRequest &request);

} // namespace dynamic_spare

#endif
