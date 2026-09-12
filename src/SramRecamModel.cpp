#include "../inc/SramRecamModel.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <string>

namespace sram_recam
{
namespace
{

std::uint64_t checkedAdd(
    std::uint64_t left,
    std::uint64_t right,
    const char *description)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
    {
        throw std::overflow_error(description);
    }
    return left + right;
}

std::uint64_t checkedMultiply(
    std::uint64_t left,
    std::uint64_t right,
    const char *description)
{
    if (left != 0 &&
        right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        throw std::overflow_error(description);
    }
    return left * right;
}

std::uint64_t ceilDiv(std::uint64_t numerator, std::uint64_t denominator)
{
    if (denominator == 0)
    {
        throw std::invalid_argument("SRAM_RECAM parallelism cannot be zero");
    }
    return numerator / denominator + (numerator % denominator != 0);
}

std::uint64_t effectiveParallelism(
    SearchPolicy policy,
    std::uint32_t configured,
    std::uint64_t entries)
{
    if (entries == 0)
    {
        return 0;
    }
    switch (policy)
    {
        case SearchPolicy::CamBaseline:
        case SearchPolicy::SramWide:
            return entries;
        case SearchPolicy::SramSerial:
            return 1;
        case SearchPolicy::SramChunked:
        case SearchPolicy::SramBloomChunked:
        case SearchPolicy::SramHash:
            if (configured == 0)
            {
                throw std::invalid_argument(
                    "Configured SRAM_RECAM parallelism cannot be zero");
            }
            return std::min<std::uint64_t>(configured, entries);
    }
    throw std::logic_error("Unknown SRAM_RECAM search policy");
}

SramArrayOrganization organization(
    std::uint64_t entries,
    std::uint64_t entryBits,
    std::uint64_t parallelism)
{
    SramArrayOrganization result;
    result.logicalEntries = entries;
    result.entryBits = entryBits;
    result.parallelEntries = parallelism;
    result.logicalBits = checkedMultiply(
        entries, entryBits, "SRAM_RECAM logical SRAM size overflow");
    if (entries == 0)
    {
        return result;
    }
    result.physicalDepth = ceilDiv(entries, parallelism);
    result.physicalWidth = checkedMultiply(
        parallelism, entryBits, "SRAM_RECAM physical width overflow");
    result.physicalBits = checkedMultiply(
        result.physicalDepth,
        result.physicalWidth,
        "SRAM_RECAM physical SRAM size overflow");
    result.paddedBits = result.physicalBits - result.logicalBits;
    result.utilization = result.physicalBits == 0
        ? 0.0
        : static_cast<double>(result.logicalBits) /
              static_cast<double>(result.physicalBits);
    return result;
}

std::uint64_t chunkCycles(const SramRecamConfig &config)
{
    return config.useAbstractSearchRoundModel ? 1 : 2;
}

std::uint64_t sumHitRounds(
    std::uint64_t occupiedEntries,
    std::uint64_t parallelism)
{
    const std::uint64_t fullChunks = occupiedEntries / parallelism;
    const std::uint64_t remainder = occupiedEntries % parallelism;
    const std::uint64_t complete = checkedMultiply(
        parallelism,
        checkedMultiply(
            fullChunks,
            fullChunks - (fullChunks != 0),
            "Runtime hit-round sum overflow") / 2,
        "Runtime hit-round sum overflow");
    const std::uint64_t tail = checkedMultiply(
        fullChunks, remainder, "Runtime hit-round tail overflow");
    return checkedAdd(
        occupiedEntries,
        checkedAdd(complete, tail, "Runtime hit-round sum overflow"),
        "Runtime hit-round sum overflow");
}

std::uint64_t hitExtraForPrefix(
    const SramRecamConfig &config,
    std::uint64_t entryCount,
    std::uint64_t parallelism)
{
    std::uint64_t total = 0;
    const std::uint64_t rounds = ceilDiv(entryCount, parallelism);
    for (std::uint64_t round = 1; round <= rounds; ++round)
    {
        const std::uint64_t before = (round - 1) * parallelism;
        const std::uint64_t entriesInRound = std::min(
            parallelism, entryCount - before);
        const std::uint64_t repairPath = checkedAdd(
            checkedMultiply(
                round, chunkCycles(config),
                "Runtime repair-path cycle overflow"),
            checkedAdd(
                config.runtimeDataReadCycles,
                config.runtimeMuxCycles,
                "Runtime repair-path cycle overflow"),
            "Runtime repair-path cycle overflow");
        const std::uint64_t exposed = repairPath > config.normalDramReadyCycles
            ? repairPath - config.normalDramReadyCycles
            : 0;
        total = checkedAdd(
            total,
            checkedMultiply(
                entriesInRound, exposed,
                "Runtime exposed-latency sum overflow"),
            "Runtime exposed-latency sum overflow");
    }
    return total;
}

} // namespace

const char *toString(SearchPolicy policy) noexcept
{
    switch (policy)
    {
        case SearchPolicy::CamBaseline: return "cam_baseline";
        case SearchPolicy::SramSerial: return "sram_serial";
        case SearchPolicy::SramChunked: return "sram_chunked";
        case SearchPolicy::SramWide: return "sram_wide";
        case SearchPolicy::SramHash: return "sram_hash";
        case SearchPolicy::SramBloomChunked:
            return "sram_bloom_chunked";
    }
    return "unknown";
}

const char *toString(MatrixPolicy policy) noexcept
{
    switch (policy)
    {
        case MatrixPolicy::SramScanRebuild: return "sram_scan_rebuild";
        case MatrixPolicy::ShadowMatrix: return "shadow_matrix";
    }
    return "unknown";
}

void SramRecamConfig::validate() const
{
    if (rows == 0 || cols == 0 ||
        (spareRows == 0 && spareCols == 0) ||
        channels == 0 || dataWordBits == 0)
    {
        throw std::invalid_argument(
            "SRAM_RECAM memory dimensions, channels, and word width must be "
            "positive, and Rs/Cs cannot both be zero");
    }
    if (addressParallelism == 0 || hybridReadParallelism == 0 ||
        runtimeParallelism == 0)
    {
        throw std::invalid_argument(
            "SRAM_RECAM parallelism values must be positive");
    }
    if (biraSearchPolicy == SearchPolicy::SramHash)
    {
        throw std::invalid_argument(
            "SRAM_HASH is a runtime-only optional policy");
    }
    if (runtimeSearchPolicy == SearchPolicy::SramBloomChunked ||
        biraSearchPolicy == SearchPolicy::SramBloomChunked ||
        runtimeSearchPolicy == SearchPolicy::SramHash)
    {
        throw std::invalid_argument(
            "Bloom/hash policies are reserved for the optional SRAM_RECAM "
            "phase and are not enabled in the initial model");
    }
}

SramRecamGeometry deriveGeometry(const SramRecamConfig &config)
{
    config.validate();
    SramRecamGeometry result;
    dynamic_spare::RecamGeometryConfig commonConfig;
    commonConfig.rows = config.rows;
    commonConfig.columns = config.cols;
    commonConfig.spareRows = config.spareRows;
    commonConfig.spareColumns = config.spareCols;
    commonConfig.channels = config.channels;
    commonConfig.dataWordBits = config.dataWordBits;
    commonConfig.onlineReuseEntries = config.camReuseEntries;
    commonConfig.minimumAddressEntryBits = config.minimumAddressEntryBits;
    static_cast<dynamic_spare::RecamGeometry &>(result) =
        dynamic_spare::deriveRecamGeometry(commonConfig);

    const std::uint64_t addressP = effectiveParallelism(
        config.biraSearchPolicy,
        config.addressParallelism,
        result.addressEntries);
    const std::uint64_t hybridP = result.hybridEntries == 0
        ? 0
        : std::min<std::uint64_t>(
              config.hybridReadParallelism, result.hybridEntries);
    const std::uint64_t runtimeP = effectiveParallelism(
        config.runtimeSearchPolicy,
        config.runtimeParallelism,
        result.runtimeEntries);
    result.addressSram = organization(
        result.addressEntries, result.addressEntryBits, addressP);
    result.hybridSram = organization(
        result.hybridEntries, result.hybridEntryBits, hybridP);
    result.runtimeSram = organization(
        result.runtimeEntries, result.runtimeEntryBits, runtimeP);

    result.rowComparatorCount = addressP;
    result.columnComparatorCount = addressP;
    result.runtimeComparatorCount = runtimeP;
    result.comparatorBitCount = checkedAdd(
        checkedAdd(
            checkedMultiply(
                result.rowComparatorCount,
                result.rowAddressBits,
                "Row comparator bit count overflow"),
            checkedMultiply(
                result.columnComparatorCount,
                result.columnAddressBits,
                "Column comparator bit count overflow"),
            "BIRA comparator bit count overflow"),
        checkedMultiply(
            result.runtimeComparatorCount,
            result.runtimeKeyBits,
            "Runtime comparator bit count overflow"),
        "Total comparator bit count overflow");
    return result;
}

AddressSramSearchEngine::AddressSramSearchEngine(std::size_t capacity)
    : entries_(capacity)
{
}

bool AddressSramSearchEngine::insert(const AddressSramEntry &entry)
{
    const auto freeEntry = std::find_if(
        entries_.begin(), entries_.end(),
        [](const AddressSramEntry &candidate) { return !candidate.valid; });
    if (freeEntry == entries_.end())
    {
        return false;
    }
    *freeEntry = entry;
    freeEntry->valid = true;
    return true;
}

AddressMatchResult AddressSramSearchEngine::search(
    std::uint32_t row,
    std::uint32_t col,
    SearchPolicy policy,
    std::uint32_t configuredParallelism) const
{
    AddressMatchResult result;
    const std::size_t occupied = size();
    result.occupiedEntries = occupied;
    if (occupied == 0)
    {
        return result;
    }
    const std::uint64_t parallelism = effectiveParallelism(
        policy, configuredParallelism, occupied);
    const std::uint64_t fullChunks = ceilDiv(occupied, parallelism);
    for (std::size_t begin = 0; begin < occupied;
         begin += static_cast<std::size_t>(parallelism))
    {
        ++result.chunksRead;
        const std::size_t end = std::min(
            occupied,
            begin + static_cast<std::size_t>(parallelism));
        for (std::size_t index = begin; index < end; ++index)
        {
            const AddressSramEntry &entry = entries_[index];
            if (!entry.valid)
            {
                throw std::logic_error(
                    "Address SRAM occupied prefix contains a free entry");
            }
            ++result.entriesCompared;
            if (entry.row == row)
            {
                result.rowMatch = true;
                result.rowMatchIndex = static_cast<int>(index);
            }
            if (entry.col == col)
            {
                result.colMatch = true;
                result.colMatchIndex = static_cast<int>(index);
            }
        }
        // Strict RECAM equivalence requires both independently relevant
        // matches.  A single row or column hit cannot terminate the scan
        // because the other match may reside in a later entry.
        if (result.rowMatch && result.colMatch)
        {
            result.earlyTerminated = result.chunksRead < fullChunks;
            break;
        }
    }
    return result;
}

std::size_t AddressSramSearchEngine::size() const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        entries_.begin(), entries_.end(),
        [](const AddressSramEntry &entry) { return entry.valid; }));
}

std::size_t AddressSramSearchEngine::capacity() const noexcept
{
    return entries_.size();
}

const std::vector<AddressSramEntry> &
AddressSramSearchEngine::entries() const noexcept
{
    return entries_;
}

BiraLatencyStats modelBiraLatency(
    const SramRecamConfig &config,
    const BiraWorkload &workload)
{
    const SramRecamGeometry geometry = deriveGeometry(config);
    BiraLatencyStats result;
    result.faultsDetected = workload.faultsDetected;
    if (workload.addressSearchRoundsByFault.size() !=
        workload.faultsDetected)
    {
        throw std::invalid_argument(
            "SRAM_RECAM requires one occupied-scan round count per fault");
    }
    for (const std::uint64_t rounds :
         workload.addressSearchRoundsByFault)
    {
        result.addressSearchRounds = checkedAdd(
            result.addressSearchRounds,
            rounds,
            "Address search round count overflow");
        result.addressSearchRoundsMaximum = std::max(
            result.addressSearchRoundsMaximum, rounds);
        result.faultCollectionServiceCycles.push_back(checkedAdd(
            checkedMultiply(
                rounds, chunkCycles(config),
                "SRAM per-fault search cycle count overflow"),
            config.stateUpdateCycles,
            "SRAM per-fault collection cycle count overflow"));
    }
    result.addressSramReads = result.addressSearchRounds;
    result.addressSearchCycles = checkedMultiply(
        result.addressSearchRounds, chunkCycles(config),
        "Address search cycle count overflow");
    result.addressSearchCyclesMaximum = checkedMultiply(
        result.addressSearchRoundsMaximum, chunkCycles(config),
        "Maximum Address search cycle count overflow");
    result.addressWrites = workload.addressWrites;
    result.hybridWrites = workload.hybridWrites;
    result.tempBufferWrites = workload.tempBufferWrites;
    result.addressEntryCount = workload.addressEntryCount;
    result.hybridEntryCount = workload.hybridEntryCount;
    result.tempEntryCount = workload.tempEntryCount;
    result.earlyTerminationHitCount =
        workload.earlyTerminationHitCount;
    result.hybridBitmapAllocations = workload.hybridBitmapAllocations;
    result.hybridFullCount = workload.hybridFullCount;
    result.pivotFaults = workload.pivotFaults;
    result.nonpivotFaults = workload.nonpivotFaults;
    result.mustRepairTriggeringFaults =
        workload.mustRepairTriggeringFaults;
    result.pivotSearchRounds = workload.pivotSearchRounds;
    result.nonpivotSearchRounds = workload.nonpivotSearchRounds;
    result.mustRepairSearchRounds = workload.mustRepairSearchRounds;
    result.addressEntriesCompared = workload.addressEntriesCompared;
    result.pivotSearchCycles = checkedMultiply(
        result.pivotSearchRounds, chunkCycles(config),
        "Pivot search cycle count overflow");
    result.nonpivotSearchCycles = checkedMultiply(
        result.nonpivotSearchRounds, chunkCycles(config),
        "Nonpivot search cycle count overflow");
    result.mustRepairSearchCycles = checkedMultiply(
        result.mustRepairSearchRounds, chunkCycles(config),
        "Must-repair search cycle count overflow");

    const std::uint64_t passes = workload.analysisPasses;
    if (config.matrixPolicy == MatrixPolicy::SramScanRebuild)
    {
        result.matrixAddressReadRounds = checkedMultiply(
            passes,
            ceilDiv(
                geometry.addressEntries,
                geometry.addressSram.parallelEntries),
            "Matrix Address-SRAM round count overflow");
        result.matrixHybridReadRounds = geometry.hybridEntries == 0
            ? 0
            : checkedMultiply(
                  passes,
                  ceilDiv(
                      geometry.hybridEntries,
                      geometry.hybridSram.parallelEntries),
                  "Matrix Hybrid-SRAM round count overflow");
        result.matrixFinalizeCycles = checkedMultiply(
            passes,
            config.matrixFinalizeCycles,
            "Matrix finalize cycle count overflow");
    }
    if (config.matrixPolicy == MatrixPolicy::SramScanRebuild)
    {
        const std::uint64_t solutionParallelism =
            config.solutionParallelism == 0
                ? geometry.solutionUnits
                : std::min<std::uint64_t>(
                      config.solutionParallelism, geometry.solutionUnits);
        result.analysisRounds = checkedMultiply(
            passes,
            ceilDiv(geometry.solutionUnits, solutionParallelism),
            "Solution-analysis round count overflow");
    }
    result.comparatorEvaluations = checkedMultiply(
        workload.addressEntriesCompared,
        2,
        "Comparator evaluation count overflow");
    result.comparatorBitComparisons = checkedMultiply(
        workload.addressEntriesCompared,
        static_cast<std::uint64_t>(geometry.rowAddressBits) +
            geometry.columnAddressBits,
        "Comparator bit-work count overflow");
    result.stateUpdateCycles = checkedMultiply(
        workload.faultsDetected,
        config.stateUpdateCycles,
        "State-update cycle count overflow");

    const std::uint64_t matrixReadRounds = checkedAdd(
        result.matrixAddressReadRounds,
        result.matrixHybridReadRounds,
        "Matrix read round count overflow");
    for (const std::uint64_t serviceCycles :
         result.faultCollectionServiceCycles)
    {
        result.faultCollectionCycles = checkedAdd(
            result.faultCollectionCycles,
            serviceCycles,
            "SRAM fault-collection cycle count overflow");
    }
    result.repairAnalysisCycles = checkedAdd(
        checkedMultiply(
            matrixReadRounds,
            chunkCycles(config),
            "SRAM matrix-read cycle count overflow"),
        checkedAdd(
            result.matrixFinalizeCycles,
            result.analysisRounds,
            "SRAM repair-analysis cycle count overflow"),
        "SRAM repair-analysis cycle count overflow");
    result.modeledCycles = checkedAdd(
        result.faultCollectionCycles,
        result.repairAnalysisCycles,
        "Total BIRA cycle count overflow");
    return result;
}

dynamic_spare::BiraLatencyMetrics modelDecoupledFifoTimeline(
    const SramRecamConfig &config,
    const BiraWorkload &workload,
    const std::vector<std::uint64_t> &arrivalCycles,
    std::uint64_t bistCompletionCycle)
{
    if (arrivalCycles.size() != workload.faultsDetected)
    {
        throw std::invalid_argument(
            "SRAM FIFO arrival count must equal faultsDetected");
    }

    const BiraLatencyStats aggregate = modelBiraLatency(config, workload);
    std::uint64_t finalAnalysis = aggregate.repairAnalysisCycles;
    std::vector<std::uint64_t> serviceCycles =
        aggregate.faultCollectionServiceCycles;
    if (workload.faultsDetected != 0 && config.analyzeAfterEachFault)
    {
        if (aggregate.repairAnalysisCycles % workload.faultsDetected != 0)
        {
            throw std::logic_error(
                "Per-fault SRAM analysis work is not uniform");
        }
        for (std::uint64_t &service : serviceCycles)
        {
            service = checkedAdd(
                service,
                aggregate.repairAnalysisCycles / workload.faultsDetected,
                "SRAM FIFO per-fault service overflow");
        }
        finalAnalysis = 0;
    }

    std::vector<dynamic_spare::BiraFaultArrivalEvent> events;
    events.reserve(arrivalCycles.size());
    for (std::size_t index = 0; index < arrivalCycles.size(); ++index)
    {
        events.push_back({arrivalCycles[index], serviceCycles[index], index});
    }
    dynamic_spare::DecoupledFifoConfig fifo;
    fifo.bistCompletionCycle = bistCompletionCycle;
    fifo.finalRepairAnalysisCycles = finalAnalysis;
    return dynamic_spare::simulateDecoupledFifo(
        dynamic_spare::BiraStorageTechnology::Sram,
        std::move(events),
        fifo);
}

void accumulate(
    BiraLatencyStats &destination,
    const BiraLatencyStats &source)
{
#define SRAM_RECAM_ACCUMULATE(field) \
    destination.field = checkedAdd( \
        destination.field, source.field, "BIRA aggregate overflow")
    SRAM_RECAM_ACCUMULATE(faultsDetected);
    SRAM_RECAM_ACCUMULATE(addressSearchRounds);
    destination.addressSearchRoundsMaximum = std::max(
        destination.addressSearchRoundsMaximum,
        source.addressSearchRoundsMaximum);
    SRAM_RECAM_ACCUMULATE(addressSearchCycles);
    destination.addressSearchCyclesMaximum = std::max(
        destination.addressSearchCyclesMaximum,
        source.addressSearchCyclesMaximum);
    SRAM_RECAM_ACCUMULATE(addressSramReads);
    SRAM_RECAM_ACCUMULATE(addressWrites);
    SRAM_RECAM_ACCUMULATE(hybridWrites);
    SRAM_RECAM_ACCUMULATE(tempBufferWrites);
    SRAM_RECAM_ACCUMULATE(matrixAddressReadRounds);
    SRAM_RECAM_ACCUMULATE(matrixHybridReadRounds);
    SRAM_RECAM_ACCUMULATE(analysisRounds);
    SRAM_RECAM_ACCUMULATE(comparatorEvaluations);
    SRAM_RECAM_ACCUMULATE(comparatorBitComparisons);
    SRAM_RECAM_ACCUMULATE(stateUpdateCycles);
    SRAM_RECAM_ACCUMULATE(matrixFinalizeCycles);
    SRAM_RECAM_ACCUMULATE(faultCollectionCycles);
    SRAM_RECAM_ACCUMULATE(repairAnalysisCycles);
    SRAM_RECAM_ACCUMULATE(modeledCycles);
    SRAM_RECAM_ACCUMULATE(addressEntryCount);
    SRAM_RECAM_ACCUMULATE(hybridEntryCount);
    SRAM_RECAM_ACCUMULATE(tempEntryCount);
    SRAM_RECAM_ACCUMULATE(earlyTerminationHitCount);
    SRAM_RECAM_ACCUMULATE(hybridBitmapAllocations);
    SRAM_RECAM_ACCUMULATE(hybridFullCount);
    SRAM_RECAM_ACCUMULATE(pivotFaults);
    SRAM_RECAM_ACCUMULATE(nonpivotFaults);
    SRAM_RECAM_ACCUMULATE(mustRepairTriggeringFaults);
    SRAM_RECAM_ACCUMULATE(pivotSearchRounds);
    SRAM_RECAM_ACCUMULATE(nonpivotSearchRounds);
    SRAM_RECAM_ACCUMULATE(mustRepairSearchRounds);
    SRAM_RECAM_ACCUMULATE(addressEntriesCompared);
    SRAM_RECAM_ACCUMULATE(pivotSearchCycles);
    SRAM_RECAM_ACCUMULATE(nonpivotSearchCycles);
    SRAM_RECAM_ACCUMULATE(mustRepairSearchCycles);
#undef SRAM_RECAM_ACCUMULATE
}

bool RuntimeKey::operator==(const RuntimeKey &other) const noexcept
{
    return channel == other.channel && row == other.row && col == other.col;
}

SramRuntimeRepairTable::SramRuntimeRepairTable(
    std::size_t capacity,
    std::uint32_t wordBits,
    SearchPolicy policy,
    std::uint32_t parallelism)
    : wordBits_(wordBits),
      policy_(policy),
      parallelism_(parallelism),
      entries_(capacity)
{
    if (wordBits_ == 0 || parallelism_ == 0)
    {
        throw std::invalid_argument(
            "Runtime SRAM word width and parallelism must be positive");
    }
    if (policy_ == SearchPolicy::SramHash ||
        policy_ == SearchPolicy::SramBloomChunked)
    {
        throw std::invalid_argument(
            "Optional hash/Bloom runtime backends are not enabled yet");
    }
}

bool SramRuntimeRepairTable::insert(
    const RuntimeKey &key,
    const RuntimeData &data)
{
    if (data.size() != wordBits_)
    {
        throw std::invalid_argument(
            "Runtime repair data width does not match the configured word");
    }
    for (RuntimeRepairEntry &entry : entries_)
    {
        if (entry.valid && entry.key == key)
        {
            entry.data = data;
            return true;
        }
    }
    const auto freeEntry = std::find_if(
        entries_.begin(), entries_.end(),
        [](const RuntimeRepairEntry &entry) { return !entry.valid; });
    if (freeEntry == entries_.end())
    {
        return false;
    }
    freeEntry->valid = true;
    freeEntry->key = key;
    freeEntry->data = data;
    return true;
}

RuntimeLookupResult SramRuntimeRepairTable::lookup(
    const RuntimeKey &key) const
{
    RuntimeLookupResult result;
    const std::uint64_t parallelism = effectiveParallelism(
        policy_, parallelism_, entries_.size());
    if (entries_.empty())
    {
        return result;
    }
    for (std::size_t begin = 0; begin < entries_.size();
         begin += static_cast<std::size_t>(parallelism))
    {
        ++result.rounds;
        const std::size_t end = std::min(
            entries_.size(),
            begin + static_cast<std::size_t>(parallelism));
        for (std::size_t index = begin; index < end; ++index)
        {
            const RuntimeRepairEntry &entry = entries_[index];
            if (!entry.valid)
            {
                continue;
            }
            ++result.entriesCompared;
            if (entry.key == key)
            {
                result.hit = true;
                result.matchIndex = static_cast<int>(index);
                result.data = entry.data;
                return result;
            }
        }
    }
    return result;
}

RuntimeLookupResult SramRuntimeRepairTable::read(
    const RuntimeKey &key,
    const RuntimeData &normalData) const
{
    if (normalData.size() != wordBits_)
    {
        throw std::invalid_argument(
            "Normal runtime data width does not match the configured word");
    }
    RuntimeLookupResult result = lookup(key);
    if (!result.hit)
    {
        result.data = normalData;
    }
    return result;
}

RuntimeLookupResult SramRuntimeRepairTable::write(
    const RuntimeKey &key,
    const RuntimeData &data)
{
    if (data.size() != wordBits_)
    {
        throw std::invalid_argument(
            "Runtime write data width does not match the configured word");
    }
    RuntimeLookupResult result = lookup(key);
    if (result.hit)
    {
        entries_[static_cast<std::size_t>(result.matchIndex)].data = data;
        result.data = data;
    }
    return result;
}

std::size_t SramRuntimeRepairTable::size() const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        entries_.begin(), entries_.end(),
        [](const RuntimeRepairEntry &entry) { return entry.valid; }));
}

std::size_t SramRuntimeRepairTable::capacity() const noexcept
{
    return entries_.size();
}

RuntimeLatencyStats modelRuntimeLatency(
    const SramRecamConfig &config,
    std::uint64_t requests,
    std::uint64_t hits,
    std::uint64_t occupiedEntries)
{
    const SramRecamGeometry geometry = deriveGeometry(config);
    if (hits > requests || occupiedEntries > geometry.runtimeEntries ||
        (hits != 0 && occupiedEntries == 0))
    {
        throw std::invalid_argument("Invalid SRAM_RECAM runtime workload");
    }
    RuntimeLatencyStats result;
    result.requests = requests;
    result.hits = hits;
    result.misses = requests - hits;
    if (requests == 0 || geometry.runtimeEntries == 0)
    {
        return result;
    }

    const std::uint64_t parallelism =
        geometry.runtimeSram.parallelEntries;
    const std::uint64_t missRoundsPerRequest = ceilDiv(
        geometry.runtimeEntries, parallelism);
    result.missRounds = checkedMultiply(
        result.misses,
        missRoundsPerRequest,
        "Runtime miss round count overflow");

    // A completely empty repair table is a valid all-miss workload.  Keep
    // the miss latency based on the provisioned SRAM depth, but avoid using
    // the zero occupancy as a divisor when there are no hits.
    const std::uint64_t fullHitPeriods = hits == 0
        ? 0
        : hits / occupiedEntries;
    const std::uint64_t partialHits = hits == 0
        ? 0
        : hits % occupiedEntries;
    result.hitRounds = checkedAdd(
        checkedMultiply(
            fullHitPeriods,
            sumHitRounds(occupiedEntries, parallelism),
            "Runtime hit round count overflow"),
        sumHitRounds(partialHits, parallelism),
        "Runtime hit round count overflow");
    result.lookupRounds = checkedAdd(
        result.hitRounds,
        result.missRounds,
        "Runtime lookup round count overflow");

    const std::uint64_t fullHitExtra = hitExtraForPrefix(
        config, occupiedEntries, parallelism);
    const std::uint64_t partialHitExtra = partialHits == 0
        ? 0
        : hitExtraForPrefix(config, partialHits, parallelism);
    const std::uint64_t missRepairPath = checkedAdd(
        checkedMultiply(
            missRoundsPerRequest,
            chunkCycles(config),
            "Runtime miss-path cycle overflow"),
        config.runtimeMuxCycles,
        "Runtime miss-path cycle overflow");
    const std::uint64_t missExtra = missRepairPath >
            config.normalDramReadyCycles
        ? missRepairPath - config.normalDramReadyCycles
        : 0;
    result.exposedExtraCycles = checkedAdd(
        checkedAdd(
            checkedMultiply(
                fullHitPeriods,
                fullHitExtra,
                "Runtime hit extra-latency overflow"),
            partialHitExtra,
            "Runtime hit extra-latency overflow"),
        checkedMultiply(
            result.misses,
            missExtra,
            "Runtime miss extra-latency overflow"),
        "Runtime total extra-latency overflow");

    result.lookupRoundsAverage = static_cast<double>(result.lookupRounds) /
        static_cast<double>(requests);
    result.hitRoundsAverage = hits == 0
        ? 0.0
        : static_cast<double>(result.hitRounds) /
              static_cast<double>(hits);
    result.missRoundsAverage = result.misses == 0
        ? 0.0
        : static_cast<double>(result.missRounds) /
              static_cast<double>(result.misses);
    result.exposedExtraCyclesAverage =
        static_cast<double>(result.exposedExtraCycles) /
        static_cast<double>(requests);
    return result;
}

HardwareCostDescriptor deriveHardwareCost(
    const SramRecamConfig &config,
    const AreaProxyParameters &parameters)
{
    if (!std::isfinite(parameters.sramBitAreaUnits) ||
        !std::isfinite(parameters.comparatorBitAreaUnits) ||
        !std::isfinite(parameters.fixedControllerAreaUnits) ||
        parameters.sramBitAreaUnits < 0.0 ||
        parameters.comparatorBitAreaUnits < 0.0 ||
        parameters.fixedControllerAreaUnits < 0.0)
    {
        throw std::invalid_argument(
            "SRAM_RECAM area-proxy coefficients must be finite and "
            "nonnegative");
    }
    HardwareCostDescriptor result;
    result.geometry = deriveGeometry(config);
    const std::uint64_t packedStorageBits = checkedAdd(
        checkedAdd(
            result.geometry.addressSram.physicalBits,
            result.geometry.hybridSram.physicalBits,
            "Packed SRAM storage bit count overflow"),
        checkedAdd(
            result.geometry.tempBufferBits,
            result.geometry.matrixBits,
            "Auxiliary storage bit count overflow"),
        "Total packed storage bit count overflow");
    result.normalizedSramArea =
        static_cast<double>(packedStorageBits) *
        parameters.sramBitAreaUnits;
    result.normalizedComparatorArea =
        static_cast<double>(result.geometry.comparatorBitCount) *
        parameters.comparatorBitAreaUnits;
    result.normalizedAreaProxy = result.normalizedSramArea +
        result.normalizedComparatorArea +
        parameters.fixedControllerAreaUnits;
    return result;
}

dynamic_spare::HardwareMetrics deriveCommonHardwareMetrics(
    const SramRecamConfig &config,
    bool offlineOnlineModeReused)
{
    const SramRecamGeometry geometry = deriveGeometry(config);
    dynamic_spare::HardwareMetrics result;
    result.storageTechnology = dynamic_spare::BiraStorageTechnology::Sram;
    result.geometry = static_cast<const dynamic_spare::RecamGeometry &>(
        geometry);
    result.addressPhysicalBits = geometry.addressSram.physicalBits;
    result.hybridPhysicalBits = geometry.hybridSram.physicalBits;
    result.temporaryPhysicalBits = geometry.tempBufferBits;
    result.runtimePhysicalBits = geometry.runtimeSram.physicalBits;
    result.matrixBits = geometry.matrixBits;
    result.comparatorBits = geometry.comparatorBitCount;
    result.offlineOnlineModeReused = offlineOnlineModeReused;
    dynamic_spare::finalizeHardwareMetrics(result);
    return result;
}

void writeHardwareCostJson(
    const std::filesystem::path &path,
    const SramRecamConfig &config,
    const HardwareCostDescriptor &descriptor)
{
    if (path.has_parent_path())
    {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream output(path);
    if (!output.is_open())
    {
        throw std::runtime_error(
            "Unable to write SRAM_RECAM hardware-cost JSON: " +
            path.string());
    }
    const SramRecamGeometry &g = descriptor.geometry;
    output << std::setprecision(12)
        << "{\n"
        << "  \"architecture\": \"SRAM_RECAM\",\n"
        << "  \"search_policy\": \""
        << toString(config.biraSearchPolicy) << "\",\n"
        << "  \"matrix_policy\": \""
        << toString(config.matrixPolicy) << "\",\n"
        << "  \"area_proxy_is_physical_area\": false,\n"
        << "  \"address_sram\": {\"depth\": "
        << g.addressSram.physicalDepth << ", \"width\": "
        << g.addressSram.physicalWidth << ", \"logical_bits\": "
        << g.addressSram.logicalBits << ", \"physical_packed_bits\": "
        << g.addressSram.physicalBits << "},\n"
        << "  \"hybrid_sram\": {\"depth\": "
        << g.hybridSram.physicalDepth << ", \"width\": "
        << g.hybridSram.physicalWidth << ", \"logical_bits\": "
        << g.hybridSram.logicalBits << ", \"physical_packed_bits\": "
        << g.hybridSram.physicalBits << "},\n"
        << "  \"runtime_sram\": {\"depth\": "
        << g.runtimeSram.physicalDepth << ", \"width\": "
        << g.runtimeSram.physicalWidth << ", \"logical_bits\": "
        << g.runtimeSram.logicalBits << ", \"physical_packed_bits\": "
        << g.runtimeSram.physicalBits << "},\n"
        << "  \"temporary_buffer_bits\": " << g.tempBufferBits << ",\n"
        << "  \"matrix_bits\": " << g.matrixBits << ",\n"
        << "  \"solution_units\": " << g.solutionUnits << ",\n"
        << "  \"comparators\": {\n"
        << "    \"row\": {\"count\": " << g.rowComparatorCount
        << ", \"width\": " << g.rowAddressBits << "},\n"
        << "    \"column\": {\"count\": "
        << g.columnComparatorCount << ", \"width\": "
        << g.columnAddressBits << "},\n"
        << "    \"runtime\": {\"count\": "
        << g.runtimeComparatorCount << ", \"width\": "
        << g.runtimeKeyBits << "}\n"
        << "  },\n"
        << "  \"normalized_sram_area\": "
        << descriptor.normalizedSramArea << ",\n"
        << "  \"normalized_comparator_area\": "
        << descriptor.normalizedComparatorArea << ",\n"
        << "  \"normalized_total_area_proxy\": "
        << descriptor.normalizedAreaProxy << "\n"
        << "}\n";
    if (!output)
    {
        throw std::runtime_error(
            "Failed while writing SRAM_RECAM hardware-cost JSON: " +
            path.string());
    }
}

} // namespace sram_recam
