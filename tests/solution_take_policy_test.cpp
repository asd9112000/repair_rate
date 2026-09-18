#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "DynamicRepairSimulator.hpp"
#include "PhysicalResourceLedger.hpp"
#include "RECAMSolverAdapter.hpp"
#include "RepairAttemptSolver.hpp"
#include "SimulationConfig.hpp"
#include "SolGenerator.hpp"
#include "V2GroupNoScratchPolicy.hpp"

namespace
{

void require(bool condition, const std::string &message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::size_t choose(std::size_t total, std::size_t selected)
{
    selected = std::min(selected, total - selected);
    std::size_t result = 1;
    for (std::size_t index = 1; index <= selected; ++index)
        result = result * (total - selected + index) / index;
    return result;
}

dynamic_spare::MatrixRepairAddress address(int subarray, int index)
{
    return {0, 0, 0, 0, subarray, 100 + index, 200 + index};
}

Fault fault(int row, int column)
{
    Fault result;
    result.HBMID = 0;
    result.ChannelID = 0;
    result.BankID = 0;
    result.SubarrayGroupID = 0;
    result.SubarrayID = 0;
    result.r = row;
    result.c = column;
    return result;
}

void verifyBitmapAndDecode()
{
    dynamic_spare::RECAMSolverRequest request;
    request.subarrayId = 0;
    request.availableRows = 2;
    request.availableColumns = 2;
    request.bufferCamEntries = 4;
    request.rowAddressWidthBits = 9;
    request.columnAddressWidthBits = 13;
    std::vector<Fault> faults{
        fault(1, 1), fault(1, 2), fault(3, 4)};
    const auto result = dynamic_spare::RECAMSolverAdapter{}.solve(
        faults, request);
    require(result.tileSolutionState.has_value(),
            "RECAM adapter did not extract compressed tile state");
    const auto &state = *result.tileSolutionState;
    require(state.matrixRowAddresses.size() == 4 &&
                state.matrixColumnAddresses.size() == 4 &&
                state.validSolutionBitmap.size() == 6,
            "Compressed tile state violates its size contract");
    for (std::size_t id = 0; id < state.validSolutionBitmap.size(); ++id)
    {
        const bool listed = std::find(
            result.validCandidateIndices.begin(),
            result.validCandidateIndices.end(), id) !=
            result.validCandidateIndices.end();
        require(state.validSolutionBitmap[id] == listed,
                "Bitmap differs from validSolList membership");
    }

    dynamic_spare::TileSolutionState synthetic;
    synthetic.subarrayId = 0;
    synthetic.spareRows = 2;
    synthetic.spareColumns = 2;
    synthetic.matrixRowAddresses.resize(4);
    synthetic.matrixColumnAddresses.resize(4);
    synthetic.validSolutionBitmap.assign(6, true);
    for (int index = 0; index < 4; ++index)
    {
        synthetic.matrixRowAddresses[index] = address(0, index);
        synthetic.matrixColumnAddresses[index] = address(0, index);
    }
    // Slot 2 represents Hybrid-CAM matrix-extension provenance, independent
    // of an Address-CAM pivot walk.
    synthetic.matrixRowAddresses[2]->row = 777;
    synthetic.matrixColumnAddresses[2]->column = 888;

    SolGenerator generated(2, 2);
    for (std::size_t id = 0; id < generated.allSolVectorsType.size(); ++id)
    {
        const auto decoded = dynamic_spare::decodeSolution(synthetic, id);
        std::size_t expectedRows = 0;
        std::size_t expectedColumns = 0;
        for (bool column : generated.allSolVectorsType[id])
            column ? ++expectedColumns : ++expectedRows;
        require(decoded.sourceRows.size() == expectedRows &&
                    decoded.sourceColumns.size() == expectedColumns,
                "Solution decoder changed SolGenerator orientation ordering");
    }
    const auto rowExtended = dynamic_spare::decodeSolution(synthetic, 1);
    require(std::any_of(
                rowExtended.sourceRows.begin(), rowExtended.sourceRows.end(),
                [](const auto &entry) { return entry.row == 777; }),
            "Decoder lost Hybrid-extended matrix row provenance");
    const auto columnExtended = dynamic_spare::decodeSolution(synthetic, 0);
    require(std::any_of(
                columnExtended.sourceColumns.begin(),
                columnExtended.sourceColumns.end(),
                [](const auto &entry) { return entry.column == 888; }),
            "Decoder lost Hybrid-extended matrix column provenance");
}

void verifyV2RoleSlotMapping()
{
    using namespace dynamic_spare;
    const std::array<std::array<int, 4>, 4> expected{{
        {{0, 1, 2, 3}}, {{0, 4, 5, 6}},
        {{0, 4, 5, 6}}, {{0, 1, 2, 3}}}};
    const std::array<V2GroupAction, 4> actions{{
        V2GroupAction::Local, V2GroupAction::ReleaseOnly,
        V2GroupAction::BorrowOnly, V2GroupAction::ReleaseAndBorrow}};
    for (std::size_t subarray = 0; subarray < 4; ++subarray)
    {
        const auto mappings = v2RoleSlotMappings(subarray);
        for (std::size_t slot = 0; slot < 4; ++slot)
        {
            require(mappings[slot].role == static_cast<char>('A' + subarray) &&
                        mappings[slot].roleSlot == slot &&
                        mappings[slot].configId == expected[subarray][slot] &&
                        mappings[slot].action == actions[slot],
                    "Frozen V2 role-slot mapping changed");
        }
    }
    require(v2RoleSlotMappings(1)[1].configId == 4 &&
                v2RoleSlotMappings(1)[3].configId == 6,
            "B-role priority was replaced by numeric ConfigID ordering");

    const std::array<std::size_t, 4> rtlSlots{{1, 0, 3, 2}};
    for (std::size_t subarray = 0; subarray < 4; ++subarray)
    {
        const auto rtlMappings = v2RtlGroupRoleSlotMappings(
            ConfigContractVersion::Rs3Cs3M1, subarray);
        for (std::size_t rank = 0; rank < rtlSlots.size(); ++rank)
        {
            const auto historical = v2RoleSlotMappings(subarray)[rtlSlots[rank]];
            require(rtlMappings[rank].roleSlot == historical.roleSlot &&
                        rtlMappings[rank].configId == historical.configId &&
                        rtlMappings[rank].action == historical.action,
                    "RTL canonical GROUP rank no longer reads slots 1,0,3,2");
        }
    }
}

class GreedyLossSolver final : public dynamic_spare::RepairAttemptSolver
{
public:
    dynamic_spare::RepairAttemptResult solve(
        const std::vector<Fault> &,
        const dynamic_spare::RECAMSolverRequest &request) const override
    {
        using namespace dynamic_spare;
        RepairAttemptResult result;
        result.subarrayId = request.subarrayId;
        result.availableRows = request.availableRows;
        result.availableColumns = request.availableColumns;
        result.provisionedRows = request.provisionedRows;
        result.provisionedColumns = request.provisionedColumns;
        result.stage = request.stage;
        result.attemptIndex = request.attemptIndex;
        const std::size_t dimension = static_cast<std::size_t>(
            request.availableRows + request.availableColumns);
        result.matrixDimension = dimension;
        result.candidateSolutions = choose(
            dimension, static_cast<std::size_t>(request.availableRows));
        result.candidateSolutionsEvaluated = result.candidateSolutions;

        TileSolutionState state;
        state.subarrayId = request.subarrayId;
        state.spareRows = request.availableRows;
        state.spareColumns = request.availableColumns;
        state.matrixRowAddresses.resize(dimension);
        state.matrixColumnAddresses.resize(dimension);
        state.validSolutionBitmap.assign(result.candidateSolutions, false);
        state.compressedStorageBits = dimension * 22 +
            2 * dimension + result.candidateSolutions;

        const bool isA = request.subarrayId == 0 &&
            request.availableRows == 2 && request.availableColumns == 2;
        const bool isC = request.subarrayId == 2 &&
            request.availableRows == 3 && request.availableColumns == 2;
        const bool isIdle = (request.subarrayId == 1 ||
                request.subarrayId == 3) &&
            request.availableRows == 2 && request.availableColumns == 2;
        if (isA)
        {
            state.matrixRowAddresses[0] = address(0, 0);
            state.matrixRowAddresses[1] = address(0, 1);
            state.matrixColumnAddresses[1] = address(0, 1);
            state.validSolutionBitmap[0] = true; // 2R, 0C
            state.validSolutionBitmap[1] = true; // 1R, 1C
        }
        else if (isC)
        {
            for (int index = 0; index < 3; ++index)
                state.matrixRowAddresses[index] = address(2, index);
            state.validSolutionBitmap[0] = true; // 3R, must borrow from A
        }
        else if (isIdle)
        {
            state.validSolutionBitmap[0] = true;
        }

        for (std::size_t id = 0; id < state.validSolutionBitmap.size(); ++id)
        {
            if (!state.validSolutionBitmap[id])
                continue;
            const DecodedSolution decoded = decodeSolution(state, id);
            CandidateRepairOption option;
            option.candidateIndex = id;
            option.usedRows = decoded.sourceRows.size();
            option.usedColumns = decoded.sourceColumns.size();
            result.validCandidateIndices.push_back(id);
            result.validCandidateOptions.push_back(std::move(option));
        }
        result.repairSuccess = !result.validCandidateOptions.empty();
        result.isRepairable = result.repairSuccess;
        result.failedCandidates = result.candidateSolutions -
            result.validCandidateOptions.size();
        result.tileSolutionState = std::move(state);
        return result;
    }
};

class PairGreedyLossSolver final : public dynamic_spare::RepairAttemptSolver
{
public:
    dynamic_spare::RepairAttemptResult solve(
        const std::vector<Fault> &,
        const dynamic_spare::RECAMSolverRequest &request) const override
    {
        using namespace dynamic_spare;
        RepairAttemptResult result;
        result.subarrayId = request.subarrayId;
        result.availableRows = request.availableRows;
        result.availableColumns = request.availableColumns;
        result.provisionedRows = request.provisionedRows;
        result.provisionedColumns = request.provisionedColumns;
        result.stage = request.stage;
        result.attemptIndex = request.attemptIndex;
        const std::size_t dimension = static_cast<std::size_t>(
            request.availableRows + request.availableColumns);
        result.matrixDimension = dimension;
        result.candidateSolutions = choose(
            dimension, static_cast<std::size_t>(request.availableRows));
        result.candidateSolutionsEvaluated = result.candidateSolutions;

        TileSolutionState state;
        state.subarrayId = request.subarrayId;
        state.spareRows = request.availableRows;
        state.spareColumns = request.availableColumns;
        state.matrixRowAddresses.resize(dimension);
        state.matrixColumnAddresses.resize(dimension);
        state.validSolutionBitmap.assign(result.candidateSolutions, false);
        const bool isA = request.subarrayId == 0 &&
            request.availableRows == 2 && request.availableColumns == 2;
        const bool isB = request.subarrayId == 1 &&
            request.availableRows == 3 && request.availableColumns == 2;
        const bool isIdle = (request.subarrayId == 2 || request.subarrayId == 3) &&
            request.availableRows == 2 && request.availableColumns == 2;
        if (isA)
        {
            state.matrixRowAddresses[0] = address(0, 0);
            state.matrixRowAddresses[1] = address(0, 1);
            state.matrixColumnAddresses[1] = address(0, 1);
            state.validSolutionBitmap[0] = true; // consumes A's shareable row
            state.validSolutionBitmap[1] = true; // leaves it available
        }
        else if (isB)
        {
            for (int index = 0; index < 3; ++index)
                state.matrixRowAddresses[index] = address(1, index);
            state.validSolutionBitmap[0] = true;
        }
        else if (isIdle)
        {
            state.validSolutionBitmap[0] = true;
        }
        for (std::size_t id = 0; id < state.validSolutionBitmap.size(); ++id)
        {
            if (!state.validSolutionBitmap[id]) continue;
            const DecodedSolution decoded = decodeSolution(state, id);
            CandidateRepairOption option;
            option.candidateIndex = id;
            option.usedRows = decoded.sourceRows.size();
            option.usedColumns = decoded.sourceColumns.size();
            result.validCandidateIndices.push_back(id);
            result.validCandidateOptions.push_back(std::move(option));
        }
        result.repairSuccess = !result.validCandidateOptions.empty();
        result.isRepairable = result.repairSuccess;
        result.failedCandidates = result.candidateSolutions -
            result.validCandidateOptions.size();
        result.tileSolutionState = std::move(state);
        return result;
    }
};

class V2CorpusSolver final : public dynamic_spare::RepairAttemptSolver
{
public:
    explicit V2CorpusSolver(bool distinction = false, bool allFeasible = false)
        : distinction_(distinction), allFeasible_(allFeasible) {}

    explicit V2CorpusSolver(std::array<std::size_t, 4> directedSlots)
        : directedSlots_(directedSlots) {}

    dynamic_spare::RepairAttemptResult solve(
        const std::vector<Fault> &,
        const dynamic_spare::RECAMSolverRequest &request) const override
    {
        using namespace dynamic_spare;
        RepairAttemptResult result;
        result.runIndex = request.runIndex;
        result.subarrayId = request.subarrayId;
        result.availableRows = request.availableRows;
        result.availableColumns = request.availableColumns;
        result.provisionedRows = request.provisionedRows;
        result.provisionedColumns = request.provisionedColumns;
        result.stage = request.stage;
        result.attemptIndex = request.attemptIndex;
        const std::size_t dimension = static_cast<std::size_t>(
            request.availableRows + request.availableColumns);
        result.matrixDimension = dimension;
        result.candidateSolutions = choose(
            dimension, static_cast<std::size_t>(request.availableRows));
        result.candidateSolutionsEvaluated = result.candidateSolutions;

        TileSolutionState state;
        state.subarrayId = request.subarrayId;
        state.spareRows = request.availableRows;
        state.spareColumns = request.availableColumns;
        state.matrixRowAddresses.resize(dimension);
        state.matrixColumnAddresses.resize(dimension);
        state.validSolutionBitmap.assign(result.candidateSolutions, false);
        for (std::size_t index = 0; index < dimension; ++index)
        {
            state.matrixRowAddresses[index] = address(request.subarrayId, index);
            state.matrixColumnAddresses[index] = address(request.subarrayId, index);
        }

        bool feasible;
        if (allFeasible_)
        {
            feasible = true;
        }
        else if (directedSlots_)
        {
            feasible = request.attemptIndex ==
                directedSlots_->at(static_cast<std::size_t>(request.subarrayId));
        }
        else if (distinction_)
        {
            feasible = request.subarrayId != 0 ||
                !(request.availableRows == request.provisionedRows &&
                  request.availableColumns + 1 == request.provisionedColumns);
            if (request.subarrayId == 0 &&
                request.availableRows == request.provisionedRows &&
                request.availableColumns == request.provisionedColumns - 1)
                feasible = false;
        }
        else
        {
            const std::uint64_t hash = request.runIndex * 0x9e3779b97f4a7c15ULL +
                static_cast<std::uint64_t>(request.subarrayId + 1) * 131ULL +
                static_cast<std::uint64_t>(request.availableRows) * 17ULL +
                static_cast<std::uint64_t>(request.availableColumns) * 29ULL;
            feasible = hash % 5 != 0;
            if (feasible && result.candidateSolutions != 0)
                state.validSolutionBitmap[hash % result.candidateSolutions] = true;
        }
        if ((allFeasible_ || distinction_ || directedSlots_) && feasible &&
            result.candidateSolutions != 0)
            state.validSolutionBitmap[0] = true;

        for (std::size_t id = 0; id < state.validSolutionBitmap.size(); ++id)
        {
            if (!state.validSolutionBitmap[id]) continue;
            const auto decoded = decodeSolution(state, id);
            CandidateRepairOption option;
            option.candidateIndex = id;
            option.usedRows = decoded.sourceRows.size();
            option.usedColumns = decoded.sourceColumns.size();
            result.validCandidateIndices.push_back(id);
            result.validCandidateOptions.push_back(std::move(option));
        }
        result.repairSuccess = !result.validCandidateOptions.empty();
        result.isRepairable = result.repairSuccess;
        result.failedCandidates = result.candidateSolutions -
            result.validCandidateOptions.size();
        result.tileSolutionState = std::move(state);
        return result;
    }

private:
    bool distinction_ = false;
    bool allFeasible_ = false;
    std::optional<std::array<std::size_t, 4>> directedSlots_;
};

struct GoldenV2
{
    bool success = false;
    std::array<std::optional<int>, 4> config;
    std::array<std::optional<std::size_t>, 4> pattern;
    std::array<std::optional<dynamic_spare::V2GroupAction>, 4> action;
    std::optional<std::size_t> failure;
    std::vector<dynamic_spare::GroupRepairResult::V2DecisionTrace> trace;
};

std::vector<int> owners(const dynamic_spare::LedgerAllocationResult &allocation)
{
    std::vector<int> result;
    for (const auto &line : allocation.lines)
        result.push_back(line.assignedSubarray ? static_cast<int>(*line.assignedSubarray) : -1);
    return result;
}

GoldenV2 goldenV2(const dynamic_spare::GroupRepairResult &observed,
                  const dynamic_spare::SimulationConfig &config)
{
    using namespace dynamic_spare;
    const std::array<std::array<int, 4>, 4> ids{{
        {{0,1,2,3}}, {{0,4,5,6}}, {{0,4,5,6}}, {{0,1,2,3}}}};
    const std::array<V2GroupAction, 4> actions{{
        V2GroupAction::Local, V2GroupAction::ReleaseOnly,
        V2GroupAction::BorrowOnly, V2GroupAction::ReleaseAndBorrow}};
    PhysicalResourceLedger ledger(config);
    std::array<SpareDemand, 4> demands{};
    GoldenV2 result;
    for (std::size_t sa = 0; sa < 4; ++sa)
    {
        const auto before = ledger.allocateSequential(demands, sa);
        bool selected = false;
        for (std::size_t slot = 0; slot < 4; ++slot)
        {
            GroupRepairResult::V2DecisionTrace trace;
            trace.role=static_cast<char>('A'+sa); trace.subarray=sa;
            trace.roleSlot=slot; trace.configId=ids[sa][slot];
            trace.action=actions[slot]; trace.ledgerOwnersBefore=owners(before);
            trace.ledgerOwnersAfter=trace.ledgerOwnersBefore;
            const auto &attempt = observed.attemptsBySubarray[sa][slot];
            trace.configFeasible = attempt.repairSuccess;
            if (attempt.repairSuccess)
            {
                const auto option = std::min_element(
                    attempt.validCandidateOptions.begin(),
                    attempt.validCandidateOptions.end(),
                    [](const auto &left, const auto &right) {
                        return left.candidateIndex < right.candidateIndex;
                    });
                trace.smallestPatternId = option->candidateIndex + 1;
                trace.requiredRows = option->usedRows;
                trace.requiredColumns = option->usedColumns;
                auto trial = demands;
                trial[sa] = {option->usedRows, option->usedColumns};
                auto allocation = ledger.allocateSequential(trial, sa + 1);
                trace.ledgerValid = allocation.success &&
                    allocation.transfers.size() <= static_cast<std::size_t>(
                        config.modifiers.maximumGroupBorrowedSpares);
                if (trace.ledgerValid)
                {
                    trace.selected=true; trace.ledgerOwnersAfter=owners(allocation);
                    result.config[sa]=ids[sa][slot];
                    result.pattern[sa]=*trace.smallestPatternId;
                    result.action[sa]=actions[slot]; demands=trial; selected=true;
                }
            }
            result.trace.push_back(trace);
            if (selected) break;
        }
        if (!selected) { result.failure=sa; return result; }
    }
    result.success=true;
    return result;
}

struct GlobalOracle
{
    bool success = false;
    std::array<std::optional<std::size_t>, 4> attempts;
    std::array<std::optional<std::size_t>, 4> candidates;
    dynamic_spare::LedgerAllocationResult allocation;
};

GlobalOracle bruteForceGlobal(
    const dynamic_spare::GroupRepairResult &observed,
    const dynamic_spare::SimulationConfig &config,
    bool sequentialLedger = false)
{
    using namespace dynamic_spare;
    struct Entry
    {
        std::size_t attempt = 0;
        std::size_t candidate = 0;
        std::size_t rows = 0;
        std::size_t columns = 0;
    };
    std::array<std::vector<Entry>, 4> entries;
    for (std::size_t sa = 0; sa < 4; ++sa)
    {
        for (const RepairAttemptResult &attempt : observed.attemptsBySubarray[sa])
        {
            for (const CandidateRepairOption &candidate : attempt.validCandidateOptions)
            {
                entries[sa].push_back({attempt.attemptIndex,
                                       candidate.candidateIndex,
                                       candidate.usedRows,
                                       candidate.usedColumns});
            }
        }
        if (entries[sa].empty()) return {};
    }
    PhysicalResourceLedger ledger(config);
    std::array<Entry, 4> selected;
    GlobalOracle best;
    const bool rowOnly = config.layout == GroupLayout::Line1x4;
    const auto better = [&](const LedgerAllocationResult &allocation) {
        if (!best.success) return true;
        const auto borrowed = rowOnly ? allocation.borrowedRows()
            : allocation.transfers.size();
        const auto bestBorrowed = rowOnly ? best.allocation.borrowedRows()
            : best.allocation.transfers.size();
        if (borrowed != bestBorrowed) return borrowed < bestBorrowed;
        const auto lines = rowOnly ? allocation.usedRows
            : allocation.usedRows + allocation.usedColumns;
        const auto bestLines = rowOnly ? best.allocation.usedRows
            : best.allocation.usedRows + best.allocation.usedColumns;
        if (lines != bestLines) return lines < bestLines;
        for (std::size_t sa = 0; sa < 4; ++sa)
            if (selected[sa].candidate != *best.candidates[sa])
                return selected[sa].candidate < *best.candidates[sa];
        for (std::size_t sa = 0; sa < 4; ++sa)
            if (selected[sa].attempt != *best.attempts[sa])
                return selected[sa].attempt < *best.attempts[sa];
        return false;
    };
    const auto visit = [&](const auto &self, std::size_t sa) -> void
    {
        if (sa < 4)
        {
            for (const Entry &entry : entries[sa])
            {
                selected[sa] = entry;
                self(self, sa + 1);
            }
            return;
        }
        std::array<SpareDemand, 4> demands;
        for (std::size_t index = 0; index < 4; ++index)
            demands[index] = {selected[index].rows, selected[index].columns};
        LedgerAllocationResult allocation = sequentialLedger
            ? ledger.allocateSequential(demands, 4)
            : ledger.allocate(demands);
        if (!allocation.success || allocation.transfers.size() >
                static_cast<std::size_t>(config.modifiers.maximumGroupBorrowedSpares))
            return;
        if (!better(allocation)) return;
        best.success = true;
        best.allocation = std::move(allocation);
        for (std::size_t index = 0; index < 4; ++index)
        {
            best.attempts[index] = selected[index].attempt;
            best.candidates[index] = selected[index].candidate;
        }
    };
    visit(visit, 0);
    return best;
}

void verifyPolicies()
{
    dynamic_spare::SimulationConfig config;
    config.topology = dynamic_spare::SharingTopology::Directional;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    config.modifiers.maximumGroupBorrowedSpares = 1;
    config.usePaperCamReuseCapacity = false;
    config.bufferCamEntries = 0;
    dynamic_spare::FaultGroup faults;
    dynamic_spare::DynamicRepairSimulator simulator(
        std::make_shared<GreedyLossSolver>());

    config.solutionTakePolicy = dynamic_spare::SolutionTakePolicy::Early;
    const auto early = simulator.run(faults, config, 0);
    const auto repeated = simulator.run(faults, config, 0);
    require(!early.groupRepairSuccess && early.greedyLoss &&
                early.groupCompressedSuccess == true,
            "Constructed EARLY greedy-loss case was not detected");
    require(early.selectedCandidateIndices == repeated.selectedCandidateIndices &&
                early.sharing.borrowedRows == repeated.sharing.borrowedRows,
            "EARLY selection is not deterministic");
    require(early.remainingResourcesAfterTile[0].has_value(),
            "EARLY did not expose post-A remaining resources");

    config.solutionTakePolicy =
        dynamic_spare::SolutionTakePolicy::GroupCompressed;
    const auto group = simulator.run(faults, config, 0);
    require(group.groupRepairSuccess && group.earlySuccess == false &&
                group.groupCompressedSuccess == true && group.greedyLoss,
            "GROUP_COMPRESSED did not recover the greedy-loss case");
    require(group.selectedCandidateIndices[0] == 1 &&
                group.sharing.borrowedRows == 1,
            "GROUP_COMPRESSED selected the wrong four-tile combination");
    require(group.solutionSelectionWork > 0 &&
                group.feasibleCombinationCount > 0 &&
                group.compressedStateBits > 0,
            "GROUP_COMPRESSED observability metrics were not populated");
    require(group.usedRows + group.unusedPhysicalRows == 8 &&
                group.usedColumns + group.unusedPhysicalColumns == 8,
            "Solution policy violated physical spare conservation");

    config.modifiers.maximumGroupBorrowedSpares = 0;
    const auto failed = simulator.run(faults, config, 0);
    require(!failed.groupRepairSuccess && failed.usedRows == 0 &&
                failed.usedColumns == 0,
            "Failed GROUP_COMPRESSED search committed partial allocation");

    config.modifiers.maximumGroupBorrowedSpares = 1;
    config.solutionTakePolicy =
        dynamic_spare::SolutionTakePolicy::GroupNoScratchV2;
    const auto v2 = simulator.run(faults, config, 0);
    require(!v2.groupRepairSuccess && v2.firstFailureSubarray == 2,
            "GROUP_NO_SCRATCH_V2 did not terminate at its first failure");
    require(v2.selectedConfigIds[0] == 0 &&
                v2.selectedPatternIds[0] == 1 &&
                v2.selectedV2Actions[0] ==
                    dynamic_spare::V2GroupAction::Local,
            "GROUP_NO_SCRATCH_V2 changed fixed slot or PatternID priority");
    require(v2.groupCompressedSuccess == true,
            "Anti-backtracking corpus no longer distinguishes legacy search");
    require(!v2.v2DecisionTrace.empty() &&
                v2.v2DecisionTrace.front().selected,
            "GROUP_NO_SCRATCH_V2 did not retain an auditable decision trace");
    std::cout << "S0R_ANTI_BACKTRACKING v2=FAIL first_failure=C "
              << "group_compressed_legacy=PASS mismatch=0\n";
    std::cout << "S0R_LEGACY_REGRESSION early=PASS "
              << "group_compressed_legacy=PASS\n";
}

bool sameTrace(const dynamic_spare::GroupRepairResult::V2DecisionTrace &left,
               const dynamic_spare::GroupRepairResult::V2DecisionTrace &right)
{
    return left.role == right.role && left.subarray == right.subarray &&
        left.roleSlot == right.roleSlot &&
        left.configId == right.configId && left.action == right.action &&
        left.configFeasible == right.configFeasible &&
        left.smallestPatternId == right.smallestPatternId &&
        left.requiredRows == right.requiredRows &&
        left.requiredColumns == right.requiredColumns &&
        left.ledgerValid == right.ledgerValid && left.selected == right.selected &&
        left.ledgerOwnersBefore == right.ledgerOwnersBefore &&
        left.ledgerOwnersAfter == right.ledgerOwnersAfter;
}

void verifyV2VsEarlyDistinction()
{
    using namespace dynamic_spare;
    SimulationConfig config;
    config.spareRows=2; config.spareColumns=2; config.sharedRows=1;
    config.sharedColumns=1; config.topology=SharingTopology::Directional;
    config.modifiers.maximumGroupBorrowedSpares=1;
    config.usePaperCamReuseCapacity=false; config.bufferCamEntries=0;
    DynamicRepairSimulator simulator(std::make_shared<V2CorpusSolver>(true));
    FaultGroup faults;
    config.solutionTakePolicy=SolutionTakePolicy::Early;
    const auto early=simulator.run(faults,config,7);
    config.solutionTakePolicy=SolutionTakePolicy::GroupNoScratchV2;
    const auto v2=simulator.run(faults,config,7);
    const auto golden=goldenV2(v2,config);
    require(early.selectedAttemptIndices[0] == 1,
            "EARLY distinction corpus did not choose its borrow attempt");
    require(v2.selectedConfigIds[0] == 1 && golden.config[0] == 1 &&
                v2.selectedV2Actions[0] == V2GroupAction::ReleaseOnly,
            "V2 distinction corpus did not choose fixed release slot");
    std::cout << "S0R_EARLY_DISTINCTION early_attempt=1 "
              << "v2_config=1 v2_action=RELEASE_ONLY golden_config=1 status=PASS\n";
}

void verifyFixedSlotPriority()
{
    using namespace dynamic_spare;
    SimulationConfig config;
    config.spareRows=2; config.spareColumns=2; config.sharedRows=1;
    config.sharedColumns=1; config.topology=SharingTopology::Directional;
    config.solutionTakePolicy=SolutionTakePolicy::GroupNoScratchV2;
    config.modifiers.maximumGroupBorrowedSpares=1;
    config.usePaperCamReuseCapacity=false; config.bufferCamEntries=0;
    DynamicRepairSimulator simulator(
        std::make_shared<V2CorpusSolver>(false, true));
    const auto actual=simulator.run(FaultGroup{},config,11);
    const auto golden=goldenV2(actual,config);
    require(actual.groupRepairSuccess && golden.success &&
                actual.selectedConfigIds == golden.config,
            "All-feasible V2 tie differs from its independent golden");
    for (std::size_t sa=0; sa<4; ++sa)
        require(actual.selectedConfigIds[sa] == 0 &&
                    actual.selectedPatternIds[sa] == 1 &&
                    actual.v2DecisionTrace[sa].roleSlot == 0,
                "V2 did not resolve an all-feasible tie at role slot zero");
    std::cout << "S0R_FIXED_PRIORITY_TIE selected_slot=0 pattern=1 status=PASS\n";
}

void verifyDirectedV2Actions()
{
    using namespace dynamic_spare;
    SimulationConfig config;
    config.spareRows=2; config.spareColumns=2; config.sharedRows=1;
    config.sharedColumns=1; config.topology=SharingTopology::Directional;
    config.solutionTakePolicy=SolutionTakePolicy::GroupNoScratchV2;
    config.modifiers.maximumGroupBorrowedSpares=1;
    config.usePaperCamReuseCapacity=false; config.bufferCamEntries=0;
    FaultGroup faults;

    // The directional donors are B-column→A, D-row→B, A-row→C,
    // and C-column→D.  Each vector forces exactly one borrow action and
    // its matching donor-side release, while every other role stays local.
    const std::array<std::array<std::size_t, 4>, 8> cases{{
        {{2,1,0,0}}, {{3,1,0,0}},
        {{0,2,0,1}}, {{0,3,0,1}},
        {{1,0,2,0}}, {{1,0,3,0}},
        {{0,0,1,2}}, {{0,0,1,3}}
    }};
    std::size_t checked = 0;
    for (const auto &slots : cases)
    {
        DynamicRepairSimulator simulator(
            std::make_shared<V2CorpusSolver>(slots));
        const auto actual=simulator.run(faults,config,checked);
        const auto golden=goldenV2(actual,config);
        require(actual.groupRepairSuccess && golden.success,
                "Directed V2 action case was rejected by the ledger");
        require(actual.selectedConfigIds == golden.config &&
                    actual.selectedPatternIds == golden.pattern &&
                    actual.selectedV2Actions == golden.action,
                "Directed V2 action selection differs from the golden model");
        require(actual.firstFailureSubarray == golden.failure &&
                    actual.v2DecisionTrace.size() == golden.trace.size(),
                "Directed V2 action trace shape differs from the golden model");
        for (std::size_t index=0; index<golden.trace.size(); ++index)
            require(sameTrace(actual.v2DecisionTrace[index],golden.trace[index]),
                    "Directed V2 ledger trace differs from the golden model");
        ++checked;
    }
    std::cout << "S0R_DIRECTED_V2 cases=" << checked
              << " mapping_entries=16 mismatch=0\n";
}

void verifyRandomV2(std::size_t spares, std::uint64_t seed)
{
    using namespace dynamic_spare;
    SimulationConfig config;
    config.spareRows=static_cast<int>(spares);
    config.spareColumns=static_cast<int>(spares);
    config.sharedRows=1; config.sharedColumns=1;
    config.topology=SharingTopology::Directional;
    config.solutionTakePolicy=SolutionTakePolicy::GroupNoScratchV2;
    config.modifiers.maximumGroupBorrowedSpares=1;
    config.usePaperCamReuseCapacity=false; config.bufferCamEntries=0;
    DynamicRepairSimulator simulator(std::make_shared<V2CorpusSolver>());
    FaultGroup faults;
    std::size_t resultMismatch=0, configMismatch=0, patternMismatch=0;
    std::size_t actionMismatch=0, ledgerMismatch=0, failureMismatch=0;
    for (std::size_t vector=0; vector<1000; ++vector)
    {
        const auto actual=simulator.run(faults,config,seed+vector);
        const auto golden=goldenV2(actual,config);
        resultMismatch += actual.groupRepairSuccess != golden.success;
        configMismatch += actual.selectedConfigIds != golden.config;
        patternMismatch += actual.selectedPatternIds != golden.pattern;
        actionMismatch += actual.selectedV2Actions != golden.action;
        failureMismatch += actual.firstFailureSubarray != golden.failure;
        if (actual.v2DecisionTrace.size() != golden.trace.size()) ++ledgerMismatch;
        else for (std::size_t i=0;i<golden.trace.size();++i)
            if (!sameTrace(actual.v2DecisionTrace[i],golden.trace[i])) {
                ++ledgerMismatch; break;
            }
    }
    std::cout << "S0R_RANDOM_" << spares << '_' << spares << "_1 vectors=1000 seed="
              << seed << " result=" << resultMismatch << " config=" << configMismatch
              << " pattern=" << patternMismatch << " action=" << actionMismatch
              << " ledger=" << ledgerMismatch << " failure=" << failureMismatch << '\n';
    require(resultMismatch+configMismatch+patternMismatch+actionMismatch+
                ledgerMismatch+failureMismatch == 0,
            "GROUP_NO_SCRATCH_V2 random golden mismatch");
}

void verifyR1CanonicalPolicyContracts()
{
    using namespace dynamic_spare;
    SimulationConfig config;
    config.spareRows = 2;
    config.spareColumns = 2;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    config.topology = SharingTopology::Directional;
    config.modifiers.maximumGroupBorrowedSpares = 1;
    config.usePaperCamReuseCapacity = false;
    config.bufferCamEntries = 0;
    FaultGroup faults;
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        Fault entry = fault(static_cast<int>(subarray), 10);
        entry.SubarrayID = static_cast<int>(subarray);
        faults[subarray].push_back(entry);
    }

    require(std::string(toString(SolutionTakePolicy::DirectionalV2Early)) ==
                "normalized_local_first" &&
                std::string(toString(SolutionTakePolicy::GroupNoScratchV2)) ==
                "normalized_early_deferred" &&
                std::string(toString(SolutionTakePolicy::GroupGreedyRtlCanonical)) ==
                "normalized_streaming_early" &&
                std::string(toString(SolutionTakePolicy::DirectionalV2GroupGlobal)) ==
                "historical_directional_v2_global" &&
                std::string(toString(
                    SolutionTakePolicy::DirectionalV2GroupGlobalCanonical)) ==
                "normalized_global",
            "canonical and historical policy display names are not stable");

    DynamicRepairSimulator allFeasible(
        std::make_shared<V2CorpusSolver>(false, true));
    config.solutionTakePolicy = SolutionTakePolicy::GroupGreedyRtlCanonical;
    const auto canonical = allFeasible.run(faults, config, 71);
    require(canonical.groupRepairSuccess,
            "RTL canonical GROUP rejected its all-feasible corpus");
    const std::array<std::optional<int>, 4> expectedIds{{4, 1, 1, 4}};
    require(canonical.selectedConfigIds == expectedIds,
            "RTL canonical GROUP did not prefer release slots by rank");
    require(canonical.configContractVersion ==
                ConfigContractVersion::FrozenDate2x2M1,
            "2x2 canonical GROUP selected the wrong ConfigID contract");
    for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
    {
        require(canonical.selectedV2Actions[subarray] ==
                    V2GroupAction::ReleaseOnly &&
                    canonical.v2DecisionTrace[subarray].roleSlot == 1,
                "RTL canonical GROUP did not use rank-0 release slot");
    }

    config.solutionTakePolicy = SolutionTakePolicy::GroupNoScratchV2;
    const auto historical = allFeasible.run(faults, config, 71);
    require(historical.selectedConfigIds != canonical.selectedConfigIds,
            "Historical V2 and RTL canonical GROUP unexpectedly share priority");

    config.spareRows = 3;
    config.spareColumns = 3;
    config.solutionTakePolicy = SolutionTakePolicy::GroupGreedyRtlCanonical;
    const auto rs3Canonical = allFeasible.run(faults, config, 72);
    const std::array<std::optional<int>, 4> rs3ExpectedIds{{1, 4, 4, 1}};
    require(rs3Canonical.groupRepairSuccess &&
                rs3Canonical.selectedConfigIds == rs3ExpectedIds &&
                rs3Canonical.configContractVersion ==
                    ConfigContractVersion::Rs3Cs3M1,
            "RS3 canonical GROUP ConfigID contract differs from target table");
    config.spareRows = 2;
    config.spareColumns = 2;

    DynamicRepairSimulator greedyLoss(
        std::make_shared<GreedyLossSolver>());
    config.solutionTakePolicy = SolutionTakePolicy::GroupGlobal;
    const auto global = greedyLoss.run(faults, config, 0);
    config.solutionTakePolicy = SolutionTakePolicy::GroupCompressed;
    const auto compressed = greedyLoss.run(faults, config, 0);
    require(global.groupRepairSuccess == compressed.groupRepairSuccess &&
                global.selectedCandidateIndices == compressed.selectedCandidateIndices &&
                global.usedRows == compressed.usedRows &&
                global.usedColumns == compressed.usedColumns &&
                global.solutionSelectionWork == compressed.solutionSelectionWork &&
                global.feasibleCombinationCount == compressed.feasibleCombinationCount,
            "GROUP_GLOBAL no longer preserves the compressed global objective");
    const GlobalOracle directedOracle = bruteForceGlobal(global, config, true);
    require(directedOracle.success &&
                global.selectedAttemptIndices == directedOracle.attempts &&
                global.selectedCandidateIndices == directedOracle.candidates &&
                global.sharing.borrowedRows == directedOracle.allocation.borrowedRows() &&
                global.sharing.borrowedColumns == directedOracle.allocation.borrowedColumns() &&
                global.usedRows == directedOracle.allocation.usedRows &&
                global.usedColumns == directedOracle.allocation.usedColumns &&
                global.finalLedgerOwners == owners(directedOracle.allocation),
            "GROUP_GLOBAL differs from independent directed PatternID oracle");
    require(global.selectedCandidateIndices[0] == 1,
            "GROUP_GLOBAL collapsed the directed same-Config PatternID choices");

    config.solutionTakePolicy = SolutionTakePolicy::GroupGreedyRtlCanonical;
    const auto canonicalGreedyLoss = greedyLoss.run(faults, config, 0);
    require(!canonicalGreedyLoss.groupRepairSuccess && global.groupRepairSuccess,
            "Directed corpus no longer distinguishes canonical greedy from global");

    DynamicRepairSimulator randomized(std::make_shared<V2CorpusSolver>());
    std::size_t replayMismatch = 0;
    std::size_t successMismatch = 0;
    std::size_t configMismatch = 0;
    std::size_t patternMismatch = 0;
    std::size_t ledgerMismatch = 0;
    std::optional<std::uint64_t> firstPriorityCounterexample;
    std::size_t globalOracleMismatch = 0;
    for (std::size_t vector = 0; vector < 128; ++vector)
    {
        const std::uint64_t seed = 5000 + vector;
        config.solutionTakePolicy = SolutionTakePolicy::GroupNoScratchV2;
        const auto oldPriority = randomized.run(faults, config, seed);
        config.solutionTakePolicy = SolutionTakePolicy::GroupGreedyRtlCanonical;
        const auto first = randomized.run(faults, config, seed);
        const auto repeated = randomized.run(faults, config, seed);
        const bool successDiff =
            oldPriority.groupRepairSuccess != first.groupRepairSuccess;
        const bool configDiff =
            oldPriority.selectedConfigIds != first.selectedConfigIds;
        const bool patternDiff =
            oldPriority.selectedPatternIds != first.selectedPatternIds;
        const bool ledgerDiff =
            oldPriority.sharing.borrowedRows != first.sharing.borrowedRows ||
            oldPriority.sharing.borrowedColumns != first.sharing.borrowedColumns ||
            oldPriority.unusedPhysicalRows != first.unusedPhysicalRows ||
            oldPriority.unusedPhysicalColumns != first.unusedPhysicalColumns;
        successMismatch += successDiff;
        configMismatch += configDiff;
        patternMismatch += patternDiff;
        ledgerMismatch += ledgerDiff;
        if (!firstPriorityCounterexample.has_value() &&
            (successDiff || configDiff || patternDiff || ledgerDiff))
        {
            firstPriorityCounterexample = seed;
        }
        if (first.groupRepairSuccess != repeated.groupRepairSuccess ||
            first.selectedConfigIds != repeated.selectedConfigIds ||
            first.selectedPatternIds != repeated.selectedPatternIds ||
            first.firstFailureSubarray != repeated.firstFailureSubarray ||
            first.v2DecisionTrace.size() != repeated.v2DecisionTrace.size())
        {
            ++replayMismatch;
        }
        config.solutionTakePolicy = SolutionTakePolicy::GroupGlobal;
        const auto globalRandom = randomized.run(faults, config, seed);
        const GlobalOracle oracle = bruteForceGlobal(globalRandom, config, true);
        if (globalRandom.groupRepairSuccess != oracle.success ||
            globalRandom.selectedAttemptIndices != oracle.attempts ||
            globalRandom.selectedCandidateIndices != oracle.candidates ||
            (oracle.success &&
             (globalRandom.sharing.borrowedRows != oracle.allocation.borrowedRows() ||
              globalRandom.sharing.borrowedColumns != oracle.allocation.borrowedColumns() ||
              globalRandom.usedRows != oracle.allocation.usedRows ||
              globalRandom.usedColumns != oracle.allocation.usedColumns ||
              globalRandom.finalLedgerOwners != owners(oracle.allocation))))
        {
            ++globalOracleMismatch;
        }
    }
    require(replayMismatch == 0,
            "RTL canonical GROUP is not deterministic on the bounded corpus");
    require(globalOracleMismatch == 0,
            "GROUP_GLOBAL randomized PatternID oracle mismatch");
    require(faults[0][0].SubarrayID == 0 && faults[1][0].SubarrayID == 1 &&
                faults[2][0].SubarrayID == 2 && faults[3][0].SubarrayID == 3,
            "Policy replay mutated the supplied fault-group corpus");
    std::cout << "R1_CANONICAL_GREEDY all_feasible=PASS rank=1,0,3,2 "
              << "historical_priority_mismatch=1\n";
    std::cout << "R1_GLOBAL contract=PASS candidate_tuple_search=PASS "
              << "global_vs_greedy_counterexample=1\n";
    std::cout << "R1_BOUNDED_REPLAY vectors=128 mismatch=" << replayMismatch
              << " non_formal=1\n";
    std::cout << "R1_PRIORITY_CHARACTERIZATION vectors=128 seed_start=5000 "
              << "success=" << successMismatch << " config=" << configMismatch
              << " pattern=" << patternMismatch << " ledger=" << ledgerMismatch
              << " first_counterexample="
              << (firstPriorityCounterexample.has_value()
                      ? std::to_string(*firstPriorityCounterexample)
                      : "none") << " non_formal=1\n";
    std::cout << "R1_GLOBAL_ORACLE directed=1 randomized=128 mismatch="
              << globalOracleMismatch << '\n';
}

void verifyR1BOneByFourPolicies()
{
    using namespace dynamic_spare;
    SimulationConfig pair;
    pair.spareRows = 2;
    pair.spareColumns = 2;
    pair.sharedRows = 1;
    pair.sharedColumns = 0;
    pair.layout = GroupLayout::Line1x4;
    pair.topology = SharingTopology::PairSharing;
    pair.modifiers.maximumGroupBorrowedSpares = 2;
    pair.usePaperCamReuseCapacity = false;
    pair.bufferCamEntries = 0;
    pair.validate();
    PhysicalResourceLedger pairLedger(pair);
    require(pairLedger.physicalRows() == 8 && pairLedger.physicalColumns() == 8,
            "1x4 pair ledger increased the physical spare budget");
    const auto allocation = [&](std::array<SpareDemand, 4> demands) {
        return pairLedger.allocate(demands);
    };
    const auto aBorrowsB = allocation({{{3, 0}, {0, 0}, {0, 0}, {0, 0}}});
    const auto bBorrowsA = allocation({{{0, 0}, {3, 0}, {0, 0}, {0, 0}}});
    const auto cBorrowsD = allocation({{{0, 0}, {0, 0}, {3, 0}, {0, 0}}});
    const auto dBorrowsC = allocation({{{0, 0}, {0, 0}, {0, 0}, {3, 0}}});
    require(aBorrowsB.success && bBorrowsA.success && cBorrowsD.success &&
                dBorrowsC.success && aBorrowsB.transfers[0].donorSubarray == 1 &&
                bBorrowsA.transfers[0].donorSubarray == 0 &&
                cBorrowsD.transfers[0].donorSubarray == 3 &&
                dBorrowsC.transfers[0].donorSubarray == 2,
            "Two-Pairwise borrow accessibility changed");
    require(!allocation({{{4, 0}, {0, 0}, {0, 0}, {0, 0}}}).success,
            "Two-Pairwise allowed more than m borrowed rows");
    require(!allocation({{{2, 0}, {3, 0}, {0, 0}, {0, 0}}}).success &&
                !allocation({{{0, 0}, {0, 0}, {3, 0}, {2, 0}}}).success,
            "Two-Pairwise crossed the B/C boundary");
    const auto independent = allocation({{{3, 0}, {0, 0}, {3, 0}, {0, 0}}});
    require(independent.success && independent.transfers.size() == 2 &&
                independent.transfers[0].donorSubarray == 1 &&
                independent.transfers[1].donorSubarray == 3,
            "Two-Pairwise domains are not independent");

    SimulationConfig noShare = pair;
    noShare.sharedRows = 0;
    PhysicalResourceLedger noShareLedger(noShare);
    require(noShareLedger.allocate({{{2, 0}, {2, 0}, {2, 0}, {2, 0}}}).success &&
                !noShareLedger.allocate({{{3, 0}, {0, 0}, {0, 0}, {0, 0}}}).success,
            "share_row=0 does not degenerate to local-only behavior");
    SimulationConfig allShare = pair;
    allShare.sharedRows = 2;
    PhysicalResourceLedger allShareLedger(allShare);
    require(allShareLedger.allocate({{{4, 0}, {0, 0}, {0, 0}, {0, 0}}}).success &&
                allShareLedger.physicalRows() + allShareLedger.physicalColumns() == 16,
            "share_row=RS violates the fixed physical budget");

    FaultGroup faults;
    DynamicRepairSimulator pairSolver(std::make_shared<PairGreedyLossSolver>());
    pair.solutionTakePolicy = SolutionTakePolicy::OneByFourTwoPairwiseEarlyV1;
    const auto early = pairSolver.run(faults, pair, 0);
    pair.solutionTakePolicy = SolutionTakePolicy::OneByFourTwoPairwisePairGlobalV1;
    const auto pairGlobal = pairSolver.run(faults, pair, 0);
    require(!early.groupRepairSuccess && pairGlobal.groupRepairSuccess &&
                pairGlobal.selectedCandidateIndices[0] == 1,
            "CONFIRMED DIRECTED COUNTEREXAMPLE greedy fail/global pass lost");
    const GlobalOracle directedOracle = bruteForceGlobal(pairGlobal, pair, true);
    require(directedOracle.success &&
                pairGlobal.selectedCandidateIndices == directedOracle.candidates &&
                pairGlobal.finalLedgerOwners == owners(directedOracle.allocation),
            "Pair-global differs from independent pair-domain oracle");
    require(pairGlobal.privateRowCountPerSubarray == 1 &&
                pairGlobal.privateColumnCountPerSubarray == 2 &&
                pairGlobal.shareableRowCountPerSubarray == 1 &&
                pairGlobal.totalPhysicalSpareLinesGroup == 16,
            "1x4 static physical resource accounting is wrong");

    DynamicRepairSimulator randomSolver(std::make_shared<V2CorpusSolver>());
    std::size_t pairMismatch = 0;
    std::size_t pairVectors = 0;
    for (const std::array<int, 2> dimensions :
         {std::array<int, 2>{{2, 1}}, std::array<int, 2>{{3, 1}},
          std::array<int, 2>{{3, 2}}})
    {
        SimulationConfig randomizedPair = pair;
        randomizedPair.spareRows = dimensions[0];
        randomizedPair.spareColumns = dimensions[0];
        randomizedPair.sharedRows = dimensions[1];
        randomizedPair.solutionTakePolicy =
            SolutionTakePolicy::OneByFourTwoPairwisePairGlobalV1;
        for (std::size_t vector = 0; vector < 128; ++vector)
        {
            const auto observed = randomSolver.run(
                faults, randomizedPair, 7000 + pairVectors);
            const GlobalOracle oracle = bruteForceGlobal(
                observed, randomizedPair, true);
            if (observed.groupRepairSuccess != oracle.success ||
                observed.selectedAttemptIndices != oracle.attempts ||
                observed.selectedCandidateIndices != oracle.candidates ||
                (oracle.success &&
                 observed.finalLedgerOwners != owners(oracle.allocation)))
            {
                ++pairMismatch;
            }
            ++pairVectors;
        }
    }
    require(pairMismatch == 0, "Pair-global randomized oracle mismatch");

    SimulationConfig neighbor = pair;
    neighbor.topology = SharingTopology::NeighborSharing;
    PhysicalResourceLedger neighborLedger(neighbor);
    const auto bLeft = neighborLedger.allocate(
        {{{0, 0}, {3, 0}, {0, 0}, {0, 0}}});
    const auto cLeft = neighborLedger.allocate(
        {{{0, 0}, {0, 0}, {3, 0}, {0, 0}}});
    require(bLeft.success && cLeft.success &&
                bLeft.transfers[0].donorSubarray == 0 &&
                cLeft.transfers[0].donorSubarray == 1 &&
                !neighborLedger.allocate({{{3, 0}, {2, 0}, {0, 0}, {0, 0}}}).success,
            "Single-Hop ledger allowed transitive or non-neighbor sharing");
    neighbor.solutionTakePolicy = SolutionTakePolicy::OneByFourSingleHopEarlyV1;
    const auto neighborEarly = randomSolver.run(faults, neighbor, 8001);
    std::size_t neighborMismatch = 0;
    for (std::size_t vector = 0; vector < 1000; ++vector)
    {
        neighbor.solutionTakePolicy = SolutionTakePolicy::OneByFourSingleHopGlobalV1;
        const auto observed = randomSolver.run(faults, neighbor, 8001 + vector);
        const GlobalOracle oracle = bruteForceGlobal(observed, neighbor, true);
        if (observed.groupRepairSuccess != oracle.success ||
            observed.selectedCandidateIndices != oracle.candidates ||
            (oracle.success && observed.finalLedgerOwners != owners(oracle.allocation)))
        {
            ++neighborMismatch;
        }
    }
    neighbor.solutionTakePolicy = SolutionTakePolicy::OneByFourSingleHopGlobalV1;
    const auto neighborGlobal = randomSolver.run(faults, neighbor, 8001);
    const GlobalOracle neighborOracle = bruteForceGlobal(neighborGlobal, neighbor, true);
    require(neighborEarly.solutionTakePolicy ==
                SolutionTakePolicy::OneByFourSingleHopEarlyV1 &&
                neighborGlobal.groupRepairSuccess == neighborOracle.success &&
                neighborGlobal.selectedCandidateIndices == neighborOracle.candidates &&
                (!neighborOracle.success ||
                 neighborGlobal.finalLedgerOwners == owners(neighborOracle.allocation)) &&
                neighborMismatch == 0,
            "Single-Hop policy or global oracle contract failed");
    std::cout << "R1B_TWO_PAIRWISE directed=15 randomized=" << pairVectors
              << " oracle_mismatch=" << pairMismatch
              << " budget=PASS cross_pair=FORBIDDEN\n";
    std::cout << "R1B_GREEDY_FAIL_PAIR_GLOBAL_PASS confirmed=1 "
              << "early_failure=B\n";
    std::cout << "R1B_SINGLE_HOP directed=5 randomized=1000 transitive=FORBIDDEN "
              << "global_oracle_mismatch=" << neighborMismatch << '\n';
}

void verifySequentialCommit()
{
    dynamic_spare::SimulationConfig config;
    config.topology = dynamic_spare::SharingTopology::Directional;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    dynamic_spare::PhysicalResourceLedger ledger(config);
    std::array<dynamic_spare::SpareDemand, 4> demands{{
        {0, 3}, {0, 2}, {0, 0}, {0, 0}}};
    const auto afterA = ledger.allocateSequential(demands, 1);
    require(afterA.success && afterA.borrowedColumns() == 1,
            "EARLY A did not commit its borrowed B column");
    const auto afterB = ledger.allocateSequential(demands, 2);
    require(!afterB.success,
            "Sequential ledger allowed B to reclaim a column committed to A");
    std::cout << "S0R_LEDGER_CONTENTION immediate_commit=PASS "
              << "double_allocation=0\n";
}

} // namespace

int main()
{
    verifyBitmapAndDecode();
    verifyV2RoleSlotMapping();
    verifySequentialCommit();
    verifyPolicies();
    verifyV2VsEarlyDistinction();
    verifyFixedSlotPriority();
    verifyDirectedV2Actions();
    verifyRandomV2(2, 20260914);
    verifyRandomV2(3, 20260915);
    verifyR1CanonicalPolicyContracts();
    verifyR1BOneByFourPolicies();
    return 0;
}
