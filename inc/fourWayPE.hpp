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

using namespace std;

class FourWayPE
{
public:
    int Rs; // number of spare rows
    int Cs; // number of spare columns
    int RsRuduced;
    int CsRuduced;
    int buf_num;
    int PEIndex;

    RECAM_PE PE_RsCs;
    RECAM_PE PE_RsReduced;
    RECAM_PE PE_CsReduced;
    RECAM_PE PE_RsCsReduced;

    bool fullRepairSuccess = false;
    vector<bool> perPERepairSuccessList {false, false, false, false};


    FourWayPE(int r, int c, int buf_num, int peIndex) :
        Rs(r), Cs(c), RsRuduced(r-1), CsRuduced(c-1), buf_num(buf_num), PEIndex(peIndex),
        PE_RsCs(Rs, Cs, buf_num),
        PE_RsReduced(RsRuduced, Cs, buf_num),
        PE_CsReduced(Rs, CsRuduced, buf_num),
        PE_RsCsReduced(RsRuduced, CsRuduced, buf_num)
    {};


    void printFourPEs(){
        PE_RsCs.printPE();
        PE_RsReduced.printPE();
        PE_CsReduced.printPE();
        PE_RsCsReduced.printPE();
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

        perPERepairSuccessList = {
            PE_RsCs.RepairSuccess,
            PE_RsReduced.RepairSuccess,
            PE_CsReduced.RepairSuccess,
            PE_RsCsReduced.RepairSuccess};
        fullRepairSuccess = all_of(perPERepairSuccessList.begin(), perPERepairSuccessList.end(), [](bool success) {return success; });
    }

    void loadFaultsToPEs(FaultList &faultListRsCs, FaultList &faultListRsReduced, FaultList &faultListCsReduced, FaultList &faultListRsCsReduced){
        PE_RsCs.loadFaultsToCAMs(faultListRsCs);
        PE_RsReduced.loadFaultsToCAMs(faultListRsReduced);
        PE_CsReduced.loadFaultsToCAMs(faultListCsReduced);
        PE_RsCsReduced.loadFaultsToCAMs(faultListRsCsReduced);
    }

    // void loadFaultsToCAMs(FaultList &faultListRsCs, FaultList &faultListRsReduced, FaultList &faultListCsReduced, FaultList &faultListRsCsReduced){
    //     PE_RsCs.loadFaultsToCAMs(faultListRsCs);
    //     PE_RsReduced.loadFaultsToCAMs(faultListRsReduced);
    //     PE_CsReduced.loadFaultsToCAMs(faultListCsReduced);
    //     PE_RsCsReduced.loadFaultsToCAMs(faultListRsCsReduced);
    // }

    void genFaultAnalyzeMatrix(){
        PE_RsCs.genFaultAnalyzeMatrix();
        PE_RsReduced.genFaultAnalyzeMatrix();
        PE_CsReduced.genFaultAnalyzeMatrix();
        PE_RsCsReduced.genFaultAnalyzeMatrix();
    }

    void printPE();
    void writeFaultAnalyzeMatrixToFile(const string &filename){
        PE_RsCs.writeFaultAnalyzeMatrixToFile(filename + "_PE_RsCs.txt");
        PE_RsReduced.writeFaultAnalyzeMatrixToFile(filename + "_PE_RsReduced.txt");
        PE_CsReduced.writeFaultAnalyzeMatrixToFile(filename + "_PE_CsReduced.txt");
        PE_RsCsReduced.writeFaultAnalyzeMatrixToFile(filename + "_PE_RsCsReduced.txt");
    }
    void printFaultAnalyzeMatrix();
    bool checkSolution(const solMatrix &solution, int solIndex);
    void genValidSolList(const vector<solMatrix> &allSolutions);
    void printValidSolList(){
        PE_RsCs.printValidSolList();
        PE_RsReduced.printValidSolList();
        PE_CsReduced.printValidSolList();
        PE_RsCsReduced.printValidSolList();

    }


};


#endif