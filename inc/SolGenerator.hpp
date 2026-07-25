#ifndef SOLGENERATOR_HPP
#define SOLGENERATOR_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip> // for std::setw
#include <unordered_map>

#include "./PairHash.hpp"

using namespace std;


using solVector = std::vector<bool>;              // Col (true),  Row (false)
using solMatrix = std::vector<std::vector<bool>>; // [Row idx][Col idx]
using AllSolVectorsType = std::vector<solVector>; // allSolutions[solIndex][Row][Col]
using AllSolMatrixsType = std::vector<solMatrix>; // allSolutions[solIndex][Row][Col]
// Candidate name for backward compatibility
using SolVectorsList = AllSolVectorsType;
using SolMatrixsList = AllSolMatrixsType;

// Generate solutions for different spare line configurations in one SolGenerator.
//  -key  : ( spare row cnt, spare col cnt )
//  -value: list of solIndex in allSolVectorsType and allSolMatrixsType
using SolVecForSpareCfgs = std::unordered_map<std::pair<int, int>, SolVectorsList, PairHash>;
using SolMatForSpareCfgs = std::unordered_map<std::pair<int, int>, SolMatrixsList, PairHash>;
// Candidate name for backward compatibility
using SolVecForSpares = SolVecForSpareCfgs;
using SolMatForSpares = SolMatForSpareCfgs;

class SolGenerator
{
public:
    int Rs, Cs;
    int matrixSize;

    // ======  generate all combinations of solutions =============
    // 1.Decide the order of Rs spare rows and Cs spare columns from (Rs + Cs) spare lines.
    // 2.For each combination, generate the solution matrix

    // All the combinations of solutions for spare lines (Rs + Cs)
    AllSolVectorsType allSolVectorsType;
    AllSolMatrixsType allSolMatrixsType;

    //Generate solutions for different spare line configurations in one SolGenerator.
    SolVecForSpareCfgs solVecForSpares;
    SolMatForSpareCfgs solMatForSpares;

    SolGenerator(int r_spare, int c_spare) : Rs(r_spare), Cs(c_spare)
    {
        matrixSize = Rs + Cs;
        initialize_allSolVectorsType();
        initialize_allSolMatrixsType();
    };

    void initialize_allSolVectorsType();
    void initialize_allSolMatrixsType();

    void writeAllSolVectorsToFile(string fileName) ;
    void writeAllSolMatrixsToFile(string fileName);

    void genSolVecForSpares(int r_spare, int c_spare);
    void genSolMatForSpares(int r_spare, int c_spare);
    void genSolForSpares(int r_spare, int c_spare);

    void writeSolVecForSparesToFile(string fileName, int r_spare, int c_spare);
    void writeSolMatForSparesToFile(string fileName, int r_spare, int c_spare);
    void writeAllSolMatForSparesToFile(string fileName);
    void writeAllSolVecForSparesToFile(string fileName);
};

#endif