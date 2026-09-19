#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace
{
constexpr int kSubarrays = 4;
constexpr int kActions = 4;
constexpr int kPatterns = 10;
constexpr int kCandidates = kSubarrays * kActions * kPatterns;
constexpr std::array<int, kActions> kSearchOrder{{1, 0, 3, 2}};
constexpr std::array<int, kSubarrays> kReleaseResource{{0, 2, 3, 1}};
constexpr std::array<int, kSubarrays> kBorrowResource{{2, 1, 0, 3}};
constexpr std::array<int, kSubarrays> kDonorOwnerByDepth{{1, 3, 0, 2}};

struct CandidateMap
{
    std::array<bool, kCandidates> valid{};
    std::array<bool, kCandidates> release{};
    std::array<bool, kCandidates> borrow{};
};

struct State
{
    int released = 0;
    int used = 0;
    int obligation = 0;
    int borrowCount = 0;
};

struct Result
{
    bool repairable = false;
    std::array<int, kSubarrays> action{{-1, -1, -1, -1}};
    std::array<int, kSubarrays> pattern{{0, 0, 0, 0}};
    std::array<int, kSubarrays> config{{-1, -1, -1, -1}};
    std::array<int, kSubarrays> donor{{-1, -1, -1, -1}};
    std::array<bool, kSubarrays> release{};
    std::array<bool, kSubarrays> borrow{};
    std::array<int, kSubarrays> obligationAfter{{-1, -1, -1, -1}};
    State finalState{};
    std::size_t candidateVisits = 0;
    std::size_t dominancePrunes = 0;
};

struct VisitStatistics
{
    std::uint64_t aggregate = 0;
    double mean = 0.0;
    std::size_t median = 0;
    std::size_t p95 = 0;
    std::size_t p99 = 0;
    std::size_t maximum = 0;
};

enum class SearchMode
{
    Exhaustive,
    ExplicitEffectCollapse,
    EffectOnlyCollapse,
    DominanceCache,
    Combined
};

int candidateIndex(int subarray, int action, int patternZero)
{
    return subarray * 40 + action * 10 + patternZero;
}

void addCandidate(CandidateMap &map, int subarray, int action, int pattern,
                  bool release, bool borrow)
{
    const int index = candidateIndex(subarray, action, pattern - 1);
    map.valid.at(index) = true;
    map.release.at(index) = release;
    map.borrow.at(index) = borrow;
}

bool explicitRelease(int action)
{
    return action == 1 || action == 3;
}

int configId(int depth, int action)
{
    if (depth == 0 || depth == 3)
    {
        constexpr std::array<int, kActions> kOuterConfigByAction{{0, 4, 5, 6}};
        return kOuterConfigByAction.at(action);
    }
    return action;
}

bool sameState(const State &left, const State &right)
{
    return left.released == right.released && left.used == right.used &&
        left.obligation == right.obligation && left.borrowCount == right.borrowCount;
}

bool dominates(const State &better, const State &worse)
{
    return (better.released | worse.released) == better.released &&
        (better.used | worse.used) == worse.used &&
        (better.obligation | worse.obligation) == worse.obligation &&
        better.borrowCount <= worse.borrowCount;
}

bool transition(const CandidateMap &map, int depth, int action, int pattern,
                const State &before, State &after)
{
    const int index = candidateIndex(depth, action, pattern - 1);
    if (!map.valid.at(index)) return false;
    const int releaseBit = 1 << kReleaseResource.at(depth);
    const int donorBit = 1 << kBorrowResource.at(depth);
    const bool release = map.release.at(index);
    const bool borrow = map.borrow.at(index);
    const bool explicit_release = explicitRelease(action);
    if ((before.obligation & releaseBit) && !(explicit_release && release)) return false;
    after = before;
    if (release)
    {
        after.released |= releaseBit;
        if (explicit_release) after.obligation &= ~releaseBit;
    }
    if (!borrow) return true;
    if (before.borrowCount == 3 || (before.used & donorBit)) return false;
    if (!(before.released & donorBit) && kDonorOwnerByDepth.at(depth) <= depth)
        return false;
    after.used |= donorBit;
    ++after.borrowCount;
    if (!(before.released & donorBit)) after.obligation |= donorBit;
    return true;
}

int collapseKey(int action, bool release, bool borrow, SearchMode mode)
{
    if (mode == SearchMode::EffectOnlyCollapse)
        return (release ? 2 : 0) | (borrow ? 1 : 0);
    return (explicitRelease(action) ? 4 : 0) | (release ? 2 : 0) |
        (borrow ? 1 : 0);
}

class Search
{
public:
    Search(const CandidateMap &map, SearchMode mode) : map_(map), mode_(mode) {}

    Result run()
    {
        Result result;
        dfs(0, State{}, result);
        return result;
    }

private:
    bool dfs(int depth, const State &state, Result &result)
    {
        if (mode_ == SearchMode::DominanceCache || mode_ == SearchMode::Combined)
        {
            for (const State &failed : failedStates_.at(depth))
            {
                if (dominates(failed, state))
                {
                    ++result.dominancePrunes;
                    return false;
                }
            }
        }

        std::array<bool, 8> represented{};
        for (const int action : kSearchOrder)
        {
            for (int pattern = 1; pattern <= kPatterns; ++pattern)
            {
                const int index = candidateIndex(depth, action, pattern - 1);
                if (!map_.valid.at(index)) continue;
                if (mode_ == SearchMode::ExplicitEffectCollapse ||
                    mode_ == SearchMode::EffectOnlyCollapse || mode_ == SearchMode::Combined)
                {
                    const SearchMode collapse_mode = mode_ == SearchMode::Combined
                        ? SearchMode::ExplicitEffectCollapse : mode_;
                    const int key = collapseKey(action, map_.release.at(index),
                                                map_.borrow.at(index), collapse_mode);
                    if (represented.at(key)) continue;
                    represented.at(key) = true;
                }
                ++result.candidateVisits;
                State next;
                if (!transition(map_, depth, action, pattern, state, next)) continue;
                result.action.at(depth) = action;
                result.pattern.at(depth) = pattern;
                result.config.at(depth) = configId(depth, action);
                result.donor.at(depth) = kBorrowResource.at(depth);
                result.release.at(depth) = map_.release.at(index);
                result.borrow.at(depth) = map_.borrow.at(index);
                result.obligationAfter.at(depth) = next.obligation;
                if (depth == kSubarrays - 1)
                {
                    if (next.obligation == 0)
                    {
                        result.repairable = true;
                        result.finalState = next;
                        return true;
                    }
                }
                else if (dfs(depth + 1, next, result))
                {
                    return true;
                }
            }
        }
        if (mode_ == SearchMode::DominanceCache || mode_ == SearchMode::Combined)
            failedStates_.at(depth).push_back(state);
        return false;
    }

    const CandidateMap &map_;
    SearchMode mode_;
    std::array<std::vector<State>, kSubarrays> failedStates_{};
};

bool sameResult(const Result &left, const Result &right)
{
    if (left.repairable != right.repairable) return false;
    if (!left.repairable) return true;
    return left.action == right.action && left.pattern == right.pattern &&
        left.config == right.config && left.donor == right.donor &&
        left.release == right.release && left.borrow == right.borrow &&
        left.obligationAfter == right.obligationAfter &&
        sameState(left.finalState, right.finalState);
}

CandidateMap group172()
{
    CandidateMap map;
    addCandidate(map, 0, 3, 2, true, true);
    addCandidate(map, 1, 1, 1, true, false);
    addCandidate(map, 2, 2, 3, false, true);
    addCandidate(map, 3, 1, 1, true, false);
    return map;
}

CandidateMap effectOnlyCounterexample()
{
    CandidateMap map;
    addCandidate(map, 0, 2, 1, false, true);
    addCandidate(map, 1, 0, 1, true, false);
    addCandidate(map, 1, 3, 1, true, false);
    addCandidate(map, 2, 1, 1, true, false);
    addCandidate(map, 3, 1, 1, true, false);
    return map;
}

CandidateMap randomMap(std::mt19937_64 &random)
{
    CandidateMap map;
    for (int subarray = 0; subarray < kSubarrays; ++subarray)
    {
        for (int action = 0; action < kActions; ++action)
        {
            for (int pattern = 1; pattern <= kPatterns; ++pattern)
            {
                if ((random() % 100) >= 12) continue;
                const bool release = (random() & 1U) != 0U;
                const bool borrow = (random() & 1U) != 0U;
                addCandidate(map, subarray, action, pattern, release, borrow);
            }
        }
    }
    return map;
}

std::size_t percentile(std::vector<std::size_t> values, unsigned numerator,
                       unsigned denominator)
{
    std::sort(values.begin(), values.end());
    const std::size_t index = (values.size() - 1) * numerator / denominator;
    return values.at(index);
}

void require(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}
} // namespace

int main()
{
    try
    {
        const Result groupBaseline = Search(group172(), SearchMode::Exhaustive).run();
        const Result groupCollapsed = Search(group172(), SearchMode::ExplicitEffectCollapse).run();
        const Result groupDominance = Search(group172(), SearchMode::DominanceCache).run();
        require(groupBaseline.repairable && groupBaseline.action == std::array<int, 4>{{3, 1, 2, 1}} &&
                    groupBaseline.pattern == std::array<int, 4>{{2, 1, 3, 1}},
                "N2/F16/group172 canonical tuple changed in the audit model");
        require(groupBaseline.obligationAfter == std::array<int, 4>{{4, 0, 0, 0}},
                "N2/F16/group172 future-donor obligation trace changed in the audit model");
        require(sameResult(groupBaseline, groupCollapsed) && sameResult(groupBaseline, groupDominance),
                "safe audit mode changed the canonical group172 tuple");

        const CandidateMap counterexample = effectOnlyCounterexample();
        const Result effectBaseline = Search(counterexample, SearchMode::Exhaustive).run();
        const Result effectOnly = Search(counterexample, SearchMode::EffectOnlyCollapse).run();
        require(effectBaseline.repairable && !effectOnly.repairable,
                "effect-only collapse counterexample was not observed");

        std::mt19937_64 random(20260918);
        std::size_t collapseMismatch = 0;
        std::size_t dominanceMismatch = 0;
        std::size_t combinedMismatch = 0;
        std::size_t dominancePrunes = 0;
        std::vector<std::size_t> exhaustiveVisits;
        std::vector<std::size_t> collapsedVisits;
        std::vector<std::size_t> dominanceVisits;
        std::vector<std::size_t> combinedVisits;
        for (int vector = 0; vector < 1000; ++vector)
        {
            const CandidateMap map = randomMap(random);
            const Result baseline = Search(map, SearchMode::Exhaustive).run();
            const Result collapsed = Search(map, SearchMode::ExplicitEffectCollapse).run();
            const Result dominance = Search(map, SearchMode::DominanceCache).run();
            const Result combined = Search(map, SearchMode::Combined).run();
            collapseMismatch += !sameResult(baseline, collapsed);
            dominanceMismatch += !sameResult(baseline, dominance);
            combinedMismatch += !sameResult(baseline, combined);
            dominancePrunes += dominance.dominancePrunes;
            exhaustiveVisits.push_back(baseline.candidateVisits);
            collapsedVisits.push_back(collapsed.candidateVisits);
            dominanceVisits.push_back(dominance.candidateVisits);
            combinedVisits.push_back(combined.candidateVisits);
        }
        require(collapseMismatch == 0 && dominanceMismatch == 0 && combinedMismatch == 0,
                "a safe audit mode changed repairability or canonical tuple");
        const auto summary = [](const std::vector<std::size_t> &values)
        {
            VisitStatistics statistics;
            for (const std::size_t value : values) statistics.aggregate += value;
            statistics.mean = static_cast<double>(statistics.aggregate) / values.size();
            statistics.median = percentile(values, 50, 100);
            statistics.p95 = percentile(values, 95, 100);
            statistics.p99 = percentile(values, 99, 100);
            statistics.maximum = *std::max_element(values.begin(), values.end());
            return statistics;
        };
        const auto exhaustive = summary(exhaustiveVisits);
        const auto collapsed = summary(collapsedVisits);
        const auto dominance = summary(dominanceVisits);
        const auto combined = summary(combinedVisits);
        std::cout << "P2DOM_DIRECTED_GROUP172=PASS\n"
                  << "P2DOM_EFFECT_ONLY_COUNTEREXAMPLE=PASS\n"
                  << "P2DOM_RANDOM_VECTORS=1000\n"
                  << "P2DOM_COLLAPSE_MISMATCHES=" << collapseMismatch << '\n'
                  << "P2DOM_DOMINANCE_MISMATCHES=" << dominanceMismatch << '\n'
                  << "P2DOM_COMBINED_MISMATCHES=" << combinedMismatch << '\n'
                  << "P2DOM_DOMINANCE_PRUNES=" << dominancePrunes << '\n'
                  << "P2DOM_EXHAUSTIVE_AVG_MEDIAN_P95_P99=" << static_cast<std::size_t>(exhaustive.mean) << ',' << exhaustive.median
                  << ',' << exhaustive.p95 << ',' << exhaustive.p99 << '\n'
                  << "P2DOM_COLLAPSE_AVG_MEDIAN_P95_P99=" << static_cast<std::size_t>(collapsed.mean) << ',' << collapsed.median
                  << ',' << collapsed.p95 << ',' << collapsed.p99 << '\n'
                  << "P2DOM_DOMINANCE_AVG_MEDIAN_P95_P99=" << static_cast<std::size_t>(dominance.mean) << ',' << dominance.median
                  << ',' << dominance.p95 << ',' << dominance.p99 << '\n'
                  << "P2DOM_COMBINED_AVG_MEDIAN_P95_P99=" << static_cast<std::size_t>(combined.mean) << ',' << combined.median
                  << ',' << combined.p95 << ',' << combined.p99 << '\n'
                  << "P2DOM_EXHAUSTIVE_VISIT_STATS_AGGREGATE_MEAN_MEDIAN_P95_P99_MAX="
                  << exhaustive.aggregate << ',' << exhaustive.mean << ',' << exhaustive.median << ','
                  << exhaustive.p95 << ',' << exhaustive.p99 << ',' << exhaustive.maximum << '\n'
                  << "P2DOM_COLLAPSE_VISIT_STATS_AGGREGATE_MEAN_MEDIAN_P95_P99_MAX="
                  << collapsed.aggregate << ',' << collapsed.mean << ',' << collapsed.median << ','
                  << collapsed.p95 << ',' << collapsed.p99 << ',' << collapsed.maximum << '\n';
    }
    catch (const std::exception &error)
    {
        std::cerr << "P2DOM_FAIL: " << error.what() << '\n';
        return 1;
    }
}
