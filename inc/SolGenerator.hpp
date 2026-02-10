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


using namespace std;


using solVector = std::vector<bool>;              // Col (true),  Row (false)
using solMatrix = std::vector<std::vector<bool>>; // [Row][Col]

class SolGenerator
{
public:
    int Rs, Cs;
    int matrixSize;

    // ======  generate all combinations of solutions =============
    // 1.generate all combinations of selecting Rs rows from Rs + Cs
    //  - Must generate the first solvector in dictionary order !!!!!
    // 2. for each combination, generate the solution matrix
    std::vector<solVector> allSolVectorsType;         // Col (true),  Row (false)
    std::vector<solMatrix> allSolMatrixsType;         // allSolutions[solIndex][Row][Col]


    SolGenerator(int r_spare, int c_spare) : Rs(r_spare), Cs(c_spare)
    {
        matrixSize = Rs + Cs;
        initialize_allSolVectorsType();
        initialize_allSolMatrixsType();
        // printAllSolVectors(true);
        // printAllSolMatrixs(true);
        writeAllSolVectorsToFile(true);
        writeAllSolMatrixsToFile(true);
    };


    void initialize_allSolVectorsType()
    {
        solVector solvector = solVector(Rs + Cs, false);
        fill(solvector.begin(), solvector.begin() + Cs, true); // 0~Cs-1  are Col (true), the rest are Row (false)
        do
        {
            allSolVectorsType.push_back(solvector);
        } while (prev_permutation(solvector.begin(), solvector.end())); // gerate all combinations
    }

    void initialize_allSolMatrixsType()
    {
        for (int i = 0; i < allSolVectorsType.size(); ++i)
        {
            solMatrix solmatrix(matrixSize, vector<bool>(matrixSize, false));
            solVector &solvector = allSolVectorsType[i];
            for (int j = 0; j < solvector.size(); ++j)
            {
                if (solvector[j] == false)
                { // spare row
                    for (int c = 0; c < matrixSize; ++c)
                    {
                        solmatrix[j][c] = true;
                    }
                }
                else
                { // spare column
                    for (int r = 0; r < matrixSize; ++r)
                    {
                        solmatrix[r][j] = true;
                    }
                }
            }
            allSolMatrixsType.push_back(solmatrix);
        }
    }

    void writeAllSolVectorsToFile(bool writeToFile)
    {
        if (writeToFile)
        {
            ofstream outFile("AllSolVectors.txt");
            if (!outFile.is_open())
            {
                cerr << "Error: Unable to open file AllSolVectors.txt for writing." << endl;
                return;
            }

            outFile << "=========== All Solution Vectors =============" << endl;
            for (int i = 0; i < allSolVectorsType.size(); ++i)
            {
                outFile << "Solution Vector " << i << ": ";
                for (bool sel : allSolVectorsType[i])
                {
                    outFile << (sel ? "C " : "R "); // true (C)，false (R)
                }
                outFile << endl;
            }

            outFile.close();
            cout << "All solution vectors have been written to AllSolVectors.txt" << endl;
        }
    }

    void writeAllSolMatrixsToFile(bool writeToFile)
    {
        if (writeToFile)
        {
            ofstream outFile("AllSolMatrixs.txt");
            if (!outFile.is_open())
            {
                cerr << "Error: Unable to open file AllSolMatrixs.txt for writing." << endl;
                return;
            }

            outFile << "=========== All Solution Matrixs =============" << endl;
            for (int i = 0; i < allSolMatrixsType.size(); ++i)
            {
                outFile << "Solution Matrix " << i << ":" << endl;
                for (int r = 0; r < matrixSize; ++r)
                {
                    for (int c = 0; c < matrixSize; ++c)
                    {
                        outFile << setw(4) << (allSolMatrixsType[i][r][c] ? " 1 " : " 0 ");
                    }
                    outFile << endl;
                }
                outFile << endl;
            }

            outFile.close();
            cout << "All solution matrices have been written to AllSolMatrixs.txt" << endl;
        }
    }
};

#endif