#include "../inc/HierarchicalRecamSimulator.hpp"
#include "../inc/FaultAddress.hpp"
#include "../inc/CamRecamModel.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace dynamic_spare
{
namespace
{

template <typename T>
double fraction(T numerator, T denominator) noexcept
{
    return denominator == 0
        ? 0.0
        : static_cast<double>(numerator) / static_cast<double>(denominator);
}

std::size_t checkedMultiply(
    std::size_t left,
    std::size_t right,
    const char *description)
{
    if (left != 0 && right > std::numeric_limits<std::size_t>::max() / left)
    {
        throw std::overflow_error(description);
    }
    return left * right;
}

std::size_t checkedAdd(
    std::size_t left,
    std::size_t right,
    const char *description)
{
    if (right > std::numeric_limits<std::size_t>::max() - left)
    {
        throw std::overflow_error(description);
    }
    return left + right;
}

std::uint64_t checkedAddCycles(
    std::uint64_t left,
    std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
    {
        throw std::overflow_error("BIRA cycle total exceeds uint64 range");
    }
    return left + right;
}

std::string formatBitmap(const std::vector<bool> &bitmap)
{
    if (bitmap.empty())
        return "-";
    std::string result;
    result.reserve(bitmap.size());
    for (bool bit : bitmap)
        result.push_back(bit ? '1' : '0');
    return result;
}

std::size_t faultCount(const FaultGroup &faults)
{
    return std::accumulate(
        faults.begin(), faults.end(), std::size_t{0},
        [](std::size_t total, const std::vector<Fault> &subarrayFaults)
        {
            return total + subarrayFaults.size();
        });
}

void validateGroupInput(const RepairGroupInput &input)
{
    if (input.address.device < 0 || input.address.domain < 0 ||
        input.address.bank < 0 || input.address.group < 0)
    {
        throw std::invalid_argument(
            "Hierarchical repair-group IDs cannot be negative");
    }
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        for (const Fault &fault : input.faults[subarray])
        {
            if (fault.HBMID != input.address.device ||
                fault.ChannelID != input.address.domain ||
                fault.BankID != input.address.bank ||
                fault.SubarrayGroupID != input.address.group ||
                fault.SubarrayID != static_cast<int>(subarray))
            {
                throw std::invalid_argument(
                    "Fault address does not match its hierarchical repair group");
            }
            if (fault.r < 0 || fault.c < 0)
            {
                throw std::invalid_argument(
                    "Fault row/column cannot be negative");
            }
        }
    }
}

SimulationConfig withoutSharing(const SimulationConfig &source)
{
    SimulationConfig result = source;
    result.topology = SharingTopology::NoSharing;
    result.sharedRows = 0;
    result.sharedColumns = 0;
    result.globalPool.reset();
    result.modifiers.localFirst = true;
    result.modifiers.singleDimensionBorrowing = false;
    result.modifiers.minimumRowReserve = 0;
    result.modifiers.minimumColumnReserve = 0;
    result.modifiers.maximumGroupBorrowedSpares = 0;
    return result;
}

SimulationConfig withFunctionalCamCapacity(
    const SimulationConfig &source,
    std::size_t entries,
    std::uint32_t dataWordBits)
{
    if (entries > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        throw std::invalid_argument(
            "Functional CAM demand exceeds the legacy solver's int range");
    }
    SimulationConfig result = source;
    result.usePaperCamReuseCapacity = false;
    result.bufferCamEntries = static_cast<int>(entries);
    result.dataWidthBits = dataWordBits;
    result.validate();
    return result;
}

std::vector<BufferRepairMapping> selectedCamMappings(
    const GroupRepairResult &analysis)
{
    std::vector<BufferRepairMapping> mappings;
    if (!analysis.groupRepairSuccess)
    {
        return mappings;
    }
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        if (!analysis.selectedCandidateOptions[subarray].has_value())
        {
            throw std::logic_error(
                "Tier-2 analysis did not retain its selected repair option");
        }
        const auto &selected = *analysis.selectedCandidateOptions[subarray];
        mappings.insert(
            mappings.end(),
            selected.bufferMappings.begin(),
            selected.bufferMappings.end());
    }
    return mappings;
}

bool sharingWasUsed(const GroupRepairResult &analysis) noexcept
{
    return analysis.sharing.borrowedRows != 0 ||
           analysis.sharing.borrowedColumns != 0;
}

std::uint64_t analysisCycles(const GroupRepairResult &analysis) noexcept
{
    return analysis.biraLatency.totalWorkCycles;
}

void applyDecoupledFifoTimeline(
    HierarchicalGroupResult &result,
    const RepairGroupInput &input,
    const SimulationConfig &groupConfig,
    const HierarchicalRecamConfig &hierarchicalConfig)
{
    SerialBistSchedule schedule;
    schedule.geometry.rows = groupConfig.memoryRows;
    schedule.geometry.cellColumns = groupConfig.memoryColumns;
    schedule.geometry.wordBits = hierarchicalConfig.dataWordBits;
    schedule.subarrayCount = static_cast<std::uint32_t>(kSubarrayCount);
    schedule.cyclesPerWord = hierarchicalConfig.bistCyclesPerWord;
    schedule.validate();

    std::vector<BiraFaultArrivalEvent> events;
    events.reserve(result.metrics.faultCount);
    std::size_t sequence = 0;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        const auto &attempts =
            result.tier0Analysis.attemptsBySubarray[subarray];
        if (attempts.empty())
        {
            throw std::logic_error(
                "Tier-0 analysis contains no attempt for FIFO timing");
        }
        const RepairAttemptResult &attempt = attempts.front();
        const std::size_t count = input.faults[subarray].size();
        if (count == 0)
        {
            continue;
        }
        const bool hasPerFaultSramService = attempt.sramRecam.has_value();
        if (hasPerFaultSramService &&
            attempt.sramRecam->bira.faultCollectionServiceCycles.size() !=
                count)
        {
            throw std::logic_error(
                "SRAM per-fault service count does not match physical faults");
        }
        std::uint64_t uniformServiceCycles = 0;
        if (!hasPerFaultSramService)
        {
            if (attempt.biraLatency.faultCollectionWorkCycles % count != 0)
            {
                throw std::logic_error(
                    "CAM collection work is not uniform per physical fault");
            }
            uniformServiceCycles =
                attempt.biraLatency.faultCollectionWorkCycles / count;
        }
        for (std::size_t faultIndex = 0; faultIndex < count; ++faultIndex)
        {
            const Fault &fault = input.faults[subarray][faultIndex];
            const std::uint64_t serviceCycles = hasPerFaultSramService
                ? attempt.sramRecam->bira
                      .faultCollectionServiceCycles[faultIndex]
                : uniformServiceCycles;
            if (serviceCycles == 0)
            {
                throw std::logic_error(
                    "A physical fault has zero BIRA collection service time");
            }
            events.push_back({
                schedule.arrivalCycle(fault), serviceCycles, sequence++});
        }
    }

    const std::uint64_t finalAnalysisCycles = checkedAddCycles(
        result.biraLatency.repairAnalysisWorkCycles,
        result.biraLatency.sharingAllocationWorkCycles);
    DecoupledFifoConfig fifo;
    fifo.bistCompletionCycle = schedule.completionCycle();
    fifo.finalRepairAnalysisCycles = finalAnalysisCycles;
    BiraLatencyMetrics timeline = simulateDecoupledFifo(
        result.biraLatency.storageTechnology,
        std::move(events),
        fifo);

    // Preserve the common work categories while removing duplicate fault
    // collection caused by functional re-analysis of sharing/Tier-2 options.
    timeline.repairAnalysisWorkCycles =
        result.biraLatency.repairAnalysisWorkCycles;
    timeline.sharingAllocationWorkCycles =
        result.biraLatency.sharingAllocationWorkCycles;
    finalizeBiraWork(timeline);
    result.biraLatency = std::move(timeline);
    result.totalBiraCycles = result.biraLatency.totalWorkCycles;
}

std::ofstream openCsv(const std::filesystem::path &path)
{
    std::ofstream output(path, std::ios::trunc);
    if (!output)
    {
        throw std::runtime_error("Unable to open CSV output: " + path.string());
    }
    output << std::setprecision(12);
    return output;
}

} // namespace

const char *toString(CamReuseGranularity granularity) noexcept
{
    switch (granularity)
    {
        case CamReuseGranularity::Cell: return "CELL";
        case CamReuseGranularity::DataWord: return "DATA_WORD";
    }
    return "UNKNOWN";
}

const char *toString(OnlineCamCapacitySource source) noexcept
{
    switch (source)
    {
        case OnlineCamCapacitySource::PaperRsPlusCs:
            return "PAPER_RS_PLUS_CS";
        case OnlineCamCapacitySource::ExplicitOverride:
            return "EXPLICIT_OVERRIDE";
    }
    return "UNKNOWN";
}

const char *toString(RepairSource source) noexcept
{
    switch (source)
    {
        case RepairSource::LocalOnly: return "LOCAL_ONLY";
        case RepairSource::SharingRequired: return "SHARING_REQUIRED";
        case RepairSource::CamReuseRequired: return "CAM_REUSE_REQUIRED";
        case RepairSource::SharingAndCamReuseRequired:
            return "SHARING_AND_CAM_REUSE_REQUIRED";
        case RepairSource::Unrepairable: return "UNREPAIRABLE";
    }
    return "UNKNOWN";
}

bool RepairGroupAddress::operator<(
    const RepairGroupAddress &other) const noexcept
{
    return std::tie(device, domain, bank, group) <
           std::tie(other.device, other.domain, other.bank, other.group);
}

bool RepairGroupAddress::operator==(
    const RepairGroupAddress &other) const noexcept
{
    return device == other.device && domain == other.domain &&
           bank == other.bank && group == other.group;
}

bool GlobalRepairTag::operator<(const GlobalRepairTag &other) const noexcept
{
    return std::tie(domain, bank, group, subarray, row, repairColumn) <
           std::tie(
               other.domain, other.bank, other.group, other.subarray,
               other.row, other.repairColumn);
}

bool GlobalRepairTag::operator==(
    const GlobalRepairTag &other) const noexcept
{
    return domain == other.domain && bank == other.bank &&
           group == other.group && subarray == other.subarray &&
           row == other.row && repairColumn == other.repairColumn;
}

GlobalRepairTag canonicalizeForCamReuse(
    const BufferRepairMapping &mapping,
    CamReuseGranularity granularity,
    std::uint32_t dataWordBits)
{
    if (mapping.ChannelID < 0 || mapping.BankID < 0 ||
        mapping.SubarrayGroupID < 0 || mapping.SubarrayID < 0 ||
        mapping.sourceRow < 0 || mapping.sourceColumn < 0)
    {
        throw std::invalid_argument(
            "CAM-reuse mapping contains a negative source address");
    }
    if (dataWordBits == 0)
    {
        throw std::invalid_argument("CAM-reuse data-word width must be positive");
    }
    const std::uint32_t cellColumn = static_cast<std::uint32_t>(
        mapping.sourceColumn);
    return GlobalRepairTag{
        mapping.ChannelID,
        mapping.BankID,
        mapping.SubarrayGroupID,
        mapping.SubarrayID,
        mapping.sourceRow,
        granularity == CamReuseGranularity::DataWord
            ? cellColumn / dataWordBits
            : cellColumn};
}

GlobalOnlineRepairPool::GlobalOnlineRepairPool(
    std::size_t capacity,
    CamReuseGranularity granularity,
    std::uint32_t dataWordBits)
    : capacity_(capacity),
      granularity_(granularity),
      dataWordBits_(dataWordBits)
{
    if (dataWordBits_ == 0)
    {
        throw std::invalid_argument("Global CAM data-word width must be positive");
    }
}

GlobalCamAllocation GlobalOnlineRepairPool::reserve(
    const std::vector<BufferRepairMapping> &mappings)
{
    GlobalCamAllocation result;
    result.wordsRequestedBeforeDedup = mappings.size();
    result.occupancyBefore = entries_.size();

    std::set<GlobalRepairTag> unique;
    for (const BufferRepairMapping &mapping : mappings)
    {
        unique.insert(canonicalizeForCamReuse(
            mapping, granularity_, dataWordBits_));
    }
    result.requestedTags.assign(unique.begin(), unique.end());
    result.wordsRequestedAfterDedup = result.requestedTags.size();

    std::vector<GlobalRepairTag> novel;
    for (const GlobalRepairTag &tag : result.requestedTags)
    {
        if (entries_.find(tag) != entries_.end())
        {
            ++result.allocationSuccessCount;
        }
        else
        {
            novel.push_back(tag);
        }
    }

    const std::size_t freeEntries = capacity_ - entries_.size();
    if (novel.size() <= freeEntries)
    {
        entries_.insert(novel.begin(), novel.end());
        result.allocationSuccessCount += novel.size();
        result.entriesNewlyReserved = novel.size();
    }
    else
    {
        // Commit repair assignments only when the whole group can be stored.
        // This preserves the offline "analyze -> commit -> clear" lifetime.
        result.allocationFailureCount = novel.size();
    }

    peakOccupancy_ = std::max(peakOccupancy_, entries_.size());
    result.occupancyAfter = entries_.size();
    return result;
}

std::size_t GlobalOnlineRepairPool::capacity() const noexcept
{
    return capacity_;
}

std::size_t GlobalOnlineRepairPool::occupancy() const noexcept
{
    return entries_.size();
}

std::size_t GlobalOnlineRepairPool::peakOccupancy() const noexcept
{
    return peakOccupancy_;
}

bool GlobalOnlineRepairPool::contains(
    const GlobalRepairTag &tag) const noexcept
{
    return entries_.find(tag) != entries_.end();
}

const std::set<GlobalRepairTag> &GlobalOnlineRepairPool::entries() const noexcept
{
    return entries_;
}

void HierarchicalRecamConfig::validate() const
{
    if (biraEngineCount == 0)
    {
        throw std::invalid_argument("At least one BIRA engine is required");
    }
    if (dataWordBits == 0)
    {
        throw std::invalid_argument("data_word_bits must be positive");
    }
    if (bistCyclesPerWord == 0)
    {
        throw std::invalid_argument("bist_cycles_per_word must be positive");
    }
    if (architectureTotalSubarrays == 0 ||
        architectureTotalRepairGroups == 0)
    {
        throw std::invalid_argument(
            "Architecture subarray/group counts must be positive");
    }
    if (architectureTotalRepairGroups >
        architectureTotalSubarrays / kSubarrayCount)
    {
        throw std::invalid_argument(
            "Architecture has fewer subarrays than its 4-SA repair groups");
    }
}

double GlobalCamMetrics::utilization() const noexcept
{
    return fraction(entriesReserved, capacity);
}

double GlobalCamMetrics::overflowRate() const noexcept
{
    return fraction(allocationFailureCount, requestCount);
}

double DeviceRepairResult::fractionGroupsRepairable() const noexcept
{
    return fraction(repairableGroupCount, modeledGroupCount);
}

double DeviceRepairResult::fractionBanksRepairable() const noexcept
{
    return fraction(repairableBankCount, modeledBankCount);
}

double DeviceRepairResult::fractionDomainsRepairable() const noexcept
{
    return fraction(repairableDomainCount, modeledDomainCount);
}

std::uint64_t DeviceRepairResult::totalBiraCompletionCycles() const noexcept
{
    std::uint64_t maximum = 0;
    for (const BiraEngineMetrics &engine : biraEngines)
    {
        maximum = std::max(maximum, engine.totalCycles);
    }
    return maximum;
}

DeviceRepairScheduler::DeviceRepairScheduler() = default;

DeviceRepairScheduler::DeviceRepairScheduler(DynamicRepairSimulator analyzer)
    : analyzer_(std::move(analyzer))
{
}

DeviceRepairScheduler::DeviceRepairScheduler(
    DynamicRepairSimulator analyzer,
    HardwareMetrics backendHardware)
    : analyzer_(std::move(analyzer)),
      backendHardware_(std::move(backendHardware))
{
}

DeviceRepairResult DeviceRepairScheduler::run(
    const std::vector<RepairGroupInput> &groups,
    const SimulationConfig &groupConfig,
    const HierarchicalRecamConfig &hierarchicalConfig) const
{
    groupConfig.validate();
    hierarchicalConfig.validate();

    DeviceRepairResult device;
    device.storageTechnology = groupConfig.storageMode ==
            FaultInformationStorage::SRAM
        ? BiraStorageTechnology::Sram
        : BiraStorageTechnology::Cam;
    device.modeledGroupCount = groups.size();
    device.biraEngines.resize(hierarchicalConfig.biraEngineCount);
    for (std::size_t engine = 0;
         engine < device.biraEngines.size(); ++engine)
    {
        device.biraEngines[engine].engineId = engine;
    }

    device.hardware.totalSubarrays =
        hierarchicalConfig.architectureTotalSubarrays;
    device.hardware.totalRepairGroups =
        hierarchicalConfig.architectureTotalRepairGroups;
    device.hardware.localSpareRows = checkedMultiply(
        device.hardware.totalSubarrays,
        static_cast<std::size_t>(groupConfig.spareRows),
        "Local spare-row hardware count overflow");
    device.hardware.localSpareColumns = checkedMultiply(
        device.hardware.totalSubarrays,
        static_cast<std::size_t>(groupConfig.spareColumns),
        "Local spare-column hardware count overflow");
    device.hardware.sharingControlDomains =
        hierarchicalConfig.architectureTotalRepairGroups;
    device.hardware.biraEngineCount = hierarchicalConfig.biraEngineCount;
    const bool camBackend = device.storageTechnology ==
        BiraStorageTechnology::Cam;
    device.hardware.addressCamStructures = camBackend
        ? hierarchicalConfig.biraEngineCount
        : 0;
    device.hardware.hybridCamStructures = camBackend
        ? hierarchicalConfig.biraEngineCount
        : 0;
    device.hardware.addressSramStructures = camBackend
        ? 0
        : hierarchicalConfig.biraEngineCount;
    device.hardware.hybridSramStructures = camBackend
        ? 0
        : hierarchicalConfig.biraEngineCount;
    device.hardware.matrixAnalyzers = hierarchicalConfig.biraEngineCount;

    const std::size_t spareRows = static_cast<std::size_t>(
        groupConfig.spareRows);
    const std::size_t spareColumns = static_cast<std::size_t>(
        groupConfig.spareColumns);
    const std::size_t paperAddressCamEntries = checkedAdd(
        spareRows, spareColumns,
        "Paper Address CAM entry count overflow");
    const std::size_t paperHybridCamEntries = checkedAdd(
        checkedMultiply(
            spareRows, spareColumns - 1,
            "Paper Hybrid CAM entry count overflow"),
        checkedMultiply(
            spareColumns, spareRows - 1,
            "Paper Hybrid CAM entry count overflow"),
        "Paper Hybrid CAM entry count overflow");
    const std::size_t onlineGlobalReuseEntries =
        hierarchicalConfig.onlineGlobalReuseEntries.value_or(
            paperAddressCamEntries);

    device.onlineCamCapacitySource =
        hierarchicalConfig.onlineGlobalReuseEntries.has_value()
            ? OnlineCamCapacitySource::ExplicitOverride
            : OnlineCamCapacitySource::PaperRsPlusCs;
    device.hardware.paperAddressCamEntriesPerStructure =
        paperAddressCamEntries;
    device.hardware.paperHybridCamEntriesPerStructure =
        paperHybridCamEntries;
    // Restricting online capacity does not shrink the offline RECAM CAM.
    // Enlarging it requires enough address and data-word entries in both
    // mode-reused structures.
    device.hardware.physicalAddressCamEntriesPerStructure = std::max(
        paperAddressCamEntries, onlineGlobalReuseEntries);
    device.hardware.physicalHybridCamEntriesPerStructure = std::max(
        paperHybridCamEntries, onlineGlobalReuseEntries);
    device.hardware.totalPhysicalAddressCamEntries = camBackend
        ? checkedMultiply(
              device.hardware.physicalAddressCamEntriesPerStructure,
              device.hardware.addressCamStructures,
              "Total physical Address CAM entry count overflow")
        : 0;
    device.hardware.totalPhysicalHybridCamEntries = camBackend
        ? checkedMultiply(
              device.hardware.physicalHybridCamEntriesPerStructure,
              device.hardware.hybridCamStructures,
              "Total physical Hybrid CAM entry count overflow")
        : 0;

    if (backendHardware_.has_value())
    {
        if (backendHardware_->storageTechnology != device.storageTechnology)
        {
            throw std::invalid_argument(
                "Device scheduler backend hardware/storage mismatch");
        }
        device.hardware.biraPerEngine = *backendHardware_;
    }
    else
    {
        if (!camBackend)
        {
            throw std::invalid_argument(
                "SRAM DeviceRepairScheduler requires SRAM HardwareMetrics");
        }
        RecamGeometryConfig geometry;
        geometry.rows = groupConfig.memoryRows;
        geometry.columns = groupConfig.memoryColumns;
        geometry.spareRows = static_cast<std::uint32_t>(
            groupConfig.spareRows);
        geometry.spareColumns = static_cast<std::uint32_t>(
            groupConfig.spareColumns);
        geometry.channels = 8;
        geometry.dataWordBits = hierarchicalConfig.dataWordBits;
        geometry.onlineReuseEntries = static_cast<std::uint32_t>(
            onlineGlobalReuseEntries);
        device.hardware.biraPerEngine = deriveCamHardwareMetrics(geometry);
    }
    device.hardware.totalBiraStorageBits = checkedMultiply(
        static_cast<std::size_t>(
            device.hardware.biraPerEngine.totalStorageBits),
        hierarchicalConfig.biraEngineCount,
        "Total BIRA storage bits overflow");
    device.hardware.totalBiraModeledBits = checkedMultiply(
        static_cast<std::size_t>(
            device.hardware.biraPerEngine.totalModeledBits),
        hierarchicalConfig.biraEngineCount,
        "Total BIRA modeled bits overflow");

    GlobalOnlineRepairPool pool(
        onlineGlobalReuseEntries,
        hierarchicalConfig.camReuseGranularity,
        hierarchicalConfig.dataWordBits);
    std::set<RepairGroupAddress> seenGroups;
    std::set<std::pair<int, int>> camBanks;
    std::set<int> camDomains;
    std::map<std::pair<int, int>, bool> bankSuccess;
    std::map<int, bool> domainSuccess;
    std::optional<int> deviceId;

    device.groups.reserve(groups.size());
    for (std::size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex)
    {
        const RepairGroupInput &input = groups[groupIndex];
        validateGroupInput(input);
        if (!seenGroups.insert(input.address).second)
        {
            throw std::invalid_argument(
                "Device input contains a duplicate repair-group address");
        }
        if (!deviceId.has_value())
        {
            deviceId = input.address.device;
        }
        else if (*deviceId != input.address.device)
        {
            throw std::invalid_argument(
                "One global CAM pool cannot span multiple memory devices");
        }

        HierarchicalGroupResult result;
        result.address = input.address;
        result.metrics.faultCount = faultCount(input.faults);
        result.assignedBiraEngine =
            groupIndex % hierarchicalConfig.biraEngineCount;
        BiraEngineMetrics &engine =
            device.biraEngines[result.assignedBiraEngine];
        engine.transientFaultsInScratch = result.metrics.faultCount;

        SimulationConfig tier0Config = withFunctionalCamCapacity(
            withoutSharing(groupConfig), 0,
            hierarchicalConfig.dataWordBits);
        result.tier0Analysis = analyzer_.run(
            input.faults, tier0Config, groupIndex, true);
        result.biraLatency = result.tier0Analysis.biraLatency;
        result.totalBiraCycles = analysisCycles(result.tier0Analysis);
        result.metrics.localSuccessMask =
            result.tier0Analysis.localRepairSuccess;
        result.metrics.tier0Success =
            result.tier0Analysis.groupRepairSuccess;

        const GroupRepairResult *lineAnalysis = &result.tier0Analysis;
        if (!result.metrics.tier0Success &&
            groupConfig.topology != SharingTopology::NoSharing)
        {
            result.metrics.tier1Attempted = true;
            SimulationConfig tier1Config = withFunctionalCamCapacity(
                groupConfig, 0, hierarchicalConfig.dataWordBits);
            result.tier1Analysis = analyzer_.run(
                input.faults, tier1Config, groupIndex, true);
            result.totalBiraCycles = checkedAddCycles(
                result.totalBiraCycles,
                analysisCycles(*result.tier1Analysis));
            accumulateBiraWork(
                result.biraLatency,
                result.tier1Analysis->biraLatency);
            lineAnalysis = &*result.tier1Analysis;
            result.metrics.tier1Success = lineAnalysis->groupRepairSuccess;
        }

        if (lineAnalysis->groupRepairSuccess)
        {
            result.metrics.sharingUsed = sharingWasUsed(*lineAnalysis);
            result.metrics.borrowedRows = lineAnalysis->sharing.borrowedRows;
            result.metrics.borrowedColumns =
                lineAnalysis->sharing.borrowedColumns;
            result.metrics.finalSuccess = true;
            result.metrics.repairSource = result.metrics.sharingUsed
                ? RepairSource::SharingRequired
                : RepairSource::LocalOnly;
        }
        else
        {
            // A large functional software container discovers the complete
            // per-group Tier-2 demand. Physical capacity is enforced once, by
            // the device-wide pool below, and is not multiplied per SA.
            SimulationConfig tier2Config = withFunctionalCamCapacity(
                groupConfig,
                result.metrics.faultCount,
                hierarchicalConfig.dataWordBits);
            result.tier2Analysis = analyzer_.run(
                input.faults, tier2Config, groupIndex, true);
            result.totalBiraCycles = checkedAddCycles(
                result.totalBiraCycles,
                analysisCycles(*result.tier2Analysis));
            accumulateBiraWork(
                result.biraLatency,
                result.tier2Analysis->biraLatency);

            if (result.tier2Analysis->groupRepairSuccess)
            {
                const std::vector<BufferRepairMapping> mappings =
                    selectedCamMappings(*result.tier2Analysis);
                GlobalCamAllocation allocation = pool.reserve(mappings);
                result.metrics.camWordsNeeded =
                    allocation.wordsRequestedAfterDedup;
                result.metrics.camWordsAllocated =
                    allocation.allocationSuccessCount;
                result.metrics.camAllocationFailures =
                    allocation.allocationFailureCount;
                result.metrics.globalCamOccupancyBefore =
                    allocation.occupancyBefore;
                result.metrics.globalCamOccupancyAfter =
                    allocation.occupancyAfter;
                result.metrics.sharingUsed =
                    sharingWasUsed(*result.tier2Analysis);
                result.metrics.borrowedRows =
                    result.tier2Analysis->sharing.borrowedRows;
                result.metrics.borrowedColumns =
                    result.tier2Analysis->sharing.borrowedColumns;

                device.globalCam.wordsRequestedBeforeDedup +=
                    allocation.wordsRequestedBeforeDedup;
                device.globalCam.wordsRequestedAfterDedup +=
                    allocation.wordsRequestedAfterDedup;
                device.globalCam.requestCount +=
                    allocation.wordsRequestedAfterDedup;
                device.globalCam.allocationSuccessCount +=
                    allocation.allocationSuccessCount;
                device.globalCam.allocationFailureCount +=
                    allocation.allocationFailureCount;
                if (allocation.wordsRequestedAfterDedup != 0)
                {
                    ++device.globalCam.groupsRequestingCam;
                    camBanks.insert({input.address.domain, input.address.bank});
                    camDomains.insert(input.address.domain);
                }

                result.metrics.finalSuccess = allocation.success();
                if (result.metrics.finalSuccess)
                {
                    result.persistentCamAssignments =
                        std::move(allocation.requestedTags);
                    result.metrics.repairSource = result.metrics.sharingUsed
                        ? RepairSource::SharingAndCamReuseRequired
                        : RepairSource::CamReuseRequired;
                }
            }
        }

        if (hierarchicalConfig.modelDecoupledFifo)
        {
            applyDecoupledFifoTimeline(
                result, input, groupConfig, hierarchicalConfig);
        }

        engine.groupsProcessed++;
        engine.totalCycles = checkedAddCycles(
            engine.totalCycles, result.totalBiraCycles);
        engine.transientFaultsInScratch = 0;
        engine.scratchClearCount++;

        if (result.biraLatency.bistTimelineModeled)
        {
            ++device.groupsWithBistTimeline;
            device.maximumFaultCollectionLatencyAfterBistCycles = std::max(
                device.maximumFaultCollectionLatencyAfterBistCycles,
                result.biraLatency.faultCollectionLatencyAfterBistCycles);
            device.maximumSolutionLatencyAfterBistCycles = std::max(
                device.maximumSolutionLatencyAfterBistCycles,
                result.biraLatency.latencyAfterBistCycles);
        }

        if (result.metrics.finalSuccess)
        {
            ++device.repairableGroupCount;
        }
        const auto bankKey =
            std::make_pair(input.address.domain, input.address.bank);
        const auto bank = bankSuccess.find(bankKey);
        if (bank == bankSuccess.end())
        {
            bankSuccess.emplace(bankKey, result.metrics.finalSuccess);
        }
        else
        {
            bank->second = bank->second && result.metrics.finalSuccess;
        }
        const auto domain = domainSuccess.find(input.address.domain);
        if (domain == domainSuccess.end())
        {
            domainSuccess.emplace(
                input.address.domain, result.metrics.finalSuccess);
        }
        else
        {
            domain->second = domain->second && result.metrics.finalSuccess;
        }
        ++device.repairSourceCounts[
            static_cast<std::size_t>(result.metrics.repairSource)];
        device.groups.push_back(std::move(result));
    }

    device.modeledBankCount = bankSuccess.size();
    device.repairableBankCount = static_cast<std::size_t>(std::count_if(
        bankSuccess.begin(), bankSuccess.end(),
        [](const auto &entry) { return entry.second; }));
    device.modeledDomainCount = domainSuccess.size();
    device.repairableDomainCount = static_cast<std::size_t>(std::count_if(
        domainSuccess.begin(), domainSuccess.end(),
        [](const auto &entry) { return entry.second; }));
    device.deviceRepairSuccess =
        !groups.empty() && device.repairableGroupCount == groups.size();
    device.globalCam.capacity = pool.capacity();
    device.globalCam.entriesReserved = pool.occupancy();
    device.globalCam.peakOccupancy = pool.peakOccupancy();
    device.globalCam.banksRequestingCam = camBanks.size();
    device.globalCam.domainsRequestingCam = camDomains.size();
    return device;
}

void HierarchicalCsvReporter::writeDeviceSummary(
    const std::filesystem::path &path,
    const DeviceRepairResult &result,
    const SimulationConfig &groupConfig,
    const HierarchicalRecamConfig &hierarchicalConfig)
{
    std::ofstream output = openCsv(path);
    output
        << "architecture_version,simulator_version,cam_scope,bira_storage,"
           "bira_engine_count,"
           "cam_reuse_granularity,data_word_bits,Rs,Cs,solution_take_policy,modeled_groups,"
           "repairable_groups,fraction_groups_repairable,modeled_banks,"
           "repairable_banks,fraction_banks_repairable,modeled_domains,"
           "repairable_domains,fraction_domains_repairable,device_success,"
           "global_cam_capacity,global_cam_capacity_source,"
           "paper_address_cam_entries_per_structure,"
           "paper_hybrid_cam_entries_per_structure,"
           "physical_address_cam_entries_per_structure,"
           "physical_hybrid_cam_entries_per_structure,"
           "total_physical_address_cam_entries,"
           "total_physical_hybrid_cam_entries,"
           "global_cam_entries_reserved,"
           "global_cam_peak_occupancy,global_cam_utilization,cam_request_count,"
           "cam_allocation_success_count,cam_allocation_failure_count,"
           "cam_overflow_rate,cam_words_requested_before_dedup,"
           "cam_words_requested_after_dedup,groups_requesting_cam,"
           "banks_requesting_cam,domains_requesting_cam,local_only_groups,"
           "sharing_required_groups,cam_reuse_required_groups,"
           "sharing_and_cam_reuse_required_groups,unrepairable_groups,"
           "total_bira_completion_cycles,groups_with_bist_timeline,"
           "maximum_fault_collection_latency_after_bist_cycles,"
           "maximum_solution_latency_after_bist_cycles,address_cam_structures,"
           "hybrid_cam_structures,address_sram_structures,"
           "hybrid_sram_structures,matrix_analyzers,"
           "bira_storage_bits_per_engine,bira_modeled_bits_per_engine,"
           "total_bira_storage_bits,total_bira_modeled_bits,"
           "global_online_cam_pools,"
           "separately_counted_online_cam_pools,offline_online_cam_mode_reused\n";
    output
        << result.architectureVersion << ',' << result.simulatorVersion << ','
        << result.camScope << ',' << toString(result.storageTechnology) << ','
        << hierarchicalConfig.biraEngineCount << ','
        << toString(hierarchicalConfig.camReuseGranularity) << ','
        << hierarchicalConfig.dataWordBits << ',' << groupConfig.spareRows << ','
        << groupConfig.spareColumns << ','
        << toString(groupConfig.solutionTakePolicy) << ','
        << result.modeledGroupCount << ','
        << result.repairableGroupCount << ','
        << result.fractionGroupsRepairable() << ',' << result.modeledBankCount
        << ',' << result.repairableBankCount << ','
        << result.fractionBanksRepairable() << ',' << result.modeledDomainCount
        << ',' << result.repairableDomainCount << ','
        << result.fractionDomainsRepairable() << ','
        << (result.deviceRepairSuccess ? 1 : 0) << ','
        << result.globalCam.capacity << ','
        << toString(result.onlineCamCapacitySource) << ','
        << result.hardware.paperAddressCamEntriesPerStructure << ','
        << result.hardware.paperHybridCamEntriesPerStructure << ','
        << result.hardware.physicalAddressCamEntriesPerStructure << ','
        << result.hardware.physicalHybridCamEntriesPerStructure << ','
        << result.hardware.totalPhysicalAddressCamEntries << ','
        << result.hardware.totalPhysicalHybridCamEntries << ','
        << result.globalCam.entriesReserved
        << ',' << result.globalCam.peakOccupancy << ','
        << result.globalCam.utilization() << ',' << result.globalCam.requestCount
        << ',' << result.globalCam.allocationSuccessCount << ','
        << result.globalCam.allocationFailureCount << ','
        << result.globalCam.overflowRate() << ','
        << result.globalCam.wordsRequestedBeforeDedup << ','
        << result.globalCam.wordsRequestedAfterDedup << ','
        << result.globalCam.groupsRequestingCam << ','
        << result.globalCam.banksRequestingCam << ','
        << result.globalCam.domainsRequestingCam;
    for (std::size_t count : result.repairSourceCounts)
    {
        output << ',' << count;
    }
    output
        << ',' << result.totalBiraCompletionCycles()
        << ',' << result.groupsWithBistTimeline
        << ',' << result.maximumFaultCollectionLatencyAfterBistCycles
        << ',' << result.maximumSolutionLatencyAfterBistCycles
        << ',' << result.hardware.addressCamStructures
        << ',' << result.hardware.hybridCamStructures
        << ',' << result.hardware.addressSramStructures
        << ',' << result.hardware.hybridSramStructures
        << ',' << result.hardware.matrixAnalyzers
        << ',' << result.hardware.biraPerEngine.totalStorageBits
        << ',' << result.hardware.biraPerEngine.totalModeledBits
        << ',' << result.hardware.totalBiraStorageBits
        << ',' << result.hardware.totalBiraModeledBits
        << ',' << result.hardware.globalOnlineCamPools
        << ',' << result.hardware.separatelyCountedOnlineCamPools
        << ',' << (result.hardware.offlineOnlineCamModeReused ? 1 : 0)
        << '\n';
}

void HierarchicalCsvReporter::writeGroups(
    const std::filesystem::path &path,
    const DeviceRepairResult &result,
    const HierarchicalRecamConfig &hierarchicalConfig)
{
    std::ofstream output = openCsv(path);
    output
        << "architecture_version,simulator_version,cam_scope,bira_engine_count,"
           "device,domain,bank,group,bira_engine,fault_count,local_success_mask,"
           "tier0_success,tier1_attempted,tier1_success,sharing_used,"
           "borrowed_rows,borrowed_columns,cam_words_needed,cam_words_allocated,"
           "cam_allocation_failures,global_cam_occupancy_before,"
           "global_cam_occupancy_after,repair_source,final_success,"
           "bira_storage,bira_timing_model,bira_fault_collection_work_cycles,"
           "bira_repair_analysis_work_cycles,"
           "bira_sharing_allocation_work_cycles,bira_total_work_cycles,"
           "bist_timeline_modeled,first_fault_arrival_cycle,"
           "last_fault_arrival_cycle,bist_completion_cycle,"
           "fault_collection_completion_cycle,solution_ready_cycle,"
           "fault_collection_latency_after_bist_cycles,"
           "latency_after_bist_cycles,maximum_fault_queue_depth,"
           "total_fault_queue_wait_cycles,bist_backpressure_cycles,"
           "bira_cycles,solution_take_policy,selected_solution_A,"
           "selected_solution_B,selected_solution_C,selected_solution_D,"
           "valid_solution_bitmap_A,valid_solution_bitmap_B,"
           "valid_solution_bitmap_C,valid_solution_bitmap_D,"
           "feasible_combination_count,solution_selection_work,"
           "compressed_state_bits,early_success,group_compressed_success,"
           "greedy_loss\n";
    for (const HierarchicalGroupResult &group : result.groups)
    {
        const GroupRepairResult &selectedAnalysis = group.tier2Analysis
            ? *group.tier2Analysis
            : (group.tier1Analysis ? *group.tier1Analysis
                                   : group.tier0Analysis);
        std::string mask;
        for (bool success : group.metrics.localSuccessMask)
        {
            mask.push_back(success ? '1' : '0');
        }
        output
            << result.architectureVersion << ',' << result.simulatorVersion
            << ',' << result.camScope << ','
            << hierarchicalConfig.biraEngineCount << ','
            << group.address.device << ',' << group.address.domain << ','
            << group.address.bank << ',' << group.address.group << ','
            << group.assignedBiraEngine << ',' << group.metrics.faultCount << ','
            << mask << ',' << (group.metrics.tier0Success ? 1 : 0) << ','
            << (group.metrics.tier1Attempted ? 1 : 0) << ','
            << (group.metrics.tier1Success ? 1 : 0) << ','
            << (group.metrics.sharingUsed ? 1 : 0) << ','
            << group.metrics.borrowedRows << ','
            << group.metrics.borrowedColumns << ','
            << group.metrics.camWordsNeeded << ','
            << group.metrics.camWordsAllocated << ','
            << group.metrics.camAllocationFailures << ','
            << group.metrics.globalCamOccupancyBefore << ','
            << group.metrics.globalCamOccupancyAfter << ','
            << toString(group.metrics.repairSource) << ','
            << (group.metrics.finalSuccess ? 1 : 0) << ','
            << toString(group.biraLatency.storageTechnology) << ','
            << toString(group.biraLatency.timingModel) << ','
            << group.biraLatency.faultCollectionWorkCycles << ','
            << group.biraLatency.repairAnalysisWorkCycles << ','
            << group.biraLatency.sharingAllocationWorkCycles << ','
            << group.biraLatency.totalWorkCycles << ','
            << (group.biraLatency.bistTimelineModeled ? 1 : 0) << ','
            << group.biraLatency.firstFaultArrivalCycle << ','
            << group.biraLatency.lastFaultArrivalCycle << ','
            << group.biraLatency.bistCompletionCycle << ','
            << group.biraLatency.faultCollectionCompletionCycle << ','
            << group.biraLatency.solutionReadyCycle << ','
            << group.biraLatency.faultCollectionLatencyAfterBistCycles << ','
            << group.biraLatency.latencyAfterBistCycles << ','
            << group.biraLatency.maximumFaultQueueDepth << ','
            << group.biraLatency.totalFaultQueueWaitCycles << ','
            << group.biraLatency.bistBackpressureCycles << ','
            << group.totalBiraCycles << ','
            << toString(selectedAnalysis.solutionTakePolicy);
        for (const auto &selected : selectedAnalysis.selectedCandidateIndices)
            output << ',' << (selected.has_value()
                    ? std::to_string(*selected) : "-1");
        for (const auto &bitmap : selectedAnalysis.validSolutionBitmaps)
            output << ',' << formatBitmap(bitmap);
        output
            << ',' << selectedAnalysis.feasibleCombinationCount
            << ',' << selectedAnalysis.solutionSelectionWork
            << ',' << selectedAnalysis.compressedStateBits
            << ',' << (selectedAnalysis.earlySuccess.has_value()
                    ? (*selectedAnalysis.earlySuccess ? "1" : "0") : "-")
            << ',' << (selectedAnalysis.groupCompressedSuccess.has_value()
                    ? (*selectedAnalysis.groupCompressedSuccess ? "1" : "0")
                    : "-")
            << ',' << (selectedAnalysis.greedyLoss ? 1 : 0)
            << '\n';
    }
}

void HierarchicalCsvReporter::writeBiraEngines(
    const std::filesystem::path &path,
    const DeviceRepairResult &result)
{
    std::ofstream output = openCsv(path);
    output
        << "architecture_version,simulator_version,cam_scope,bira_engine_count,"
           "engine_id,groups_processed,scratch_clear_count,"
           "transient_faults_remaining,total_cycles\n";
    for (const BiraEngineMetrics &engine : result.biraEngines)
    {
        output
            << result.architectureVersion << ',' << result.simulatorVersion
            << ',' << result.camScope << ',' << result.biraEngines.size() << ','
            << engine.engineId << ',' << engine.groupsProcessed << ','
            << engine.scratchClearCount << ','
            << engine.transientFaultsInScratch << ',' << engine.totalCycles
            << '\n';
    }
}

} // namespace dynamic_spare
