#ifndef RECAM_PE_HPP
#define RECAM_PE_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip> // for std::setw

// #include "inc/RECAM_addressCAM.hpp"
#include "./SolGenerator.hpp"
#include "./RECAM_CAM.hpp"
#include "./RECAM_bufferCAM.hpp"

using namespace std;

class RECAM_PE
{
public:
    int Rs ; // number of spare rows
    int Cs ; // number of spare columns
    int buff_num ;
    int matrixSize ;
    bool isRepairable = false;
    bool RepairSuccess = false;

    std::unique_ptr<RECAM_addressCAM> addressCAM;
    std::unique_ptr<RECAM_hybridCAM> hybridCAM;
    std::unique_ptr<RECAM_bufferCAM> bufferCAM ;

    using FaultAnalyzeMatrix = vector<vector<bool>>;
    FaultAnalyzeMatrix faultAnalyzeMatrixHardware;
    std::vector<int> validSolList; // store the index of valid solutions

    RECAM_PE(int r_spare, int c_spare, int buff_num)
        : Rs(r_spare), Cs(c_spare), buff_num(buff_num), matrixSize(r_spare + c_spare),
          addressCAM(std::make_unique<RECAM_addressCAM>(r_spare, c_spare, buff_num)),
          hybridCAM(std::make_unique<RECAM_hybridCAM>(r_spare, c_spare, buff_num)),
          bufferCAM(std::make_unique<RECAM_bufferCAM>(buff_num))
    {
        faultAnalyzeMatrixHardware.resize(matrixSize, vector<bool>(matrixSize, false));
    }

    void loadFaultsToCAMs(FaultList &faultList);
    void printPE() ;
    void genFaultAnalyzeMatrix() ;
    void writeFaultAnalyzeMatrixToFile(const string &filename);
    void printFaultAnalyzeMatrix() ;
    bool checkSolution(const solMatrix &solution, int solIndex);
    void genValidSolList(const vector<solMatrix> &allSolutions);
    void printValidSolList() ;
};


#endif // RECAM_PE_HPP