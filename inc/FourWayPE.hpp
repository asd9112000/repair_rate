/*
    This programs aims to know the repair efficiency of different spare line configurations (Rs, Cs) for a PE array.
      - The same fault patterns
      - Different spare line configurations: (Rs, Cs), (Rs-1, Cs), (Rs, Cs-1), (Rs-1, Cs-1)
*/
#ifndef FOURWAYPE_HPP
#define FOURWAYPE_HPP

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
#include "./RECAM_PE.hpp"
#include "./SolGenerator.hpp"
#include "./PairHash.hpp"

using namespace std;


/*
    ========== FourWayPE Class ==========
    set your own spare line for a single PE
        - PE_RsCs: (Rs, Cs)
        - This will try to fix the faults by 4 ways
            (Rs, Cs), (Rs-1, Cs), (Rs, Cs-1), (Rs-1, Cs-1)
*/

class FourWayPE
{
public:
    int Rs; // number of spare rows
    int Cs; // number of spare columns
    int RsRuduced;
    int CsRuduced;
    int buf_num;
    int PEIndex;
    bool usePaperCamReuse;

    RECAM_PE PE_RsCs;
    RECAM_PE PE_RsReduced;
    RECAM_PE PE_CsReduced;
    RECAM_PE PE_RsCsReduced;

    bool allWayRepairSuccess = false; // whether all 4 ways can repair the PE
    bool PERepairSuccess     = false; // whether at least one way can repair the PE


    vector<bool> perWayRepairSuccessList{false, false, false, false}; // ( RsCs, RsReduced, CsReduced, RsCsReduced )
    unordered_map<pair<int, int>, bool, PairHash> perWayRepairSuccessListForSparess; // key: (Rs, Cs)

    FourWayPE(
        int r,
        int c,
        int buf_num,
        int peIndex,
        bool use_paper_cam_reuse = false) :
        Rs(r), Cs(c), RsRuduced(r-1), CsRuduced(c-1), buf_num(buf_num),
        PEIndex(peIndex), usePaperCamReuse(use_paper_cam_reuse),
        PE_RsCs(
            r, c, use_paper_cam_reuse ? r + c : buf_num),
        PE_RsReduced(
            r - 1, c, use_paper_cam_reuse ? r - 1 + c : buf_num),
        PE_CsReduced(
            r, c - 1, use_paper_cam_reuse ? r + c - 1 : buf_num),
        PE_RsCsReduced(
            r - 1, c - 1,
            use_paper_cam_reuse ? r + c - 2 : buf_num)
    {};

    void loadFaultsToPEs(FaultList &faultListRsCs, FaultList &faultListRsReduced, FaultList &faultListCsReduced, FaultList &faultListRsCsReduced){
        PE_RsCs.loadFaultsToCAMs(faultListRsCs);
        PE_RsReduced.loadFaultsToCAMs(faultListRsReduced);
        PE_CsReduced.loadFaultsToCAMs(faultListCsReduced);
        PE_RsCsReduced.loadFaultsToCAMs(faultListRsCsReduced);
    }

    void genValidSolList(
        AllSolMatrixsType &allSolMatrixsTypeRsCS,
        AllSolMatrixsType &allSolMatrixsTypeRsReduced,
        AllSolMatrixsType &allSolMatrixsTypeCsReduced,
        AllSolMatrixsType &allSolMatrixsTypeRsCsReduced) {
        PE_RsCs.genValidSolList(allSolMatrixsTypeRsCS);
        PE_RsReduced.genValidSolList(allSolMatrixsTypeRsReduced);
        PE_CsReduced.genValidSolList(allSolMatrixsTypeCsReduced);
        PE_RsCsReduced.genValidSolList(allSolMatrixsTypeRsCsReduced);

        perWayRepairSuccessList = {
            PE_RsCs.RepairSuccess,
            PE_RsReduced.RepairSuccess,
            PE_CsReduced.RepairSuccess,
            PE_RsCsReduced.RepairSuccess
        };

        perWayRepairSuccessListForSparess[{Rs, Cs}] = PE_RsCs.RepairSuccess;
        perWayRepairSuccessListForSparess[{RsRuduced, Cs}] = PE_RsReduced.RepairSuccess;
        perWayRepairSuccessListForSparess[{Rs, CsRuduced}] = PE_CsReduced.RepairSuccess;
        perWayRepairSuccessListForSparess[{RsRuduced, CsRuduced}] = PE_RsCsReduced.RepairSuccess;

        allWayRepairSuccess = all_of(perWayRepairSuccessList.begin(), perWayRepairSuccessList.end(), [](bool success) {return success; });
        PERepairSuccess = PE_RsCs.RepairSuccess || PE_RsReduced.RepairSuccess || PE_CsReduced.RepairSuccess || PE_RsCsReduced.RepairSuccess;
    }

    void genFaultAnalyzeMatrix(){
        PE_RsCs.genFaultAnalyzeMatrix();
        PE_RsReduced.genFaultAnalyzeMatrix();
        PE_CsReduced.genFaultAnalyzeMatrix();
        PE_RsCsReduced.genFaultAnalyzeMatrix();
    }

    bool checkSolution(const solMatrix &solution, int solIndex);
    // void genValidSolList(const vector<solMatrix> &allSolutions);

    void printPE();
    void printFourPEs(){
        PE_RsCs.printPE();
        PE_RsReduced.printPE();
        PE_CsReduced.printPE();
        PE_RsCsReduced.printPE();
    }
    void printFaultAnalyzeMatrix();
    void printValidSolList(){
        PE_RsCs.printValidSolList();
        PE_RsReduced.printValidSolList();
        PE_CsReduced.printValidSolList();
        PE_RsCsReduced.printValidSolList();
    }

    void writeFaultAnalyzeMatrixToFile(const string &filename){
        PE_RsCs.writeFaultAnalyzeMatrixToFile(filename + "_PE_RsCs.txt");
        PE_RsReduced.writeFaultAnalyzeMatrixToFile(filename + "_PE_RsReduced.txt");
        PE_CsReduced.writeFaultAnalyzeMatrixToFile(filename + "_PE_CsReduced.txt");
        PE_RsCsReduced.writeFaultAnalyzeMatrixToFile(filename + "_PE_RsCsReduced.txt");
    }

};


#endif
