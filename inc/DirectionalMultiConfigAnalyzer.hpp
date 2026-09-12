#ifndef DYNAMIC_SPARE_SHARING_DIRECTIONAL_MULTI_CONFIG_ANALYZER_HPP
#define DYNAMIC_SPARE_SHARING_DIRECTIONAL_MULTI_CONFIG_ANALYZER_HPP

#include "RECAMSolverAdapter.hpp"
#include "PhysicalResourceLedger.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dynamic_spare
{

// These IDs describe the physical spare envelope, never a RECAM candidate.
enum class DirectionalConfigId : std::size_t
{
    Cfg0_2R2C = 0,
    Cfg1_2R1C,
    Cfg2_3R2C,
    Cfg3_3R1C,
    Cfg4_1R2C,
    Cfg5_2R3C,
    Cfg6_1R3C,
    Count
};

constexpr std::size_t kDirectionalConfigCount =
    static_cast<std::size_t>(DirectionalConfigId::Count);
constexpr std::size_t kDirectionalMaxPivotPayload = 5;

struct PivotPayloadEntry
{
    Fault address;
    bool valid = false;
};

struct DirectionalConfigDescriptor
{
    DirectionalConfigId id;
    int rows;
    int columns;
    // CFG4--CFG6 reuse a canonical analyzer with its complete R/C view
    // transposed; addresses returned by this API remain physical addresses.
    bool transposeCanonicalView;
};

struct DirectionalConfigAnalysis
{
    DirectionalConfigDescriptor descriptor;
    std::array<bool, kDirectionalMaxPivotPayload> addressActive{};
    std::vector<bool> candidateValidity;
    // Zero is invalid; nonzero values are the one-based externally retained
    // PatternID.  The RECAM solver's internal candidate index is zero based.
    std::uint8_t lowestValidPatternId = 0;
    std::size_t hybridLogicalOccupancy = 0;
    std::vector<Fault> camReuseTemporaryFaults;
    std::vector<RepairLineMapping> reconstructedMappings;
};

struct DirectionalMultiConfigAnalysis
{
    std::array<PivotPayloadEntry, kDirectionalMaxPivotPayload> pivotPayload;
    std::vector<Fault> greedyPivotSequence;
    std::vector<Fault> camReuseOverflowBeyondPayload;
    std::vector<int> rowFaultCounts;
    std::vector<int> columnFaultCounts;
    std::array<DirectionalConfigAnalysis, kDirectionalConfigCount> configs;
    std::size_t sharedPivotOccupancy = 0;
    std::size_t hybridUnionOccupancy = 0;
    std::size_t maximumHybridUnionOccupancy = 0;
};

enum class DirectionalSolutionTakePolicy { Early, Group };

struct PendingDirectionalRepairRecord
{
    std::size_t subarrayId = 0;
    DirectionalConfigId configId = DirectionalConfigId::Cfg0_2R2C;
    std::uint8_t patternId = 0;
    std::vector<RepairLineMapping> conventionalMappings;
    std::vector<Fault> camReusePending;
};

struct DirectionalGroupResult
{
    std::array<std::array<std::uint8_t, kDirectionalConfigCount>,
               kSubarrayCount> configPatternMap{};
    bool success = false;
    bool earlySuccess = false;
    bool groupSuccess = false;
    bool greedyLoss = false;
    std::uint64_t groupCombinationsChecked = 0;
    std::uint64_t groupCombinationsPruned = 0;
    LedgerAllocationResult ledger;
    std::vector<PendingDirectionalRepairRecord> pendingRepairBuffer;
};

// Functional analyzer for the finalized 2x2 directional envelope set.  The
// physical fault payload, greedy pivot sequence and counters are collected
// once.  Per-config records contain only logical interpretation and the
// retained lowest PatternID.  RECAMSolverAdapter remains the normative
// matrix/candidate implementation while this layer owns multi-config state.
class DirectionalMultiConfigAnalyzer
{
public:
    DirectionalMultiConfigAnalysis analyze(
        const std::vector<Fault> &faults,
        int subarrayId,
        int camReuseEntries = kDirectionalMaxPivotPayload) const;

    DirectionalGroupResult analyzeGroup(
        const std::array<std::vector<Fault>, kSubarrayCount> &faults,
        const SimulationConfig &ledgerConfig,
        DirectionalSolutionTakePolicy policy,
        int camReuseEntries = kDirectionalMaxPivotPayload) const;

    static const std::array<DirectionalConfigDescriptor,
                            kDirectionalConfigCount> &descriptors() noexcept;
};

} // namespace dynamic_spare

#endif
