#include "DssTimelineCorrelation.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

Fault fault(int subarray, int row, int column)
{
    Fault result{};
    result.SubarrayID = subarray;
    result.r = row;
    result.c = column;
    return result;
}

dynamic_spare::FaultGroup lateFaultCorpus()
{
    dynamic_spare::FaultGroup faults;
    faults[1].push_back(fault(1, 0, 0));
    faults[2].push_back(fault(2, 0, 0));
    faults[2].push_back(fault(2, 1, 0));
    faults[3].push_back(fault(3, 511, 8191));
    return faults;
}

dynamic_spare::DssDecisionRelativeTrace allLocalDecision(
    dynamic_spare::DssTimelinePolicy policy,
    const dynamic_spare::FaultGroup &faults)
{
    dynamic_spare::DssDecisionRelativeTrace trace;
    trace.policy = policy;
    trace.candidateContractId = policy == dynamic_spare::DssTimelinePolicy::Early
        ? "S1B1_EARLY_ALL_LOCAL_CFG0_PATTERN_1_2_3_4"
        : "S1B1_GROUP_ALL_LOCAL_CFG0_PATTERN_1_2_3_4";
    for (std::size_t subarray = 0; subarray < faults.size(); ++subarray)
    {
        trace.saFaultCounts[subarray] = faults[subarray].size();
        trace.selectedConfigIds[subarray] = 0;
        trace.selectedPatternIds[subarray] = static_cast<unsigned>(subarray + 1);
    }
    trace.saDecisionCycles = policy == dynamic_spare::DssTimelinePolicy::Early
        ? std::array<std::optional<std::uint64_t>, 4>{{1, 2, 3, 4}}
        : std::array<std::optional<std::uint64_t>, 4>{{18, 20, 22, 24}};
    trace.groupDecisionCycle = policy == dynamic_spare::DssTimelinePolicy::Early
        ? 4 : 24;
    trace.repairable = true;
    return trace;
}

dynamic_spare::DssTimelineCorrelationRequest request(
    const std::string &transactionId,
    const dynamic_spare::FaultGroup &faults,
    const dynamic_spare::DssDecisionRelativeTrace &decision)
{
    dynamic_spare::DssTimelineCorrelationRequest result;
    result.transactionId = transactionId;
    result.faultCorpusId = "S1C_DIRECTED_LATE_D_ZERO_A";
    result.faults = faults;
    result.decision = decision;
    result.harnessStartOffsetCycles = 1;
    return result;
}

void verifyEarlyAndGroupCorrelation()
{
    const dynamic_spare::FaultGroup faults = lateFaultCorpus();
    const auto early = dynamic_spare::correlateDssTimeline(request(
        "S1C_EARLY_SUCCESS_LATE_D", faults,
        allLocalDecision(dynamic_spare::DssTimelinePolicy::Early, faults)));
    require(early.bist.group.testDoneCycle == 65536 &&
                early.bist.group.lastFaultAcceptCycle == 65536 &&
                early.dssStartCycle == 65537 &&
                early.selectedConfigIds ==
                    std::array<unsigned int, 4>{{0, 0, 0, 0}} &&
                early.selectedPatternIds ==
                    std::array<unsigned int, 4>{{1, 2, 3, 4}} &&
                early.saDecisionCycles ==
                    std::array<std::optional<std::uint64_t>, 4>{{65538, 65539, 65540, 65541}} &&
                early.groupDecisionCycle == 65541,
            "EARLY absolute timeline is wrong");
    require(early.deltaTestDoneToDssStart == 1 &&
                early.deltaTestDoneToGroupDone == 5 &&
                early.deltaLastFaultToGroupDone == 5 &&
                early.bistTailCycles == 0 && early.decompositionIdentityHolds,
            "EARLY timeline decomposition is wrong");

    const auto group = dynamic_spare::correlateDssTimeline(request(
        "S1C_GROUP_SUCCESS_LATE_D", faults,
        allLocalDecision(dynamic_spare::DssTimelinePolicy::GroupNoScratchV2,
                         faults)));
    require(group.dssStartCycle == 65537 &&
                group.saDecisionCycles ==
                    std::array<std::optional<std::uint64_t>, 4>{{65555, 65557, 65559, 65561}} &&
                group.groupDecisionCycle == 65561,
            "GROUP absolute timeline is wrong");
    require(group.deltaTestDoneToDssStart == 1 &&
                group.deltaTestDoneToGroupDone == 25 &&
                group.deltaLastFaultToGroupDone == 25 &&
                group.bistTailCycles == 0 && group.decompositionIdentityHolds,
            "GROUP timeline decomposition is wrong");

    std::cout << "S1C_TRACE transaction=" << early.transactionId
              << " policy=" << dynamic_spare::toString(early.policy)
              << " group_test_done=" << early.bist.group.testDoneCycle
              << " group_last_fault=" << *early.bist.group.lastFaultAcceptCycle
              << " dss_start=" << early.dssStartCycle
              << " group_done=" << early.groupDecisionCycle
              << " delta_testdone_to_dss_start=" << early.deltaTestDoneToDssStart
              << " delta_testdone_to_group_done=" << early.deltaTestDoneToGroupDone
              << " delta_lastfault_to_group_done=" << *early.deltaLastFaultToGroupDone
              << " PASS\n";
    std::cout << "S1C_TRACE transaction=" << group.transactionId
              << " policy=" << dynamic_spare::toString(group.policy)
              << " group_test_done=" << group.bist.group.testDoneCycle
              << " group_last_fault=" << *group.bist.group.lastFaultAcceptCycle
              << " dss_start=" << group.dssStartCycle
              << " group_done=" << group.groupDecisionCycle
              << " delta_testdone_to_dss_start=" << group.deltaTestDoneToDssStart
              << " delta_testdone_to_group_done=" << group.deltaTestDoneToGroupDone
              << " delta_lastfault_to_group_done=" << *group.deltaLastFaultToGroupDone
              << " PASS\n";
}

void verifyFailureAndZeroFaultCorrelation()
{
    const dynamic_spare::FaultGroup faults = lateFaultCorpus();
    dynamic_spare::DssDecisionRelativeTrace failure;
    failure.policy = dynamic_spare::DssTimelinePolicy::Early;
    failure.candidateContractId = "S1B1_EARLY_FAILURE_C_NO_ROLLBACK";
    for (std::size_t subarray = 0; subarray < faults.size(); ++subarray)
    {
        failure.saFaultCounts[subarray] = faults[subarray].size();
    }
    failure.saDecisionCycles = {{2, 3, std::nullopt, std::nullopt}};
    failure.groupDecisionCycle = 7;
    failure.repairable = false;
    failure.failurePosition = 2;
    const auto failed = dynamic_spare::correlateDssTimeline(request(
        "S1C_EARLY_FAILURE_C", faults, failure));
    require(!failed.repairable && failed.failurePosition == 2 &&
                failed.groupDecisionCycle == 65544 &&
                failed.saDecisionCycles[0] == 65539 &&
                failed.saDecisionCycles[1] == 65540 &&
                !failed.saDecisionCycles[2],
            "Terminal failure timeline is wrong");

    dynamic_spare::FaultGroup noFaults;
    const auto zero = dynamic_spare::correlateDssTimeline(request(
        "S1C_EARLY_ZERO_FAULT_A", noFaults,
        allLocalDecision(dynamic_spare::DssTimelinePolicy::Early, noFaults)));
    require(!zero.bist.subarrays[0].lastFaultAcceptCycle &&
                !zero.bist.group.lastFaultAcceptCycle &&
                zero.bist.subarrays[0].testDoneCycle == 16384 &&
                zero.groupDecisionCycle == 65541 &&
                !zero.deltaLastFaultToGroupDone &&
                !zero.decompositionIdentityHolds,
            "Zero-fault correlation created a synthetic last-fault event");

    std::cout << "S1C_TRACE transaction=" << failed.transactionId
              << " policy=" << dynamic_spare::toString(failed.policy)
              << " failure_position=C group_test_done="
              << failed.bist.group.testDoneCycle
              << " dss_start=" << failed.dssStartCycle
              << " group_done=" << failed.groupDecisionCycle
              << " no_rollback_preserved=PASS\n";
    std::cout << "S1C_TRACE transaction=" << zero.transactionId
              << " policy=" << dynamic_spare::toString(zero.policy)
              << " SA_A_test_done=" << zero.bist.subarrays[0].testDoneCycle
              << " SA_A_last_fault=N/A group_last_fault=N/A"
              << " group_done=" << zero.groupDecisionCycle << " PASS\n";
}

} // namespace

int main()
{
    try
    {
        verifyEarlyAndGroupCorrelation();
        verifyFailureAndZeroFaultCorrelation();
        std::cout << "dss_timeline_correlation_test PASS\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "dss_timeline_correlation_test FAIL: "
                  << error.what() << '\n';
        return 1;
    }
}
