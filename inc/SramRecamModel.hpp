#ifndef SRAM_RECAM_MODEL_HPP
#define SRAM_RECAM_MODEL_HPP

#include "BiraLatency.hpp"
#include "HardwareMetrics.hpp"
#include "RecamGeometry.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace sram_recam
{

enum class SearchPolicy
{
    CamBaseline,
    SramSerial,
    SramChunked,
    SramWide,
    SramHash,
    SramBloomChunked
};

enum class MatrixPolicy
{
    SramScanRebuild,
    ShadowMatrix
};

const char *toString(SearchPolicy policy) noexcept;
const char *toString(MatrixPolicy policy) noexcept;

struct SramRecamConfig
{
    std::uint32_t rows = 1024;
    std::uint32_t cols = 1024;
    std::uint32_t spareRows = 2;
    std::uint32_t spareCols = 2;
    std::uint32_t channels = 1;
    std::uint32_t dataWordBits = 64;

    SearchPolicy biraSearchPolicy = SearchPolicy::SramChunked;
    SearchPolicy runtimeSearchPolicy = SearchPolicy::SramChunked;
    std::uint32_t addressParallelism = 2;
    std::uint32_t hybridReadParallelism = 2;
    std::uint32_t runtimeParallelism = 2;
    // Zero means all paper candidates are evaluated in parallel.
    std::uint32_t solutionParallelism = 0;

    MatrixPolicy matrixPolicy = MatrixPolicy::ShadowMatrix;
    bool useAbstractSearchRoundModel = true;
    // DynamicSpareSharing currently builds/analyzes the matrix once after
    // fault collection.  The per-fault mode is exposed for the architecture
    // experiment described by docs/SRAM_RECAM_ARCHITECTURE_GUIDE.md.
    bool analyzeAfterEachFault = false;

    std::uint32_t stateUpdateCycles = 1;
    std::uint32_t matrixFinalizeCycles = 1;
    std::uint32_t runtimeDataReadCycles = 0;
    std::uint32_t runtimeMuxCycles = 1;
    std::uint32_t normalDramReadyCycles = 10;

    // nullopt implements the paper capacity K=Rs+Cs.  Dynamic integration
    // sets an explicit value to reproduce --buffer 0/N exactly.
    std::optional<std::uint32_t> camReuseEntries;
    std::optional<std::uint64_t> minimumAddressEntryBits;

    void validate() const;
};

struct SramArrayOrganization
{
    std::uint64_t logicalEntries = 0;
    std::uint64_t entryBits = 0;
    std::uint64_t parallelEntries = 0;
    std::uint64_t physicalDepth = 0;
    std::uint64_t physicalWidth = 0;
    std::uint64_t logicalBits = 0;
    std::uint64_t physicalBits = 0;
    std::uint64_t paddedBits = 0;
    double utilization = 0.0;
};

struct SramRecamGeometry : public dynamic_spare::RecamGeometry
{
    SramArrayOrganization addressSram;
    SramArrayOrganization hybridSram;
    SramArrayOrganization runtimeSram;

    std::uint64_t rowComparatorCount = 0;
    std::uint64_t columnComparatorCount = 0;
    std::uint64_t runtimeComparatorCount = 0;
    std::uint64_t comparatorBitCount = 0;
};

SramRecamGeometry deriveGeometry(const SramRecamConfig &config);

struct AddressSramEntry
{
    bool valid = false;
    std::uint32_t row = 0;
    std::uint32_t col = 0;
    bool rowMust = false;
    bool colMust = false;
    std::uint32_t rowFailCount = 0;
    std::uint32_t colFailCount = 0;
};

struct AddressMatchResult
{
    bool rowMatch = false;
    bool colMatch = false;
    int rowMatchIndex = -1;
    int colMatchIndex = -1;
    std::uint64_t chunksRead = 0;
    std::uint64_t entriesCompared = 0;
    std::uint64_t occupiedEntries = 0;
    bool earlyTerminated = false;
};

class AddressSramSearchEngine
{
public:
    explicit AddressSramSearchEngine(std::size_t capacity);

    bool insert(const AddressSramEntry &entry);
    AddressMatchResult search(
        std::uint32_t row,
        std::uint32_t col,
        SearchPolicy policy,
        std::uint32_t configuredParallelism) const;

    std::size_t size() const noexcept;
    std::size_t capacity() const noexcept;
    const std::vector<AddressSramEntry> &entries() const noexcept;

private:
    std::vector<AddressSramEntry> entries_;
};

struct BiraWorkload
{
    std::uint64_t faultsDetected = 0;
    std::uint64_t addressWrites = 0;
    std::uint64_t hybridWrites = 0;
    std::uint64_t tempBufferWrites = 0;
    std::uint64_t analysisPasses = 1;
    // Strict occupied-prefix Address-SRAM rounds in fault arrival order.
    // This is intentionally per fault because occupancy and dual-match early
    // termination make collection service non-uniform.
    std::vector<std::uint64_t> addressSearchRoundsByFault;

    std::uint64_t addressEntryCount = 0;
    std::uint64_t hybridEntryCount = 0;
    std::uint64_t tempEntryCount = 0;
    std::uint64_t earlyTerminationHitCount = 0;
    std::uint64_t hybridBitmapAllocations = 0;
    std::uint64_t hybridFullCount = 0;
    std::uint64_t pivotFaults = 0;
    std::uint64_t nonpivotFaults = 0;
    std::uint64_t mustRepairTriggeringFaults = 0;
    std::uint64_t pivotSearchRounds = 0;
    std::uint64_t nonpivotSearchRounds = 0;
    std::uint64_t mustRepairSearchRounds = 0;
    std::uint64_t addressEntriesCompared = 0;
};

struct BiraLatencyStats
{
    std::uint64_t faultsDetected = 0;
    std::uint64_t addressSearchRounds = 0;
    std::uint64_t addressSearchRoundsMaximum = 0;
    std::uint64_t addressSearchCycles = 0;
    std::uint64_t addressSearchCyclesMaximum = 0;
    std::uint64_t addressSramReads = 0;
    std::uint64_t addressWrites = 0;
    std::uint64_t hybridWrites = 0;
    std::uint64_t tempBufferWrites = 0;
    std::uint64_t matrixAddressReadRounds = 0;
    std::uint64_t matrixHybridReadRounds = 0;
    std::uint64_t analysisRounds = 0;
    std::uint64_t comparatorEvaluations = 0;
    std::uint64_t comparatorBitComparisons = 0;
    std::uint64_t stateUpdateCycles = 0;
    std::uint64_t matrixFinalizeCycles = 0;
    std::uint64_t faultCollectionCycles = 0;
    std::uint64_t repairAnalysisCycles = 0;
    std::uint64_t modeledCycles = 0;

    std::uint64_t addressEntryCount = 0;
    std::uint64_t hybridEntryCount = 0;
    std::uint64_t tempEntryCount = 0;
    std::uint64_t earlyTerminationHitCount = 0;
    std::uint64_t hybridBitmapAllocations = 0;
    std::uint64_t hybridFullCount = 0;
    std::uint64_t pivotFaults = 0;
    std::uint64_t nonpivotFaults = 0;
    std::uint64_t mustRepairTriggeringFaults = 0;
    std::uint64_t pivotSearchRounds = 0;
    std::uint64_t nonpivotSearchRounds = 0;
    std::uint64_t mustRepairSearchRounds = 0;
    std::uint64_t addressEntriesCompared = 0;
    std::uint64_t pivotSearchCycles = 0;
    std::uint64_t nonpivotSearchCycles = 0;
    std::uint64_t mustRepairSearchCycles = 0;

    std::vector<std::uint64_t> faultCollectionServiceCycles;
};

BiraLatencyStats modelBiraLatency(
    const SramRecamConfig &config,
    const BiraWorkload &workload);

// Convert an address-arrival stream into an SRAM fault-collection timeline.
// Each fault uses the analytical Address-SRAM search plus state-update service
// time; final matrix/solution work begins after both FIFO drain and BIST end.
dynamic_spare::BiraLatencyMetrics modelDecoupledFifoTimeline(
    const SramRecamConfig &config,
    const BiraWorkload &workload,
    const std::vector<std::uint64_t> &arrivalCycles,
    std::uint64_t bistCompletionCycle);

void accumulate(
    BiraLatencyStats &destination,
    const BiraLatencyStats &source);

struct RuntimeKey
{
    std::uint32_t channel = 0;
    std::uint32_t row = 0;
    std::uint32_t col = 0;

    bool operator==(const RuntimeKey &other) const noexcept;
};

using RuntimeData = std::vector<bool>;

struct RuntimeRepairEntry
{
    bool valid = false;
    RuntimeKey key;
    RuntimeData data;
};

struct RuntimeLookupResult
{
    bool hit = false;
    int matchIndex = -1;
    std::uint64_t rounds = 0;
    std::uint64_t entriesCompared = 0;
    RuntimeData data;
};

class SramRuntimeRepairTable
{
public:
    SramRuntimeRepairTable(
        std::size_t capacity,
        std::uint32_t wordBits,
        SearchPolicy policy,
        std::uint32_t parallelism);

    bool insert(const RuntimeKey &key, const RuntimeData &data);
    RuntimeLookupResult lookup(const RuntimeKey &key) const;
    RuntimeLookupResult read(
        const RuntimeKey &key,
        const RuntimeData &normalData) const;
    RuntimeLookupResult write(
        const RuntimeKey &key,
        const RuntimeData &data);

    std::size_t size() const noexcept;
    std::size_t capacity() const noexcept;

private:
    std::uint32_t wordBits_;
    SearchPolicy policy_;
    std::uint32_t parallelism_;
    std::vector<RuntimeRepairEntry> entries_;
};

struct RuntimeLatencyStats
{
    std::uint64_t requests = 0;
    std::uint64_t hits = 0;
    std::uint64_t misses = 0;
    std::uint64_t lookupRounds = 0;
    std::uint64_t hitRounds = 0;
    std::uint64_t missRounds = 0;
    std::uint64_t exposedExtraCycles = 0;
    double lookupRoundsAverage = 0.0;
    double hitRoundsAverage = 0.0;
    double missRoundsAverage = 0.0;
    double exposedExtraCyclesAverage = 0.0;
};

RuntimeLatencyStats modelRuntimeLatency(
    const SramRecamConfig &config,
    std::uint64_t requests,
    std::uint64_t hits,
    std::uint64_t occupiedEntries);

struct AreaProxyParameters
{
    double sramBitAreaUnits = 1.0;
    double comparatorBitAreaUnits = 1.0;
    double fixedControllerAreaUnits = 0.0;
};

struct HardwareCostDescriptor
{
    SramRecamGeometry geometry;
    double normalizedSramArea = 0.0;
    double normalizedComparatorArea = 0.0;
    double normalizedAreaProxy = 0.0;
};

HardwareCostDescriptor deriveHardwareCost(
    const SramRecamConfig &config,
    const AreaProxyParameters &parameters = {});

dynamic_spare::HardwareMetrics deriveCommonHardwareMetrics(
    const SramRecamConfig &config,
    bool offlineOnlineModeReused = true);

void writeHardwareCostJson(
    const std::filesystem::path &path,
    const SramRecamConfig &config,
    const HardwareCostDescriptor &descriptor);

struct SramRecamAttemptMetrics
{
    SearchPolicy biraPolicy = SearchPolicy::SramSerial;
    SearchPolicy runtimePolicy = SearchPolicy::SramSerial;
    MatrixPolicy matrixPolicy = MatrixPolicy::SramScanRebuild;
    SramRecamGeometry geometry;
    dynamic_spare::HardwareMetrics hardware;
    BiraLatencyStats bira;
    bool addressClassificationMatchesCam = false;
};

} // namespace sram_recam

#endif
