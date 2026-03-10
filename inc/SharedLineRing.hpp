/*
    This programs aims to know the repair efficiency of different spare line configurations (Rs, Cs) for a PE array.
      - The same fault patterns
      - Different spare line configurations: (Rs, Cs), (Rs-1, Cs), (Rs, Cs-1), (Rs-1, Cs-1)
*/
#ifndef SHARED_LINE_RING_HPP
#define SHARED_LINE_RING_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip>    // for std::setw
#include <filesystem> // for std::filesystem::create_directories

// #include "inc/RECAM_addressCAM.hpp"
#include "FourWayPE.hpp"

using namespace std;


class SharedLinePE
{
    int Rs; // number of spare rows
    int Cs; // number of spare columns
    int RsRuduced;
    int CsRuduced;
    int buf_num;
    int PEIndex;
    bool leaveSpareRowFirst;
    int leaveRowCnt = 0;
    int leaveColCnt = 0;

    bool allWayRepairSuccess = false;
    bool PERepairSuccess = false;
    vector<bool> perWayRepairSuccessList{false, false, false, false};
    FourWayPE fourWayPE;

    SharedLinePE(int r, int c, int buf_num, int peIndex, bool leaveSpareRowFirst) :
        Rs(r), Cs(c), RsRuduced(r-1), CsRuduced(c-1), buf_num(buf_num),
        PEIndex(peIndex), leaveSpareRowFirst(leaveSpareRowFirst),
        fourWayPE(Rs, Cs, buf_num, peIndex)
    {
    };

    void loadFaultsToSharedLinePE(FaultList &faultListRsCs, FaultList &faultListRsReduced, FaultList &faultListCsReduced, FaultList &faultListRsCsReduced)
    {
        fourWayPE.loadFaultsToPEs(faultListRsCs, faultListRsReduced, faultListCsReduced, faultListRsCsReduced);
    }

    void genValidSolList(
        AllSolMatrixsType &allSolMatrixsTypeRsCS,
        AllSolMatrixsType &allSolMatrixsTypeRsReduced,
        AllSolMatrixsType &allSolMatrixsTypeCsReduced,
        AllSolMatrixsType &allSolMatrixsTypeRsCsReduced)
    {
        fourWayPE.genValidSolList(allSolMatrixsTypeRsCS, allSolMatrixsTypeRsReduced, allSolMatrixsTypeCsReduced, allSolMatrixsTypeRsCsReduced);
        perWayRepairSuccessList = fourWayPE.perWayRepairSuccessList;

        // vector<bool> perPERepairSuccessList{false, false, false, false}; // ( RsCs, RsReduced, CsReduced, RsCsReduced )
        allWayRepairSuccess = fourWayPE.allWayRepairSuccess;
        PERepairSuccess = fourWayPE.PERepairSuccess;
        if (perWayRepairSuccessList[3]) { //Rsc
            leaveRowCnt = 1;
            leaveColCnt = 1;
        }
        else if (leaveSpareRowFirst) {
            if (perWayRepairSuccessList[1]) { //RsReduced
                leaveRowCnt = 1;
                leaveColCnt = 0;
            }
            else if (perWayRepairSuccessList[2]) { //CsReduced
                leaveRowCnt = 0;
                leaveColCnt = 1;
            }
        }
        else if (!leaveSpareRowFirst) {
            if (perWayRepairSuccessList[2]) { //CsReduced
                leaveRowCnt = 0;
                leaveColCnt = 1;
            }
            else if (perWayRepairSuccessList[1]) { //RsReduced
                leaveRowCnt = 1;
                leaveColCnt = 0;
            }
        }


    }

};


class SharedLineRing
{
public:
};

#endif