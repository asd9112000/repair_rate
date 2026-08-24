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

enum class FaultCountModel
{
    FileProvided,
    Uniform,
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
    PairwiseEdge
};

enum class FaultInformationStorage
{
    CAM,
    SRAM
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

    SharingTopology topology = SharingTopology::NoSharing;
    FaultInformationStorage storageMode = FaultInformationStorage::CAM;
    AnalysisLatencyParameters latency;

    // CAM reuse is the dynamic simulator default. A fixed entry count remains
    // available through --buffer for legacy SharedLine comparisons.
    int bufferCamEntries = 2;
    bool usePaperCamReuseCapacity = true;

    void validate() const;
};

const char *toString(FaultCountModel model) noexcept;
const char *toString(FaultSpatialModel model) noexcept;
const char *toString(SharingTopology topology) noexcept;
const char *toString(FaultInformationStorage storage) noexcept;

} // namespace dynamic_spare

#endif
