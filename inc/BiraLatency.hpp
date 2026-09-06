#ifndef BIRA_LATENCY_HPP
#define BIRA_LATENCY_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dynamic_spare
{

enum class BiraStorageTechnology
{
    Cam,
    Sram
};

enum class BiraTimingModel
{
    AggregateWorkOnly,
    EventDrivenQueue
};

const char *toString(BiraStorageTechnology technology) noexcept;
const char *toString(BiraTimingModel model) noexcept;

// Technology-neutral BIRA latency result. Work-cycle fields are available
// from the current aggregate CAM/SRAM models. Timeline fields are valid only
// when bistTimelineModeled is true and an event-driven BIST/fault-arrival
// schedule has been applied.
struct BiraLatencyMetrics
{
    BiraStorageTechnology storageTechnology = BiraStorageTechnology::Cam;
    BiraTimingModel timingModel = BiraTimingModel::AggregateWorkOnly;

    std::uint64_t faultCollectionWorkCycles = 0;
    std::uint64_t repairAnalysisWorkCycles = 0;
    std::uint64_t sharingAllocationWorkCycles = 0;
    std::uint64_t totalWorkCycles = 0;

    bool bistTimelineModeled = false;
    std::uint64_t firstFaultArrivalCycle = 0;
    std::uint64_t lastFaultArrivalCycle = 0;
    std::uint64_t bistCompletionCycle = 0;
    std::uint64_t faultCollectionCompletionCycle = 0;
    std::uint64_t solutionReadyCycle = 0;
    std::uint64_t faultCollectionLatencyAfterBistCycles = 0;
    std::uint64_t latencyAfterBistCycles = 0;

    std::size_t maximumFaultQueueDepth = 0;
    std::uint64_t totalFaultQueueWaitCycles = 0;
    std::uint64_t bistBackpressureCycles = 0;
};

// Backward-compatible name retained while reporters and historical tests are
// migrated to the common CAM/SRAM metric terminology.
using BiraLatencyResult = BiraLatencyMetrics;

struct BiraFaultArrivalEvent
{
    std::uint64_t arrivalCycle = 0;
    std::uint64_t serviceCycles = 0;
    // Stable tie-break for faults detected in the same BIST cycle.
    std::size_t sequence = 0;
};

struct DecoupledFifoConfig
{
    std::uint64_t bistCompletionCycle = 0;
    std::uint64_t finalRepairAnalysisCycles = 0;
    // A final group solution normally cannot be committed until BIST reports
    // that no more faults will arrive. Set false only for an explicitly
    // modeled dynamic solution that may become ready before BIST completion.
    bool finalAnalysisRequiresBistCompletion = true;
};

void finalizeBiraWork(BiraLatencyResult &result);

// Adds work counters only. Event timelines cannot be summed without an
// explicit engine schedule, so the destination becomes AggregateWorkOnly.
void accumulateBiraWork(
    BiraLatencyResult &destination,
    const BiraLatencyResult &source);

void setBiraEventTimeline(
    BiraLatencyResult &result,
    std::uint64_t firstFaultArrivalCycle,
    std::uint64_t lastFaultArrivalCycle,
    std::uint64_t bistCompletionCycle,
    std::uint64_t faultCollectionCompletionCycle,
    std::uint64_t solutionReadyCycle,
    std::size_t maximumFaultQueueDepth,
    std::uint64_t totalFaultQueueWaitCycles,
    std::uint64_t bistBackpressureCycles);

// Models one unbounded, single-server FIFO between BIST and one BIRA engine.
// BIST never stalls; consequently bistBackpressureCycles is always zero.
BiraLatencyResult simulateDecoupledFifo(
    BiraStorageTechnology technology,
    std::vector<BiraFaultArrivalEvent> events,
    const DecoupledFifoConfig &config);

} // namespace dynamic_spare

#endif
