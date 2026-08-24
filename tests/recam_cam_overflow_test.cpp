#include <iostream>
#include <stdexcept>

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

FaultList makeHybridOverflowPattern(int bufferEntries)
{
    FaultList faults(1, 2, bufferEntries);
    faults.addFault(makeFault(0, 0));
    faults.addFault(makeFault(1, 1));
    faults.addFault(makeFault(2, 2));
    faults.addFault(makeFault(0, 1));
    faults.addFault(makeFault(1, 2));
    faults.classifyFaults();
    return faults;
}

void solve(RECAM_PE &pe, FaultList &faults)
{
    SolGenerator solutions(pe.Rs, pe.Cs);
    pe.loadFaultsToCAMs(faults);
    pe.genFaultAnalyzeMatrix();
    pe.genValidSolList(solutions.allSolMatrixsType);
}
}

int main()
{
    {
        // A disabled buffer must not make an otherwise representable fault
        // fail, and a successful remap must never contain BUFFMAP entries.
        FaultList faults(1, 1, 0);
        faults.addFault(makeFault(0, 0));
        faults.classifyFaults();
        RECAM_PE pe(1, 1, 0);
        solve(pe, faults);
        require(!pe.camStorageOverflow,
                "zero-capacity buffer caused a false CAM overflow");
        require(pe.RepairSuccess,
                "representable fault failed with the buffer disabled");
        require(!pe.remapTableList.empty(),
                "zero-buffer success did not produce a remap table");
        for (const RemapTable &table : pe.remapTableList)
        {
            require(table.BufferRemapEntries.empty(),
                    "zero-buffer success unexpectedly emitted BUFFMAP");
        }
    }

    {
        FaultList faults = makeHybridOverflowPattern(0);
        RECAM_PE pe(1, 2, 0);
        solve(pe, faults);
        require(pe.hybridCAM->hybridCAM_overflow,
                "Hybrid CAM overflow was not reported");
        require(pe.bufferCAM->bufferFaults.empty(),
                "zero-capacity buffer accepted a fault");
        require(pe.camStorageOverflow,
                "combined CAM exhaustion was not fatal");
        require(!pe.RepairSuccess,
                "PE succeeded after combined CAM exhaustion");
    }

    {
        FaultList faults = makeHybridOverflowPattern(1);
        RECAM_PE pe(1, 2, 1);
        solve(pe, faults);
        require(pe.hybridCAM->hybridCAM_overflow,
                "Hybrid CAM overflow was not reported");
        require(pe.bufferCAM->bufferFaults.empty(),
                "Hybrid overflow nonpivot was incorrectly moved to Buffer CAM");
        require(pe.camStorageOverflow && !pe.RepairSuccess,
                "Hybrid overflow was accepted because pivot-buffer space existed");
    }

    {
        // H=1. The first nonpivot fills Hybrid CAM. The next one raises
        // RowMust, so it must reclaim the old same-row entry before any
        // capacity failure; a later same-row fault also consumes no entry.
        FaultList faults(1, 2, 0);
        faults.addFault(makeFault(0, 0));
        faults.addFault(makeFault(0, 1));
        faults.addFault(makeFault(0, 2));
        faults.addFault(makeFault(0, 3));
        faults.classifyFaults();

        RECAM_PE pe(1, 2, 0);
        solve(pe, faults);
        require(pe.addressCAM->addressCAMEntries.front().rowMust,
                "must row was not raised before allocation");
        require(pe.hybridCAM->hybridCAMEntries.empty(),
                "must row did not clear its Hybrid CAM entries");
        require(!pe.hybridCAM->hybridCAM_overflow && !pe.camStorageOverflow,
                "full Hybrid CAM rejected a fault that triggered row must");
        require(pe.RepairSuccess,
                "row-must-covered faults failed in zero-buffer mode");
    }

    {
        // Column-symmetric must behavior.
        FaultList faults(2, 1, 0);
        faults.addFault(makeFault(0, 0));
        faults.addFault(makeFault(1, 0));
        faults.addFault(makeFault(2, 0));
        faults.addFault(makeFault(3, 0));
        faults.classifyFaults();

        RECAM_PE pe(2, 1, 0);
        solve(pe, faults);
        require(pe.addressCAM->addressCAMEntries.front().colMust,
                "must column was not raised before allocation");
        require(pe.hybridCAM->hybridCAMEntries.empty(),
                "must column did not clear its Hybrid CAM entries");
        require(!pe.hybridCAM->hybridCAM_overflow && !pe.camStorageOverflow,
                "full Hybrid CAM rejected a fault that triggered column must");
        require(pe.RepairSuccess,
                "column-must-covered faults failed in zero-buffer mode");
    }

    {
        FaultList faults(1, 2, 0);
        faults.addFault(makeFault(0, 0));
        faults.addFault(makeFault(1, 1));
        faults.addFault(makeFault(2, 2));
        faults.addFault(makeFault(3, 3));
        faults.classifyFaults();

        RECAM_PE pe(1, 2, 0);
        solve(pe, faults);
        require(faults.bufferCAM_overflow,
                "pivot overflow was not classified");
        require(pe.hybridCAM->hybridCAMEntries.empty(),
                "pivot overflow was incorrectly placed in Hybrid CAM");
        require(pe.camStorageOverflow,
                "unbuffered pivot overflow was not fatal");
        require(!pe.RepairSuccess,
                "PE succeeded after an unrepresented pivot overflow");
    }

    {
        FaultList faults(1, 1, 2);
        faults.addFault(makeFault(0, 0));
        faults.addFault(makeFault(1, 1));
        faults.addFault(makeFault(2, 2));
        faults.classifyFaults();
        RECAM_PE pe(1, 1, 2);
        solve(pe, faults);
        require(pe.RepairSuccess,
                "pivot-only buffer rejected an additional pivot");
        require(pe.bufferCAM->bufferFaults.size() == 1,
                "additional pivot was not stored in Buffer CAM");
        require(!pe.remapTableList.empty() &&
                    pe.remapTableList.front().BufferRemapEntries.size() == 1,
                "additional pivot was omitted from BUFFMAP data");
    }

    std::cout << "RECAM CAM overflow tests passed\n";
}
