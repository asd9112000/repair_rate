#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "DirectionalMultiConfigAnalyzer.hpp"
#include "DssPostBistLatency.hpp"
#include "DynamicFaultGenerator.hpp"
#include "SimulationConfig.hpp"

namespace
{

using dynamic_spare::DirectionalConfigAnalysis;
using dynamic_spare::DirectionalConfigId;
using dynamic_spare::DirectionalMultiConfigAnalysis;
using dynamic_spare::DirectionalMultiConfigAnalyzer;
using dynamic_spare::DssDecisionWorkPlan;
using dynamic_spare::DssFaultSetEvent;
using dynamic_spare::DssPostBistLatencyResult;
using dynamic_spare::DssPostBistPolicy;
using dynamic_spare::DssPostBistRequest;
using dynamic_spare::FaultGroup;

constexpr std::uint64_t kSeed = 20260910;
constexpr std::size_t kTraceCount = 10000;
constexpr std::uint64_t kArrivalIntervalCycles = 1;
constexpr std::array<std::uint64_t, 33> kGaps{{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32}};

enum class ResourceId
{
    ARow,
    DRow,
    BColumn,
    CColumn
};

struct ResourceState
{
    bool released = false;
    int borrower = -1;
};

struct CandidateChoice
{
    bool selected = false;
    bool release = false;
    ResourceId releaseResource = ResourceId::ARow;
    bool borrow = false;
    ResourceId donorResource = ResourceId::BColumn;
    int configId = -1;
    std::uint8_t patternId = 0;
};

struct FrozenPolicyOutcome
{
    std::array<unsigned int, 4> candidateEvaluations{{0, 0, 0, 0}};
    std::array<int, 4> selectedConfig{{-1, -1, -1, -1}};
    std::array<std::uint8_t, 4> selectedPattern{{0, 0, 0, 0}};
    std::array<int, 4> selectedDonor{{-1, -1, -1, -1}};
    bool groupRepairable = false;
    int failurePosition = -1;
    std::size_t borrowCount = 0;
    std::size_t releaseCount = 0;
};

struct RawRecord
{
    std::size_t vectorId = 0;
    DssPostBistLatencyResult latency;
    FrozenPolicyOutcome outcome;
};

struct Distribution
{
    double mean = 0.0;
    double p50 = 0.0;
    double p95 = 0.0;
    std::uint64_t maximum = 0;
};

struct SignedDistribution
{
    double mean = 0.0;
    double p50 = 0.0;
    double p95 = 0.0;
    std::int64_t maximum = 0;
};

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

const DirectionalConfigAnalysis &config(
    const DirectionalMultiConfigAnalysis &analysis,
    int configId)
{
    return analysis.configs.at(static_cast<std::size_t>(configId));
}

ResourceState &resource(
    std::array<ResourceState, 4> &ledger,
    ResourceId resourceId)
{
    return ledger.at(static_cast<std::size_t>(resourceId));
}

int resourceIndex(ResourceId resourceId)
{
    return static_cast<int>(resourceId);
}

ResourceId ownReleasedResource(std::size_t sa)
{
    switch (sa)
    {
        case 0: return ResourceId::ARow;
        case 1: return ResourceId::BColumn;
        case 2: return ResourceId::CColumn;
        case 3: return ResourceId::DRow;
    }
    throw std::logic_error("Invalid 2x2 DSS subarray index");
}

std::array<ResourceId, 2> donorPriority(std::size_t sa)
{
    switch (sa)
    {
        case 0: return {{ResourceId::BColumn, ResourceId::CColumn}};
        case 1: return {{ResourceId::ARow, ResourceId::DRow}};
        case 2: return {{ResourceId::DRow, ResourceId::ARow}};
        case 3: return {{ResourceId::CColumn, ResourceId::BColumn}};
    }
    throw std::logic_error("Invalid 2x2 DSS subarray index");
}

bool needsRelease(std::size_t sa, int configId)
{
    return (sa == 0 || sa == 3)
        ? (configId == 4 || configId == 6)
        : (configId == 1 || configId == 3);
}

bool needsBorrow(std::size_t sa, int configId)
{
    return (sa == 0 || sa == 3)
        ? (configId == 5 || configId == 6)
        : (configId == 2 || configId == 3);
}

std::array<int, 4> rankFor(DssPostBistPolicy policy, std::size_t sa)
{
    if (policy == DssPostBistPolicy::Early)
    {
        // Frozen EARLY rank: A/D 0→4→5→6; B/C 0→2→1→3.
        return (sa == 0 || sa == 3)
            ? std::array<int, 4>{{0, 4, 5, 6}}
            : std::array<int, 4>{{0, 2, 1, 3}};
    }

    // Frozen GROUP rank: A/D 4→0→6→5; B/C 1→0→3→2.
    return (sa == 0 || sa == 3)
        ? std::array<int, 4>{{4, 0, 6, 5}}
        : std::array<int, 4>{{1, 0, 3, 2}};
}

FrozenPolicyOutcome evaluateFrozenPolicy(
    const std::array<DirectionalMultiConfigAnalysis, 4> &analysis,
    DssPostBistPolicy policy)
{
    std::array<ResourceState, 4> ledger{};
    FrozenPolicyOutcome outcome;
    outcome.groupRepairable = true;

    // The actual candidate-valid/PatternID data come from the existing
    // DirectionalMultiConfigAnalyzer.  This adapter only applies the frozen
    // Phase 3G/4F rank, directional donor order, and atomic ledger semantics
    // needed to obtain a timing work plan.
    for (std::size_t sa = 0; sa < analysis.size(); ++sa)
    {
        bool found = false;
        for (const int configId : rankFor(policy, sa))
        {
            ++outcome.candidateEvaluations[sa];
            const DirectionalConfigAnalysis &candidate = config(analysis[sa], configId);
            if (candidate.lowestValidPatternId == 0)
            {
                continue;
            }

            CandidateChoice choice;
            choice.release = needsRelease(sa, configId);
            choice.releaseResource = ownReleasedResource(sa);
            choice.borrow = needsBorrow(sa, configId);
            choice.configId = configId;
            choice.patternId = candidate.lowestValidPatternId;
            if (choice.borrow)
            {
                bool donorFound = false;
                for (const ResourceId donor : donorPriority(sa))
                {
                    const ResourceState &state = resource(ledger, donor);
                    if (state.released && state.borrower == -1)
                    {
                        choice.donorResource = donor;
                        donorFound = true;
                        break;
                    }
                }
                if (!donorFound)
                {
                    continue;
                }
            }

            choice.selected = true;
            // Atomic commit: no rejected candidate mutates the ledger.
            if (choice.release)
            {
                ResourceState &state = resource(ledger, choice.releaseResource);
                require(!state.released && state.borrower == -1,
                        "Frozen DSS attempted to release an unavailable resource");
                state.released = true;
                ++outcome.releaseCount;
            }
            if (choice.borrow)
            {
                ResourceState &state = resource(ledger, choice.donorResource);
                require(state.released && state.borrower == -1,
                        "Frozen DSS selected an unavailable donor");
                state.borrower = static_cast<int>(sa);
                outcome.selectedDonor[sa] = resourceIndex(choice.donorResource);
                ++outcome.borrowCount;
            }
            outcome.selectedConfig[sa] = choice.configId;
            outcome.selectedPattern[sa] = choice.patternId;
            found = true;
            break;
        }
        if (!found)
        {
            outcome.groupRepairable = false;
            outcome.failurePosition = static_cast<int>(sa);
            break;
        }
    }
    return outcome;
}

DssDecisionWorkPlan planFor(
    DssPostBistPolicy policy,
    const FrozenPolicyOutcome &outcome)
{
    DssDecisionWorkPlan plan;
    plan.policy = policy;
    plan.candidateEvaluations = outcome.candidateEvaluations;
    plan.groupRepairable = outcome.groupRepairable;
    plan.failurePosition = outcome.failurePosition;
    return plan;
}

std::vector<DssFaultSetEvent> arrivalTrace(const FaultGroup &faults)
{
    std::vector<DssFaultSetEvent> events;
    std::uint64_t cycle = 0;
    std::size_t faultId = 0;
    // DynamicFaultGenerator creates each SA's faults in deterministic order.
    // BIST consumes the generated group in frozen A→B→C→D order at one
    // fault-set-changing event per cycle.
    for (std::size_t sa = 0; sa < faults.size(); ++sa)
    {
        for (const auto &fault : faults[sa])
        {
            (void)fault;
            events.push_back({cycle, faultId, true});
            cycle += kArrivalIntervalCycles;
            ++faultId;
        }
    }
    return events;
}

DssPostBistLatencyResult runEventModel(
    const std::vector<DssFaultSetEvent> &events,
    std::uint64_t gap,
    const DssDecisionWorkPlan &plan)
{
    require(!events.empty(), "Generated Phase 4I trace is unexpectedly empty");
    DssPostBistRequest request;
    request.faultEvents = events;
    request.bistEndCycle = events.back().cycle + gap;
    request.finalDecisionPlan = plan;
    return dynamic_spare::modelDssPostBistLatency(request);
}

Distribution distribution(const std::vector<std::uint64_t> &values)
{
    require(!values.empty(), "Cannot summarize an empty latency distribution");
    std::vector<std::uint64_t> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    const std::uint64_t sum = std::accumulate(
        sorted.begin(), sorted.end(), std::uint64_t{0});
    Distribution result;
    result.mean = static_cast<double>(sum) / static_cast<double>(sorted.size());
    result.p50 = sorted.size() % 2 == 0
        ? (static_cast<double>(sorted[sorted.size() / 2 - 1]) +
           static_cast<double>(sorted[sorted.size() / 2])) / 2.0
        : static_cast<double>(sorted[sorted.size() / 2]);
    const std::size_t p95Index =
        (95 * sorted.size() + 99) / 100 - 1; // nearest-rank P95
    result.p95 = static_cast<double>(sorted.at(p95Index));
    result.maximum = sorted.back();
    return result;
}

SignedDistribution signedDistribution(const std::vector<std::int64_t> &values)
{
    require(!values.empty(), "Cannot summarize an empty paired distribution");
    std::vector<std::int64_t> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    const std::int64_t sum = std::accumulate(
        sorted.begin(), sorted.end(), std::int64_t{0});
    SignedDistribution result;
    result.mean = static_cast<double>(sum) / static_cast<double>(sorted.size());
    result.p50 = sorted.size() % 2 == 0
        ? (static_cast<double>(sorted[sorted.size() / 2 - 1]) +
           static_cast<double>(sorted[sorted.size() / 2])) / 2.0
        : static_cast<double>(sorted[sorted.size() / 2]);
    const std::size_t p95Index =
        (95 * sorted.size() + 99) / 100 - 1;
    result.p95 = static_cast<double>(sorted.at(p95Index));
    result.maximum = sorted.back();
    return result;
}

std::string policyName(DssPostBistPolicy policy)
{
    return dynamic_spare::toString(policy);
}

void writeRawRow(std::ofstream &output, const RawRecord &record)
{
    const DssPostBistLatencyResult &latency = record.latency;
    const FrozenPolicyOutcome &outcome = record.outcome;
    output << record.vectorId << ',' << kSeed << ',' << policyName(latency.policy)
           << ',' << latency.faultCount << ',' << latency.lastFaultCycle
           << ',' << latency.bistEndCycle << ',' << latency.gapCycles
           << ',' << latency.decisionReadyCycle << ','
           << latency.latencyFromLastFaultCycles << ','
           << latency.latencyPostBistCycles << ','
           << (latency.groupRepairable ? 1 : 0) << ','
           << latency.failurePosition << ','
           << outcome.candidateEvaluations[0] << ':'
           << outcome.candidateEvaluations[1] << ':'
           << outcome.candidateEvaluations[2] << ':'
           << outcome.candidateEvaluations[3] << ','
           << outcome.selectedConfig[0] << ':' << outcome.selectedConfig[1]
           << ':' << outcome.selectedConfig[2] << ':'
           << outcome.selectedConfig[3] << ','
           << outcome.selectedDonor[0] << ':' << outcome.selectedDonor[1]
           << ':' << outcome.selectedDonor[2] << ':'
           << outcome.selectedDonor[3] << ','
           << outcome.borrowCount << ',' << outcome.releaseCount << '\n';
}

void writeSummaryHeader(std::ofstream &output)
{
    output << "policy,G,N,mean_Lpost,P50_Lpost,P95_Lpost,max_Lpost,"
              "zero_latency_fraction,mean_Llast_fault,group_repairable_rate\n";
}

void writeSummaryRow(
    std::ofstream &output,
    DssPostBistPolicy policy,
    std::uint64_t gap,
    const std::vector<RawRecord> &records)
{
    std::vector<std::uint64_t> post;
    std::vector<std::uint64_t> last;
    std::size_t zeroCount = 0;
    std::size_t repairableCount = 0;
    for (const RawRecord &record : records)
    {
        post.push_back(record.latency.latencyPostBistCycles);
        last.push_back(record.latency.latencyFromLastFaultCycles);
        zeroCount += record.latency.latencyPostBistCycles == 0 ? 1 : 0;
        repairableCount += record.latency.groupRepairable ? 1 : 0;
    }
    const Distribution postStats = distribution(post);
    const Distribution lastStats = distribution(last);
    output << policyName(policy) << ',' << gap << ',' << records.size() << ','
           << std::fixed << std::setprecision(6) << postStats.mean << ','
           << postStats.p50 << ',' << postStats.p95 << ',' << postStats.maximum
           << ',' << static_cast<double>(zeroCount) / records.size() << ','
           << lastStats.mean << ','
           << static_cast<double>(repairableCount) / records.size() << '\n';
}

void writeZeroThresholdSummary(
    std::ofstream &output,
    DssPostBistPolicy policy,
    const std::vector<std::uint64_t> &thresholds)
{
    const Distribution stats = distribution(thresholds);
    output << policyName(policy) << ',' << thresholds.size() << ','
           << std::fixed << std::setprecision(6) << stats.mean << ','
           << stats.p50 << ',' << stats.p95 << ',' << stats.maximum << ','
           << thresholds.size() << '\n';
}

void writeStratifiedSummaryRow(
    std::ofstream &output,
    DssPostBistPolicy policy,
    std::uint64_t gap,
    bool groupRepairable,
    const std::vector<RawRecord> &records)
{
    std::vector<std::uint64_t> post;
    std::vector<std::uint64_t> last;
    std::size_t zeroCount = 0;
    for (const RawRecord &record : records)
    {
        if (record.latency.groupRepairable != groupRepairable)
        {
            continue;
        }
        post.push_back(record.latency.latencyPostBistCycles);
        last.push_back(record.latency.latencyFromLastFaultCycles);
        zeroCount += record.latency.latencyPostBistCycles == 0 ? 1 : 0;
    }
    if (post.empty())
    {
        return;
    }
    const Distribution postStats = distribution(post);
    const Distribution lastStats = distribution(last);
    output << policyName(policy) << ',' << gap << ','
           << (groupRepairable ? "repairable" : "unrepairable") << ','
           << post.size() << ',' << std::fixed << std::setprecision(6)
           << postStats.mean << ',' << postStats.p50 << ',' << postStats.p95
           << ',' << postStats.maximum << ','
           << static_cast<double>(zeroCount) / post.size() << ','
           << lastStats.mean << '\n';
}

std::size_t writePairedSummaryRow(
    std::ofstream &output,
    std::uint64_t gap,
    const std::vector<RawRecord> &early,
    const std::vector<RawRecord> &group)
{
    require(early.size() == group.size(), "Paired latency corpus size differs");
    std::vector<std::int64_t> deltas;
    std::size_t earlyLess = 0;
    std::size_t equal = 0;
    std::size_t earlyGreater = 0;
    for (std::size_t index = 0; index < early.size(); ++index)
    {
        require(early[index].vectorId == group[index].vectorId &&
                    early[index].latency.lastFaultCycle ==
                        group[index].latency.lastFaultCycle &&
                    early[index].latency.bistEndCycle ==
                        group[index].latency.bistEndCycle,
                "Paired EARLY/GROUP event traces diverged");
        const std::uint64_t earlyLatency = early[index].latency.latencyPostBistCycles;
        const std::uint64_t groupLatency = group[index].latency.latencyPostBistCycles;
        deltas.push_back(static_cast<std::int64_t>(groupLatency) -
                         static_cast<std::int64_t>(earlyLatency));
        earlyLess += earlyLatency < groupLatency ? 1 : 0;
        equal += earlyLatency == groupLatency ? 1 : 0;
        earlyGreater += earlyLatency > groupLatency ? 1 : 0;
    }
    const SignedDistribution stats = signedDistribution(deltas);
    output << gap << ',' << deltas.size() << ',' << std::fixed
           << std::setprecision(6) << stats.mean << ',' << stats.p50 << ','
           << stats.p95 << ',' << stats.maximum << ','
           << static_cast<double>(earlyLess) / deltas.size() << ','
           << static_cast<double>(equal) / deltas.size() << ','
           << static_cast<double>(earlyGreater) / deltas.size() << '\n';
    return earlyGreater;
}

} // namespace

int main()
{
    try
    {
        std::filesystem::create_directories("results/phase4i");
        std::ofstream raw("results/phase4i/phase4i2_raw_latency.csv");
        std::ofstream summary("results/phase4i/phase4i2_summary.csv");
        std::ofstream thresholds("results/phase4i/phase4i2_zero_thresholds.csv");
        std::ofstream paired("results/phase4i/phase4i2_paired_delta.csv");
        std::ofstream stratified("results/phase4i/phase4i2_stratified_summary.csv");
        require(raw.good() && summary.good() && thresholds.good() && paired.good() &&
                    stratified.good(),
                "Unable to create Phase 4I-2 CSV artifacts");
        raw << "vector_id,seed,policy,fault_count,T_last_fault,T_BIST_end,G,"
               "T_decision_ready,L_last_fault,L_post,group_repairable,"
               "failure_position,candidate_evaluations_A_B_C_D,"
               "selected_config_A_B_C_D,selected_donor_A_B_C_D,"
               "borrow_count,release_count\n";
        writeSummaryHeader(summary);
        thresholds << "policy,N,mean_Gzero,P50_Gzero,P95_Gzero,max_Gzero,"
                      "within_sweep_count\n";
        paired << "G,N,mean_Delta_Lpost,P50_Delta_Lpost,P95_Delta_Lpost,"
                  "max_Delta_Lpost,early_less_group_fraction,equal_fraction,"
                  "early_greater_group_fraction\n";
        stratified << "policy,G,repairability,N,mean_Lpost,P50_Lpost,P95_Lpost,"
                      "max_Lpost,zero_latency_fraction,mean_Llast_fault\n";

        dynamic_spare::SimulationConfig config;
        // Reuse the repository's standard DynamicFaultGenerator baseline,
        // rather than creating a latency-specific fault distribution.
        config.faultCount = 20;
        config.simulationRuns = kTraceCount;
        config.randomSeed = kSeed;
        config.memoryRows = 1024;
        config.memoryColumns = 1024;
        config.faultCountModel = dynamic_spare::FaultCountModel::Uniform;
        config.faultSpatialModel = dynamic_spare::FaultSpatialModel::Mixed;
        config.topology = dynamic_spare::SharingTopology::Directional;
        config.sharedRows = 1;
        config.sharedColumns = 1;
        config.modifiers.maximumGroupBorrowedSpares = 1;
        dynamic_spare::DynamicFaultGenerator generator(config);
        const DirectionalMultiConfigAnalyzer analyzer;

        std::map<std::pair<DssPostBistPolicy, std::uint64_t>,
                 std::vector<RawRecord>> records;
        std::map<DssPostBistPolicy, std::vector<std::uint64_t>> zeroThresholds;
        std::size_t monotonicityViolations = 0;
        std::size_t negativePostLatencyCount = 0;
        std::size_t invalidTimestampOrdering = 0;
        std::size_t g0InterpretationMismatches = 0;
        std::size_t earlyGreaterGroupObservations = 0;

        for (std::size_t vectorId = 0; vectorId < kTraceCount; ++vectorId)
        {
            const FaultGroup faults = generator.generate(vectorId);
            std::array<DirectionalMultiConfigAnalysis, 4> analysis;
            for (std::size_t sa = 0; sa < analysis.size(); ++sa)
            {
                analysis[sa] = analyzer.analyze(faults[sa], static_cast<int>(sa));
            }
            const std::vector<DssFaultSetEvent> events = arrivalTrace(faults);
            require(events.size() == config.faultCount,
                    "Generated fault count and arrival trace disagree");

            for (const DssPostBistPolicy policy :
                 {DssPostBistPolicy::Early, DssPostBistPolicy::GroupNoScratch})
            {
                const FrozenPolicyOutcome outcome =
                    evaluateFrozenPolicy(analysis, policy);
                const DssDecisionWorkPlan plan = planFor(policy, outcome);
                std::uint64_t previousPost = UINT64_MAX;
                bool haveZero = false;
                for (const std::uint64_t gap : kGaps)
                {
                    const DssPostBistLatencyResult latency =
                        runEventModel(events, gap, plan);
                    if (latency.latencyPostBistCycles > previousPost)
                    {
                        ++monotonicityViolations;
                    }
                    previousPost = latency.latencyPostBistCycles;
                    negativePostLatencyCount += 0; // unsigned by construction
                    invalidTimestampOrdering += latency.lastFaultCycle >
                        latency.bistEndCycle ? 1 : 0;
                    if (gap == 0 &&
                        latency.latencyPostBistCycles !=
                            latency.latencyFromLastFaultCycles)
                    {
                        ++g0InterpretationMismatches;
                    }
                    RawRecord record{vectorId, latency, outcome};
                    records[{policy, gap}].push_back(record);
                    writeRawRow(raw, record);
                    if (!haveZero && latency.latencyPostBistCycles == 0)
                    {
                        zeroThresholds[policy].push_back(gap);
                        haveZero = true;
                    }
                }
                require(haveZero,
                        "The full G=0..32 sweep did not reach zero latency");
            }
        }

        require(monotonicityViolations == 0 && negativePostLatencyCount == 0 &&
                    invalidTimestampOrdering == 0 && g0InterpretationMismatches == 0,
                "Phase 4I-2 event-model sanity invariant failed");

        for (const DssPostBistPolicy policy :
             {DssPostBistPolicy::Early, DssPostBistPolicy::GroupNoScratch})
        {
            for (const std::uint64_t gap : kGaps)
            {
                const auto found = records.find({policy, gap});
                require(found != records.end() && found->second.size() == kTraceCount,
                        "Policy/G corpus is incomplete");
                writeSummaryRow(summary, policy, gap, found->second);
                writeStratifiedSummaryRow(
                    stratified, policy, gap, true, found->second);
                writeStratifiedSummaryRow(
                    stratified, policy, gap, false, found->second);
            }
            writeZeroThresholdSummary(thresholds, policy, zeroThresholds.at(policy));
        }
        for (const std::uint64_t gap : kGaps)
        {
            earlyGreaterGroupObservations += writePairedSummaryRow(
                paired,
                gap,
                records.at({DssPostBistPolicy::Early, gap}),
                records.at({DssPostBistPolicy::GroupNoScratch, gap}));
        }
        require(earlyGreaterGroupObservations == 0,
                "EARLY post-BIST latency exceeded GROUP; paired traces require inspection");

        std::cout << "TRACE_COUNT=" << kTraceCount << '\n'
                  << "SEED=" << kSeed << '\n'
                  << "FAULT_ARRIVAL_MODEL=generated A_to_B_to_C_to_D order; "
                     "one fault-set-changing event per cycle\n"
                  << "EARLY_GROUP_SAME_TRACE_COUNT=PASS\n"
                  << "EARLY_GROUP_SAME_VECTOR_IDS=PASS\n"
                  << "EARLY_GROUP_SAME_G_POINTS=PASS\n"
                  << "EARLY_GROUP_SAME_FAULT_ARRIVAL_TRACES=PASS\n"
                  << "MONOTONICITY_VIOLATIONS=" << monotonicityViolations << '\n'
                  << "NEGATIVE_L_POST_COUNT=" << negativePostLatencyCount << '\n'
                  << "INVALID_TIMESTAMP_ORDERING=" << invalidTimestampOrdering << '\n'
                  << "G0_DIRECTED_INTERPRETATION_MISMATCHES="
                  << g0InterpretationMismatches << '\n'
                  << "EARLY_GREATER_GROUP_OBSERVATIONS="
                  << earlyGreaterGroupObservations << '\n'
                  << "REPAIRABILITY_STRATIFICATION=PASS\n"
                  << "PHASE_4I_2_LATENCY_SWEEP=PASS\n"
                  << "READY_FOR_PHASE_4I_3=YES\n"
                  << "V2_PHASE=PHASE_4I\n"
                  << "V2_NEXT_PHASE_AUTHORIZED=YES\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Phase 4I-2 latency sweep failed: " << error.what() << '\n';
        return 1;
    }
}
