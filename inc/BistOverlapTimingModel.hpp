#ifndef DSS_BIST_OVERLAP_TIMING_MODEL_HPP
#define DSS_BIST_OVERLAP_TIMING_MODEL_HPP

#include "DynamicRepairSimulator.hpp"
#include "FaultAddress.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace dynamic_spare
{

constexpr std::size_t kBistOverlapMaxFaultsPerSa = 12;
constexpr std::size_t kBistOverlapSweepEdges = 4;

// One model-level retained-update event. sourceFaultCount records all physical
// faults observed in one tested BIST word; the timing controller receives only
// this one selected-SA update at the completed-word edge.
struct FaultArrivalEvent
{
    std::uint64_t cycle = 0;
    std::size_t subarrayId = 0;
    std::size_t sourceFaultCount = 1;
};

struct BistOverlapTimingInput
{
    std::array<std::uint64_t, kSubarrayCount> testStartCycles{};
    std::array<std::uint64_t, kSubarrayCount> testDoneCycles{};
    std::vector<FaultArrivalEvent> faultEvents;
};

struct SaBistOverlapTiming
{
    std::size_t physicalFaultCount = 0;
    std::size_t acceptedUpdateCount = 0;
    std::uint64_t bistStartCycle = 0;
    std::uint64_t testDoneCycle = 0;
    std::optional<std::uint64_t> firstFaultAcceptCycle;
    std::optional<std::uint64_t> lastFaultAcceptCycle;
    std::uint64_t latestGeneration = 0;
    std::uint64_t readyGeneration = 0;
    std::uint64_t candidateReadyCycle = 0;
    std::optional<std::uint64_t> ownershipWaitCycles;
    std::optional<std::uint64_t> candidateCatchupLatency;
    std::int64_t hiddenAnalysisSlack = 0;
    std::uint64_t provisionalPostBistLatency = 0;
    bool fullyHidden = false;
};

struct GroupBistOverlapTiming
{
    std::uint64_t groupTestDoneCycle = 0;
    std::uint64_t latestCandidateReadyCycle = 0;
    std::int64_t hiddenAnalysisSlack = 0;
    std::uint64_t provisionalPostBistLatency = 0;
    bool allCandidatesReadyBeforeTestDone = false;
};

struct BistOverlapTimingResult
{
    std::array<SaBistOverlapTiming, kSubarrayCount> subarrays{};
    GroupBistOverlapTiming group;
    std::size_t aggregatedFaultEventCount = 0;
};

// Standalone timing-only model of the frozen S1G-A2G retained-controller
// schedule. It models latest-generation candidate readiness and the successful
// control-path ownership release; it does not evaluate repair candidates,
// allocate ledger resources, or integrate with HierarchicalRECAM.
class BistOverlapTimingModel
{
public:
    BistOverlapTimingResult evaluateGroup(
        const FaultGroup &faults,
        const SerialBistSchedule &schedule) const;

    // The event-stream entry point is used by directed RTL-equivalence tests.
    // Each event must be the sole selected-SA update at its cycle.
    BistOverlapTimingResult evaluateEventStream(
        const BistOverlapTimingInput &input) const;

    // Test-only cycle-by-cycle oracle for evaluateEventStream().
    BistOverlapTimingResult evaluateEventStreamCycleReference(
        const BistOverlapTimingInput &input) const;
};

} // namespace dynamic_spare

#endif
