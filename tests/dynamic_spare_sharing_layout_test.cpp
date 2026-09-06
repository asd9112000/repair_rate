#include <array>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

#include "DynamicFaultGenerator.hpp"
#include "DynamicRepairSimulator.hpp"
#include "PhysicalResourceLedger.hpp"
#include "SimulationConfig.hpp"

namespace
{

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

dynamic_spare::SimulationConfig lineConfig(
    dynamic_spare::SharingTopology topology)
{
    dynamic_spare::SimulationConfig config;
    config.layout = dynamic_spare::GroupLayout::Line1x4;
    config.topology = topology;
    config.sharedRows = topology == dynamic_spare::SharingTopology::NoSharing
        ? 0
        : 1;
    config.sharedColumns = 0;
    config.modifiers.localFirst = true;
    config.modifiers.maximumGroupBorrowedSpares = 3;
    if (topology == dynamic_spare::SharingTopology::GlobalPool)
    {
        config.globalPool =
            dynamic_spare::GlobalPoolConfiguration{1, 2, 4, 0};
    }
    config.validate();
    return config;
}

void verifyNeighborGraph()
{
    const dynamic_spare::PhysicalResourceLedger ledger(
        lineConfig(dynamic_spare::SharingTopology::NeighborSharing));
    for (std::size_t owner = 0; owner < 4; ++owner)
    {
        for (std::size_t borrower = 0; borrower < 4; ++borrower)
        {
            const bool expected = owner != borrower &&
                (owner + 1 == borrower || borrower + 1 == owner);
            require(
                ledger.canBorrow(
                    static_cast<int>(owner), borrower,
                    dynamic_spare::SpareDimension::Row) == expected,
                "1x4 neighbor row graph is incorrect");
            require(
                !ledger.canBorrow(
                    static_cast<int>(owner), borrower,
                    dynamic_spare::SpareDimension::Column),
                "1x4 neighbor topology allowed column borrowing");
        }
    }
}

void verifyPairGraph()
{
    const dynamic_spare::PhysicalResourceLedger ledger(
        lineConfig(dynamic_spare::SharingTopology::PairSharing));
    for (std::size_t owner = 0; owner < 4; ++owner)
    {
        for (std::size_t borrower = 0; borrower < 4; ++borrower)
        {
            const bool expected =
                (owner == 0 && borrower == 1) ||
                (owner == 1 && borrower == 0) ||
                (owner == 2 && borrower == 3) ||
                (owner == 3 && borrower == 2);
            require(
                ledger.canBorrow(
                    static_cast<int>(owner), borrower,
                    dynamic_spare::SpareDimension::Row) == expected,
                "1x4 pair row graph is incorrect");
        }
    }
}

void verifyConservationAndSingleAllocation()
{
    auto config = lineConfig(
        dynamic_spare::SharingTopology::NeighborSharing);
    const dynamic_spare::PhysicalResourceLedger ledger(config);
    require(ledger.physicalRows() == 8 && ledger.physicalColumns() == 8,
            "1x4 neighbor policy changed physical spare totals");

    const std::array<dynamic_spare::SpareDemand, 4> localDemands{{
        {2, 2}, {2, 2}, {2, 2}, {2, 2}}};
    const auto local = ledger.allocate(localDemands);
    require(local.success && local.transfers.empty(),
            "local-first allocation borrowed for a locally repairable group");

    config.spareRows = 1;
    config.sharedRows = 1;
    const dynamic_spare::PhysicalResourceLedger scarce(config);
    const std::array<dynamic_spare::SpareDemand, 4> competing{{
        {1, 1}, {2, 1}, {0, 1}, {2, 1}}};
    const auto allocation = scarce.allocate(competing);
    require(!allocation.success && allocation.successfulBorrows == 1 &&
                allocation.failedBorrows == 1,
            "one scarce neighbor row was not allocated exactly once");
    std::set<std::size_t> physicalIds;
    for (const auto &transfer : allocation.transfers)
    {
        require(physicalIds.insert(transfer.physicalLineId).second,
                "one physical row was assigned to multiple borrowers");
    }
}

void verifyGlobalRowPool()
{
    auto config = lineConfig(dynamic_spare::SharingTopology::GlobalPool);
    config.globalPool = dynamic_spare::GlobalPoolConfiguration{1, 2, 4, 0};
    config.validate();
    const dynamic_spare::PhysicalResourceLedger ledger(config);
    require(ledger.physicalRows() == 8 && ledger.physicalColumns() == 8,
            "1x4 global row pool does not conserve physical spares");

    const std::array<dynamic_spare::SpareDemand, 4> demands{{
        {2, 2}, {1, 2}, {1, 2}, {1, 2}}};
    const auto allocation = ledger.allocate(demands);
    require(allocation.success && allocation.globalRowsUsed == 1 &&
                allocation.globalColumnsUsed == 0 &&
                allocation.remainingGlobalRows == 3,
            "1x4 global row-pool accounting is incorrect");
}

void verifyInvalidCombinations()
{
    auto invalid = lineConfig(dynamic_spare::SharingTopology::NoSharing);
    invalid.topology = dynamic_spare::SharingTopology::Directional;
    bool rejected = false;
    try
    {
        invalid.validate();
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    require(rejected, "1x4 directional topology was not rejected");

    invalid = lineConfig(dynamic_spare::SharingTopology::NeighborSharing);
    invalid.sharedColumns = 1;
    rejected = false;
    try
    {
        invalid.validate();
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    require(rejected, "1x4 column sharing was not rejected");

    invalid = lineConfig(dynamic_spare::SharingTopology::GlobalPool);
    invalid.globalPool =
        dynamic_spare::GlobalPoolConfiguration{1, 1, 4, 4};
    rejected = false;
    try
    {
        invalid.validate();
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    require(rejected, "1x4 global policy redistributed column spares");

    dynamic_spare::SimulationConfig grid;
    grid.layout = dynamic_spare::GroupLayout::Grid2x2;
    grid.topology = dynamic_spare::SharingTopology::NeighborSharing;
    rejected = false;
    try
    {
        grid.validate();
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    require(rejected, "1x4-only neighbor topology was accepted for 2x2");
}

void verifyFaultCorpusIndependence()
{
    dynamic_spare::SimulationConfig grid;
    grid.randomSeed = 20260820;
    grid.faultCount = 20;
    grid.memoryRows = 64;
    grid.memoryColumns = 64;
    const auto gridFaults =
        dynamic_spare::DynamicFaultGenerator(grid).generate(0);

    auto line = lineConfig(dynamic_spare::SharingTopology::NeighborSharing);
    line.randomSeed = grid.randomSeed;
    line.faultCount = grid.faultCount;
    line.memoryRows = grid.memoryRows;
    line.memoryColumns = grid.memoryColumns;
    const auto lineFaults =
        dynamic_spare::DynamicFaultGenerator(line).generate(0);

    for (std::size_t subarray = 0; subarray < 4; ++subarray)
    {
        require(gridFaults[subarray].size() == lineFaults[subarray].size(),
                "layout changed generated fault counts");
        for (std::size_t index = 0;
             index < gridFaults[subarray].size(); ++index)
        {
            const Fault &gridFault = gridFaults[subarray][index];
            const Fault &lineFault = lineFaults[subarray][index];
            require(gridFault.r == lineFault.r &&
                        gridFault.c == lineFault.c &&
                        gridFault.SubarrayID == lineFault.SubarrayID,
                    "layout or topology changed the generated fault corpus");
        }
    }

    const auto result = dynamic_spare::DynamicRepairSimulator().run(
        lineFaults, line, 0);
    require(result.layout == dynamic_spare::GroupLayout::Line1x4,
            "simulator result lost 1x4 layout metadata");
    require(result.sharing.borrowedColumns == 0 &&
                result.sharing.globalColumnsUsed == 0,
            "1x4 simulator reported column borrowing");
    if (result.groupRepairSuccess)
    {
        require(result.usedRows + result.unusedPhysicalRows == 8 &&
                    result.usedColumns + result.unusedPhysicalColumns == 8,
                "successful 1x4 repair did not conserve physical spares");
    }

    line.modifiers.localFirst = false;
    line.modifiers.maximumGroupBorrowedSpares = 2;
    const auto exhaustive = dynamic_spare::DynamicRepairSimulator().run(
        lineFaults, line, 0);
    bool middleCanRequestTwoRows = false;
    for (const auto &attempt : exhaustive.attemptsBySubarray[1])
    {
        require(attempt.availableColumns == line.spareColumns,
                "1x4 capacity enumeration changed column spares");
        middleCanRequestTwoRows = middleCanRequestTwoRows ||
            attempt.availableRows == line.spareRows + 2;
    }
    require(middleCanRequestTwoRows,
            "middle 1x4 SA cannot request one row from each neighbor");
}

} // namespace

int main()
{
    try
    {
        verifyNeighborGraph();
        verifyPairGraph();
        verifyConservationAndSingleAllocation();
        verifyGlobalRowPool();
        verifyInvalidCombinations();
        verifyFaultCorpusIndependence();
        std::cout << "Dynamic spare sharing layout tests passed\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
