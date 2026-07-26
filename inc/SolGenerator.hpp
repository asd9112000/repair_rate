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

// A solution represented as line selections.
// true: spare column
// false: spare row
using SolutionVector = std::vector<bool>;

// A solution represented as a 2D repair map: [row][column].
using SolutionMatrix = std::vector<std::vector<bool>>;

// Collections indexed by solution ID.
using SolutionVectorList = std::vector<SolutionVector>; // [solutionIndex]
using SolutionMatrixList = std::vector<SolutionMatrix>; // [solutionIndex]

// Spare-line configuration used to group generated solutions.
struct SpareConfig
{
    int rowCount{};
    int columnCount{};

    bool operator==(const SpareConfig &other) const noexcept
    {
        return rowCount == other.rowCount &&
               columnCount == other.columnCount;
    }
};

struct SpareConfigHash
{
    std::size_t operator()(const SpareConfig &config) const noexcept
    {
        const auto rowHash = std::hash<int>{}(config.rowCount);
        const auto columnHash = std::hash<int>{}(config.columnCount);

        return rowHash ^ (columnHash << 1);
    }
};

// Generated solutions grouped by spare-line configuration.
using SolutionVectorsBySpareConfig =
    std::unordered_map<SpareConfig, SolutionVectorList, SpareConfigHash>;

using SolutionMatricesBySpareConfig =
    std::unordered_map<SpareConfig, SolutionMatrixList, SpareConfigHash>;

// Deprecated compatibility aliases.
// Remove after all call sites have migrated to the new names.
using solVector = SolutionVector;
using solMatrix = SolutionMatrix;
using AllSolVectorsType = SolutionVectorList;
using AllSolMatrixsType = SolutionMatrixList;
using SolVectorsList = SolutionVectorList;
using SolMatrixsList = SolutionMatrixList;
using SolVecForSpareCfgs = SolutionVectorsBySpareConfig;
using SolMatForSpareCfgs = SolutionMatricesBySpareConfig;
using SolVecForSpares = SolutionVectorsBySpareConfig;
using SolMatForSpares = SolutionMatricesBySpareConfig;


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