#include <iostream>
#include <stdexcept>
#include <string>

#include "BiraLatency.hpp"

namespace
{

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void verifyWorkBreakdown()
{
    dynamic_spare::BiraLatencyResult latency;
    latency.storageTechnology =
        dynamic_spare::BiraStorageTechnology::Sram;
    latency.faultCollectionWorkCycles = 20;
    latency.repairAnalysisWorkCycles = 7;
    latency.sharingAllocationWorkCycles = 3;
    dynamic_spare::finalizeBiraWork(latency);

    require(latency.totalWorkCycles == 30,
            "Common BIRA work-cycle total is incorrect");
    require(!latency.bistTimelineModeled &&
                latency.timingModel ==
                    dynamic_spare::BiraTimingModel::AggregateWorkOnly,
            "Work-only result incorrectly claims a BIST timeline");
}

void verifyCollectionFinishesAfterBist()
{
    dynamic_spare::BiraLatencyResult latency;
    dynamic_spare::setBiraEventTimeline(
        latency,
        10,   // first fault arrival
        90,   // last fault arrival
        100,  // BIST done
        125,  // collection done after draining the queue
        140,  // final solution ready
        6,
        75,
        0);

    require(latency.bistTimelineModeled &&
                latency.timingModel ==
                    dynamic_spare::BiraTimingModel::EventDrivenQueue,
            "Event-driven BIRA timeline was not marked valid");
    require(latency.faultCollectionLatencyAfterBistCycles == 25 &&
                latency.latencyAfterBistCycles == 40,
            "After-BIST latency is incorrect when collection drains late");
    require(latency.maximumFaultQueueDepth == 6 &&
                latency.totalFaultQueueWaitCycles == 75,
            "Fault-collection congestion metrics were lost");
}

void verifyCollectionFinishesBeforeBist()
{
    dynamic_spare::BiraLatencyResult latency;
    dynamic_spare::setBiraEventTimeline(
        latency, 10, 70, 100, 80, 90, 1, 0, 0);
    require(latency.faultCollectionLatencyAfterBistCycles == 0 &&
                latency.latencyAfterBistCycles == 0,
            "Pre-BIST solution should have zero latency after BIST");
}

void verifyInvalidTimelineRejected()
{
    dynamic_spare::BiraLatencyResult latency;
    bool rejected = false;
    try
    {
        dynamic_spare::setBiraEventTimeline(
            latency, 10, 90, 100, 120, 110, 0, 0, 0);
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    require(rejected,
            "A solution preceding fault-collection completion was accepted");
}

void verifyDecoupledFifoCongestion()
{
    const std::vector<dynamic_spare::BiraFaultArrivalEvent> events{
        {1, 5, 0},
        {2, 5, 1},
        {3, 5, 2}};
    dynamic_spare::DecoupledFifoConfig config;
    config.bistCompletionCycle = 10;
    config.finalRepairAnalysisCycles = 2;

    const dynamic_spare::BiraLatencyResult latency =
        dynamic_spare::simulateDecoupledFifo(
            dynamic_spare::BiraStorageTechnology::Sram,
            events,
            config);

    require(latency.faultCollectionWorkCycles == 15 &&
                latency.repairAnalysisWorkCycles == 2 &&
                latency.totalWorkCycles == 17,
            "Decoupled FIFO lost its work-cycle breakdown");
    require(latency.faultCollectionCompletionCycle == 16 &&
                latency.solutionReadyCycle == 18 &&
                latency.faultCollectionLatencyAfterBistCycles == 6 &&
                latency.latencyAfterBistCycles == 8,
            "Decoupled FIFO completion/after-BIST timing is incorrect");
    require(latency.maximumFaultQueueDepth == 2 &&
                latency.totalFaultQueueWaitCycles == 12 &&
                latency.bistBackpressureCycles == 0,
            "Decoupled FIFO congestion accounting is incorrect");
}

void verifyDecoupledFifoDoesNotDelayBist()
{
    dynamic_spare::DecoupledFifoConfig config;
    config.bistCompletionCycle = 100;
    config.finalRepairAnalysisCycles = 0;
    const dynamic_spare::BiraLatencyResult latency =
        dynamic_spare::simulateDecoupledFifo(
            dynamic_spare::BiraStorageTechnology::Cam,
            {{10, 1, 0}, {20, 1, 1}},
            config);
    require(latency.faultCollectionCompletionCycle == 21 &&
                latency.solutionReadyCycle == 100 &&
                latency.latencyAfterBistCycles == 0 &&
                latency.bistBackpressureCycles == 0,
            "Decoupled FIFO incorrectly delayed an independent BIST");
}

void verifyDecoupledFifoSimultaneousArrivals()
{
    dynamic_spare::DecoupledFifoConfig config;
    config.bistCompletionCycle = 4;
    const dynamic_spare::BiraLatencyResult latency =
        dynamic_spare::simulateDecoupledFifo(
            dynamic_spare::BiraStorageTechnology::Cam,
            {{4, 2, 2}, {4, 2, 0}, {4, 2, 1}},
            config);
    require(latency.maximumFaultQueueDepth == 2 &&
                latency.totalFaultQueueWaitCycles == 6 &&
                latency.faultCollectionCompletionCycle == 10 &&
                latency.latencyAfterBistCycles == 6,
            "Decoupled FIFO mishandled simultaneous arrivals");
}

void verifyDecoupledFifoRejectsInvalidEvents()
{
    dynamic_spare::DecoupledFifoConfig config;
    config.bistCompletionCycle = 5;
    bool zeroServiceRejected = false;
    try
    {
        (void)dynamic_spare::simulateDecoupledFifo(
            dynamic_spare::BiraStorageTechnology::Cam,
            {{1, 0, 0}},
            config);
    }
    catch (const std::invalid_argument &)
    {
        zeroServiceRejected = true;
    }
    require(zeroServiceRejected,
            "Decoupled FIFO accepted a zero-cycle service event");

    bool lateArrivalRejected = false;
    try
    {
        (void)dynamic_spare::simulateDecoupledFifo(
            dynamic_spare::BiraStorageTechnology::Cam,
            {{6, 1, 0}},
            config);
    }
    catch (const std::invalid_argument &)
    {
        lateArrivalRejected = true;
    }
    require(lateArrivalRejected,
            "Decoupled FIFO accepted a post-BIST fault arrival");
}

} // namespace

int main()
{
    try
    {
        verifyWorkBreakdown();
        verifyCollectionFinishesAfterBist();
        verifyCollectionFinishesBeforeBist();
        verifyInvalidTimelineRejected();
        verifyDecoupledFifoCongestion();
        verifyDecoupledFifoDoesNotDelayBist();
        verifyDecoupledFifoSimultaneousArrivals();
        verifyDecoupledFifoRejectsInvalidEvents();
        std::cout << "BIRA latency tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "BIRA latency test failed: " << error.what() << '\n';
        return 1;
    }
}
