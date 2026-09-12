#ifndef HIERARCHICAL_RECAM_SIMULATOR_HPP
#define HIERARCHICAL_RECAM_SIMULATOR_HPP

#include "DynamicRepairSimulator.hpp"
#include "HardwareMetrics.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace dynamic_spare
{

inline constexpr const char *kHierarchicalArchitectureVersion = "WoW-v1.0";
inline constexpr const char *kHierarchicalSimulatorVersion =
    "Hierarchical-RECAM-v2.1";
inline constexpr const char *kGlobalLogicDieCamScope = "GLOBAL_LOGIC_DIE";

enum class CamReuseGranularity
{
    Cell,
    DataWord
};

enum class OnlineCamCapacitySource
{
    PaperRsPlusCs,
    ExplicitOverride
};

enum class RepairSource
{
    LocalOnly,
    SharingRequired,
    CamReuseRequired,
    SharingAndCamReuseRequired,
    Unrepairable
};

const char *toString(CamReuseGranularity granularity) noexcept;
const char *toString(OnlineCamCapacitySource source) noexcept;
const char *toString(RepairSource source) noexcept;

struct RepairGroupAddress
{
    int device = 0;
    int domain = 0;
    int bank = 0;
    int group = 0;

    bool operator<(const RepairGroupAddress &other) const noexcept;
    bool operator==(const RepairGroupAddress &other) const noexcept;
};

// Complete source tag used by the device-wide online repair directory.
// The device ID is deliberately absent: one scheduler/pool models one memory
// device, matching the 27-bit tag in the v2 simulator contract.
struct GlobalRepairTag
{
    int domain = 0;
    int bank = 0;
    int group = 0;
    int subarray = 0;
    int row = 0;
    std::uint32_t repairColumn = 0;

    bool operator<(const GlobalRepairTag &other) const noexcept;
    bool operator==(const GlobalRepairTag &other) const noexcept;
};

GlobalRepairTag canonicalizeForCamReuse(
    const BufferRepairMapping &mapping,
    CamReuseGranularity granularity,
    std::uint32_t dataWordBits);

struct GlobalCamAllocation
{
    std::size_t wordsRequestedBeforeDedup = 0;
    std::size_t wordsRequestedAfterDedup = 0;
    std::size_t allocationSuccessCount = 0;
    std::size_t allocationFailureCount = 0;
    std::size_t entriesNewlyReserved = 0;
    std::size_t occupancyBefore = 0;
    std::size_t occupancyAfter = 0;
    std::vector<GlobalRepairTag> requestedTags;

    bool success() const noexcept
    {
        return allocationFailureCount == 0;
    }
};

class GlobalOnlineRepairPool
{
public:
    GlobalOnlineRepairPool(
        std::size_t capacity,
        CamReuseGranularity granularity,
        std::uint32_t dataWordBits);

    GlobalCamAllocation reserve(
        const std::vector<BufferRepairMapping> &mappings);

    std::size_t capacity() const noexcept;
    std::size_t occupancy() const noexcept;
    std::size_t peakOccupancy() const noexcept;
    bool contains(const GlobalRepairTag &tag) const noexcept;
    const std::set<GlobalRepairTag> &entries() const noexcept;

private:
    std::size_t capacity_ = 0;
    std::size_t peakOccupancy_ = 0;
    CamReuseGranularity granularity_ = CamReuseGranularity::DataWord;
    std::uint32_t dataWordBits_ = 256;
    std::set<GlobalRepairTag> entries_;
};

struct HierarchicalRecamConfig
{
    // No value selects the RECAM-paper capacity: Address CAM entries = Rs+Cs.
    // An explicit value models a restricted or physically enlarged CAM.
    std::optional<std::size_t> onlineGlobalReuseEntries;
    std::size_t biraEngineCount = 1;
    CamReuseGranularity camReuseGranularity =
        CamReuseGranularity::DataWord;
    std::uint32_t dataWordBits = 256;

    // Optional group-relative BIST timeline. Faults are scanned in strict
    // A/B/C/D order through an unbounded decoupled FIFO.
    bool modelDecoupledFifo = false;
    std::uint64_t bistCyclesPerWord = 1;

    // Hardware accounting uses the frozen WoW-v1.0 geometry by default even
    // when a test or scaled experiment processes only a subset of its groups.
    std::size_t architectureDomains = 8;
    std::size_t architectureBanksPerDomain = 4;
    std::size_t architectureGroupsPerBank = 64;
    std::size_t architectureTotalSubarrays = 8192;
    std::size_t architectureTotalRepairGroups = 2048;

    void validate() const;
    // Checks the hierarchy together with the per-SA DRAM and word geometry.
    void validateDramConfig(const SimulationConfig &groupConfig) const;
    std::uint64_t globalWordAddressEntryBits(
        const SimulationConfig &groupConfig) const;
};

struct RepairGroupInput
{
    RepairGroupAddress address;
    FaultGroup faults;
};

struct HierarchicalGroupMetrics
{
    std::size_t faultCount = 0;
    std::array<bool, kSubarrayCount> localSuccessMask{{false, false, false, false}};
    bool tier0Success = false;
    bool tier1Attempted = false;
    bool tier1Success = false;
    bool sharingUsed = false;
    std::size_t borrowedRows = 0;
    std::size_t borrowedColumns = 0;
    std::size_t camWordsNeeded = 0;
    std::size_t camWordsAllocated = 0;
    std::size_t camAllocationFailures = 0;
    std::size_t globalCamOccupancyBefore = 0;
    std::size_t globalCamOccupancyAfter = 0;
    bool finalSuccess = false;
    RepairSource repairSource = RepairSource::Unrepairable;
};

struct HierarchicalGroupResult
{
    RepairGroupAddress address;
    HierarchicalGroupMetrics metrics;
    GroupRepairResult tier0Analysis;
    std::optional<GroupRepairResult> tier1Analysis;
    std::optional<GroupRepairResult> tier2Analysis;
    std::vector<GlobalRepairTag> persistentCamAssignments;
    BiraLatencyResult biraLatency;
    std::uint64_t totalBiraCycles = 0;
    std::size_t assignedBiraEngine = 0;
};

struct BiraEngineMetrics
{
    std::size_t engineId = 0;
    std::size_t groupsProcessed = 0;
    std::size_t scratchClearCount = 0;
    std::size_t transientFaultsInScratch = 0;
    std::uint64_t totalCycles = 0;
};

struct GlobalCamMetrics
{
    std::size_t capacity = 0;
    std::size_t entriesReserved = 0;
    std::size_t peakOccupancy = 0;
    std::size_t requestCount = 0;
    std::size_t allocationSuccessCount = 0;
    std::size_t allocationFailureCount = 0;
    std::size_t wordsRequestedBeforeDedup = 0;
    std::size_t wordsRequestedAfterDedup = 0;
    std::size_t groupsRequestingCam = 0;
    std::size_t banksRequestingCam = 0;
    std::size_t domainsRequestingCam = 0;

    double utilization() const noexcept;
    double overflowRate() const noexcept;
};

struct HierarchicalHardwareAccounting
{
    std::size_t totalSubarrays = 0;
    std::size_t totalRepairGroups = 0;
    std::size_t localSpareRows = 0;
    std::size_t localSpareColumns = 0;
    std::size_t sharingControlDomains = 0;
    std::size_t biraEngineCount = 0;
    std::size_t addressCamStructures = 0;
    std::size_t hybridCamStructures = 0;
    std::size_t addressSramStructures = 0;
    std::size_t hybridSramStructures = 0;
    std::size_t matrixAnalyzers = 0;
    std::size_t paperAddressCamEntriesPerStructure = 0;
    std::size_t paperHybridCamEntriesPerStructure = 0;
    std::size_t physicalAddressCamEntriesPerStructure = 0;
    std::size_t physicalHybridCamEntriesPerStructure = 0;
    std::size_t totalPhysicalAddressCamEntries = 0;
    std::size_t totalPhysicalHybridCamEntries = 0;
    std::size_t globalOnlineCamPools = 1;
    std::size_t separatelyCountedOnlineCamPools = 0;
    bool offlineOnlineCamModeReused = true;
    HardwareMetrics biraPerEngine;
    std::uint64_t totalBiraStorageBits = 0;
    std::uint64_t totalBiraModeledBits = 0;
};

struct DeviceRepairResult
{
    std::string architectureVersion = kHierarchicalArchitectureVersion;
    std::string simulatorVersion = kHierarchicalSimulatorVersion;
    std::string camScope = kGlobalLogicDieCamScope;
    BiraStorageTechnology storageTechnology =
        BiraStorageTechnology::Cam;
    OnlineCamCapacitySource onlineCamCapacitySource =
        OnlineCamCapacitySource::PaperRsPlusCs;
    bool deviceRepairSuccess = false;
    std::size_t modeledGroupCount = 0;
    std::size_t repairableGroupCount = 0;
    std::size_t modeledBankCount = 0;
    std::size_t repairableBankCount = 0;
    std::size_t modeledDomainCount = 0;
    std::size_t repairableDomainCount = 0;
    std::size_t groupsWithBistTimeline = 0;
    std::uint64_t maximumFaultCollectionLatencyAfterBistCycles = 0;
    std::uint64_t maximumSolutionLatencyAfterBistCycles = 0;
    std::array<std::size_t, 5> repairSourceCounts{{0, 0, 0, 0, 0}};
    std::vector<HierarchicalGroupResult> groups;
    std::vector<BiraEngineMetrics> biraEngines;
    GlobalCamMetrics globalCam;
    HierarchicalHardwareAccounting hardware;

    double fractionGroupsRepairable() const noexcept;
    double fractionBanksRepairable() const noexcept;
    double fractionDomainsRepairable() const noexcept;
    std::uint64_t totalBiraCompletionCycles() const noexcept;
};

class DeviceRepairScheduler
{
public:
    DeviceRepairScheduler();
    explicit DeviceRepairScheduler(DynamicRepairSimulator analyzer);
    DeviceRepairScheduler(
        DynamicRepairSimulator analyzer,
        HardwareMetrics backendHardware);

    DeviceRepairResult run(
        const std::vector<RepairGroupInput> &groups,
        const SimulationConfig &groupConfig,
        const HierarchicalRecamConfig &hierarchicalConfig) const;

private:
    DynamicRepairSimulator analyzer_;
    std::optional<HardwareMetrics> backendHardware_;
};

class HierarchicalCsvReporter
{
public:
    static void writeDeviceSummary(
        const std::filesystem::path &path,
        const DeviceRepairResult &result,
        const SimulationConfig &groupConfig,
        const HierarchicalRecamConfig &hierarchicalConfig);
    static void writeGroups(
        const std::filesystem::path &path,
        const DeviceRepairResult &result,
        const HierarchicalRecamConfig &hierarchicalConfig);
    static void writeBiraEngines(
        const std::filesystem::path &path,
        const DeviceRepairResult &result);
};

} // namespace dynamic_spare

#endif
