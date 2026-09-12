#include "../inc/DirectionalMultiConfigAnalyzer.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace dynamic_spare
{
namespace
{

const std::array<DirectionalConfigDescriptor, kDirectionalConfigCount>
    kDescriptors{{
        {DirectionalConfigId::Cfg0_2R2C, 2, 2, false},
        {DirectionalConfigId::Cfg1_2R1C, 2, 1, false},
        {DirectionalConfigId::Cfg2_3R2C, 3, 2, false},
        {DirectionalConfigId::Cfg3_3R1C, 3, 1, false},
        {DirectionalConfigId::Cfg4_1R2C, 1, 2, true},
        {DirectionalConfigId::Cfg5_2R3C, 2, 3, true},
        {DirectionalConfigId::Cfg6_1R3C, 1, 3, true},
    }};

bool isIndependentPivot(const Fault &fault, const std::vector<Fault> &pivots)
{
    return std::all_of(pivots.begin(), pivots.end(), [&fault](const Fault &p)
    {
        return fault.r != p.r && fault.c != p.c;
    });
}

} // namespace

const std::array<DirectionalConfigDescriptor, kDirectionalConfigCount> &
DirectionalMultiConfigAnalyzer::descriptors() noexcept
{
    return kDescriptors;
}

DirectionalMultiConfigAnalysis DirectionalMultiConfigAnalyzer::analyze(
    const std::vector<Fault> &faults,
    int subarrayId,
    int camReuseEntries) const
{
    if (subarrayId < 0 || subarrayId >= static_cast<int>(kSubarrayCount))
        throw std::invalid_argument("Directional analyzer subarray ID is invalid");
    if (camReuseEntries < 0)
        throw std::invalid_argument("CAM-reuse entry count cannot be negative");

    DirectionalMultiConfigAnalysis result;
    std::map<int, int> rows;
    std::map<int, int> columns;
    for (const Fault &fault : faults)
    {
        if (fault.SubarrayID != subarrayId)
            throw std::invalid_argument("Fault does not belong to requested subarray");
        ++rows[fault.r];
        ++columns[fault.c];
        if (isIndependentPivot(fault, result.greedyPivotSequence))
            result.greedyPivotSequence.push_back(fault);
    }
    for (const auto &entry : rows)
        result.rowFaultCounts.push_back(entry.second);
    for (const auto &entry : columns)
        result.columnFaultCounts.push_back(entry.second);

    result.sharedPivotOccupancy = std::min(
        result.greedyPivotSequence.size(), kDirectionalMaxPivotPayload);
    for (std::size_t index = 0; index < result.sharedPivotOccupancy; ++index)
    {
        result.pivotPayload[index].address = result.greedyPivotSequence[index];
        result.pivotPayload[index].valid = true;
    }
    if (result.greedyPivotSequence.size() > kDirectionalMaxPivotPayload)
    {
        result.camReuseOverflowBeyondPayload.assign(
            result.greedyPivotSequence.begin() + kDirectionalMaxPivotPayload,
            result.greedyPivotSequence.end());
    }

    RECAMSolverAdapter solver;
    // A physical record is shareable only when all configuration-dependent
    // fields agree.  The map value is its configuration-valid mask.
    using HybridKey = std::tuple<int, int, int, bool>;
    std::map<HybridKey, std::array<bool, kDirectionalConfigCount>> hybridUnion;
    for (std::size_t cfg = 0; cfg < kDirectionalConfigCount; ++cfg)
    {
        DirectionalConfigAnalysis &view = result.configs[cfg];
        view.descriptor = kDescriptors[cfg];
        const std::size_t capacity = static_cast<std::size_t>(
            view.descriptor.rows + view.descriptor.columns);
        for (std::size_t index = 0;
             index < result.sharedPivotOccupancy; ++index)
        {
            view.addressActive[index] = index < capacity;
        }
        if (result.sharedPivotOccupancy < capacity &&
            result.greedyPivotSequence.size() >= capacity)
        {
            throw std::logic_error("Shared pivot payload failed its required prefix invariant");
        }
        if (result.greedyPivotSequence.size() > capacity)
        {
            const auto first = result.greedyPivotSequence.begin() + capacity;
            const auto last = result.greedyPivotSequence.begin() + std::min(
                result.greedyPivotSequence.size(),
                capacity + static_cast<std::size_t>(camReuseEntries));
            view.camReuseTemporaryFaults.assign(first, last);
        }

        RECAMSolverRequest request;
        request.subarrayId = subarrayId;
        request.availableRows = view.descriptor.rows;
        request.availableColumns = view.descriptor.columns;
        request.bufferCamEntries = camReuseEntries;
        request.provisionedRows = 3;
        request.provisionedColumns = 3;
        const RepairAttemptResult attempt = solver.solve(faults, request);
        view.candidateValidity.assign(attempt.candidateSolutions, false);
        for (std::size_t candidate : attempt.validCandidateIndices)
            view.candidateValidity[candidate] = true;
        if (!attempt.validCandidateIndices.empty())
        {
            const std::size_t candidate = attempt.validCandidateIndices.front();
            if (candidate + 1 > 15)
                throw std::logic_error("PatternID no longer fits the 4-bit contract");
            view.lowestValidPatternId = static_cast<std::uint8_t>(candidate + 1);
            const auto found = std::find_if(
                attempt.validCandidateOptions.begin(),
                attempt.validCandidateOptions.end(),
                [candidate](const CandidateRepairOption &option)
                { return option.candidateIndex == candidate; });
            if (found == attempt.validCandidateOptions.end())
                throw std::logic_error("Valid PatternID has no repair reconstruction");
            view.reconstructedMappings = found->mappings;
        }
        view.hybridLogicalOccupancy = attempt.hybridCamEntriesActive;
        if (view.hybridLogicalOccupancy != attempt.hybridRecords.size())
            throw std::logic_error("Hybrid occupancy and payload snapshot disagree");
        for (const HybridRecordSnapshot &record : attempt.hybridRecords)
        {
            hybridUnion[{record.row, record.column, record.pivotPointer,
                         record.descriptorRowIsDiff}][cfg] = true;
        }
    }
    // A tagged implementation may merge equivalent records further.  Until
    // descriptor/pointer identity is represented at that level, max logical
    // occupancy is the safe lower-level union measurement, not a fixed guess.
    result.hybridUnionOccupancy = hybridUnion.size();
    result.maximumHybridUnionOccupancy = hybridUnion.size();
    return result;
}

DirectionalGroupResult DirectionalMultiConfigAnalyzer::analyzeGroup(
    const std::array<std::vector<Fault>, kSubarrayCount> &faults,
    const SimulationConfig &ledgerConfig,
    DirectionalSolutionTakePolicy policy,
    int camReuseEntries) const
{
    ledgerConfig.validate();
    PhysicalResourceLedger ledger(ledgerConfig);
    DirectionalGroupResult result;
    std::array<DirectionalMultiConfigAnalysis, kSubarrayCount> analysis;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        analysis[subarray] = analyze(
            faults[subarray], static_cast<int>(subarray), camReuseEntries);
        for (std::size_t cfg = 0; cfg < kDirectionalConfigCount; ++cfg)
            result.configPatternMap[subarray][cfg] =
                analysis[subarray].configs[cfg].lowestValidPatternId;
    }

    const auto demandFor = [](const DirectionalConfigAnalysis &view)
    {
        return SpareDemand{static_cast<std::size_t>(view.descriptor.rows),
                           static_cast<std::size_t>(view.descriptor.columns)};
    };
    const auto rank = [&ledgerConfig](const DirectionalConfigAnalysis &view)
    {
        const int borrowed = std::max(0, view.descriptor.rows - ledgerConfig.spareRows) +
            std::max(0, view.descriptor.columns - ledgerConfig.spareColumns);
        const int unused = std::max(0, ledgerConfig.spareRows - view.descriptor.rows) +
            std::max(0, ledgerConfig.spareColumns - view.descriptor.columns);
        return std::make_tuple(borrowed, -unused,
            static_cast<std::size_t>(view.descriptor.id));
    };
    const auto stage = [&](const std::array<std::size_t, kSubarrayCount> &chosen,
                           const LedgerAllocationResult &allocation)
    {
        result.success = allocation.success;
        result.ledger = allocation;
        if (!allocation.success)
            return;
        for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
        {
            const DirectionalConfigAnalysis &view =
                analysis[subarray].configs[chosen[subarray]];
            result.pendingRepairBuffer.push_back(PendingDirectionalRepairRecord{
                subarray, view.descriptor.id, view.lowestValidPatternId,
                view.reconstructedMappings, view.camReuseTemporaryFaults});
        }
    };

    if (policy == DirectionalSolutionTakePolicy::Early)
    {
        std::array<SpareDemand, kSubarrayCount> demands;
        std::array<std::size_t, kSubarrayCount> chosen{};
        for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
        {
            std::vector<std::size_t> valid;
            for (std::size_t cfg = 0; cfg < kDirectionalConfigCount; ++cfg)
                if (analysis[subarray].configs[cfg].lowestValidPatternId != 0)
                    valid.push_back(cfg);
            std::sort(valid.begin(), valid.end(), [&](std::size_t left, std::size_t right)
            { return rank(analysis[subarray].configs[left]) < rank(analysis[subarray].configs[right]); });
            bool committed = false;
            for (std::size_t cfg : valid)
            {
                ++result.groupCombinationsChecked;
                demands[subarray] = demandFor(analysis[subarray].configs[cfg]);
                LedgerAllocationResult allocation = ledger.allocateSequential(demands, subarray + 1);
                if (!allocation.success)
                {
                    ++result.groupCombinationsPruned;
                    continue;
                }
                chosen[subarray] = cfg;
                committed = true;
                break;
            }
            if (!committed)
            {
                result.earlySuccess = false;
                return result;
            }
        }
        const LedgerAllocationResult allocation = ledger.allocateSequential(demands, kSubarrayCount);
        stage(chosen, allocation);
        result.earlySuccess = result.success;
        return result;
    }

    std::array<std::size_t, kSubarrayCount> chosen{};
    std::array<std::size_t, kSubarrayCount> best{};
    LedgerAllocationResult bestAllocation;
    bool found = false;
    const auto visit = [&](const auto &self, std::size_t subarray) -> void
    {
        if (subarray != kSubarrayCount)
        {
            ++result.groupCombinationsChecked;
            std::array<SpareDemand, kSubarrayCount> demands;
            for (std::size_t index = 0; index < kSubarrayCount; ++index)
                demands[index] = demandFor(analysis[index].configs[chosen[index]]);
            LedgerAllocationResult allocation = ledger.allocate(demands);
            if (!allocation.success)
            {
                ++result.groupCombinationsPruned;
                return;
            }
            if (!found || allocation.transfers.size() < bestAllocation.transfers.size() ||
                (allocation.transfers.size() == bestAllocation.transfers.size() && chosen < best))
            {
                found = true;
                best = chosen;
                bestAllocation = std::move(allocation);
            }
            return;
        }
        for (std::size_t cfg = 0; cfg < kDirectionalConfigCount; ++cfg)
        {
            if (analysis[subarray].configs[cfg].lowestValidPatternId == 0)
                continue;
            chosen[subarray] = cfg;
            self(self, subarray + 1);
        }
    };
    visit(visit, 0);
    if (found)
        stage(best, bestAllocation);
    result.groupSuccess = result.success;
    const DirectionalGroupResult early = analyzeGroup(
        faults, ledgerConfig, DirectionalSolutionTakePolicy::Early,
        camReuseEntries);
    result.earlySuccess = early.success;
    result.greedyLoss = result.success && !early.success;
    return result;
}

} // namespace dynamic_spare
