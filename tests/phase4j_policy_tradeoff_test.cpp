#include "DirectionalMultiConfigAnalyzer.hpp"
#include "DynamicFaultGenerator.hpp"
#include "SimulationConfig.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

namespace
{

using dynamic_spare::DirectionalMultiConfigAnalysis;
using dynamic_spare::DirectionalMultiConfigAnalyzer;
using dynamic_spare::FaultCountModel;
using dynamic_spare::FaultGroup;

constexpr std::uint64_t kSeed = 20260910;
constexpr std::size_t kCasesPerPoint = 10000;

enum class Policy { Early, GroupNoScratch };
enum class ResourceId { ARow, DRow, BColumn, CColumn };
enum class OutcomeClass { BothPass, EarlyOnly, GroupOnly, BothFail };

struct ResourceState
{
    bool released = false;
    int borrower = -1;
};

struct PolicyOutcome
{
    bool repairable = false;
    int failurePosition = -1;
    std::array<int, 4> selectedConfig{{-1, -1, -1, -1}};
    std::array<std::uint8_t, 4> selectedPattern{};
    std::array<int, 4> selectedDonor{{-1, -1, -1, -1}};
    std::array<unsigned, 4> ledgerBefore{};
    std::array<unsigned, 4> ledgerAfter{};
};

struct Counts
{
    std::size_t bothPass = 0;
    std::size_t earlyOnly = 0;
    std::size_t groupOnly = 0;
    std::size_t bothFail = 0;
    std::size_t pairedIdMismatches = 0;
    std::size_t duplicateIds = 0;
    std::size_t missingResults = 0;
};

struct ExperimentPoint
{
    const char *severity;
    std::uint64_t faultCount;
    FaultCountModel imbalance;
    const char *imbalanceName;
};

void require(bool condition, const std::string &message)
{
    if (!condition) throw std::runtime_error(message);
}

const char *outcomeName(OutcomeClass outcome)
{
    switch (outcome)
    {
        case OutcomeClass::BothPass: return "BOTH_PASS";
        case OutcomeClass::EarlyOnly: return "EARLY_ONLY";
        case OutcomeClass::GroupOnly: return "GROUP_ONLY";
        case OutcomeClass::BothFail: return "BOTH_FAIL";
    }
    return "INVALID";
}

ResourceId ownResource(std::size_t sa)
{
    static constexpr std::array<ResourceId, 4> table{{
        ResourceId::ARow, ResourceId::BColumn, ResourceId::CColumn, ResourceId::DRow}};
    return table.at(sa);
}

std::array<ResourceId, 2> donorPriority(std::size_t sa)
{
    static constexpr std::array<std::array<ResourceId, 2>, 4> table{{
        {{ResourceId::BColumn, ResourceId::CColumn}},
        {{ResourceId::ARow, ResourceId::DRow}},
        {{ResourceId::DRow, ResourceId::ARow}},
        {{ResourceId::CColumn, ResourceId::BColumn}}}};
    return table.at(sa);
}

bool needsRelease(std::size_t sa, int configId)
{
    return (sa == 0 || sa == 3) ? (configId == 4 || configId == 6)
                                 : (configId == 1 || configId == 3);
}

bool needsBorrow(std::size_t sa, int configId)
{
    return (sa == 0 || sa == 3) ? (configId == 5 || configId == 6)
                                 : (configId == 2 || configId == 3);
}

std::array<int, 4> rankFor(Policy policy, std::size_t sa)
{
    if (policy == Policy::Early)
    {
        return (sa == 0 || sa == 3) ? std::array<int, 4>{{0, 4, 5, 6}}
                                     : std::array<int, 4>{{0, 2, 1, 3}};
    }
    return (sa == 0 || sa == 3) ? std::array<int, 4>{{4, 0, 6, 5}}
                                 : std::array<int, 4>{{1, 0, 3, 2}};
}

unsigned packLedger(const std::array<ResourceState, 4> &ledger)
{
    unsigned result = 0;
    for (std::size_t resource = 0; resource < ledger.size(); ++resource)
    {
        result |= static_cast<unsigned>(ledger[resource].released) << resource;
        if (ledger[resource].borrower >= 0)
            result |= static_cast<unsigned>(ledger[resource].borrower + 1)
                << (4 + 2 * resource);
    }
    return result;
}

PolicyOutcome evaluateFrozenV2Policy(
    const std::array<DirectionalMultiConfigAnalysis, 4> &analysis,
    Policy policy)
{
    std::array<ResourceState, 4> ledger{};
    PolicyOutcome result;
    result.repairable = true;

    // This is the Phase 4I frozen adapter: it consumes only existing analyzer
    // candidate-valid/PatternID results and applies the V2 policy-specific
    // rank, directional donor priority, atomic commit, and A→B→C→D traversal.
    for (std::size_t sa = 0; sa < analysis.size(); ++sa)
    {
        bool found = false;
        result.ledgerBefore[sa] = packLedger(ledger);
        for (const int configId : rankFor(policy, sa))
        {
            const auto &candidate = analysis[sa].configs.at(
                static_cast<std::size_t>(configId));
            if (candidate.lowestValidPatternId == 0) continue;

            const bool release = needsRelease(sa, configId);
            const bool borrow = needsBorrow(sa, configId);
            const ResourceId released = ownResource(sa);
            ResourceId donor = ResourceId::ARow;
            if (release && (ledger.at(static_cast<std::size_t>(released)).released ||
                            ledger.at(static_cast<std::size_t>(released)).borrower >= 0))
                continue;
            if (borrow)
            {
                bool donorFound = false;
                for (const ResourceId candidateDonor : donorPriority(sa))
                {
                    const auto &state = ledger.at(static_cast<std::size_t>(candidateDonor));
                    if (state.released && state.borrower < 0)
                    {
                        donor = candidateDonor;
                        donorFound = true;
                        break;
                    }
                }
                if (!donorFound) continue;
            }

            // Rejected candidates have not touched `ledger`; accepted release
            // and borrow are one atomic policy transaction.
            if (release) ledger.at(static_cast<std::size_t>(released)).released = true;
            if (borrow) ledger.at(static_cast<std::size_t>(donor)).borrower = static_cast<int>(sa);
            result.selectedConfig[sa] = configId;
            result.selectedPattern[sa] = candidate.lowestValidPatternId;
            result.selectedDonor[sa] = borrow ? static_cast<int>(donor) : -1;
            result.ledgerAfter[sa] = packLedger(ledger);
            found = true;
            break;
        }
        if (!found)
        {
            result.repairable = false;
            result.failurePosition = static_cast<int>(sa);
            break; // Frozen first-failure/no-rollback behavior.
        }
    }
    return result;
}

OutcomeClass classify(const PolicyOutcome &early, const PolicyOutcome &group)
{
    if (early.repairable && group.repairable) return OutcomeClass::BothPass;
    if (early.repairable) return OutcomeClass::EarlyOnly;
    if (group.repairable) return OutcomeClass::GroupOnly;
    return OutcomeClass::BothFail;
}

std::string joinInts(const std::array<int, 4> &values)
{
    std::string result;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i) result += ':';
        result += std::to_string(values[i]);
    }
    return result;
}

std::string joinPatterns(const std::array<std::uint8_t, 4> &values)
{
    std::string result;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i) result += ':';
        result += std::to_string(values[i]);
    }
    return result;
}

std::string joinLedgers(const std::array<unsigned, 4> &values)
{
    std::string result;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i) result += ':';
        result += std::to_string(values[i]);
    }
    return result;
}

std::string candidateMap(const std::array<DirectionalMultiConfigAnalysis, 4> &analysis)
{
    std::string result;
    for (std::size_t sa = 0; sa < analysis.size(); ++sa)
    {
        if (sa) result += ';';
        result += static_cast<char>('A' + sa);
        result += '=';
        for (std::size_t cfg = 0; cfg < analysis[sa].configs.size(); ++cfg)
        {
            if (cfg) result += '|';
            result += "C" + std::to_string(cfg) + ":P" +
                std::to_string(analysis[sa].configs[cfg].lowestValidPatternId);
        }
    }
    return result;
}

void writeRaw(std::ofstream &raw, const ExperimentPoint &point,
              std::size_t vectorId,
              const std::array<DirectionalMultiConfigAnalysis, 4> &analysis,
              const PolicyOutcome &early, const PolicyOutcome &group,
              OutcomeClass classification)
{
    raw << point.severity << ',' << point.faultCount << ',' << point.imbalanceName
        << ',' << vectorId << ',' << kSeed << ',' << outcomeName(classification)
        << ',' << early.repairable << ',' << group.repairable << ','
        << early.failurePosition << ',' << group.failurePosition << ','
        << '"' << candidateMap(analysis) << "\",\""
        << joinInts(early.selectedConfig) << "\",\"" << joinPatterns(early.selectedPattern)
        << "\",\"" << joinInts(early.selectedDonor) << "\",\""
        << joinLedgers(early.ledgerBefore) << "\",\"" << joinLedgers(early.ledgerAfter)
        << "\",\"" << joinInts(group.selectedConfig) << "\",\""
        << joinPatterns(group.selectedPattern) << "\",\"" << joinInts(group.selectedDonor)
        << "\",\"" << joinLedgers(group.ledgerBefore) << "\",\""
        << joinLedgers(group.ledgerAfter) << "\"\n";
}

void writeExample(std::ofstream &examples, const ExperimentPoint &point,
                  std::size_t vectorId,
                  const std::array<DirectionalMultiConfigAnalysis, 4> &analysis,
                  const PolicyOutcome &early, const PolicyOutcome &group,
                  OutcomeClass classification)
{
    examples << point.severity << ',' << point.faultCount << ',' << point.imbalanceName
             << ',' << vectorId << ',' << kSeed << ',' << outcomeName(classification)
             << ',' << '"' << candidateMap(analysis) << "\",\""
             << joinInts(early.selectedConfig) << "\",\"" << joinPatterns(early.selectedPattern)
             << "\",\"" << joinInts(early.selectedDonor) << "\",\""
             << joinLedgers(early.ledgerBefore) << "\",\"" << joinLedgers(early.ledgerAfter)
             << "\",\"" << early.failurePosition << "\",\""
             << joinInts(group.selectedConfig) << "\",\"" << joinPatterns(group.selectedPattern)
             << "\",\"" << joinInts(group.selectedDonor) << "\",\""
             << joinLedgers(group.ledgerBefore) << "\",\"" << joinLedgers(group.ledgerAfter)
             << "\",\"" << group.failurePosition << "\"\n";
}

void add(Counts &counts, OutcomeClass classification)
{
    switch (classification)
    {
        case OutcomeClass::BothPass: ++counts.bothPass; break;
        case OutcomeClass::EarlyOnly: ++counts.earlyOnly; break;
        case OutcomeClass::GroupOnly: ++counts.groupOnly; break;
        case OutcomeClass::BothFail: ++counts.bothFail; break;
    }
}

double percentage(std::size_t numerator, std::size_t denominator)
{
    return 100.0 * static_cast<double>(numerator) / static_cast<double>(denominator);
}

void writeSummary(std::ofstream &summary, const ExperimentPoint &point, const Counts &counts)
{
    const std::size_t total = counts.bothPass + counts.earlyOnly +
        counts.groupOnly + counts.bothFail;
    require(total == kCasesPerPoint, "Outcome classification does not conserve cases");
    const double earlyRate = percentage(counts.bothPass + counts.earlyOnly, total);
    const double groupRate = percentage(counts.bothPass + counts.groupOnly, total);
    summary << point.severity << ',' << point.faultCount << ',' << point.imbalanceName
            << ',' << total << ',' << counts.bothPass << ',' << counts.earlyOnly
            << ',' << counts.groupOnly << ',' << counts.bothFail << ','
            << std::fixed << std::setprecision(6) << earlyRate << ',' << groupRate
            << ',' << groupRate - earlyRate << ',' << percentage(counts.earlyOnly, total)
            << ',' << percentage(counts.groupOnly, total) << ','
            << (counts.pairedIdMismatches == 0 ? "PASS" : "FAIL") << ','
            << (counts.duplicateIds == 0 ? "PASS" : "FAIL") << ','
            << (counts.missingResults == 0 ? "PASS" : "FAIL") << "\n";
}

} // namespace

int main()
{
    try
    {
        // Preserve the established DATE group-scope severity sweep (8..32,
        // step 4) under its moderate/mixed baseline.  Uniform and strong
        // 20-fault points reuse existing imbalance models as paired sensitivity
        // checks without inventing a new fault distribution.
        const std::array<ExperimentPoint, 9> points{{
            {"LOW", 8, FaultCountModel::ModerateImbalance, "moderate"},
            {"LOW", 12, FaultCountModel::ModerateImbalance, "moderate"},
            {"LOW", 16, FaultCountModel::ModerateImbalance, "moderate"},
            {"MEDIUM", 20, FaultCountModel::ModerateImbalance, "moderate"},
            {"HIGH", 24, FaultCountModel::ModerateImbalance, "moderate"},
            {"HIGH", 28, FaultCountModel::ModerateImbalance, "moderate"},
            {"HIGH", 32, FaultCountModel::ModerateImbalance, "moderate"},
            {"MEDIUM", 20, FaultCountModel::Uniform, "uniform"},
            {"MEDIUM", 20, FaultCountModel::StrongImbalance, "strong"}}};

        std::filesystem::create_directories("results/phase4j");
        std::ofstream raw("results/phase4j/phase4j_policy_tradeoff_raw.csv");
        std::ofstream summary("results/phase4j/phase4j_policy_tradeoff_summary.csv");
        std::ofstream examples("results/phase4j/phase4j_divergence_examples.csv");
        require(raw.good() && summary.good() && examples.good(),
                "Unable to create Phase 4J result artifacts");
        raw << "severity,fault_count,imbalance,vector_id,seed,outcome,early_repairable,"
               "group_repairable,early_failure_position,group_failure_position,"
               "candidate_valid_pattern_map,early_selected_config,early_selected_pattern,"
               "early_selected_donor,early_ledger_before,early_ledger_after,"
               "group_selected_config,group_selected_pattern,group_selected_donor,"
               "group_ledger_before,group_ledger_after\n";
        summary << "severity,fault_count,imbalance,total_cases,both_pass,early_only,"
                   "group_only,both_fail,early_repair_rate_percent,"
                   "group_repair_rate_percent,group_minus_early_pp,early_only_rate_percent,"
                   "group_only_rate_percent,paired_vector_ids,duplicate_vector_ids,"
                   "missing_policy_result\n";
        examples << "severity,fault_count,imbalance,vector_id,seed,outcome,"
                     "candidate_valid_pattern_map,early_selected_config,early_selected_pattern,"
                     "early_selected_donor,early_ledger_before,early_ledger_after,"
                     "early_failure_position,group_selected_config,group_selected_pattern,"
                     "group_selected_donor,group_ledger_before,group_ledger_after,"
                     "group_failure_position\n";

        std::map<std::string, bool> savedExample;
        Counts aggregate;
        const DirectionalMultiConfigAnalyzer analyzer;
        for (const ExperimentPoint &point : points)
        {
            dynamic_spare::SimulationConfig config;
            config.spareRows = 2;
            config.spareColumns = 2;
            config.sharedRows = 1;
            config.sharedColumns = 1;
            config.topology = dynamic_spare::SharingTopology::Directional;
            config.modifiers.maximumGroupBorrowedSpares = 1;
            config.faultCount = point.faultCount;
            config.simulationRuns = kCasesPerPoint;
            config.randomSeed = kSeed;
            config.memoryRows = 1024;
            config.memoryColumns = 1024;
            config.faultCountModel = point.imbalance;
            config.faultSpatialModel = dynamic_spare::FaultSpatialModel::Mixed;
            dynamic_spare::DynamicFaultGenerator generator(config);
            Counts pointCounts;
            std::set<std::size_t> pointVectorIds;
            for (std::size_t vectorId = 0; vectorId < kCasesPerPoint; ++vectorId)
            {
                if (!pointVectorIds.insert(vectorId).second)
                {
                    ++pointCounts.duplicateIds;
                    continue;
                }
                const FaultGroup faults = generator.generate(vectorId);
                if (faults.size() != 4)
                {
                    ++pointCounts.missingResults;
                    continue;
                }
                std::array<DirectionalMultiConfigAnalysis, 4> analysis;
                for (std::size_t sa = 0; sa < analysis.size(); ++sa)
                    analysis[sa] = analyzer.analyze(faults[sa], static_cast<int>(sa));
                // Both policy results carry the same stable generator vector ID.
                const std::size_t earlyVectorId = vectorId;
                const std::size_t groupVectorId = vectorId;
                if (earlyVectorId != groupVectorId)
                {
                    ++pointCounts.pairedIdMismatches;
                    continue;
                }
                const PolicyOutcome early = evaluateFrozenV2Policy(analysis, Policy::Early);
                const PolicyOutcome group = evaluateFrozenV2Policy(analysis, Policy::GroupNoScratch);
                const OutcomeClass classification = classify(early, group);
                add(pointCounts, classification);
                add(aggregate, classification);
                writeRaw(raw, point, vectorId, analysis, early, group, classification);
                const std::string key = std::string(point.imbalanceName) + ":" +
                    outcomeName(classification);
                if ((classification == OutcomeClass::EarlyOnly ||
                     classification == OutcomeClass::GroupOnly) && !savedExample[key])
                {
                    writeExample(examples, point, vectorId, analysis, early, group, classification);
                    savedExample[key] = true;
                }
            }
            require(pointCounts.pairedIdMismatches == 0 && pointCounts.duplicateIds == 0 &&
                        pointCounts.missingResults == 0,
                    "Phase 4J paired-vector integrity check failed");
            writeSummary(summary, point, pointCounts);
            std::cout << "PHASE4J_POINT severity=" << point.severity
                      << " imbalance=" << point.imbalanceName
                      << " BOTH_PASS=" << pointCounts.bothPass
                      << " EARLY_ONLY=" << pointCounts.earlyOnly
                      << " GROUP_ONLY=" << pointCounts.groupOnly
                      << " BOTH_FAIL=" << pointCounts.bothFail << '\n';
        }
        const std::size_t total = aggregate.bothPass + aggregate.earlyOnly +
            aggregate.groupOnly + aggregate.bothFail;
        require(total == points.size() * kCasesPerPoint,
                "Aggregate outcome classification does not conserve cases");
        std::cout << "PHASE4J_INTEGRITY paired_vector_ids=PASS same_policy_case_count=PASS "
                     "outcome_accounting=PASS duplicate_vector_ids=PASS "
                     "missing_policy_result=PASS\n";
        std::cout << "PHASE4J_TOTAL cases=" << total << " BOTH_PASS=" << aggregate.bothPass
                  << " EARLY_ONLY=" << aggregate.earlyOnly << " GROUP_ONLY="
                  << aggregate.groupOnly << " BOTH_FAIL=" << aggregate.bothFail << '\n';
    }
    catch (const std::exception &error)
    {
        std::cerr << "PHASE4J_POLICY_TRADEOFF FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
