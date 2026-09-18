#include "DssTimelineCorrelation.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

using dynamic_spare::DssDecisionRelativeTrace;
using dynamic_spare::DssTimelineCorrelationRequest;
using dynamic_spare::DssTimelineCorrelationTrace;
using dynamic_spare::DssTimelinePolicy;
using dynamic_spare::FaultGroup;

constexpr std::uint64_t kPreflightSeed = 20260914ULL;

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

::Fault fault(int subarray, int row, int column)
{
    ::Fault result{};
    result.SubarrayID = subarray;
    result.r = row;
    result.c = column;
    return result;
}

FaultGroup faultsForCase(std::size_t index)
{
    FaultGroup faults;
    switch (index)
    {
        case 0: return faults;
        case 1: faults[0].push_back(fault(0, 0, 0)); break;
        case 2: faults[1].push_back(fault(1, 1, 0)); break;
        case 3: faults[2].push_back(fault(2, 3, 256)); break;
        case 4: faults[3].push_back(fault(3, 511, 8191)); break;
        case 5: faults[0].push_back(fault(0, 4, 0)); break;
        case 6: faults[1].push_back(fault(1, 7, 0)); break;
        case 7: faults[2].push_back(fault(2, 9, 0)); break;
        case 8: faults[3].push_back(fault(3, 511, 0)); break;
        case 9:
            faults[0].push_back(fault(0, 0, 0));
            faults[1].push_back(fault(1, 1, 0));
            faults[2].push_back(fault(2, 2, 0));
            faults[3].push_back(fault(3, 511, 8191));
            break;
        default: throw std::logic_error("Unexpected preflight case");
    }
    return faults;
}

std::array<std::size_t, dynamic_spare::kSubarrayCount> counts(
    const FaultGroup &faults)
{
    std::array<std::size_t, dynamic_spare::kSubarrayCount> result{};
    for (std::size_t subarray = 0; subarray < faults.size(); ++subarray)
    {
        result[subarray] = faults[subarray].size();
    }
    return result;
}

DssDecisionRelativeTrace decision(
    DssTimelinePolicy policy,
    const FaultGroup &faults,
    std::optional<std::size_t> failurePosition)
{
    DssDecisionRelativeTrace result;
    result.policy = policy;
    result.saFaultCounts = counts(faults);
    result.candidateContractId = policy == DssTimelinePolicy::Early
        ? "S1B1_EARLY_DIRECTED_PRELIGHT_TRACE"
        : "S1B1_GROUP_DIRECTED_PRELIGHT_TRACE";

    if (!failurePosition)
    {
        result.saDecisionCycles = policy == DssTimelinePolicy::Early
            ? std::array<std::optional<std::uint64_t>, 4>{{1, 2, 3, 4}}
            : std::array<std::optional<std::uint64_t>, 4>{{18, 20, 22, 24}};
        result.groupDecisionCycle = policy == DssTimelinePolicy::Early ? 4 : 24;
        result.repairable = true;
        for (std::size_t subarray = 0; subarray < 4; ++subarray)
        {
            result.selectedConfigIds[subarray] = 0;
            result.selectedPatternIds[subarray] =
                static_cast<unsigned int>(subarray + 1);
        }
        return result;
    }

    result.repairable = false;
    result.failurePosition = failurePosition;
    if (policy == DssTimelinePolicy::Early)
    {
        static const std::array<std::array<std::optional<std::uint64_t>, 4>, 4>
            kCommitCycles{{
                {{std::nullopt, std::nullopt, std::nullopt, std::nullopt}},
                {{2, std::nullopt, std::nullopt, std::nullopt}},
                {{2, 3, std::nullopt, std::nullopt}},
                {{2, 3, 4, std::nullopt}}}};
        static const std::array<std::uint64_t, 4> kDone{{4, 6, 7, 8}};
        result.saDecisionCycles = kCommitCycles[*failurePosition];
        result.groupDecisionCycle = kDone[*failurePosition];
    }
    else
    {
        static const std::array<std::array<std::optional<std::uint64_t>, 4>, 4>
            kFinalizeCycles{{
                {{std::nullopt, std::nullopt, std::nullopt, std::nullopt}},
                {{18, std::nullopt, std::nullopt, std::nullopt}},
                {{18, 20, std::nullopt, std::nullopt}},
                {{18, 20, 22, std::nullopt}}}};
        static const std::array<std::uint64_t, 4> kDone{{20, 22, 24, 26}};
        result.saDecisionCycles = kFinalizeCycles[*failurePosition];
        result.groupDecisionCycle = kDone[*failurePosition];
    }
    return result;
}

DssTimelineCorrelationTrace correlate(
    const std::string &transactionId,
    const FaultGroup &faults,
    const DssDecisionRelativeTrace &trace)
{
    DssTimelineCorrelationRequest request;
    request.transactionId = transactionId;
    request.faultCorpusId = "S1D_PREFLIGHT_DIRECTED_NOT_FORMAL_CORPUS";
    request.seed = kPreflightSeed;
    request.faults = faults;
    request.decision = trace;
    request.harnessStartOffsetCycles = 1;
    return dynamic_spare::correlateDssTimeline(request);
}

std::string optionalCycle(const std::optional<std::uint64_t> &value)
{
    return value ? std::to_string(*value) : "N/A";
}

std::string optionalFailure(const std::optional<std::size_t> &value)
{
    return value ? std::string(1, static_cast<char>('A' + *value)) : "N/A";
}

void writeRow(
    std::ofstream &csv,
    const DssTimelineCorrelationTrace &trace)
{
    const std::uint64_t postBistRaw = trace.deltaTestDoneToGroupDone;
    const std::uint64_t postBistArch = postBistRaw - trace.deltaTestDoneToDssStart;
    const std::string faultTailRaw = optionalCycle(trace.deltaLastFaultToGroupDone);
    const std::string faultTailArch = trace.deltaLastFaultToGroupDone
        ? std::to_string(*trace.deltaLastFaultToGroupDone -
                         trace.deltaTestDoneToDssStart)
        : "N/A";
    const std::string source = trace.bist.group.hasAcceptedFault
        ? "MODEL_DERIVED_FAULT_TIMELINE" : "NONE";

    csv << "S1D_PREFLIGHT_ONLY_NOT_FOR_PUBLICATION," << trace.transactionId
        << ',' << kPreflightSeed << ',' << dynamic_spare::toString(trace.policy)
        << ",2,2,1,N/A,N/A,N/A,N/A,"
        << (trace.saFaultCounts[0] + trace.saFaultCounts[1] +
            trace.saFaultCounts[2] + trace.saFaultCounts[3])
        << ',' << trace.saFaultCounts[0] << ',' << trace.saFaultCounts[1]
        << ',' << trace.saFaultCounts[2] << ',' << trace.saFaultCounts[3]
        << ',' << (trace.bist.group.hasAcceptedFault ? "true" : "false")
        << ',' << optionalCycle(trace.bist.group.lastFaultAcceptCycle)
        << ',' << trace.bist.group.testDoneCycle << ',' << trace.dssStartCycle
        << ',' << trace.groupDecisionCycle << ','
        << optionalCycle(trace.bistTailCycles) << ','
        << trace.deltaTestDoneToDssStart << ','
        << (trace.groupDecisionCycle - trace.dssStartCycle) << ','
        << postBistRaw << ',' << postBistArch << ',' << faultTailRaw << ','
        << faultTailArch << ',' << (trace.repairable ? "true" : "false")
        << ',' << optionalFailure(trace.failurePosition) << ',' << source
        << ",HARNESS_ARTIFACT,";
    for (std::size_t subarray = 0; subarray < 4; ++subarray)
    {
        csv << optionalCycle(trace.saDecisionCycles[subarray]);
        csv << (subarray == 3 ? '\n' : ',');
    }
}

std::uint64_t percentile95(std::vector<std::uint64_t> values)
{
    require(!values.empty(), "Cannot summarize an empty latency sample");
    std::sort(values.begin(), values.end());
    const std::size_t nearestRank =
        (95 * values.size() + 99) / 100;
    return values[nearestRank - 1];
}

void verifyAndWrite(const std::string &outputPath)
{
    std::ofstream csv(outputPath);
    require(csv.good(), "Unable to create preflight CSV");
    csv << "experiment_id,transaction_id,seed,policy,RS,CS,SHARE_M,"
           "lambda_sa,prob_cluster,prob_same_line,D0,fault_count_group,"
           "fault_count_A,fault_count_B,fault_count_C,fault_count_D,"
           "group_has_accepted_fault,group_last_fault_accept_cycle,"
           "group_test_done_cycle,dss_start_cycle,group_done_cycle,"
           "bist_tail_cycles,handoff_gap_cycles,dss_decision_cycles,"
           "group_post_bist_raw,group_post_bist_arch,group_fault_tail_raw,"
           "group_fault_tail_arch,repairable,failure_position,"
           "fault_timeline_source,handoff_gap_class,"
           "A_commit_or_internal_finalize_cycle,"
           "B_commit_or_internal_finalize_cycle,"
           "C_commit_or_internal_finalize_cycle,"
           "D_commit_or_internal_finalize_cycle\n";

    std::map<std::string, std::array<DssTimelineCorrelationTrace, 2>> pairs;
    for (std::size_t index = 0; index < 10; ++index)
    {
        const std::string transactionId = std::string("S1D-PREFLIGHT-") +
            (index < 10 ? "0" : "") + std::to_string(index);
        const FaultGroup faults = faultsForCase(index);
        const std::optional<std::size_t> failure =
            index >= 5 && index <= 8 ? std::optional<std::size_t>(index - 5)
                                     : std::nullopt;
        const auto early = correlate(transactionId, faults,
            decision(DssTimelinePolicy::Early, faults, failure));
        const auto group = correlate(transactionId, faults,
            decision(DssTimelinePolicy::GroupNoScratchV2, faults, failure));

        require(early.saFaultCounts == group.saFaultCounts &&
                    early.bist.group.testDoneCycle == group.bist.group.testDoneCycle &&
                    early.dssStartCycle == group.dssStartCycle,
                "Paired policies no longer use identical replay inputs");
        require(early.repairable == group.repairable &&
                    early.failurePosition == group.failurePosition,
                "Paired outcome classification is inconsistent");
        for (const auto *trace : {&early, &group})
        {
            require(trace->deltaTestDoneToGroupDone ==
                        trace->deltaTestDoneToDssStart +
                        (trace->groupDecisionCycle - trace->dssStartCycle),
                    "Post-BIST decomposition identity failed");
            if (trace->bist.group.hasAcceptedFault)
            {
                require(trace->decompositionIdentityHolds &&
                            *trace->deltaLastFaultToGroupDone ==
                                *trace->bistTailCycles +
                                trace->deltaTestDoneToDssStart +
                                (trace->groupDecisionCycle - trace->dssStartCycle),
                        "Positive-fault decomposition identity failed");
            }
            else
            {
                require(!trace->deltaLastFaultToGroupDone &&
                            !trace->bistTailCycles,
                        "Zero-fault row materialized a fault-tail value");
            }
            writeRow(csv, *trace);
        }
        pairs.emplace(transactionId, std::array<DssTimelineCorrelationTrace, 2>{{early, group}});
    }
    csv.close();
    require(pairs.size() == 10, "CSV preflight did not retain ten aligned pairs");
    std::vector<std::uint64_t> earlyDss;
    std::vector<std::uint64_t> groupDss;
    std::vector<std::uint64_t> pairedDeltas;
    std::vector<std::uint64_t> positiveFaultTail;
    for (const auto &entry : pairs)
    {
        const auto &early = entry.second[0];
        const auto &group = entry.second[1];
        const std::uint64_t earlyCycles =
            early.groupDecisionCycle - early.dssStartCycle;
        const std::uint64_t groupCycles =
            group.groupDecisionCycle - group.dssStartCycle;
        earlyDss.push_back(earlyCycles);
        groupDss.push_back(groupCycles);
        pairedDeltas.push_back(groupCycles - earlyCycles);
        if (early.bist.group.hasAcceptedFault)
        {
            positiveFaultTail.push_back(
                *early.deltaLastFaultToGroupDone -
                early.deltaTestDoneToDssStart);
        }
    }
    require(earlyDss.size() == 10 && groupDss.size() == 10 &&
                pairedDeltas.size() == 10 && positiveFaultTail.size() == 9 &&
                percentile95(earlyDss) >= 4 && percentile95(groupDss) >= 24 &&
                percentile95(pairedDeltas) >= 20,
            "Summary statistics did not receive the expected eligible rows");
    std::cout << "S1D_PREFLIGHT rows=20 pairs=10 positive_fault_pairs=9 "
                 "zero_fault_pairs=1 both_pass_pairs=6 both_fail_pairs=4 "
                 "csv_schema=PASS na_handling=PASS paired_alignment=PASS "
                 "metric_identities=PASS summary_statistics=PASS\n";
}

} // namespace

int main(int argc, char **argv)
{
    try
    {
        const std::string outputPath = argc == 2
            ? argv[1] : "build/tests/s1d_date_2x2_preflight_only.csv";
        verifyAndWrite(outputPath);
        std::cout << "dss_formal_latency_preflight_test PASS\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "dss_formal_latency_preflight_test FAIL: "
                  << error.what() << '\n';
        return 1;
    }
}
