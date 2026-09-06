#include "../inc/SramRecamSolverAdapter.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace dynamic_spare
{
namespace
{

struct ClassificationCounts
{
    std::size_t addressPivots = 0;
    std::size_t nonpivots = 0;
    std::size_t bufferedPivots = 0;
    std::size_t overflowPivots = 0;
    std::uint64_t chunksRead = 0;
    std::uint64_t maximumChunksRead = 0;
    std::uint64_t entriesCompared = 0;
    std::uint64_t earlyTerminationHits = 0;
    std::uint64_t hybridBitmapAllocations = 0;
    std::uint64_t hybridFullCount = 0;
    std::uint64_t hybridEntries = 0;
    std::uint64_t mustRepairTriggeringFaults = 0;
    std::uint64_t pivotSearchRounds = 0;
    std::uint64_t nonpivotSearchRounds = 0;
    std::uint64_t mustRepairSearchRounds = 0;
    std::vector<std::uint64_t> searchRoundsByFault;
};

struct HybridBitmapEntry
{
    bool valid = false;
    std::size_t pointer = 0;
    bool descriptorRowIsDiff = false;
};

ClassificationCounts replayAddressSram(
    const std::vector<Fault> &faults,
    std::size_t addressCapacity,
    std::size_t bufferCapacity,
    const sram_recam::SramRecamConfig &config)
{
    sram_recam::AddressSramSearchEngine address(addressCapacity);
    ClassificationCounts counts;
    std::vector<bool> nonpivot(faults.size(), false);
    std::vector<bool> mustTrigger(faults.size(), false);
    for (std::size_t faultIndex = 0; faultIndex < faults.size(); ++faultIndex)
    {
        const Fault &fault = faults[faultIndex];
        if (fault.r < 0 || fault.c < 0 ||
            static_cast<std::uint64_t>(fault.r) >= config.rows ||
            static_cast<std::uint64_t>(fault.c) >= config.cols)
        {
            throw std::invalid_argument(
                "Fault address is outside the SRAM_RECAM repair domain");
        }
        const auto match = address.search(
            static_cast<std::uint32_t>(fault.r),
            static_cast<std::uint32_t>(fault.c),
            config.biraSearchPolicy,
            config.addressParallelism);
        counts.chunksRead += match.chunksRead;
        counts.maximumChunksRead = std::max(
            counts.maximumChunksRead, match.chunksRead);
        counts.entriesCompared += match.entriesCompared;
        counts.searchRoundsByFault.push_back(match.chunksRead);
        counts.earlyTerminationHits += match.earlyTerminated ? 1 : 0;
        if (match.rowMatch || match.colMatch)
        {
            ++counts.nonpivots;
            nonpivot[faultIndex] = true;
            continue;
        }

        sram_recam::AddressSramEntry entry;
        entry.valid = true;
        entry.row = static_cast<std::uint32_t>(fault.r);
        entry.col = static_cast<std::uint32_t>(fault.c);
        entry.rowFailCount = 1;
        entry.colFailCount = 1;
        if (address.insert(entry))
        {
            ++counts.addressPivots;
        }
        else if (counts.bufferedPivots < bufferCapacity)
        {
            ++counts.bufferedPivots;
        }
        else
        {
            ++counts.overflowPivots;
        }
    }

    // Reproduce the legacy Hybrid-CAM update order with a free-entry bitmap.
    // Search latency was already measured in arrival order above; this pass
    // only determines must transitions, allocations, reclamation, and full
    // events without adding Hybrid content-search cycles.
    const std::size_t hybridCapacity = config.spareRows == 0 ||
            config.spareCols == 0
        ? 0
        : static_cast<std::size_t>(config.spareRows) *
              (config.spareCols - 1) +
              static_cast<std::size_t>(config.spareCols) *
              (config.spareRows - 1);
    std::vector<HybridBitmapEntry> hybrid(hybridCapacity);
    std::vector<sram_recam::AddressSramEntry> addressState =
        address.entries();
    auto allocateHybrid = [&](std::size_t pointer, bool rowIsDiff)
    {
        const auto freeEntry = std::find_if(
            hybrid.begin(), hybrid.end(),
            [](const HybridBitmapEntry &entry) { return !entry.valid; });
        if (freeEntry == hybrid.end())
        {
            ++counts.hybridFullCount;
            return;
        }
        freeEntry->valid = true;
        freeEntry->pointer = pointer;
        freeEntry->descriptorRowIsDiff = rowIsDiff;
        ++counts.hybridBitmapAllocations;
    };

    for (std::size_t faultIndex = 0; faultIndex < faults.size(); ++faultIndex)
    {
        if (!nonpivot[faultIndex])
        {
            continue;
        }
        const Fault &fault = faults[faultIndex];
        for (std::size_t index = 0; index < address.size(); ++index)
        {
            auto &entry = addressState[index];
            const bool rowMatch = entry.row ==
                static_cast<std::uint32_t>(fault.r);
            const bool colMatch = entry.col ==
                static_cast<std::uint32_t>(fault.c);
            if (rowMatch)
            {
                if (entry.rowMust)
                {
                    break;
                }
                ++entry.rowFailCount;
                if (entry.rowFailCount > config.spareCols)
                {
                    entry.rowMust = true;
                    mustTrigger[faultIndex] = true;
                    for (HybridBitmapEntry &candidate : hybrid)
                    {
                        if (candidate.valid && candidate.pointer == index &&
                            !candidate.descriptorRowIsDiff)
                        {
                            candidate.valid = false;
                        }
                    }
                }
                else
                {
                    allocateHybrid(index, false);
                }
                break;
            }
            if (colMatch)
            {
                if (entry.colMust)
                {
                    break;
                }
                ++entry.colFailCount;
                if (entry.colFailCount > config.spareRows)
                {
                    entry.colMust = true;
                    mustTrigger[faultIndex] = true;
                    for (HybridBitmapEntry &candidate : hybrid)
                    {
                        if (candidate.valid && candidate.pointer == index &&
                            candidate.descriptorRowIsDiff)
                        {
                            candidate.valid = false;
                        }
                    }
                }
                else
                {
                    allocateHybrid(index, true);
                }
                break;
            }
        }
    }
    counts.hybridEntries = static_cast<std::uint64_t>(std::count_if(
        hybrid.begin(), hybrid.end(),
        [](const HybridBitmapEntry &entry) { return entry.valid; }));
    for (std::size_t index = 0; index < faults.size(); ++index)
    {
        const std::uint64_t rounds = counts.searchRoundsByFault[index];
        if (nonpivot[index])
        {
            counts.nonpivotSearchRounds += rounds;
        }
        else
        {
            counts.pivotSearchRounds += rounds;
        }
        if (mustTrigger[index])
        {
            ++counts.mustRepairTriggeringFaults;
            counts.mustRepairSearchRounds += rounds;
        }
    }
    return counts;
}

} // namespace

SramRecamSolverAdapter::SramRecamSolverAdapter(
    sram_recam::SramRecamConfig config)
    : config_(std::move(config))
{
    config_.validate();
}

RepairAttemptResult SramRecamSolverAdapter::solve(
    const std::vector<Fault> &faults,
    const RECAMSolverRequest &request) const
{
    RepairAttemptResult result = golden_.solve(faults, request);

    sram_recam::SramRecamConfig attemptConfig = config_;
    attemptConfig.spareRows = static_cast<std::uint32_t>(
        request.availableRows);
    attemptConfig.spareCols = static_cast<std::uint32_t>(
        request.availableColumns);
    attemptConfig.camReuseEntries = static_cast<std::uint32_t>(
        request.bufferCamEntries);
    attemptConfig.validate();

    const std::size_t addressCapacity = static_cast<std::size_t>(
        request.availableRows) + static_cast<std::size_t>(
        request.availableColumns);
    const ClassificationCounts classification = replayAddressSram(
        faults,
        addressCapacity,
        static_cast<std::size_t>(request.bufferCamEntries),
        attemptConfig);
    const bool classificationMatches =
        classification.addressPivots == result.pivotFaultCount &&
        classification.nonpivots == result.nonpivotFaultCount &&
        classification.bufferedPivots == result.bufferedPivotFaultCount &&
        classification.overflowPivots == result.overflowPivotFaultCount;
    if (!classificationMatches)
    {
        throw std::logic_error(
            "SRAM_RECAM Address-SRAM classification diverged from the "
            "RECAM golden model");
    }

    sram_recam::BiraWorkload workload;
    workload.faultsDetected = result.faultCount;
    workload.addressWrites = result.pivotFaultCount;
    workload.hybridWrites = result.hybridCamWriteOperations;
    workload.tempBufferWrites = result.bufferedPivotFaultCount;
    workload.analysisPasses = attemptConfig.analyzeAfterEachFault
        ? result.faultCount
        : 1;
    workload.addressSearchRoundsByFault =
        classification.searchRoundsByFault;
    workload.addressEntryCount = result.addressCamEntriesActive;
    workload.hybridEntryCount = classification.hybridEntries;
    workload.tempEntryCount = result.bufferCamEntriesActive;
    workload.earlyTerminationHitCount =
        classification.earlyTerminationHits;
    workload.hybridBitmapAllocations =
        classification.hybridBitmapAllocations;
    workload.hybridFullCount = classification.hybridFullCount;
    workload.pivotFaults = classification.addressPivots +
        classification.bufferedPivots + classification.overflowPivots;
    workload.nonpivotFaults = classification.nonpivots;
    workload.mustRepairTriggeringFaults =
        classification.mustRepairTriggeringFaults;
    workload.pivotSearchRounds = classification.pivotSearchRounds;
    workload.nonpivotSearchRounds = classification.nonpivotSearchRounds;
    workload.mustRepairSearchRounds =
        classification.mustRepairSearchRounds;
    workload.addressEntriesCompared = classification.entriesCompared;

    sram_recam::SramRecamAttemptMetrics metrics;
    metrics.biraPolicy = attemptConfig.biraSearchPolicy;
    metrics.runtimePolicy = attemptConfig.runtimeSearchPolicy;
    metrics.matrixPolicy = attemptConfig.matrixPolicy;
    metrics.geometry = sram_recam::deriveGeometry(attemptConfig);
    metrics.hardware = sram_recam::deriveCommonHardwareMetrics(
        attemptConfig);
    metrics.bira = sram_recam::modelBiraLatency(
        attemptConfig, workload);
    metrics.addressClassificationMatchesCam = true;
    if (classification.chunksRead != metrics.bira.addressSearchRounds ||
        classification.maximumChunksRead !=
            metrics.bira.addressSearchRoundsMaximum)
    {
        throw std::logic_error(
            "SRAM_RECAM replay rounds disagree with the analytical model");
    }
    if (classification.hybridBitmapAllocations !=
            result.hybridCamWriteOperations ||
        classification.hybridEntries != result.hybridCamEntriesActive)
    {
        throw std::logic_error(
            "SRAM_RECAM Hybrid bitmap replay diverged from the RECAM "
            "golden model");
    }
    result.sramRecam = std::move(metrics);
    return result;
}

const sram_recam::SramRecamConfig &
SramRecamSolverAdapter::config() const noexcept
{
    return config_;
}

} // namespace dynamic_spare
