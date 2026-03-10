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
using solMatrix = std::vector<std::vector<bool>>; // [Row][Col]
using AllSolVectorsType = std::vector<solVector>; // allSolutions[solIndex][Row][Col]
using AllSolMatrixsType = std::vector<solMatrix>; // allSolutions[solIndex][Row][Col]
using SolVectorsList = std::vector<solVector>; // allSolutions[solIndex][Row][Col]
using SolMatrixsList = std::vector<solMatrix>; // allSolutions[solIndex][Row][Col]

using SolVecForSpares = std::unordered_map<std::pair<int, int>, SolVectorsList, PairHash>;    // key: ( spare row cnt, spare col cnt ), value: list of solIndex in allSolVectorsType and allSolMatrixsType
using SolMatForSpares = std::unordered_map<std::pair<int, int>, SolMatrixsList, PairHash>; // key: ( spare row cnt, spare col cnt ), value: list of solIndex in allSolVectorsType and allSolMatrixsType

class SolGenerator
{
public:
    int Rs, Cs;
    int matrixSize;

    // ======  generate all combinations of solutions =============
    // 1.generate all combinations of selecting Rs rows from Rs + Cs
    //  - Must generate the first solvector in dictionary order !!!!!
    // 2. for each combination, generate the solution matrix

    AllSolVectorsType allSolVectorsType;
    AllSolMatrixsType allSolMatrixsType;

    SolVecForSpares solVecForSpares;
    SolMatForSpares solMatForSpares;

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