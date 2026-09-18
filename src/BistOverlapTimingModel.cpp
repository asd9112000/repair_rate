#include "BistOverlapTimingModel.hpp"

#include "DssBistFaultTimeline.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <stdexcept>

namespace dynamic_spare
{
namespace
{

enum class OwnerState
{
    Sweep,
    WaitTestDone,
    FinalSelect,
    Complete
};

struct ControllerState
{
    OwnerState state = OwnerState::Sweep;
    std::size_t owner = 0;
    std::size_t sweepProgress = 0;
    std::array<bool, kSubarrayCount> testDone{};
    std::array<std::uint64_t, kSubarrayCount> faultGeneration{};
    std::array<std::optional<std::uint64_t>, kSubarrayCount> readyGeneration{};
    std::array<std::optional<std::uint64_t>, kSubarrayCount> candidateReadyCycle{};
};

struct ExternalEdge
{
    std::optional<FaultArrivalEvent> faultEvent;
    std::array<bool, kSubarrayCount> testDone{};
};

using ExternalEdges = std::map<std::uint64_t, ExternalEdge>;

std::int64_t signedDifference(
    std::uint64_t left,
    std::uint64_t right,
    const char *description)
{
    if (left >= right)
    {
        const std::uint64_t difference = left - right;
        if (difference > static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max()))
        {
            throw std::overflow_error(description);
        }
        return static_cast<std::int64_t>(difference);
    }
    const std::uint64_t difference = right - left;
    if (difference > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()))
    {
        throw std::overflow_error(description);
    }
    return -static_cast<std::int64_t>(difference);
}

void validateInput(const BistOverlapTimingInput &input)
{
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        if (input.testDoneCycles[subarray] < input.testStartCycles[subarray])
        {
            throw std::invalid_argument(
                "BIST test completion precedes its subarray start");
        }
    }
}

ExternalEdges buildExternalEdges(const BistOverlapTimingInput &input)
{
    validateInput(input);
    ExternalEdges externalEdges;
    std::array<std::size_t, kSubarrayCount> physicalFaultCounts{};

    for (const FaultArrivalEvent &event : input.faultEvents)
    {
        if (event.subarrayId >= kSubarrayCount || event.sourceFaultCount == 0)
        {
            throw std::invalid_argument(
                "BIST overlap event has invalid SA or zero physical faults");
        }
        if (event.cycle < input.testStartCycles[event.subarrayId] ||
            event.cycle > input.testDoneCycles[event.subarrayId])
        {
            throw std::invalid_argument(
                "BIST overlap event is outside its SA test interval");
        }
        physicalFaultCounts[event.subarrayId] += event.sourceFaultCount;
        if (physicalFaultCounts[event.subarrayId] > kBistOverlapMaxFaultsPerSa)
        {
            throw std::invalid_argument(
                "BIST overlap timing input exceeds the 12-fault SA contract");
        }
        ExternalEdge &edge = externalEdges[event.cycle];
        if (edge.faultEvent)
        {
            throw std::invalid_argument(
                "BIST overlap model permits only one accepted update per cycle");
        }
        edge.faultEvent = event;
    }

    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        externalEdges[input.testDoneCycles[subarray]].testDone[subarray] = true;
    }
    return externalEdges;
}

bool ownerHasMatchingReady(const ControllerState &state)
{
    return state.readyGeneration[state.owner] &&
           *state.readyGeneration[state.owner] ==
               state.faultGeneration[state.owner];
}

void applyEdge(
    ControllerState &state,
    std::uint64_t cycle,
    const ExternalEdge *externalEdge)
{
    const bool oldOwnerTestDone = state.testDone[state.owner];
    const FaultArrivalEvent *faultEvent = nullptr;
    if (externalEdge && externalEdge->faultEvent)
    {
        faultEvent = &*externalEdge->faultEvent;
        if (faultEvent->subarrayId < state.owner)
        {
            throw std::invalid_argument(
                "BIST overlap event targets an SA after its ownership release");
        }
        if (state.state == OwnerState::Complete)
        {
            throw std::invalid_argument(
                "BIST overlap event occurs after final ownership release");
        }
        ++state.faultGeneration[faultEvent->subarrayId];
        state.readyGeneration[faultEvent->subarrayId].reset();
        state.candidateReadyCycle[faultEvent->subarrayId].reset();
    }
    const bool ownerFaultUpdate = faultEvent &&
                                  faultEvent->subarrayId == state.owner;

    switch (state.state)
    {
        case OwnerState::Sweep:
            if (ownerFaultUpdate)
            {
                state.sweepProgress = 0;
            }
            else if (state.sweepProgress + 1 == kBistOverlapSweepEdges)
            {
                state.readyGeneration[state.owner] =
                    state.faultGeneration[state.owner];
                state.candidateReadyCycle[state.owner] = cycle;
                state.sweepProgress = 0;
                state.state = oldOwnerTestDone
                    ? OwnerState::FinalSelect
                    : OwnerState::WaitTestDone;
            }
            else
            {
                ++state.sweepProgress;
            }
            break;

        case OwnerState::WaitTestDone:
            if (ownerFaultUpdate)
            {
                state.sweepProgress = 0;
                state.state = OwnerState::Sweep;
            }
            else if (oldOwnerTestDone && ownerHasMatchingReady(state))
            {
                state.sweepProgress = 0;
                state.state = OwnerState::FinalSelect;
            }
            break;

        case OwnerState::FinalSelect:
            if (ownerFaultUpdate)
            {
                state.sweepProgress = 0;
                state.state = OwnerState::Sweep;
            }
            else if (state.owner + 1 == kSubarrayCount)
            {
                state.state = OwnerState::Complete;
            }
            else
            {
                ++state.owner;
                state.sweepProgress = 0;
                state.state = OwnerState::Sweep;
            }
            break;

        case OwnerState::Complete:
            break;
    }

    if (externalEdge)
    {
        for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
        {
            if (externalEdge->testDone[subarray])
            {
                state.testDone[subarray] = true;
            }
        }
    }
}

std::uint64_t nextInternalEdge(
    const ControllerState &state,
    std::uint64_t currentCycle)
{
    switch (state.state)
    {
        case OwnerState::Sweep:
            return currentCycle +
                (kBistOverlapSweepEdges - state.sweepProgress);
        case OwnerState::WaitTestDone:
            return state.testDone[state.owner] && ownerHasMatchingReady(state)
                ? currentCycle + 1
                : std::numeric_limits<std::uint64_t>::max();
        case OwnerState::FinalSelect:
            return currentCycle + 1;
        case OwnerState::Complete:
            return std::numeric_limits<std::uint64_t>::max();
    }
    return std::numeric_limits<std::uint64_t>::max();
}

bool hasFinalReady(const ControllerState &state)
{
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        if (!state.readyGeneration[subarray] ||
            *state.readyGeneration[subarray] != state.faultGeneration[subarray])
        {
            return false;
        }
    }
    return true;
}

BistOverlapTimingResult buildResult(
    const BistOverlapTimingInput &input,
    const ControllerState &state)
{
    BistOverlapTimingResult result;
    result.aggregatedFaultEventCount = input.faultEvents.size();
    std::array<std::size_t, kSubarrayCount> physicalFaultCounts{};
    std::array<std::size_t, kSubarrayCount> acceptedUpdateCounts{};

    for (const FaultArrivalEvent &event : input.faultEvents)
    {
        SaBistOverlapTiming &saResult = result.subarrays[event.subarrayId];
        physicalFaultCounts[event.subarrayId] += event.sourceFaultCount;
        ++acceptedUpdateCounts[event.subarrayId];
        if (!saResult.firstFaultAcceptCycle)
        {
            saResult.firstFaultAcceptCycle = event.cycle;
        }
        saResult.lastFaultAcceptCycle = event.cycle;
    }

    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        SaBistOverlapTiming &saResult = result.subarrays[subarray];
        if (!state.candidateReadyCycle[subarray] ||
            !state.readyGeneration[subarray])
        {
            throw std::logic_error(
                "BIST overlap timing ended without latest candidate readiness");
        }
        saResult.physicalFaultCount = physicalFaultCounts[subarray];
        saResult.acceptedUpdateCount = acceptedUpdateCounts[subarray];
        saResult.bistStartCycle = input.testStartCycles[subarray];
        saResult.testDoneCycle = input.testDoneCycles[subarray];
        saResult.latestGeneration = state.faultGeneration[subarray];
        saResult.readyGeneration = *state.readyGeneration[subarray];
        saResult.candidateReadyCycle = *state.candidateReadyCycle[subarray];
        saResult.hiddenAnalysisSlack = signedDifference(
            saResult.testDoneCycle, saResult.candidateReadyCycle,
            "Per-SA hidden-analysis slack exceeds signed range");
        saResult.provisionalPostBistLatency =
            saResult.candidateReadyCycle > saResult.testDoneCycle
            ? saResult.candidateReadyCycle - saResult.testDoneCycle
            : 0;
        saResult.fullyHidden = saResult.provisionalPostBistLatency == 0;
        if (saResult.lastFaultAcceptCycle)
        {
            saResult.candidateCatchupLatency =
                saResult.candidateReadyCycle - *saResult.lastFaultAcceptCycle;
            if (*saResult.candidateCatchupLatency < kBistOverlapSweepEdges)
            {
                throw std::logic_error(
                    "Candidate readiness violates the four-edge lower bound");
            }
            saResult.ownershipWaitCycles =
                *saResult.candidateCatchupLatency - kBistOverlapSweepEdges;
        }
        result.group.latestCandidateReadyCycle = std::max(
            result.group.latestCandidateReadyCycle,
            saResult.candidateReadyCycle);
        result.group.groupTestDoneCycle = std::max(
            result.group.groupTestDoneCycle, saResult.testDoneCycle);
    }

    result.group.hiddenAnalysisSlack = signedDifference(
        result.group.groupTestDoneCycle, result.group.latestCandidateReadyCycle,
        "Group hidden-analysis slack exceeds signed range");
    result.group.provisionalPostBistLatency =
        result.group.latestCandidateReadyCycle > result.group.groupTestDoneCycle
        ? result.group.latestCandidateReadyCycle - result.group.groupTestDoneCycle
        : 0;
    result.group.allCandidatesReadyBeforeTestDone =
        result.group.provisionalPostBistLatency == 0;
    return result;
}

BistOverlapTimingResult evaluateEventDriven(
    const BistOverlapTimingInput &input)
{
    const ExternalEdges externalEdges = buildExternalEdges(input);
    ControllerState state;
    std::uint64_t currentCycle = 0;
    auto external = externalEdges.begin();

    while (external != externalEdges.end())
    {
        const std::uint64_t nextExternalCycle = external->first;
        for (;;)
        {
            const std::uint64_t nextInternalCycle =
                nextInternalEdge(state, currentCycle);
            if (nextInternalCycle >= nextExternalCycle)
            {
                break;
            }
            if (nextInternalCycle == std::numeric_limits<std::uint64_t>::max())
            {
                throw std::logic_error(
                    "BIST overlap controller cannot advance to the next event");
            }
            currentCycle = nextInternalCycle;
            if (state.state == OwnerState::Sweep)
            {
                // The event-driven jump crosses the remaining idle role
                // edges. Leave the final edge to applyEdge(), which creates
                // the ready pulse and state transition exactly once.
                state.sweepProgress = kBistOverlapSweepEdges - 1;
            }
            applyEdge(state, currentCycle, nullptr);
        }
        if (state.state == OwnerState::Sweep && nextExternalCycle > currentCycle)
        {
            // No internal milestone lies before this external edge. Account
            // for the idle role edges preceding it; applyEdge() accounts for
            // the external edge itself and preserves update priority there.
            const std::uint64_t idleSweepEdges =
                nextExternalCycle - currentCycle - 1;
            state.sweepProgress += static_cast<std::size_t>(idleSweepEdges);
        }
        currentCycle = nextExternalCycle;
        applyEdge(state, currentCycle, &external->second);
        ++external;
    }

    while (!hasFinalReady(state))
    {
        const std::uint64_t nextInternalCycle = nextInternalEdge(state, currentCycle);
        if (nextInternalCycle == std::numeric_limits<std::uint64_t>::max())
        {
            throw std::logic_error(
                "BIST overlap controller cannot reach latest candidate readiness");
        }
        currentCycle = nextInternalCycle;
        if (state.state == OwnerState::Sweep)
        {
            state.sweepProgress = kBistOverlapSweepEdges - 1;
        }
        applyEdge(state, currentCycle, nullptr);
    }
    return buildResult(input, state);
}

BistOverlapTimingResult evaluateCycleReference(
    const BistOverlapTimingInput &input)
{
    const ExternalEdges externalEdges = buildExternalEdges(input);
    const std::uint64_t finalExternalCycle = externalEdges.empty()
        ? 0
        : externalEdges.rbegin()->first;
    const std::uint64_t maximumCycles = finalExternalCycle +
        (kSubarrayCount * (kBistOverlapSweepEdges + 2)) + 8;
    ControllerState state;
    std::uint64_t currentCycle = 0;

    while (currentCycle < maximumCycles)
    {
        ++currentCycle;
        const auto external = externalEdges.find(currentCycle);
        applyEdge(state, currentCycle,
                  external == externalEdges.end() ? nullptr : &external->second);
        if (currentCycle >= finalExternalCycle && hasFinalReady(state))
        {
            return buildResult(input, state);
        }
    }
    throw std::logic_error(
        "Cycle-reference BIST overlap model did not reach readiness bound");
}

BistOverlapTimingInput makeTimingInput(
    const FaultGroup &faults,
    const SerialBistSchedule &schedule)
{
    const DssBistFaultTimeline timeline = materializeDssBistFaultTimeline(
        faults, schedule);
    BistOverlapTimingInput input;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        input.testStartCycles[subarray] = timeline.subarrays[subarray].testStartCycle;
        input.testDoneCycles[subarray] = timeline.subarrays[subarray].testDoneCycle;
    }

    for (const AcceptedFaultEvent &physicalEvent : timeline.acceptedFaults)
    {
        if (!input.faultEvents.empty() &&
            input.faultEvents.back().cycle == physicalEvent.acceptCycle &&
            input.faultEvents.back().subarrayId == physicalEvent.subarrayId)
        {
            ++input.faultEvents.back().sourceFaultCount;
        }
        else
        {
            input.faultEvents.push_back(
                {physicalEvent.acceptCycle, physicalEvent.subarrayId, 1});
        }
    }
    return input;
}

} // namespace

BistOverlapTimingResult BistOverlapTimingModel::evaluateGroup(
    const FaultGroup &faults,
    const SerialBistSchedule &schedule) const
{
    return evaluateEventDriven(makeTimingInput(faults, schedule));
}

BistOverlapTimingResult BistOverlapTimingModel::evaluateEventStream(
    const BistOverlapTimingInput &input) const
{
    return evaluateEventDriven(input);
}

BistOverlapTimingResult BistOverlapTimingModel::evaluateEventStreamCycleReference(
    const BistOverlapTimingInput &input) const
{
    return evaluateCycleReference(input);
}

} // namespace dynamic_spare
