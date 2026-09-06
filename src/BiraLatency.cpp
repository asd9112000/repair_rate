#include "../inc/BiraLatency.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

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

} // namespace

const char *toString(BiraStorageTechnology technology) noexcept
{
    switch (technology)
    {
        case BiraStorageTechnology::Cam: return "CAM";
        case BiraStorageTechnology::Sram: return "SRAM";
    }
    return "UNKNOWN";
}

const char *toString(BiraTimingModel model) noexcept
{
    switch (model)
    {
        case BiraTimingModel::AggregateWorkOnly:
            return "AGGREGATE_WORK_ONLY";
        case BiraTimingModel::EventDrivenQueue:
            return "EVENT_DRIVEN_QUEUE";
    }
    return "UNKNOWN";
}

void finalizeBiraWork(BiraLatencyResult &result)
{
    result.totalWorkCycles = checkedAdd(
        result.faultCollectionWorkCycles,
        checkedAdd(
            result.repairAnalysisWorkCycles,
            result.sharingAllocationWorkCycles,
            "BIRA work-cycle total overflow"),
        "BIRA work-cycle total overflow");
}

void accumulateBiraWork(
    BiraLatencyResult &destination,
    const BiraLatencyResult &source)
{
    const bool destinationHasWork = destination.totalWorkCycles != 0 ||
        destination.faultCollectionWorkCycles != 0 ||
        destination.repairAnalysisWorkCycles != 0 ||
        destination.sharingAllocationWorkCycles != 0;
    if (destinationHasWork &&
        destination.storageTechnology != source.storageTechnology)
    {
        throw std::invalid_argument(
            "Cannot aggregate CAM and SRAM BIRA work into one result");
    }
    if (!destinationHasWork)
    {
        destination.storageTechnology = source.storageTechnology;
    }

    destination.faultCollectionWorkCycles = checkedAdd(
        destination.faultCollectionWorkCycles,
        source.faultCollectionWorkCycles,
        "BIRA fault-collection work overflow");
    destination.repairAnalysisWorkCycles = checkedAdd(
        destination.repairAnalysisWorkCycles,
        source.repairAnalysisWorkCycles,
        "BIRA repair-analysis work overflow");
    destination.sharingAllocationWorkCycles = checkedAdd(
        destination.sharingAllocationWorkCycles,
        source.sharingAllocationWorkCycles,
        "BIRA sharing-allocation work overflow");
    destination.timingModel = BiraTimingModel::AggregateWorkOnly;
    destination.bistTimelineModeled = false;
    finalizeBiraWork(destination);
}

void setBiraEventTimeline(
    BiraLatencyResult &result,
    std::uint64_t firstFaultArrivalCycle,
    std::uint64_t lastFaultArrivalCycle,
    std::uint64_t bistCompletionCycle,
    std::uint64_t faultCollectionCompletionCycle,
    std::uint64_t solutionReadyCycle,
    std::size_t maximumFaultQueueDepth,
    std::uint64_t totalFaultQueueWaitCycles,
    std::uint64_t bistBackpressureCycles)
{
    if (firstFaultArrivalCycle > lastFaultArrivalCycle)
    {
        throw std::invalid_argument(
            "First BIRA fault arrival cannot follow the last arrival");
    }
    if (lastFaultArrivalCycle > bistCompletionCycle)
    {
        throw std::invalid_argument(
            "A BIST-generated fault cannot arrive after BIST completion");
    }
    if (lastFaultArrivalCycle > faultCollectionCompletionCycle)
    {
        throw std::invalid_argument(
            "Fault collection cannot finish before the last fault arrival");
    }
    if (faultCollectionCompletionCycle > solutionReadyCycle)
    {
        throw std::invalid_argument(
            "The final repair solution cannot precede fault collection");
    }

    result.timingModel = BiraTimingModel::EventDrivenQueue;
    result.bistTimelineModeled = true;
    result.firstFaultArrivalCycle = firstFaultArrivalCycle;
    result.lastFaultArrivalCycle = lastFaultArrivalCycle;
    result.bistCompletionCycle = bistCompletionCycle;
    result.faultCollectionCompletionCycle =
        faultCollectionCompletionCycle;
    result.solutionReadyCycle = solutionReadyCycle;
    result.faultCollectionLatencyAfterBistCycles =
        faultCollectionCompletionCycle > bistCompletionCycle
            ? faultCollectionCompletionCycle - bistCompletionCycle
            : 0;
    result.latencyAfterBistCycles = solutionReadyCycle > bistCompletionCycle
        ? solutionReadyCycle - bistCompletionCycle
        : 0;
    result.maximumFaultQueueDepth = maximumFaultQueueDepth;
    result.totalFaultQueueWaitCycles = totalFaultQueueWaitCycles;
    result.bistBackpressureCycles = bistBackpressureCycles;
}

BiraLatencyResult simulateDecoupledFifo(
    BiraStorageTechnology technology,
    std::vector<BiraFaultArrivalEvent> events,
    const DecoupledFifoConfig &config)
{
    std::stable_sort(
        events.begin(), events.end(),
        [](const BiraFaultArrivalEvent &left,
           const BiraFaultArrivalEvent &right)
        {
            if (left.arrivalCycle != right.arrivalCycle)
            {
                return left.arrivalCycle < right.arrivalCycle;
            }
            return left.sequence < right.sequence;
        });

    BiraLatencyResult result;
    result.storageTechnology = technology;
    result.repairAnalysisWorkCycles =
        config.finalRepairAnalysisCycles;

    std::uint64_t serverAvailableCycle = 0;
    std::uint64_t totalWaitCycles = 0;
    std::vector<std::pair<std::uint64_t, int>> queueTransitions;
    queueTransitions.reserve(events.size() * 2);
    for (const BiraFaultArrivalEvent &event : events)
    {
        if (event.arrivalCycle > config.bistCompletionCycle)
        {
            throw std::invalid_argument(
                "A decoupled-FIFO fault arrives after BIST completion");
        }
        if (event.serviceCycles == 0)
        {
            throw std::invalid_argument(
                "A decoupled-FIFO fault requires positive service cycles");
        }

        const std::uint64_t serviceStart = std::max(
            event.arrivalCycle, serverAvailableCycle);
        const std::uint64_t wait = serviceStart - event.arrivalCycle;
        totalWaitCycles = checkedAdd(
            totalWaitCycles, wait,
            "Decoupled-FIFO total wait-cycle overflow");
        result.faultCollectionWorkCycles = checkedAdd(
            result.faultCollectionWorkCycles,
            event.serviceCycles,
            "Decoupled-FIFO service-cycle overflow");
        if (wait != 0)
        {
            // At equal timestamps, departures are processed before arrivals,
            // matching a FIFO entry that starts service in that cycle.
            queueTransitions.push_back({event.arrivalCycle, 1});
            queueTransitions.push_back({serviceStart, -1});
        }
        serverAvailableCycle = checkedAdd(
            serviceStart, event.serviceCycles,
            "Decoupled-FIFO completion-cycle overflow");
    }
    finalizeBiraWork(result);

    std::sort(
        queueTransitions.begin(), queueTransitions.end(),
        [](const auto &left, const auto &right)
        {
            if (left.first != right.first)
            {
                return left.first < right.first;
            }
            return left.second < right.second;
        });
    std::size_t queueDepth = 0;
    std::size_t maximumQueueDepth = 0;
    for (const auto &transition : queueTransitions)
    {
        if (transition.second < 0)
        {
            if (queueDepth == 0)
            {
                throw std::logic_error(
                    "Decoupled-FIFO queue transition underflow");
            }
            --queueDepth;
        }
        else
        {
            ++queueDepth;
            maximumQueueDepth = std::max(maximumQueueDepth, queueDepth);
        }
    }

    const std::uint64_t firstArrival = events.empty()
        ? 0
        : events.front().arrivalCycle;
    const std::uint64_t lastArrival = events.empty()
        ? 0
        : events.back().arrivalCycle;
    const std::uint64_t collectionDone = events.empty()
        ? 0
        : serverAvailableCycle;
    const std::uint64_t analysisStart =
        config.finalAnalysisRequiresBistCompletion
            ? std::max(collectionDone, config.bistCompletionCycle)
            : collectionDone;
    const std::uint64_t solutionReady = checkedAdd(
        analysisStart,
        config.finalRepairAnalysisCycles,
        "Decoupled-FIFO solution-ready cycle overflow");

    setBiraEventTimeline(
        result,
        firstArrival,
        lastArrival,
        config.bistCompletionCycle,
        collectionDone,
        solutionReady,
        maximumQueueDepth,
        totalWaitCycles,
        0);
    return result;
}

} // namespace dynamic_spare
