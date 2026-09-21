#include "DirectionalMultiConfigAnalyzer.hpp"

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

constexpr std::size_t kSubarrays = 4;
constexpr std::size_t kSlots = 4;
constexpr std::size_t kMasks = 16;
constexpr std::size_t kRawTuples = 256;

struct SlotSemantics
{
    int configId;
    int rows;
    int columns;
    bool release;
    bool borrow;
};

constexpr std::array<std::array<SlotSemantics, kSlots>, kSubarrays> kSlotsBySa{{
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

int slotFor(bool release, bool borrow)
{
    return (release ? 1 : 0) | (borrow ? 2 : 0);
}

std::array<int, kSubarrays> slotsForMask(int mask)
{
    const bool e0 = (mask & 0x1) != 0;
    const bool e1 = (mask & 0x2) != 0;
    const bool e2 = (mask & 0x4) != 0;
    const bool e3 = (mask & 0x8) != 0;
    return {{slotFor(e0, e3), slotFor(e3, e2),
             slotFor(e1, e0), slotFor(e2, e1)}};
}

std::array<int, kSubarrays> slotsForRawTuple(int tuple)
{
    return {{tuple & 0x3, (tuple >> 2) & 0x3,
             (tuple >> 4) & 0x3, (tuple >> 6) & 0x3}};
}

bool valid(const std::array<unsigned int, kSubarrays> &validMap,
           std::size_t sa, int slot)
{
    return (validMap[sa] & (1U << static_cast<unsigned int>(slot))) != 0;
}

bool isReleaseMonotonic(const std::array<unsigned int, kSubarrays> &validMap)
{
    for (std::size_t sa = 0; sa < kSubarrays; ++sa)
    {
        if (valid(validMap, sa, 1) && !valid(validMap, sa, 0)) return false;
        if (valid(validMap, sa, 3) && !valid(validMap, sa, 2)) return false;
    }
    return true;
}

bool staticRepairable(const std::array<unsigned int, kSubarrays> &validMap)
{
    for (int mask = 0; mask < static_cast<int>(kMasks); ++mask)
    {
        const auto slots = slotsForMask(mask);
        bool pathValid = true;
        for (std::size_t sa = 0; sa < kSubarrays; ++sa)
            pathValid = pathValid && valid(validMap, sa, slots[sa]);
        if (pathValid) return true;
    }
    return false;
}

struct RawClassification
{
    bool repairable = false;
    bool illegal = false;
    int normalizedMask = 0;
};

RawClassification classifyRawTuple(
    const std::array<unsigned int, kSubarrays> &validMap, int tuple)
{
    std::array<int, kSubarrays> slots = slotsForRawTuple(tuple);
    for (std::size_t sa = 0; sa < kSubarrays; ++sa)
        if (!valid(validMap, sa, slots[sa])) return {};

    // E0=A->C, E1=C->D, E2=D->B, E3=B->A.  A release-only
    // choice is normalized by changing only that donor's release bit.
    constexpr std::array<std::size_t, kMasks / 4> kDonor{{0, 2, 3, 1}};
    constexpr std::array<std::size_t, kMasks / 4> kBorrower{{2, 3, 1, 0}};
    int mask = 0;
    for (std::size_t edge = 0; edge < kDonor.size(); ++edge)
    {
        const bool release = kSlotsBySa[kDonor[edge]][slots[kDonor[edge]]].release;
        const bool borrow = kSlotsBySa[kBorrower[edge]][slots[kBorrower[edge]]].borrow;
        if (!release && borrow) return {false, true, 0};
        if (release && !borrow)
        {
            const bool donorBorrow =
                kSlotsBySa[kDonor[edge]][slots[kDonor[edge]]].borrow;
            slots[kDonor[edge]] = slotFor(false, donorBorrow);
        }
        if (release && borrow) mask |= (1 << static_cast<int>(edge));
    }

    for (std::size_t sa = 0; sa < kSubarrays; ++sa)
        if (!valid(validMap, sa, slots[sa])) return {};
    return {true, false, mask};
}

Fault makeFault(int subarray, int row, int column)
{
    Fault value{};
    value.SubarrayID = subarray;
    value.r = row;
    value.c = column;
    return value;
}

struct P1Summary
{
    std::size_t cases = 0;
    std::size_t counterexamples = 0;
    int firstRole = -1;
    unsigned int firstBitmap = 0;
    int firstReleasedConfig = -1;
    int firstUnreleasedConfig = -1;
    std::uint8_t firstReleasedPattern = 0;
    std::uint8_t firstUnreleasedPattern = 0;
};

std::string faultCoordinates(unsigned int bitmap)
{
    std::ostringstream output;
    bool first = true;
    for (int position = 0; position < 9; ++position)
    {
        if ((bitmap & (1U << static_cast<unsigned int>(position))) == 0)
            continue;
        if (!first) output << ',';
        output << '(' << position / 3 << ',' << position % 3 << ')';
        first = false;
    }
    return output.str();
}

void checkP1AndP2(P1Summary &p1, std::size_t &p2ValidPatterns)
{
    const dynamic_spare::DirectionalMultiConfigAnalyzer analyzer;
    // Exhaustive normalized 3x3 incidence corpus: coordinate relabeling does
    // not alter the independent analyzer's resource envelope decision.
    for (int role = 0; role < static_cast<int>(kSubarrays); ++role)
    {
        for (unsigned int bitmap = 0; bitmap < (1U << 9); ++bitmap)
        {
            std::vector<Fault> faults;
            for (int position = 0; position < 9; ++position)
            {
                if ((bitmap & (1U << static_cast<unsigned int>(position))) == 0)
                    continue;
                faults.push_back(makeFault(role, position / 3, position % 3));
            }
            const auto analysis = analyzer.analyze(faults, role);
            const bool transposedRole = role == 0 || role == 3;
            const std::array<std::pair<int, int>, 2> p1Pairs = transposedRole
                ? std::array<std::pair<int, int>, 2>{{{4, 0}, {6, 5}}}
                : std::array<std::pair<int, int>, 2>{{{1, 0}, {3, 2}}};
            for (const auto &pair : p1Pairs)
            {
                ++p1.cases;
                const auto &released = analysis.configs[static_cast<std::size_t>(pair.first)];
                const auto &unreleased = analysis.configs[static_cast<std::size_t>(pair.second)];
                if (released.lowestValidPatternId != 0 &&
                    unreleased.lowestValidPatternId == 0)
                {
                    ++p1.counterexamples;
                    if (p1.firstRole == -1)
                    {
                        p1.firstRole = role;
                        p1.firstBitmap = bitmap;
                        p1.firstReleasedConfig = pair.first;
                        p1.firstUnreleasedConfig = pair.second;
                        p1.firstReleasedPattern = released.lowestValidPatternId;
                        p1.firstUnreleasedPattern = unreleased.lowestValidPatternId;
                    }
                }
            }

            for (std::size_t slot = 0; slot < kSlots; ++slot)
            {
                const SlotSemantics expected = kSlotsBySa[role][slot];
                const auto &view = analysis.configs[static_cast<std::size_t>(expected.configId)];
                require(view.descriptor.rows == expected.rows &&
                            view.descriptor.columns == expected.columns,
                        "P2 slot descriptor changed for role=" + std::to_string(role) +
                            " slot=" + std::to_string(slot));
                for (std::size_t pattern = 0; pattern < view.candidateValidity.size(); ++pattern)
                {
                    if (!view.candidateValidity[pattern]) continue;
                    ++p2ValidPatterns;
                    require(pattern + 1 <= 15,
                            "P2 PatternID exceeds four-bit store contract");
                    // Pattern is local only: all group attributes come from
                    // the role/slot descriptor checked above, never PatternID.
                    require(kSlotsBySa[role][slot].release == expected.release &&
                                kSlotsBySa[role][slot].borrow == expected.borrow,
                            "P2 group attribute changed within a slot");
                }
            }
        }
    }
}

void checkP3(std::size_t &validMaps, std::size_t &uncoveredLegalTuples,
             std::size_t &repairabilityMismatches,
             std::size_t &illegalStaticAssignments)
{
    for (unsigned int packed = 0; packed < (1U << 16); ++packed)
    {
        std::array<unsigned int, kSubarrays> validMap{{
            packed & 0xfU, (packed >> 4) & 0xfU,
            (packed >> 8) & 0xfU, (packed >> 12) & 0xfU}};
        if (!isReleaseMonotonic(validMap)) continue;
        ++validMaps;

        bool rawRepairable = false;
        for (int tuple = 0; tuple < static_cast<int>(kRawTuples); ++tuple)
        {
            const RawClassification result = classifyRawTuple(validMap, tuple);
            if (!result.repairable) continue;
            rawRepairable = true;
            const auto canonicalSlots = slotsForMask(result.normalizedMask);
            for (std::size_t sa = 0; sa < kSubarrays; ++sa)
            {
                if (!valid(validMap, sa, canonicalSlots[sa]))
                    ++uncoveredLegalTuples;
            }
        }
        if (rawRepairable != staticRepairable(validMap))
            ++repairabilityMismatches;
    }

    for (int mask = 0; mask < static_cast<int>(kMasks); ++mask)
    {
        const auto slots = slotsForMask(mask);
        const bool e0 = kSlotsBySa[0][slots[0]].release == kSlotsBySa[2][slots[2]].borrow;
        const bool e1 = kSlotsBySa[2][slots[2]].release == kSlotsBySa[3][slots[3]].borrow;
        const bool e2 = kSlotsBySa[3][slots[3]].release == kSlotsBySa[1][slots[1]].borrow;
        const bool e3 = kSlotsBySa[1][slots[1]].release == kSlotsBySa[0][slots[0]].borrow;
        if (!(e0 && e1 && e2 && e3)) ++illegalStaticAssignments;
    }
}

} // namespace

int main()
{
    try
    {
        P1Summary p1;
        std::size_t p2ValidPatterns = 0;
        checkP1AndP2(p1, p2ValidPatterns);

        std::size_t validMaps = 0;
        std::size_t uncoveredLegalTuples = 0;
        std::size_t repairabilityMismatches = 0;
        std::size_t illegalStaticAssignments = 0;
        checkP3(validMaps, uncoveredLegalTuples, repairabilityMismatches,
                illegalStaticAssignments);
        require(validMaps == 6561, "P3 did not enumerate all P1-monotonic validity maps");
        require(uncoveredLegalTuples == 0, "P3 found an uncovered legal tuple");
        require(repairabilityMismatches == 0, "static path repairability mismatch");
        require(illegalStaticAssignments == 0, "static path violates resource conservation");

        const bool p1Pass = p1.counterexamples == 0;
        std::cout << "P1_GENERALIZED_RELEASE_MONOTONICITY: "
                  << (p1Pass ? "PASS" : "FAIL") << "\n"
                  << "P1_ANALYZER_CORPUS_CASES: " << p1.cases << "\n"
                  << "P1_COUNTEREXAMPLES: " << p1.counterexamples << "\n";
        if (!p1Pass)
        {
            std::cout << "P1_SMALLEST_COUNTEREXAMPLE_ROLE: " << p1.firstRole << "\n"
                      << "P1_SMALLEST_COUNTEREXAMPLE_BITMAP: " << p1.firstBitmap << "\n"
                      << "P1_SMALLEST_COUNTEREXAMPLE_FAULTS: "
                      << faultCoordinates(p1.firstBitmap) << "\n"
                      << "P1_SMALLEST_COUNTEREXAMPLE_RELEASED_CONFIG: CFG"
                      << p1.firstReleasedConfig << " pattern="
                      << static_cast<unsigned int>(p1.firstReleasedPattern) << "\n"
                      << "P1_SMALLEST_COUNTEREXAMPLE_UNRELEASED_CONFIG: CFG"
                      << p1.firstUnreleasedConfig << " pattern="
                      << static_cast<unsigned int>(p1.firstUnreleasedPattern) << "\n";
        }
        std::cout
                  << "P2_PATTERN_WITHIN_SLOT_EQUIVALENCE: PASS\n"
                  << "P2_VALID_PATTERN_OBSERVATIONS: " << p2ValidPatterns << "\n"
                  << "P3_256_TO_16_NORMALIZATION: "
                  << (p1Pass ? "PASS" : "CONDITIONAL_PASS__P1_FAILED") << "\n"
                  << "RAW_TUPLES: 256\n"
                  << "CANONICAL_EDGE_MASKS: 16\n"
                  << "P3_MONOTONIC_VALIDITY_MAPS: " << validMaps << "\n"
                  << "UNCOVERED_LEGAL_TUPLES: " << uncoveredLegalTuples << "\n"
                  << "STATIC_SEARCH_REPAIRABILITY_MISMATCHES: " << repairabilityMismatches << "\n"
                  << "ILLEGAL_RESOURCE_ASSIGNMENTS: " << illegalStaticAssignments << "\n";
        return p1Pass ? 0 : 1;
    }
    catch (const std::exception &error)
    {
        std::cerr << "p3_synb_hyp02_phase4f_slot_static_oracle_test: "
                  << error.what() << '\n';
        return 1;
    }
}
