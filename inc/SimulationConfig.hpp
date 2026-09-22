#ifndef DYNAMIC_SPARE_SHARING_SIMULATION_CONFIG_HPP
#define DYNAMIC_SPARE_SHARING_SIMULATION_CONFIG_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace dynamic_spare
{

constexpr std::size_t kSubarrayCount = 4;

enum class GroupLayout
{
    Grid2x2,
    Line1x4
};

enum class FaultCountModel
{
    FileProvided,
    Uniform,
    // Independent equal-probability SA assignments for each group fault.
    // This is the R2 fixed-total multinomial contract, not the historical
    // deterministic equal-count allocator represented by Uniform.
    MultinomialUniform,
    ModerateImbalance,
    StrongImbalance,
    Hotspot,
    UserDefined
};

enum class FaultSpatialModel
{
    FileProvided,
    Uniform,
    Mixed,
    Clustered
};

enum class SharingTopology
{
    NoSharing,
    Directional,
    GlobalPool,
    PairwiseEdge,
    PairSharing,
    NeighborSharing
};

enum class FaultInformationStorage
{
    CAM,
    SRAM
};

enum class SolutionTakePolicy
{
    Legacy,
    // Generic RECAM candidates committed A->B->C->D in their natural
    // local-capacity / PatternID order.  This is distinct from Early, which
    // ranks all currently legal candidates to preserve shared resources.
    LocalFirst,
    Early,
    GroupCompressed,
    GroupNoScratchV2,
    // Preserves the historical C++ slot order (0, 1, 2, 3).
    GroupGreedyRtlCanonical,
    // Group-wide, atomic candidate-tuple search with the compressed-state
    // objective; separate from the legacy compatibility name above.
    GroupGlobal,
    // Exact four-SA search over the frozen directional V2 capacity-slot and
    // PatternID contract.  This is intentionally distinct from GroupGlobal,
    // whose candidate universe is the historical generic RECAM contract.
    DirectionalV2GroupGlobal,
    // Canonical directional V2 joint oracle.  Candidate order is R,L,RB,B
    // and any speculative borrow from a future owner creates an exact
    // release obligation that must be discharged by the completed tuple.
    DirectionalV2GroupGlobalCanonical,
    // Frozen directional V2 candidate contract with sequential canonical
    // R,L,RB,B commitment (1, 0, 3, 2). This is deliberately separate from
    // Early, which retains the historical generic RECAM candidate contract.
    DirectionalV2Early,
    OneByFourTwoPairwiseEarlyV1,
    OneByFourTwoPairwisePairGlobalV1,
    OneByFourSingleHopEarlyV1,
    OneByFourSingleHopGlobalV1,
    // R1B keeps the same row-only candidate generator as its v1 policies;
    // only the sequential priority changes from first-legal to
    // resource-preserving ranked commit.
    OneByFourTwoPairwiseReleaseAwareEarlyV1,
    OneByFourSingleHopReleaseAwareEarlyV1
};

// Numeric ConfigIDs are scoped to an RTL architecture point.  They are debug
// and replay metadata, not a cross-point capacity identifier.
enum class ConfigContractVersion
{
    GenericRecamCandidateV1,
    FrozenDate2x2M1,
    Rs3Cs3M1,
    HistoricalCppV2SlotMapV1
};

struct PolicyModifiers
{
    bool localFirst = false;
    bool singleDimensionBorrowing = false;
    int minimumRowReserve = 0;
    int minimumColumnReserve = 0;
    int maximumGroupBorrowedSpares = 3;
};

struct GlobalPoolConfiguration
{
    int localRowsPerSubarray = 0;
    int localColumnsPerSubarray = 0;
    int globalRows = 0;
    int globalColumns = 0;
};

struct StorageLatencyParameters
{
    std::uint64_t insertCyclesPerOperation = 0;
    std::uint64_t lookupCyclesPerOperation = 0;
    std::uint64_t readCyclesPerOperation = 0;
};

struct AnalysisLatencyParameters
{
    StorageLatencyParameters cam{1, 1, 1};
    StorageLatencyParameters sram{2, 2, 2};
    std::uint64_t matrixGenerationCyclesPerCell = 1;
    std::uint64_t solutionGenerationCyclesPerCandidate = 1;
    std::uint64_t solutionEvaluationCyclesPerCandidate = 1;
    std::uint64_t sharingAllocationCyclesPerRequest = 1;
};

struct SimulationConfig
{
    std::size_t numSubarrays = kSubarrayCount;

    int spareRows = 2;
    int spareColumns = 2;
    int sharedRows = 0;
    int sharedColumns = 0;

    std::optional<GlobalPoolConfiguration> globalPool;
    PolicyModifiers modifiers;

    std::uint32_t dataWidthBits = 64;
    std::uint32_t rowAddressWidthBits = 10;
    std::uint32_t columnAddressWidthBits = 10;
    // This is intentionally independent of data/address width.  The dynamic
    // sharing specification does not define a derivation formula.
    std::optional<std::uint32_t> hybridCamEntryWidthBits;

    std::uint64_t faultCount = 20; // Total across A/B/C/D for one run.
    std::uint64_t simulationRuns = 10000;
    std::uint64_t randomSeed = 20260820;
    std::uint32_t memoryRows = 1024;
    std::uint32_t memoryColumns = 1024;
    FaultCountModel faultCountModel = FaultCountModel::Uniform;
    FaultSpatialModel faultSpatialModel = FaultSpatialModel::Mixed;
    std::array<int, kSubarrayCount> userDefinedFaultCounts{{0, 0, 0, 0}};

    GroupLayout layout = GroupLayout::Grid2x2;
    SharingTopology topology = SharingTopology::NoSharing;
    FaultInformationStorage storageMode = FaultInformationStorage::CAM;
    // Legacy preserves the pre-policy group selector exactly.  The explicit
    // policies operate on compressed, pointer-free RECAM solution state.
    SolutionTakePolicy solutionTakePolicy = SolutionTakePolicy::Legacy;
    // Experiment-facing identity is intentionally separate from the
    // implementation policy selected above.  The R3 runner supplies these
    // fields for canonical sweeps; manual invocations remain explicitly
    // UNSPECIFIED rather than silently claiming canonical membership.
    std::string canonicalPolicyId = "UNSPECIFIED";
    bool paperCanonical = false;
    std::string legacyAliasOf = "";
    AnalysisLatencyParameters latency;

    // CAM reuse is the dynamic simulator default. A fixed entry count remains
    // available through --buffer for legacy SharedLine comparisons.
    int bufferCamEntries = 2;
    bool usePaperCamReuseCapacity = true;

    void validate() const;
};

const char *toString(FaultCountModel model) noexcept;
const char *toString(FaultSpatialModel model) noexcept;
const char *toString(GroupLayout layout) noexcept;
const char *toString(SharingTopology topology) noexcept;
const char *toString(FaultInformationStorage storage) noexcept;
const char *toString(SolutionTakePolicy policy) noexcept;
const char *toString(ConfigContractVersion version) noexcept;

} // namespace dynamic_spare

#endif
