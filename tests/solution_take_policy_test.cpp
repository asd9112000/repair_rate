#include <algorithm>
#include <array>
#include <cstddef>
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
}

} // namespace

int main()
{
    verifyBitmapAndDecode();
    verifySequentialCommit();
    verifyPolicies();
    return 0;
}
