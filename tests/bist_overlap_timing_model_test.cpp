#include "BistOverlapTimingModel.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

namespace
{

using dynamic_spare::BistOverlapTimingInput;
using dynamic_spare::BistOverlapTimingModel;
using dynamic_spare::BistOverlapTimingResult;
using dynamic_spare::FaultArrivalEvent;
using dynamic_spare::FaultGroup;
using dynamic_spare::SerialBistSchedule;

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

Fault makeFault(int subarray, int row, int physicalColumn)
{
    Fault result{};
    result.SubarrayID = subarray;
    result.r = row;
    result.c = physicalColumn;
    return result;
}

void requireEqual(
    const BistOverlapTimingResult &eventDriven,
    const BistOverlapTimingResult &reference,
    const std::string &label)
{
    require(eventDriven.aggregatedFaultEventCount ==
                reference.aggregatedFaultEventCount,
            label + ": aggregated event count differs");
    for (std::size_t sa = 0; sa < 4; ++sa)
    {
        const auto &left = eventDriven.subarrays[sa];
        const auto &right = reference.subarrays[sa];
        if (!(left.physicalFaultCount == right.physicalFaultCount &&
              left.acceptedUpdateCount == right.acceptedUpdateCount &&
              left.firstFaultAcceptCycle == right.firstFaultAcceptCycle &&
              left.lastFaultAcceptCycle == right.lastFaultAcceptCycle &&
              left.latestGeneration == right.latestGeneration &&
              left.readyGeneration == right.readyGeneration &&
              left.candidateReadyCycle == right.candidateReadyCycle &&
              left.ownershipWaitCycles == right.ownershipWaitCycles &&
              left.candidateCatchupLatency == right.candidateCatchupLatency &&
              left.hiddenAnalysisSlack == right.hiddenAnalysisSlack &&
              left.provisionalPostBistLatency == right.provisionalPostBistLatency &&
              left.fullyHidden == right.fullyHidden))
        {
            throw std::runtime_error(
                label + ": per-SA mismatch at " + std::to_string(sa) +
                " event_ready=" + std::to_string(left.candidateReadyCycle) +
                " reference_ready=" + std::to_string(right.candidateReadyCycle) +
                " event_generation=" + std::to_string(left.latestGeneration) +
                " reference_generation=" + std::to_string(right.latestGeneration) +
                " event_slack=" + std::to_string(left.hiddenAnalysisSlack) +
                " reference_slack=" + std::to_string(right.hiddenAnalysisSlack));
        }
    }
    require(eventDriven.group.groupTestDoneCycle == reference.group.groupTestDoneCycle &&
                eventDriven.group.latestCandidateReadyCycle ==
                    reference.group.latestCandidateReadyCycle &&
                eventDriven.group.hiddenAnalysisSlack == reference.group.hiddenAnalysisSlack &&
                eventDriven.group.provisionalPostBistLatency ==
                    reference.group.provisionalPostBistLatency &&
                eventDriven.group.allCandidatesReadyBeforeTestDone ==
                    reference.group.allCandidatesReadyBeforeTestDone,
            label + ": group event-driven/reference mismatch");
}

BistOverlapTimingResult evaluateAndCompare(
    const FaultGroup &faults,
    const std::string &label)
{
    BistOverlapTimingModel model;
    SerialBistSchedule schedule;
    const auto eventDriven = model.evaluateGroup(faults, schedule);

    BistOverlapTimingInput input;
    for (std::size_t sa = 0; sa < 4; ++sa)
    {
        input.testStartCycles[sa] = schedule.subarrayStartCycle(sa);
        input.testDoneCycles[sa] = schedule.subarrayCompletionCycle(sa);
    }
    for (std::size_t sa = 0; sa < 4; ++sa)
    {
        for (const Fault &fault : faults[sa])
        {
            const std::uint64_t cycle = schedule.arrivalCycle(fault);
            bool aggregated = false;
            for (FaultArrivalEvent &event : input.faultEvents)
            {
                if (event.cycle == cycle && event.subarrayId == sa)
                {
                    ++event.sourceFaultCount;
                    aggregated = true;
                    break;
                }
            }
            if (!aggregated)
            {
                input.faultEvents.push_back({cycle, sa, 1});
            }
        }
    }
    std::sort(input.faultEvents.begin(), input.faultEvents.end(),
              [](const FaultArrivalEvent &left, const FaultArrivalEvent &right) {
                  if (left.cycle != right.cycle)
                  {
                      return left.cycle < right.cycle;
                  }
                  return left.subarrayId < right.subarrayId;
              });
    const auto reference = model.evaluateEventStreamCycleReference(input);
    requireEqual(eventDriven, reference, label);
    return eventDriven;
}

void verifyDirectedTiming()
{
    FaultGroup faults;

    // T1-D1: zero-fault generation 0 is ready only after each SA owner sweep.
    {
        BistOverlapTimingInput zeroInput;
        SerialBistSchedule schedule;
        for (std::size_t sa = 0; sa < 4; ++sa)
        {
            zeroInput.testStartCycles[sa] = schedule.subarrayStartCycle(sa);
            zeroInput.testDoneCycles[sa] = schedule.subarrayCompletionCycle(sa);
        }
        BistOverlapTimingModel model;
        const auto standalone = model.evaluateEventStream(zeroInput);
        require(standalone.subarrays[0].candidateReadyCycle == 4,
                "T1-D1 direct event-driven zero-fault A readiness is wrong: " +
                    std::to_string(standalone.subarrays[0].candidateReadyCycle));
    }
    const auto zero = evaluateAndCompare(faults, "T1-D1");
    require(zero.subarrays[0].candidateReadyCycle == 4 &&
                zero.subarrays[1].candidateReadyCycle == 16390 &&
                zero.subarrays[2].candidateReadyCycle == 32774 &&
                zero.subarrays[3].candidateReadyCycle == 49158,
            "T1-D1 zero-fault ownership schedule is wrong");
    for (const auto &sa : zero.subarrays)
    {
        require(!sa.lastFaultAcceptCycle && !sa.candidateCatchupLatency,
                "T1-D1 zero fault must retain N/A catchup latency");
    }

    // T1-D2 and T1-D3: early and final-word current-owner A updates.
    faults[0].push_back(makeFault(0, 0, 0));
    const auto earlyA = evaluateAndCompare(faults, "T1-D2");
    require(earlyA.subarrays[0].candidateReadyCycle == 5 &&
                earlyA.subarrays[0].candidateCatchupLatency == 4,
            "T1-D2 early A catchup is not four sweep edges");
    faults = {};
    faults[0].push_back(makeFault(0, 511, 8191));
    const auto lateA = evaluateAndCompare(faults, "T1-D3");
    require(lateA.subarrays[0].candidateReadyCycle == 16388 &&
                lateA.subarrays[0].candidateCatchupLatency == 4,
            "T1-D3 late A catchup is not four sweep edges");

    // T1-D4/D5: equivalent first-word faults differ only by ownership wait.
    faults = {};
    faults[1].push_back(makeFault(1, 0, 0));
    const auto bWhileAOwns = evaluateAndCompare(faults, "T1-D4");
    require(bWhileAOwns.subarrays[1].candidateReadyCycle == 16390 &&
                bWhileAOwns.subarrays[1].candidateCatchupLatency == 5 &&
                bWhileAOwns.subarrays[1].ownershipWaitCycles == 1,
            "T1-D4 non-owner ownership wait is wrong");
    faults = {};
    for (int sa = 0; sa < 4; ++sa)
    {
        faults[sa].push_back(makeFault(sa, 0, 0));
    }
    const auto equivalent = evaluateAndCompare(faults, "T1-D5");
    require(equivalent.subarrays[0].candidateCatchupLatency == 4 &&
                equivalent.subarrays[1].candidateCatchupLatency == 5 &&
                equivalent.subarrays[2].candidateCatchupLatency == 5 &&
                equivalent.subarrays[3].candidateCatchupLatency == 5,
            "T1-D5 did not preserve the per-SA ownership effect");

    // T1-D6 and T1-D7: back-to-back updates and a slot-3-edge replacement.
    faults = {};
    faults[0].push_back(makeFault(0, 0, 0));
    faults[0].push_back(makeFault(0, 0, 256));
    faults[0].push_back(makeFault(0, 0, 512));
    const auto backToBack = evaluateAndCompare(faults, "T1-D6");
    require(backToBack.subarrays[0].latestGeneration == 3 &&
                backToBack.subarrays[0].candidateReadyCycle == 7 &&
                backToBack.subarrays[0].candidateCatchupLatency == 4,
            "T1-D6 retained stale sweep progress");
    faults = {};
    faults[0].push_back(makeFault(0, 0, 0));
    faults[0].push_back(makeFault(0, 0, 1024));
    const auto sameEdge = evaluateAndCompare(faults, "T1-D7");
    require(sameEdge.subarrays[0].latestGeneration == 2 &&
                sameEdge.subarrays[0].candidateReadyCycle == 9 &&
                sameEdge.subarrays[0].candidateCatchupLatency == 4,
            "T1-D7 did not give same-edge update priority");

    // T1-D8/D9: B receives a fault just before and on A's ownership-release edge.
    faults = {};
    faults[1].push_back(makeFault(1, 0, 0));
    const auto beforeBOwnership = evaluateAndCompare(faults, "T1-D8");
    require(beforeBOwnership.subarrays[1].candidateReadyCycle == 16390,
            "T1-D8 B readiness is wrong");
    faults = {};
    faults[1].push_back(makeFault(1, 0, 256));
    const auto transitionB = evaluateAndCompare(faults, "T1-D9");
    require(transitionB.subarrays[1].candidateReadyCycle == 16390 &&
                transitionB.subarrays[1].candidateCatchupLatency == 4,
            "T1-D9 transition-edge B fault is wrong");

    // T1-D10: the retained-bank physical-fault bound is exactly twelve.
    faults = {};
    for (int word = 0; word < 12; ++word)
    {
        faults[0].push_back(makeFault(0, 0, word * 256));
    }
    const auto maximum = evaluateAndCompare(faults, "T1-D10");
    require(maximum.subarrays[0].physicalFaultCount == 12 &&
                maximum.subarrays[0].acceptedUpdateCount == 12 &&
                maximum.subarrays[0].candidateReadyCycle == 16,
            "T1-D10 maximum legal retained trace is wrong");
    faults[0].push_back(makeFault(0, 0, 12 * 256));
    bool overLimitRejected = false;
    try
    {
        (void)evaluateAndCompare(faults, "T1-D10-over-limit");
    }
    catch (const std::invalid_argument &)
    {
        overLimitRejected = true;
    }
    require(overLimitRejected,
            "T1-D10 timing model accepted an unreachable 13-fault SA trace");

    // T1-D11: two physical bits in one tested word aggregate to one selected
    // update event, while preserving a physical-fault count of two.
    faults = {};
    faults[0].push_back(makeFault(0, 0, 17));
    faults[0].push_back(makeFault(0, 0, 255));
    const auto sameWord = evaluateAndCompare(faults, "T1-D11");
    require(sameWord.subarrays[0].physicalFaultCount == 2 &&
                sameWord.subarrays[0].acceptedUpdateCount == 1 &&
                sameWord.subarrays[0].latestGeneration == 1 &&
                sameWord.subarrays[0].candidateReadyCycle == 5,
            "T1-D11 BIST word aggregation is wrong");

    // T1-D12: adjacent BIST words remain separate consecutive updates.
    faults = {};
    faults[0].push_back(makeFault(0, 0, 0));
    faults[0].push_back(makeFault(0, 0, 256));
    const auto adjacentWords = evaluateAndCompare(faults, "T1-D12");
    require(adjacentWords.subarrays[0].acceptedUpdateCount == 2 &&
                adjacentWords.subarrays[0].candidateReadyCycle == 6,
            "T1-D12 adjacent BIST words were incorrectly aggregated");

    // T1-D13/D14/D15: fully hidden and exposed group arithmetic, then a mixed
    // serial trace with D's last-word update.
    require(earlyA.group.allCandidatesReadyBeforeTestDone &&
                earlyA.group.provisionalPostBistLatency == 0,
            "T1-D13 hidden group candidate timing is wrong");
    faults = {};
    faults[3].push_back(makeFault(3, 511, 8191));
    const auto lateD = evaluateAndCompare(faults, "T1-D14");
    require(lateD.group.latestCandidateReadyCycle == 65540 &&
                lateD.group.provisionalPostBistLatency == 4 &&
                !lateD.group.allCandidatesReadyBeforeTestDone,
            "T1-D14 exposed group candidate timing is wrong");
    faults = {};
    faults[0].push_back(makeFault(0, 10, 0));
    faults[1].push_back(makeFault(1, 200, 1024));
    faults[2].push_back(makeFault(2, 400, 2048));
    faults[3].push_back(makeFault(3, 511, 8191));
    const auto mixedLateD = evaluateAndCompare(faults, "T1-D15");
    require(mixedLateD.subarrays[3].candidateReadyCycle == 65540 &&
                mixedLateD.group.provisionalPostBistLatency == 4,
            "T1-D15 late-D mixed trace is wrong");
}

void verifyDirectSameEdgeReference()
{
    BistOverlapTimingInput input;
    input.testStartCycles = {{0, 10, 20, 30}};
    input.testDoneCycles = {{10, 20, 30, 40}};
    input.faultEvents = {{1, 0, 1}, {5, 0, 1}};
    BistOverlapTimingModel model;
    const auto eventDriven = model.evaluateEventStream(input);
    const auto reference = model.evaluateEventStreamCycleReference(input);
    requireEqual(eventDriven, reference, "direct same-edge reference");
    require(eventDriven.subarrays[0].candidateReadyCycle == 9 &&
                eventDriven.subarrays[0].latestGeneration == 2,
            "Direct same-edge reference did not restart generation 2");
}

void verifyRandomizedEquivalence()
{
    std::mt19937 random(20260915U);
    BistOverlapTimingModel model;
    SerialBistSchedule schedule;
    for (unsigned vector = 0; vector < 1000; ++vector)
    {
        FaultGroup faults;
        for (int sa = 0; sa < 4; ++sa)
        {
            const unsigned count = random() % 13U;
            for (unsigned index = 0; index < count; ++index)
            {
                const int row = static_cast<int>(random() % 512U);
                const int word = static_cast<int>(random() % 32U);
                const int bit = static_cast<int>(random() % 256U);
                faults[sa].push_back(makeFault(sa, row, word * 256 + bit));
            }
        }
        const auto eventDriven = model.evaluateGroup(faults, schedule);

        BistOverlapTimingInput input;
        for (std::size_t sa = 0; sa < 4; ++sa)
        {
            input.testStartCycles[sa] = schedule.subarrayStartCycle(sa);
            input.testDoneCycles[sa] = schedule.subarrayCompletionCycle(sa);
        }
        for (std::size_t sa = 0; sa < 4; ++sa)
        {
            for (const Fault &fault : faults[sa])
            {
                const std::uint64_t cycle = schedule.arrivalCycle(fault);
                auto event = std::find_if(
                    input.faultEvents.begin(), input.faultEvents.end(),
                    [cycle, sa](const FaultArrivalEvent &candidate) {
                        return candidate.cycle == cycle && candidate.subarrayId == sa;
                    });
                if (event == input.faultEvents.end())
                {
                    input.faultEvents.push_back({cycle, sa, 1});
                }
                else
                {
                    ++event->sourceFaultCount;
                }
            }
        }
        std::sort(input.faultEvents.begin(), input.faultEvents.end(),
                  [](const FaultArrivalEvent &left, const FaultArrivalEvent &right) {
                      if (left.cycle != right.cycle)
                      {
                          return left.cycle < right.cycle;
                      }
                      return left.subarrayId < right.subarrayId;
                  });
        const auto reference = model.evaluateEventStreamCycleReference(input);
        requireEqual(eventDriven, reference, "randomized vector " +
            std::to_string(vector));
    }
}

} // namespace

int main()
{
    try
    {
        verifyDirectedTiming();
        verifyDirectSameEdgeReference();
        verifyRandomizedEquivalence();
        std::cout << "T1_BIST_OVERLAP_TIMING_MODEL PASS directed=15 randomized=1000 mismatches=0\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "T1_BIST_OVERLAP_TIMING_MODEL FAIL: " << error.what() << '\n';
        return 1;
    }
}
