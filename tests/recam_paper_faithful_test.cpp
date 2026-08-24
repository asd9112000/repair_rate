#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "RECAM_PE.hpp"

namespace
{

void require(bool condition, const char *message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

Fault makeFault(int row, int column)
{
    Fault fault{};
    fault.r = row;
    fault.c = column;
    fault.HBMID = 0;
    fault.ChannelID = 0;
    fault.BankID = 0;
    fault.SubarrayGroupID = 0;
    fault.SubarrayID = 0;
    return fault;
}

std::string vectorName(const solVector &solution)
{
    std::string name;
    for (bool isColumn : solution)
    {
        name += isColumn ? 'C' : 'R';
    }
    return name;
}

void solve(RECAM_PE &pe, FaultList &faults)
{
    SolGenerator solutions(pe.Rs, pe.Cs);
    pe.loadFaultsToCAMs(faults);
    pe.genFaultAnalyzeMatrix();
    pe.genValidSolList(solutions.allSolMatrixsType);
}

bool solveOrderedLPattern(
    const std::array<std::pair<int, int>, 3> &orderedFaults)
{
    FaultList faults(1, 1, 0);
    for (const auto &[row, column] : orderedFaults)
    {
        faults.addFault(makeFault(row, column));
    }
    faults.classifyFaults();
    RECAM_PE pe(1, 1, 0);
    solve(pe, faults);
    return pe.RepairSuccess;
}

} // namespace

int main()
{
    {
        SolGenerator solutions(2, 2);
        const std::array<std::string, 6> paperFig8 = {
            "RRCC", "RCRC", "RCCR", "CRRC", "CRCR", "CCRR"};
        require(solutions.allSolVectorsType.size() == paperFig8.size(),
                "Rs=Cs=2 did not produce six paper candidates");
        for (std::size_t index = 0; index < paperFig8.size(); ++index)
        {
            require(vectorName(solutions.allSolVectorsType[index]) == paperFig8[index],
                    "solution numbering does not match paper Fig. 8");
        }
    }

    {
        const std::array<std::size_t, 4> expectedCounts = {2, 6, 20, 70};
        for (int spareCount = 1; spareCount <= 4; ++spareCount)
        {
            SolGenerator solutions(spareCount, spareCount);
            require(
                solutions.allSolVectorsType.size() ==
                    expectedCounts[static_cast<std::size_t>(spareCount - 1)],
                "paper candidate count C(Rs+Cs,Rs) is incorrect");
            for (std::size_t solutionIndex = 0;
                 solutionIndex < solutions.allSolVectorsType.size();
                 ++solutionIndex)
            {
                const auto &vector = solutions.allSolVectorsType[solutionIndex];
                const auto &matrix = solutions.allSolMatrixsType[solutionIndex];
                for (int row = 0; row < spareCount * 2; ++row)
                {
                    for (int column = 0; column < spareCount * 2; ++column)
                    {
                        const bool expected =
                            !vector[static_cast<std::size_t>(row)] ||
                            vector[static_cast<std::size_t>(column)];
                        require(
                            matrix[static_cast<std::size_t>(row)]
                                  [static_cast<std::size_t>(column)] == expected,
                            "solution matrix is not Row_i OR Column_j");
                    }
                }
            }
        }
    }

    {
        SolGenerator solutions(2, 2);
        solutions.genSolMatForSpares(2, 2);
        require(solutions.solVecForSpares[{2, 2}] ==
                    solutions.allSolVectorsType,
                "cached solution vectors differ from direct generation");
        require(solutions.solMatForSpares[{2, 2}] ==
                    solutions.allSolMatrixsType,
                "cached solution matrices differ from direct generation");

        RECAM_PE pe(2, 2, 0);
        require(pe.checkSolution(solutions.allSolMatrixsType[0], 0),
                "paper solution #1 rejected an empty fault-free area");
        pe.faultAnalyzeMatrixHardware[3][1] = true;
        require(!pe.checkSolution(solutions.allSolMatrixsType[0], 0),
                "paper solution #1 accepted a fault in its uncovered area");
    }

    {
        FaultList faults(1, 1, 0);
        faults.addFault(makeFault(0, 0));
        faults.addFault(makeFault(0, 1));
        faults.addFault(makeFault(1, 0));
        faults.classifyFaults();

        RECAM_PE pe(1, 1, 0);
        solve(pe, faults);

        require(pe.addressCAM->addressCAMEntries.size() == 1,
                "L pattern should retain one greedy pivot");
        require(pe.addressCAM->addressCAMEntries[0].rowMust,
                "L pattern did not set row-must");
        require(pe.addressCAM->addressCAMEntries[0].colMust,
                "L pattern did not set column-must");
        require(!pe.camStorageOverflow,
                "paper false negative was incorrectly caused by CAM overflow");
        require(pe.faultAnalyzeMatrixHardware ==
                    RECAM_PE::FaultAnalyzeMatrix{{true, true}, {true, false}},
                "L pattern matrix does not match the paper-derived matrix");
        require(pe.validSolList.empty() && !pe.RepairSuccess,
                "paper-faithful L-pattern false negative was not preserved");

        // Independent physical oracle: row 0 covers (0,0),(0,1), and column
        // 0 covers (1,0).  This intentionally disagrees with RECAM.
        for (const Fault &fault : faults.PEFaults)
        {
            require(fault.r == 0 || fault.c == 0,
                    "physical row0+column0 oracle is unexpectedly invalid");
        }
    }

    {
        const std::array<std::array<std::pair<int, int>, 3>, 6> orders = {{
            {{{0, 0}, {0, 1}, {1, 0}}},
            {{{0, 0}, {1, 0}, {0, 1}}},
            {{{0, 1}, {0, 0}, {1, 0}}},
            {{{0, 1}, {1, 0}, {0, 0}}},
            {{{1, 0}, {0, 0}, {0, 1}}},
            {{{1, 0}, {0, 1}, {0, 0}}}
        }};
        const std::array<bool, 6> expected = {
            false, false, true, true, true, true};
        for (std::size_t index = 0; index < orders.size(); ++index)
        {
            require(solveOrderedLPattern(orders[index]) == expected[index],
                    "greedy pivot order sensitivity changed");
        }
    }

    {
        // Hybrid-CAM Case 1 assigns column 1 to a previously free matrix
        // index.  A selected extended axis must survive into MAP output.
        FaultList faults(1, 2, 0);
        faults.addFault(makeFault(0, 0));
        faults.addFault(makeFault(0, 1));
        faults.classifyFaults();
        RECAM_PE pe(1, 2, 0);
        solve(pe, faults);
        require(pe.RepairSuccess && !pe.remapTableList.empty(),
                "Hybrid extension fixture was not repairable");

        bool foundExtendedColumn = false;
        for (const RemapTable &table : pe.remapTableList)
        {
            for (const RemapTable::RemapEntry &entry : table.RemapEntries)
            {
                if (!entry.isSpareRow && entry.addressEntry.c == 1)
                {
                    foundExtendedColumn = true;
                }
            }
        }
        require(foundExtendedColumn,
                "Hybrid-extended matrix column was omitted from MAP data");
    }

    std::cout << "RECAM paper-faithful solution tests passed\n";
}
