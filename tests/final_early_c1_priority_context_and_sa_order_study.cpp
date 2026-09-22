#include "DynamicFaultGenerator.hpp"
#include "DynamicRepairSimulator.hpp"
#include "SimulationConfig.hpp"
#include "V2GroupNoScratchPolicy.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

using namespace dynamic_spare;

constexpr std::uint64_t kSeed = 20260922;
constexpr std::size_t kGroupsPerPoint = 300;
constexpr std::size_t kKnownReplayGroupsPerPoint = 250;
constexpr std::array<std::uint64_t, 4> kFaultLoads{{16, 20, 24, 28}};
constexpr std::array<std::size_t, 4> kPriority{{1, 0, 3, 2}};
constexpr std::array<std::size_t, 4> kOrderAbcd{{0, 1, 2, 3}};
constexpr std::array<std::size_t, 4> kOrderAbdc{{0, 1, 3, 2}};
constexpr std::array<std::size_t, 19> kKnownMismatchVectors{{
    216, 530, 595, 751, 780, 807, 810, 825, 836, 845,
    848, 854, 871, 878, 925, 944, 954, 956, 963}};

constexpr std::uint8_t kArow = 1U << 0;
constexpr std::uint8_t kDrow = 1U << 1;
constexpr std::uint8_t kBcol = 1U << 2;
constexpr std::uint8_t kCcol = 1U << 3;

void require(bool condition, const std::string &message)
{
    if (!condition)
        throw std::runtime_error(message);
}

SimulationConfig configFor(std::uint64_t faultCount,
                           std::size_t groupsPerPoint)
{
    SimulationConfig config;
    config.spareRows = 2;
    config.spareColumns = 2;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    config.layout = GroupLayout::Grid2x2;
    config.topology = SharingTopology::Directional;
    config.solutionTakePolicy = SolutionTakePolicy::DirectionalV2Early;
    config.modifiers.maximumGroupBorrowedSpares = 1;
    config.usePaperCamReuseCapacity = true;
    config.faultCount = faultCount;
    config.simulationRuns = groupsPerPoint;
    config.randomSeed = kSeed;
    config.memoryRows = 1024;
    config.memoryColumns = 1024;
    config.faultCountModel = FaultCountModel::ModerateImbalance;
    config.faultSpatialModel = FaultSpatialModel::Mixed;
    return config;
}

const char *roleName(std::size_t slot)
{
    switch (slot)
    {
        case 1: return "R";
        case 0: return "L";
        case 3: return "RB";
        case 2: return "B";
        default: return "INVALID";
    }
}

char saName(std::size_t sa)
{
    return static_cast<char>('A' + sa);
}

int configId(std::size_t sa, std::size_t slot)
{
    for (const V2RoleSlotMapping &mapping :
         v2RtlGroupRoleSlotMappings(ConfigContractVersion::FrozenDate2x2M1, sa))
    {
        if (mapping.roleSlot == slot)
            return mapping.configId;
    }
    throw std::logic_error("Frozen V2 slot has no ConfigID");
}

std::uint8_t ownerToken(std::size_t sa)
{
    constexpr std::array<std::uint8_t, 4> kTokens{{
        kArow, kBcol, kCcol, kDrow}};
    return kTokens.at(sa);
}

std::uint8_t borrowerToken(std::size_t sa)
{
    constexpr std::array<std::uint8_t, 4> kTokens{{
        kBcol, kDrow, kArow, kCcol}};
    return kTokens.at(sa);
}

bool ownerDimensionClaimed(std::size_t sa,
                           const CandidateRepairOption &candidate)
{
    return (sa == 0 || sa == 3)
        ? candidate.usedRows >= 2
        : candidate.usedColumns >= 2;
}

bool borrowerDimensionClaimed(std::size_t sa,
                              const CandidateRepairOption &candidate)
{
    return (sa == 0 || sa == 3)
        ? candidate.usedColumns > 2
        : candidate.usedRows > 2;
}

std::uint8_t claimMask(std::size_t sa, const CandidateRepairOption &candidate)
{
    std::uint8_t mask = 0;
    if (ownerDimensionClaimed(sa, candidate))
        mask |= ownerToken(sa);
    if (borrowerDimensionClaimed(sa, candidate))
        mask |= borrowerToken(sa);
    return mask;
}

std::string maskName(std::uint8_t mask)
{
    if (mask == 0)
        return "NONE";
    std::string result;
    const std::array<std::pair<std::uint8_t, const char *>, 4> names{{
        {kArow, "A_ROW"}, {kDrow, "D_ROW"},
        {kBcol, "B_COL"}, {kCcol, "C_COL"}}};
    for (const auto &entry : names)
    {
        if ((mask & entry.first) == 0)
            continue;
        if (!result.empty())
            result += '|';
        result += entry.second;
    }
    return result;
}

const CandidateRepairOption *firstPatternCandidate(
    const GroupRepairResult &source, std::size_t sa, std::size_t slot)
{
    const auto &attempts = source.attemptsBySubarray.at(sa);
    if (slot >= attempts.size())
        return nullptr;
    const RepairAttemptResult &attempt = attempts[slot];
    if (!attempt.repairSuccess || !attempt.tileSolutionState.has_value() ||
        attempt.validCandidateOptions.empty())
    {
        return nullptr;
    }
    return &*std::min_element(
        attempt.validCandidateOptions.begin(), attempt.validCandidateOptions.end(),
        [](const CandidateRepairOption &left, const CandidateRepairOption &right)
        {
            return left.candidateIndex < right.candidateIndex;
        });
}

struct Step
{
    std::size_t sa = 0;
    std::size_t slot = 0;
    int config = -1;
    std::size_t pattern = 0;
    std::size_t usedRows = 0;
    std::size_t usedColumns = 0;
    std::string higherFailureHistory;
    std::uint8_t claim = 0;
    std::uint8_t tokensBefore = 0;
    std::uint8_t tokensAfter = 0;
};

struct BorrowBudgetProbe
{
    std::size_t sa = 0;
    std::size_t slot = 0;
    std::string higherFailureHistory;
    std::uint8_t claim = 0;
    std::uint8_t tokensBefore = 0;
    std::size_t borrowCountBefore = 0;
};

struct Evaluation
{
    bool repairable = false;
    bool contextDecodeMissing = false;
    int failureSa = -1;
    std::array<int, 4> selectedSlots{{-1, -1, -1, -1}};
    std::array<std::size_t, 4> selectedPatterns{{0, 0, 0, 0}};
    std::vector<Step> accepted;
    std::vector<BorrowBudgetProbe> borrowBudgetProbes;
};

std::string contextKey(std::size_t sa, std::size_t slot,
                       const std::string &history)
{
    return std::string(1, saName(sa)) + ":" + roleName(slot) + ":" + history;
}

std::string appendFailure(const std::string &history, std::size_t slot,
                          char reason)
{
    return history.empty()
        ? std::string(roleName(slot)) + '=' + reason
        : history + ';' + roleName(slot) + '=' + reason;
}

using ClaimDecode = std::map<std::string, std::set<std::uint8_t>>;

Evaluation evaluateFirstClaim(const GroupRepairResult &source,
                              const std::array<std::size_t, 4> &order,
                              const ClaimDecode *contextDecode = nullptr)
{
    Evaluation result;
    std::uint8_t available = 0x0f;
    std::size_t borrowCount = 0;
    for (const std::size_t sa : order)
    {
        std::string history;
        bool accepted = false;
        for (const std::size_t slot : kPriority)
        {
            const CandidateRepairOption *candidate =
                firstPatternCandidate(source, sa, slot);
            if (candidate == nullptr)
            {
                history = appendFailure(history, slot, 'I');
                continue;
            }

            const std::string key = contextKey(sa, slot, history);
            std::uint8_t mask = claimMask(sa, *candidate);
            if (contextDecode != nullptr)
            {
                const auto found = contextDecode->find(key);
                if (found == contextDecode->end() || found->second.size() != 1)
                {
                    result.contextDecodeMissing = true;
                    result.failureSa = static_cast<int>(sa);
                    return result;
                }
                mask = *found->second.begin();
            }

            const bool borrow = (mask & borrowerToken(sa)) != 0;
            if (borrow && (available & mask) == mask)
            {
                result.borrowBudgetProbes.push_back(BorrowBudgetProbe{
                    sa, slot, history, mask, available, borrowCount});
            }
            const bool legal = (available & mask) == mask &&
                (!borrow || borrowCount < 1);
            if (!legal)
            {
                history = appendFailure(history, slot, 'X');
                continue;
            }

            const std::uint8_t before = available;
            available = static_cast<std::uint8_t>(available & ~mask);
            if (borrow)
                ++borrowCount;
            result.selectedSlots[sa] = static_cast<int>(slot);
            result.selectedPatterns[sa] = candidate->candidateIndex + 1;
            result.accepted.push_back(Step{
                sa, slot, configId(sa, slot), candidate->candidateIndex + 1,
                candidate->usedRows, candidate->usedColumns, history, mask,
                before, available});
            accepted = true;
            break;
        }
        if (!accepted)
        {
            result.failureSa = static_cast<int>(sa);
            return result;
        }
    }
    result.repairable = true;
    return result;
}

std::array<int, 4> productionSlots(const GroupRepairResult &source)
{
    std::array<int, 4> slots{{-1, -1, -1, -1}};
    for (const GroupRepairResult::V2DecisionTrace &trace : source.v2DecisionTrace)
    {
        if (trace.selected)
            slots.at(trace.subarray) = static_cast<int>(trace.roleSlot);
    }
    return slots;
}

std::array<std::size_t, 4> productionPatterns(const GroupRepairResult &source)
{
    std::array<std::size_t, 4> patterns{{0, 0, 0, 0}};
    for (const GroupRepairResult::V2DecisionTrace &trace : source.v2DecisionTrace)
    {
        if (trace.selected && trace.smallestPatternId.has_value())
            patterns.at(trace.subarray) = *trace.smallestPatternId;
    }
    return patterns;
}

bool matchesProduction(const GroupRepairResult &source,
                       const Evaluation &evaluation)
{
    return source.groupRepairSuccess == evaluation.repairable &&
           productionSlots(source) == evaluation.selectedSlots &&
           productionPatterns(source) == evaluation.selectedPatterns;
}

struct ContextWitness
{
    std::uint64_t faultLoad = 0;
    std::size_t group = 0;
    std::uint8_t mask = 0;
};

struct ContextBucket
{
    std::set<std::uint8_t> masks;
    std::vector<ContextWitness> witnesses;
};

using ContextBuckets = std::map<std::string, ContextBucket>;

struct BorrowBudgetWitness
{
    std::uint64_t faultLoad = 0;
    std::size_t group = 0;
    std::size_t borrowCount = 0;
};

struct BorrowBudgetBucket
{
    std::set<std::size_t> borrowCounts;
    std::vector<BorrowBudgetWitness> witnesses;
};

using BorrowBudgetBuckets = std::map<std::string, BorrowBudgetBucket>;

void recordContexts(ContextBuckets &buckets, const Evaluation &evaluation,
                    std::uint64_t faultLoad, std::size_t group)
{
    for (const Step &step : evaluation.accepted)
    {
        const std::string key = contextKey(
            step.sa, step.slot, step.higherFailureHistory);
        ContextBucket &bucket = buckets[key];
        bucket.masks.insert(step.claim);
        bucket.witnesses.push_back({faultLoad, group, step.claim});
    }
}

void recordBorrowBudgetProbes(BorrowBudgetBuckets &buckets,
                              const Evaluation &evaluation,
                              std::uint64_t faultLoad, std::size_t group)
{
    for (const BorrowBudgetProbe &probe : evaluation.borrowBudgetProbes)
    {
        const std::string key = contextKey(
            probe.sa, probe.slot, probe.higherFailureHistory) +
            ":tokens=" + std::to_string(probe.tokensBefore) +
            ":claim=" + std::to_string(probe.claim);
        BorrowBudgetBucket &bucket = buckets[key];
        bucket.borrowCounts.insert(probe.borrowCountBefore);
        bucket.witnesses.push_back(
            {faultLoad, group, probe.borrowCountBefore});
    }
}

ClaimDecode makeDecode(const ContextBuckets &buckets)
{
    ClaimDecode decode;
    for (const auto &entry : buckets)
        decode.emplace(entry.first, entry.second.masks);
    return decode;
}

std::string slotsText(const std::array<int, 4> &slots)
{
    return std::to_string(slots[0]) + ':' + std::to_string(slots[1]) + ':' +
        std::to_string(slots[2]) + ':' + std::to_string(slots[3]);
}

std::string traceText(const Evaluation &evaluation)
{
    std::ostringstream text;
    for (const Step &step : evaluation.accepted)
    {
        if (text.tellp() != std::streampos(0))
            text << '|';
        text << saName(step.sa) << ':' << roleName(step.slot)
             << ":pattern=" << step.pattern
             << ":demand=" << step.usedRows << 'R' << step.usedColumns << 'C'
             << ":claim=" << maskName(step.claim)
             << ":tokens=" << maskName(step.tokensBefore)
             << "->" << maskName(step.tokensAfter);
    }
    if (!evaluation.repairable)
        text << "|FAIL=" << (evaluation.failureSa < 0 ? '?' :
            saName(static_cast<std::size_t>(evaluation.failureSa)));
    return text.str();
}

struct PairCounts
{
    std::size_t bothPass = 0;
    std::size_t bothFail = 0;
    std::size_t abdcOnly = 0;
    std::size_t abcdOnly = 0;
};

void addPair(PairCounts &counts, const Evaluation &abcd,
             const Evaluation &abdc)
{
    if (abcd.repairable && abdc.repairable)
        ++counts.bothPass;
    else if (!abcd.repairable && !abdc.repairable)
        ++counts.bothFail;
    else if (abdc.repairable)
        ++counts.abdcOnly;
    else
        ++counts.abcdOnly;
}

const char *pairOutcome(const Evaluation &abcd, const Evaluation &abdc)
{
    if (abcd.repairable && abdc.repairable) return "BOTH_PASS";
    if (!abcd.repairable && !abdc.repairable) return "BOTH_FAIL";
    return abdc.repairable ? "ABDC_ONLY" : "ABCD_ONLY";
}

void writeManifest(std::ofstream &output, const char *phase,
                   std::size_t groupsPerPoint)
{
    output << "phase=" << phase << '\n'
           << "corpus=deterministic_dynamic_fault_generator_moderate_imbalance_mixed\n"
           << "seed=" << kSeed << '\n'
           << "rs=2\ncs=2\nm=1\ntopology=directional\n"
           << "role_priority=R,L,RB,B\nslot_order=1,0,3,2\n"
           << "f_group=16,20,24,28\n"
           << "groups_per_f_group=" << groupsPerPoint << '\n'
           << "token_reset=0x0f\n"
           << "token_order=C_COL,B_COL,D_ROW,A_ROW\n"
           << "token_transition=1_to_0_only\n";
}

void runStudy()
{
    const std::filesystem::path root = "tmp/date2026/final_early_c1";
    const std::filesystem::path claimRoot = root / "claim_mask_proof";
    const std::filesystem::path orderRoot = root / "order_ab";
    std::filesystem::create_directories(claimRoot);
    std::filesystem::create_directories(orderRoot);

    std::ofstream claimManifest(claimRoot / "manifest.txt");
    std::ofstream ambiguities(claimRoot / "ambiguities.csv");
    std::ofstream budgetAmbiguities(claimRoot / "borrow_budget_ambiguities.csv");
    std::ofstream replay(claimRoot / "known_mismatch_replay.csv");
    std::ofstream paired(orderRoot / "paired_results.csv");
    std::ofstream divergences(orderRoot / "divergence_cases.csv");
    std::ofstream summary(orderRoot / "summary.txt");
    std::ofstream orderManifest(orderRoot / "manifest.txt");
    require(claimManifest.good() && ambiguities.good() && budgetAmbiguities.good() && replay.good() &&
                paired.good() && divergences.good() && summary.good() &&
                orderManifest.good(),
            "Unable to create FINAL-EARLY-C1 tmp outputs");
    writeManifest(claimManifest, "FINAL-EARLY-C1A", kGroupsPerPoint);
    writeManifest(orderManifest, "FINAL-EARLY-C1B", kGroupsPerPoint);

    ambiguities << "key,claim_masks,first_f_group,first_group,second_f_group,second_group\n";
    budgetAmbiguities << "key,borrow_counts,first_f_group,first_group,second_f_group,second_group\n";
    replay << "vector,f_group,group,production_success,exact_model_match,"
              "context_model_resolved,context_model_match,selected_slots,context_slots\n";
    paired << "f_group,group,outcome,abcd_success,abdc_success,abcd_slots,abdc_slots,"
              "abcd_trace,abdc_trace\n";
    divergences << "f_group,group,outcome,abcd_trace,abdc_trace\n";
    summary << "f_group,total,both_pass,both_fail,abdc_only,abcd_only,"
               "abcd_repairable,abdc_repairable,abcd_rate_percent,abdc_rate_percent,delta_pp\n";

    DynamicRepairSimulator simulator;
    ContextBuckets contexts;
    BorrowBudgetBuckets borrowBudgetContexts;
    PairCounts aggregate;
    std::map<std::uint64_t, PairCounts> pointCounts;
    for (const std::uint64_t faultLoad : kFaultLoads)
    {
        const SimulationConfig config = configFor(faultLoad, kGroupsPerPoint);
        DynamicFaultGenerator generator(config);
        PairCounts counts;
        for (std::size_t group = 0; group < kGroupsPerPoint; ++group)
        {
            const FaultGroup faults = generator.generate(group);
            const GroupRepairResult production = simulator.run(
                faults, config, group, true);
            const Evaluation abcd = evaluateFirstClaim(production, kOrderAbcd);
            const Evaluation abdc = evaluateFirstClaim(production, kOrderAbdc);
            require(matchesProduction(production, abcd),
                    "First-claim ABCD oracle differs from production DirectionalV2Early");
            recordContexts(contexts, abcd, faultLoad, group);
            recordBorrowBudgetProbes(borrowBudgetContexts, abcd, faultLoad, group);
            addPair(counts, abcd, abdc);
            addPair(aggregate, abcd, abdc);
            paired << faultLoad << ',' << group << ',' << pairOutcome(abcd, abdc)
                   << ',' << abcd.repairable << ',' << abdc.repairable << ','
                   << '"' << slotsText(abcd.selectedSlots) << "\",\""
                   << slotsText(abdc.selectedSlots) << "\",\""
                   << traceText(abcd) << "\",\"" << traceText(abdc) << "\"\n";
            if (std::string(pairOutcome(abcd, abdc)) == "ABDC_ONLY" ||
                std::string(pairOutcome(abcd, abdc)) == "ABCD_ONLY")
            {
                divergences << faultLoad << ',' << group << ','
                            << pairOutcome(abcd, abdc) << ','
                            << '"' << traceText(abcd) << "\",\""
                            << traceText(abdc) << "\"\n";
            }
        }
        pointCounts.emplace(faultLoad, counts);
    }

    std::size_t ambiguityCount = 0;
    for (const auto &entry : contexts)
    {
        const ContextBucket &bucket = entry.second;
        if (bucket.masks.size() <= 1)
            continue;
        ++ambiguityCount;
        const ContextWitness &first = bucket.witnesses.front();
        const auto second = std::find_if(
            bucket.witnesses.begin(), bucket.witnesses.end(),
            [&first](const ContextWitness &witness)
            {
                return witness.mask != first.mask;
            });
        std::string masks;
        for (const std::uint8_t mask : bucket.masks)
        {
            if (!masks.empty()) masks += '|';
            masks += maskName(mask);
        }
        ambiguities << entry.first << ',' << '"' << masks << '"' << ','
                    << first.faultLoad << ',' << first.group << ','
                    << second->faultLoad << ',' << second->group << '\n';
    }
    const ClaimDecode decode = makeDecode(contexts);

    std::size_t borrowBudgetAmbiguityCount = 0;
    for (const auto &entry : borrowBudgetContexts)
    {
        const BorrowBudgetBucket &bucket = entry.second;
        if (bucket.borrowCounts.size() <= 1)
            continue;
        ++borrowBudgetAmbiguityCount;
        const BorrowBudgetWitness &first = bucket.witnesses.front();
        const auto second = std::find_if(
            bucket.witnesses.begin(), bucket.witnesses.end(),
            [&first](const BorrowBudgetWitness &witness)
            {
                return witness.borrowCount != first.borrowCount;
            });
        std::string counts;
        for (const std::size_t count : bucket.borrowCounts)
        {
            if (!counts.empty()) counts += '|';
            counts += std::to_string(count);
        }
        budgetAmbiguities << entry.first << ',' << '"' << counts << '"' << ','
                          << first.faultLoad << ',' << first.group << ','
                          << second->faultLoad << ',' << second->group << '\n';
    }

    std::size_t replayResolved = 0;
    for (const std::size_t vector : kKnownMismatchVectors)
    {
        const std::size_t point = vector / kKnownReplayGroupsPerPoint;
        const std::size_t group = vector % kKnownReplayGroupsPerPoint;
        const std::uint64_t faultLoad = kFaultLoads.at(point);
        const SimulationConfig config = configFor(
            faultLoad, kKnownReplayGroupsPerPoint);
        DynamicFaultGenerator generator(config);
        const FaultGroup faults = generator.generate(group);
        const GroupRepairResult production = simulator.run(
            faults, config, group, true);
        const Evaluation exact = evaluateFirstClaim(production, kOrderAbcd);
        const Evaluation contextual = evaluateFirstClaim(
            production, kOrderAbcd, &decode);
        const bool exactMatch = matchesProduction(production, exact);
        const bool contextualResolved = !contextual.contextDecodeMissing;
        const bool contextualMatch = contextualResolved &&
            matchesProduction(production, contextual);
        if (contextualMatch)
            ++replayResolved;
        replay << vector << ',' << faultLoad << ',' << group << ','
               << production.groupRepairSuccess << ',' << exactMatch << ','
               << contextualResolved << ',' << contextualMatch << ','
               << '"' << slotsText(productionSlots(production)) << "\",\""
               << slotsText(contextual.selectedSlots) << "\"\n";
        require(exactMatch, "Exact-demand first-claim replay differs from production");
    }

    for (const std::uint64_t faultLoad : kFaultLoads)
    {
        const PairCounts &counts = pointCounts.at(faultLoad);
        require(counts.bothPass + counts.bothFail + counts.abdcOnly +
                    counts.abcdOnly == kGroupsPerPoint,
                "C1B paired accounting does not conserve groups");
        const std::size_t abcdRepairable = counts.bothPass + counts.abcdOnly;
        const std::size_t abdcRepairable = counts.bothPass + counts.abdcOnly;
        const double abcdRate = 100.0 * abcdRepairable / kGroupsPerPoint;
        const double abdcRate = 100.0 * abdcRepairable / kGroupsPerPoint;
        summary << faultLoad << ',' << kGroupsPerPoint << ',' << counts.bothPass
                << ',' << counts.bothFail << ',' << counts.abdcOnly << ','
                << counts.abcdOnly << ',' << abcdRepairable << ','
                << abdcRepairable << ',' << std::fixed << std::setprecision(6)
                << abcdRate << ',' << abdcRate << ',' << abdcRate - abcdRate << '\n';
    }
    const std::size_t total = aggregate.bothPass + aggregate.bothFail +
        aggregate.abdcOnly + aggregate.abcdOnly;
    require(total == kFaultLoads.size() * kGroupsPerPoint,
            "C1B aggregate accounting does not conserve groups");
    const std::size_t abcdRepairable = aggregate.bothPass + aggregate.abcdOnly;
    const std::size_t abdcRepairable = aggregate.bothPass + aggregate.abdcOnly;
    const double abcdRate = 100.0 * abcdRepairable / total;
    const double abdcRate = 100.0 * abdcRepairable / total;
    summary << "aggregate," << total << ',' << aggregate.bothPass << ','
            << aggregate.bothFail << ',' << aggregate.abdcOnly << ','
            << aggregate.abcdOnly << ',' << abcdRepairable << ','
            << abdcRepairable << ',' << std::fixed << std::setprecision(6)
            << abcdRate << ',' << abdcRate << ',' << abdcRate - abcdRate << '\n';

    std::cout << "FINAL_EARLY_C1A_CONTEXTS=" << contexts.size()
              << " AMBIGUITIES=" << ambiguityCount
              << " BORROW_BUDGET_AMBIGUITIES=" << borrowBudgetAmbiguityCount
              << " KNOWN_19_RESOLVED=" << replayResolved << "/19\n";
    std::cout << "FINAL_EARLY_C1B_TOTAL=" << total
              << " BOTH_PASS=" << aggregate.bothPass
              << " BOTH_FAIL=" << aggregate.bothFail
              << " ABDC_ONLY=" << aggregate.abdcOnly
              << " ABCD_ONLY=" << aggregate.abcdOnly
              << " ABCD_RATE=" << std::fixed << std::setprecision(6) << abcdRate
              << " ABDC_RATE=" << abdcRate
              << " DELTA_PP=" << abdcRate - abcdRate << '\n';
}

} // namespace

int main()
{
    try
    {
        runStudy();
    }
    catch (const std::exception &error)
    {
        std::cerr << "FINAL_EARLY_C1 STUDY FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
