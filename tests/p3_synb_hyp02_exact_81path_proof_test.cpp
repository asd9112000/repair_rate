#include "DynamicRepairSimulator.hpp"
#include "SimulationConfig.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using namespace dynamic_spare;

constexpr std::size_t kSubarrays = 4;
constexpr std::size_t kSlots = 4;
constexpr std::size_t kRawTuples = 256;
constexpr std::size_t kStaticPaths = 81;

struct SlotSemantics
{
    int configId;
    int rows;
    int columns;
    bool release;
    bool borrow;
};

struct StaticPath
{
    std::array<unsigned int, kSubarrays> slots;
};

constexpr std::array<std::array<SlotSemantics, kSlots>, kSubarrays>
    kSlotSemantics{{
        {{{0, 2, 2, false, false}, {4, 1, 2, true, false},
          {5, 2, 3, false, true}, {6, 1, 3, true, true}}},
        {{{0, 2, 2, false, false}, {1, 2, 1, true, false},
          {2, 3, 2, false, true}, {3, 3, 1, true, true}}},
        {{{0, 2, 2, false, false}, {1, 2, 1, true, false},
          {2, 3, 2, false, true}, {3, 3, 1, true, true}}},
        {{{0, 2, 2, false, false}, {4, 1, 2, true, false},
          {5, 2, 3, false, true}, {6, 1, 3, true, true}}},
    }};

void require(bool condition, const std::string &message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::array<unsigned int, kSubarrays> slotsForTuple(unsigned int tuple)
{
    return {{tuple & 0x3U, (tuple >> 2U) & 0x3U,
             (tuple >> 4U) & 0x3U, (tuple >> 6U) & 0x3U}};
}

bool fixedEdgeLegal(const std::array<unsigned int, kSubarrays> &slots)
{
    const SlotSemantics &a = kSlotSemantics[0][slots[0]];
    const SlotSemantics &b = kSlotSemantics[1][slots[1]];
    const SlotSemantics &c = kSlotSemantics[2][slots[2]];
    const SlotSemantics &d = kSlotSemantics[3][slots[3]];
    return (!c.borrow || a.release) && (!b.borrow || d.release) &&
           (!a.borrow || b.release) && (!d.borrow || c.release);
}

std::array<StaticPath, kStaticPaths> buildStaticPaths()
{
    std::array<StaticPath, kStaticPaths> paths{};
    std::size_t next = 0;
    for (unsigned int tuple = 0; tuple < kRawTuples; ++tuple)
    {
        const auto slots = slotsForTuple(tuple);
        if (!fixedEdgeLegal(slots))
            continue;
        require(next < paths.size(), "fixed-edge path count exceeds 81");
        paths[next++] = {slots};
    }
    require(next == paths.size(), "fixed-edge tuple enumeration is not 81");
    return paths;
}

bool slotValid(const std::array<unsigned int, kSubarrays> &validMap,
               std::size_t subarray, unsigned int slot)
{
    return (validMap[subarray] & (1U << slot)) != 0U;
}

bool referenceRepairable(const std::array<unsigned int, kSubarrays> &validMap)
{
    for (unsigned int tuple = 0; tuple < kRawTuples; ++tuple)
    {
        const auto slots = slotsForTuple(tuple);
        if (!fixedEdgeLegal(slots))
            continue;
        bool allValid = true;
        for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray)
            allValid = allValid && slotValid(validMap, subarray, slots[subarray]);
        if (allValid)
            return true;
    }
    return false;
}

bool staticRepairable(const std::array<unsigned int, kSubarrays> &validMap,
                      const std::array<StaticPath, kStaticPaths> &paths)
{
    for (const StaticPath &path : paths)
    {
        bool allValid = true;
        for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray)
            allValid = allValid &&
                slotValid(validMap, subarray, path.slots[subarray]);
        if (allValid)
            return true;
    }
    return false;
}

SimulationConfig canonicalDirectionalConfig()
{
    SimulationConfig config;
    config.spareRows = 2;
    config.spareColumns = 2;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    config.layout = GroupLayout::Grid2x2;
    config.topology = SharingTopology::Directional;
    config.solutionTakePolicy =
        SolutionTakePolicy::DirectionalV2GroupGlobalCanonical;
    config.usePaperCamReuseCapacity = true;
    return config;
}

Fault makeFault(int subarray, int row, int column)
{
    Fault fault{};
    fault.HBMID = 0;
    fault.ChannelID = 0;
    fault.BankID = 51;
    fault.SubarrayGroupID = 0;
    fault.SubarrayID = subarray;
    fault.r = row;
    fault.c = column;
    return fault;
}

FaultGroup knownGlobalWitness()
{
    FaultGroup group;
    group[0] = {makeFault(0, 161, 944), makeFault(0, 1015, 944),
                makeFault(0, 117, 822), makeFault(0, 161, 943),
                makeFault(0, 1015, 1000), makeFault(0, 636, 100),
                makeFault(0, 726, 944)};
    group[1] = {makeFault(1, 580, 175), makeFault(1, 136, 430),
                makeFault(1, 112, 263), makeFault(1, 7, 706),
                makeFault(1, 580, 63)};
    group[2] = {makeFault(2, 21, 191)};
    group[3] = {makeFault(3, 237, 213), makeFault(3, 235, 214),
                makeFault(3, 1003, 130)};
    return group;
}

FaultGroup p1CounterexampleGroup()
{
    FaultGroup group;
    group[0] = {makeFault(0, 0, 0), makeFault(0, 0, 1),
                makeFault(0, 1, 1), makeFault(0, 1, 2),
                makeFault(0, 2, 0)};
    return group;
}

FaultGroup randomGroup(std::uint64_t seed)
{
    FaultGroup group;
    for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray)
    {
        const std::size_t count = 1 + ((seed >> (subarray * 4U)) & 0x7U);
        for (std::size_t index = 0; index < count; ++index)
        {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            const int row = static_cast<int>((seed >> 20U) % 1024U);
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            const int column = static_cast<int>((seed >> 20U) % 1024U);
            group[subarray].push_back(
                makeFault(static_cast<int>(subarray), row, column));
        }
    }
    return group;
}

std::array<unsigned int, kSubarrays> validityMap(
    const GroupRepairResult &result)
{
    std::array<unsigned int, kSubarrays> validMap{};
    for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray)
    {
        require(result.attemptsBySubarray[subarray].size() == kSlots,
                "canonical V2 run did not produce exactly four slots");
        for (std::size_t slot = 0; slot < kSlots; ++slot)
        {
            const RepairAttemptResult &attempt =
                result.attemptsBySubarray[subarray][slot];
            if (attempt.repairSuccess && attempt.tileSolutionState.has_value() &&
                !attempt.validCandidateOptions.empty())
            {
                validMap[subarray] |= 1U << static_cast<unsigned int>(slot);
            }
        }
    }
    return validMap;
}

std::string formatBits(const std::array<unsigned int, kSubarrays> &validMap)
{
    std::ostringstream output;
    for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray)
    {
        if (subarray != 0)
            output << '/';
        for (int slot = static_cast<int>(kSlots) - 1; slot >= 0; --slot)
            output << ((validMap[subarray] >> static_cast<unsigned int>(slot)) & 1U);
    }
    return output.str();
}

std::string selectedTuple(const GroupRepairResult &result)
{
    std::ostringstream output;
    for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray)
    {
        if (subarray != 0)
            output << ',';
        if (result.selectedAttemptIndices[subarray].has_value())
            output << *result.selectedAttemptIndices[subarray];
        else
            output << '-';
    }
    return output.str();
}

struct Walkthrough
{
    std::size_t pathId = 0;
    std::array<unsigned int, kSubarrays> slots{};
    std::array<unsigned int, kSubarrays> patternIds{};
    std::array<unsigned int, kSubarrays> validMap{};
};

Walkthrough buildWalkthrough(const std::array<StaticPath, kStaticPaths> &paths)
{
    DynamicRepairSimulator simulator;
    const GroupRepairResult result = simulator.run(
        knownGlobalWitness(), canonicalDirectionalConfig(), 9001, false);
    Walkthrough walkthrough;
    walkthrough.validMap = validityMap(result);
    for (; walkthrough.pathId < paths.size(); ++walkthrough.pathId)
    {
        const StaticPath &path = paths[walkthrough.pathId];
        bool allValid = true;
        for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray)
            allValid = allValid &&
                slotValid(walkthrough.validMap, subarray, path.slots[subarray]);
        if (allValid)
        {
            walkthrough.slots = path.slots;
            break;
        }
    }
    require(walkthrough.pathId < paths.size(),
            "known GLOBAL witness has no static path");
    for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray)
    {
        const RepairAttemptResult &attempt =
            result.attemptsBySubarray[subarray][walkthrough.slots[subarray]];
        require(!attempt.validCandidateOptions.empty(),
                "walkthrough slot has no representative PatternID");
        walkthrough.patternIds[subarray] = static_cast<unsigned int>(
            attempt.validCandidateOptions.front().candidateIndex + 1U);
        require(walkthrough.patternIds[subarray] <= 15U,
                "walkthrough PatternID exceeds the four-bit store contract");
    }
    return walkthrough;
}

struct CorpusSummary
{
    std::size_t cases = 0;
    std::size_t staticReferenceMismatches = 0;
    std::size_t cppMismatches = 0;
    bool sawLocal = false;
    bool sawRowBorrow = false;
    bool sawColumnBorrow = false;
    bool sawSimultaneousBorrow = false;
    bool sawReleaseOnly = false;
    bool sawFourEdgeSharing = false;
    std::string firstCppMismatch;
};

void checkCorpusCase(const std::string &name, const FaultGroup &faults,
                     std::uint64_t runIndex,
                     const std::array<StaticPath, kStaticPaths> &paths,
                     CorpusSummary &summary)
{
    DynamicRepairSimulator simulator;
    const GroupRepairResult result = simulator.run(
        faults, canonicalDirectionalConfig(), runIndex, false);
    const auto validMap = validityMap(result);
    const bool reference = referenceRepairable(validMap);
    const bool staticResult = staticRepairable(validMap, paths);
    const bool cppResult = result.groupRepairSuccess;
    for (const StaticPath &path : paths)
    {
        bool allValid = true;
        unsigned int borrowCount = 0;
        bool releaseOnly = false;
        for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray)
        {
            allValid = allValid &&
                slotValid(validMap, subarray, path.slots[subarray]);
            const SlotSemantics &slot =
                kSlotSemantics[subarray][path.slots[subarray]];
            borrowCount += slot.borrow ? 1U : 0U;
            releaseOnly = releaseOnly || (slot.release && !slot.borrow);
        }
        if (!allValid)
            continue;
        summary.sawLocal = summary.sawLocal || path.slots ==
            std::array<unsigned int, kSubarrays>{{0, 0, 0, 0}};
        summary.sawRowBorrow = summary.sawRowBorrow ||
            kSlotSemantics[1][path.slots[1]].borrow ||
            kSlotSemantics[2][path.slots[2]].borrow;
        summary.sawColumnBorrow = summary.sawColumnBorrow ||
            kSlotSemantics[0][path.slots[0]].borrow ||
            kSlotSemantics[3][path.slots[3]].borrow;
        summary.sawSimultaneousBorrow = summary.sawSimultaneousBorrow ||
            borrowCount >= 2U;
        summary.sawReleaseOnly = summary.sawReleaseOnly || releaseOnly;
        summary.sawFourEdgeSharing = summary.sawFourEdgeSharing ||
            path.slots == std::array<unsigned int, kSubarrays>{{3, 3, 3, 3}};
    }
    ++summary.cases;
    if (reference != staticResult)
        ++summary.staticReferenceMismatches;
    if (staticResult != cppResult)
    {
        ++summary.cppMismatches;
        if (summary.firstCppMismatch.empty())
        {
            std::ostringstream output;
            output << "case=" << name << " valid_bits=" << formatBits(validMap)
                   << " static=" << staticResult << " cpp=" << cppResult
                   << " cpp_selected_slots=" << selectedTuple(result);
            summary.firstCppMismatch = output.str();
        }
    }
}

CorpusSummary checkAnalyzerCorpus(
    const std::array<StaticPath, kStaticPaths> &paths)
{
    CorpusSummary summary;
    checkCorpusCase("normal_local", FaultGroup{}, 1, paths, summary);
    checkCorpusCase("p1_counterexample", p1CounterexampleGroup(), 2, paths,
                    summary);
    checkCorpusCase("global_witness", knownGlobalWitness(), 3, paths, summary);
    for (std::uint64_t seed = 1; seed <= 512; ++seed)
        checkCorpusCase("random_" + std::to_string(seed), randomGroup(seed),
                        1000 + seed, paths, summary);
    return summary;
}

} // namespace

int main()
{
    try
    {
        const auto paths = buildStaticPaths();
        std::size_t duplicatePaths = 0;
        for (std::size_t left = 0; left < paths.size(); ++left)
            for (std::size_t right = left + 1; right < paths.size(); ++right)
                if (paths[left].slots == paths[right].slots)
                    ++duplicatePaths;
        require(duplicatePaths == 0, "static path table contains duplicates");

        std::size_t validityMismatches = 0;
        for (unsigned int packed = 0; packed < (1U << 16U); ++packed)
        {
            const std::array<unsigned int, kSubarrays> validMap{{
                packed & 0xfU, (packed >> 4U) & 0xfU,
                (packed >> 8U) & 0xfU, (packed >> 12U) & 0xfU}};
            if (referenceRepairable(validMap) != staticRepairable(validMap, paths))
                ++validityMismatches;
        }
        require(validityMismatches == 0,
                "81-path Boolean engine differs from 256-tuple reference");

        const CorpusSummary corpus = checkAnalyzerCorpus(paths);
        require(corpus.staticReferenceMismatches == 0,
                "analyzer corpus differs from 256-tuple reference");
        require(corpus.cppMismatches == 0,
                "C++ GLOBAL differs from static 81-path model: " +
                    corpus.firstCppMismatch);
        require(corpus.sawLocal && corpus.sawRowBorrow && corpus.sawColumnBorrow &&
                    corpus.sawSimultaneousBorrow && corpus.sawReleaseOnly &&
                    corpus.sawFourEdgeSharing,
                "analyzer corpus did not cover every required sharing category");
        const Walkthrough walkthrough = buildWalkthrough(paths);

        std::cout << "RAW_CONFIG_TUPLES: 256\n"
                  << "LEGAL_STATIC_PATHS: 81\n"
                  << "DUPLICATE_PATHS: " << duplicatePaths << "\n"
                  << "MISSING_LEGAL_PATHS: 0\n"
                  << "ILLEGAL_PATHS_INCLUDED: 0\n"
                  << "VALIDITY_MAPS_CHECKED: 65536\n"
                  << "REFERENCE_STATIC_REPAIRABILITY_MISMATCHES: "
                  << validityMismatches << "\n"
                  << "ANALYZER_CORPUS_CASES: " << corpus.cases << "\n"
                  << "ANALYZER_REFERENCE_MISMATCHES: "
                  << corpus.staticReferenceMismatches << "\n"
                  << "CPP_GLOBAL_REPAIRABILITY_MISMATCHES: "
                  << corpus.cppMismatches << "\n"
                  << "ANALYZER_CORPUS_LOCAL_CASE: " << corpus.sawLocal << "\n"
                  << "ANALYZER_CORPUS_ROW_BORROW_CASE: " << corpus.sawRowBorrow << "\n"
                  << "ANALYZER_CORPUS_COLUMN_BORROW_CASE: "
                  << corpus.sawColumnBorrow << "\n"
                  << "ANALYZER_CORPUS_SIMULTANEOUS_BORROW_CASE: "
                  << corpus.sawSimultaneousBorrow << "\n"
                  << "ANALYZER_CORPUS_RELEASE_ONLY_CASE: "
                  << corpus.sawReleaseOnly << "\n"
                  << "ANALYZER_CORPUS_FOUR_EDGE_CASE: "
                  << corpus.sawFourEdgeSharing << "\n"
                  << "WALKTHROUGH_VALID_BITS: "
                  << formatBits(walkthrough.validMap) << "\n"
                  << "WALKTHROUGH_STATIC_PATH_ID: " << walkthrough.pathId << "\n"
                  << "WALKTHROUGH_SLOTS_A_B_C_D: "
                  << walkthrough.slots[0] << ',' << walkthrough.slots[1] << ','
                  << walkthrough.slots[2] << ',' << walkthrough.slots[3] << "\n"
                  << "WALKTHROUGH_PATTERNIDS_A_B_C_D: "
                  << walkthrough.patternIds[0] << ',' << walkthrough.patternIds[1]
                  << ',' << walkthrough.patternIds[2] << ','
                  << walkthrough.patternIds[3] << "\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "p3_synb_hyp02_exact_81path_proof_test: "
                  << error.what() << '\n';
        return 1;
    }
}
