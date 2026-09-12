#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "DssPostBistLatency.hpp"

namespace
{

using dynamic_spare::DssDecisionWorkPlan;
using dynamic_spare::DssFaultSetEvent;
using dynamic_spare::DssPostBistLatencyResult;
using dynamic_spare::DssPostBistPolicy;
using dynamic_spare::DssPostBistRequest;

constexpr std::uint64_t kLastFaultCycle = 20;
const std::vector<DssFaultSetEvent> kTrace{{10, 100, true},
                                            {kLastFaultCycle, 101, true}};

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

DssDecisionWorkPlan makePlan(
    DssPostBistPolicy policy,
    std::array<unsigned int, 4> evaluations,
    bool repairable = true,
    int failurePosition = -1)
{
    DssDecisionWorkPlan plan;
    plan.policy = policy;
    plan.candidateEvaluations = evaluations;
    plan.groupRepairable = repairable;
    plan.failurePosition = failurePosition;
    return plan;
}

DssPostBistLatencyResult run(
    const DssDecisionWorkPlan &plan,
    std::uint64_t gapCycles)
{
    DssPostBistRequest request;
    request.faultEvents = kTrace;
    request.bistEndCycle = kLastFaultCycle + gapCycles;
    request.finalDecisionPlan = plan;
    return dynamic_spare::modelDssPostBistLatency(request);
}

void writeCsvRow(
    std::ofstream &csv,
    const char *caseName,
    const DssPostBistLatencyResult &result)
{
    csv << caseName << ','
        << dynamic_spare::toString(result.policy) << ','
        << result.faultCount << ','
        << result.lastFaultCycle << ','
        << result.bistEndCycle << ','
        << result.gapCycles << ','
        << result.decisionReadyCycle << ','
        << result.latencyFromLastFaultCycles << ','
        << result.latencyPostBistCycles << ','
        << (result.groupRepairable ? 1 : 0) << '\n';
}

void verifyPlanDepthAndFailureBehavior(std::ofstream &csv)
{
    const auto earlyDepth = run(
        makePlan(DssPostBistPolicy::Early, {{2, 1, 1, 1}}), 0);
    require(earlyDepth.decisionExecutionCycles == 5,
            "EARLY candidate depth was flattened to four cycles");
    writeCsvRow(csv, "EARLY_DEPTH_SENSITIVE_G0", earlyDepth);

    const auto earlyFailure = run(
        makePlan(DssPostBistPolicy::Early, {{1, 3, 0, 0}}, false, 1), 0);
    require(earlyFailure.decisionExecutionCycles == 4 &&
                !earlyFailure.groupRepairable &&
                earlyFailure.failurePosition == 1,
            "EARLY first-failure work is not candidate-depth sensitive");
    writeCsvRow(csv, "EARLY_FAILURE_B_G0", earlyFailure);

    const auto groupFailure = run(
        makePlan(DssPostBistPolicy::GroupNoScratch,
                 {{1, 4, 0, 0}}, false, 1), 0);
    require(groupFailure.decisionExecutionCycles == 21 &&
                groupFailure.failurePosition == 1,
            "GROUP fixed collection/allocation timing is incorrect");
    writeCsvRow(csv, "GROUP_FAILURE_B_G0", groupFailure);
}

void verifySameTraceAndMonotonicPostBistLatency(
    DssPostBistPolicy policy,
    const DssDecisionWorkPlan &plan,
    std::uint64_t expectedExecutionCycles,
    std::uint64_t zeroLatencyGap,
    std::ofstream &csv)
{
    const std::array<std::uint64_t, 8> gaps{{0, 1, 2, 3, 4, 8, 16, 32}};
    std::uint64_t previousPostLatency = UINT64_MAX;
    for (const std::uint64_t gap : gaps)
    {
        const auto result = run(plan, gap);
        require(result.policy == policy && result.faultCount == kTrace.size() &&
                    result.lastFaultCycle == kLastFaultCycle,
                "EARLY/GROUP did not receive the identical fault trace");
        require(result.decisionExecutionCycles == expectedExecutionCycles &&
                    result.decisionReadyCycle ==
                        kLastFaultCycle + expectedExecutionCycles &&
                    result.latencyFromLastFaultCycles == expectedExecutionCycles,
                "Decision-ready timestamp does not match the frozen plan");
        require(result.latencyPostBistCycles <= previousPostLatency,
                "Post-BIST latency increased as BIST gap increased");
        previousPostLatency = result.latencyPostBistCycles;
        const std::string caseName = std::string(dynamic_spare::toString(policy)) +
            "_G" + std::to_string(gap);
        writeCsvRow(csv, caseName.c_str(), result);
    }

    const auto g0 = run(plan, 0);
    require(g0.latencyPostBistCycles == expectedExecutionCycles &&
                g0.invalidatedDecisionCount == 1,
            "G=0 did not expose the final concurrent decision work");

    const auto intermediate = run(plan, zeroLatencyGap / 2);
    require(intermediate.latencyPostBistCycles < g0.latencyPostBistCycles &&
                intermediate.latencyPostBistCycles > 0,
            "Intermediate BIST gap did not hide only part of the work");

    const auto zero = run(plan, zeroLatencyGap);
    require(zero.latencyPostBistCycles == 0 &&
                zero.decisionReadyCycle == zero.bistEndCycle,
            "The earliest zero-latency BIST gap is incorrect");
    if (zeroLatencyGap > 16)
    {
        const std::string caseName = std::string(dynamic_spare::toString(policy)) +
            "_EARLIEST_ZERO";
        writeCsvRow(csv, caseName.c_str(), zero);
    }
}

void verifyInvalidEventRejected()
{
    DssPostBistRequest request;
    request.faultEvents = {{21, 1, true}};
    request.bistEndCycle = 20;
    request.finalDecisionPlan =
        makePlan(DssPostBistPolicy::Early, {{1, 1, 1, 1}});
    bool rejected = false;
    try
    {
        (void)dynamic_spare::modelDssPostBistLatency(request);
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    require(rejected, "A fault after BIST completion was accepted");
}

} // namespace

int main()
{
    try
    {
        std::filesystem::create_directories("results/phase4i");
        std::ofstream csv("results/phase4i/phase4i1_directed_event_model.csv");
        require(csv.good(), "Unable to create the Phase 4I-1 CSV artifact");
        csv << "case,policy,fault_count,T_last_fault,T_BIST_end,G,"
               "T_decision_ready,L_last_fault,L_post,group_repairable\n";

        verifyPlanDepthAndFailureBehavior(csv);
        verifySameTraceAndMonotonicPostBistLatency(
            DssPostBistPolicy::Early,
            makePlan(DssPostBistPolicy::Early, {{1, 1, 1, 1}}),
            4,
            4,
            csv);
        verifySameTraceAndMonotonicPostBistLatency(
            DssPostBistPolicy::GroupNoScratch,
            makePlan(DssPostBistPolicy::GroupNoScratch, {{1, 1, 1, 1}}),
            20,
            20,
            csv);
        verifyInvalidEventRejected();

        std::cout << "EVENT_MODEL_IMPLEMENTED=YES\n"
                  << "TIMESTAMP_CONVENTION=fault-set change at rising edge N "
                     "restarts concurrent DSS analysis; a W-cycle plan is "
                     "ready at rising edge N+W; ready at T_BIST_end gives "
                     "L_post=0\n"
                  << "EARLY_DIRECTED_G0=PASS\n"
                  << "EARLY_ZERO_LATENCY_CASE=PASS\n"
                  << "GROUP_DIRECTED_G0=PASS\n"
                  << "GROUP_ZERO_LATENCY_CASE=PASS\n"
                  << "POST_BIST_LATENCY_MONOTONIC=PASS\n"
                  << "PHASE_4I_1_EVENT_MODEL=PASS\n"
                  << "READY_FOR_PHASE_4I_2=YES\n"
                  << "V2_PHASE=PHASE_4I\n"
                  << "V2_NEXT_PHASE_AUTHORIZED=YES\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Phase 4I-1 event model failed: " << error.what() << '\n';
        return 1;
    }
}
