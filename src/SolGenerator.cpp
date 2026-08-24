#include "../inc/SolGenerator.hpp"

void SolGenerator::initialize_allSolVectorsType()
{
    // Paper-faithful Fig. 8 ordering: RR...CC is solution #1 and
    // CC...RR is the last solution.  false denotes Row and true denotes
    // Column, so start with Rs rows followed by Cs columns and enumerate in
    // ascending lexicographic order.
    solVector solvector = solVector(Rs + Cs, false);
    fill(solvector.begin() + Rs, solvector.end(), true);
    do
    {
        allSolVectorsType.push_back(solvector);
    } while (next_permutation(solvector.begin(), solvector.end()));
};


void SolGenerator::initialize_allSolMatrixsType()
{
    for (size_t i = 0; i < allSolVectorsType.size(); ++i)
    {
        solMatrix solmatrix(matrixSize, vector<bool>(matrixSize, false));
        solVector &solvector = allSolVectorsType[i];
        for (size_t j = 0; j < solvector.size(); ++j)
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
};


void SolGenerator::writeAllSolVectorsToFile(string fileName)
{
    ofstream outFile(fileName);
    if (!outFile.is_open())
    {
        cerr << "Error: Unable to open file " << fileName << " for writing." << endl;
        return;
    }

    outFile << "=========== All Solution Vectors =============" << endl;
    for (size_t i = 0; i < allSolVectorsType.size(); ++i)
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
};


void SolGenerator::writeAllSolMatrixsToFile(string fileName)
{

    ofstream outFile(fileName);
    if (!outFile.is_open())
    {
        cerr << "Error: Unable to open file " << fileName << " for writing." << endl;
        return;
    }

    outFile << "=========== All Solution Matrixs =============" << endl;
    for (size_t i = 0; i < allSolMatrixsType.size(); ++i)
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
};


void SolGenerator::genSolVecForSpares(int r_spare, int c_spare)
{
    // check if the solution has already been generated
    if (solVecForSpares.find({r_spare, c_spare}) == solVecForSpares.end())
    {
        solVector solvector = solVector(r_spare + c_spare, false);
        AllSolVectorsType tmpAllSolVectorsType;
        // Keep the per-configuration solution indices identical to Fig. 8:
        // all Row choices first, then advance toward all Column choices.
        fill(solvector.begin() + r_spare, solvector.end(), true);
        do
        {
            tmpAllSolVectorsType.push_back(solvector);
        } while (next_permutation(solvector.begin(), solvector.end()));
        solVecForSpares.insert({{r_spare, c_spare}, tmpAllSolVectorsType});
    }
};


void SolGenerator::genSolMatForSpares(int r_spare, int c_spare)
{
    // This method is part of the public API; do not require callers to know
    // that vectors must be generated first.
    genSolVecForSpares(r_spare, c_spare);
    // check if the solution has already been generated
    if (solMatForSpares.find({r_spare, c_spare}) == solMatForSpares.end())
    {
        int tmpMatrixSize = r_spare + c_spare;
        AllSolMatrixsType tmpAllSolMatrixsType;
        for (size_t i = 0; i < solVecForSpares[{r_spare, c_spare}].size(); ++i)
        {
            solMatrix solmatrix(tmpMatrixSize, vector<bool>(tmpMatrixSize, false));
            solVector &solvector = solVecForSpares[{r_spare, c_spare}][i];
            for (size_t j = 0; j < solvector.size(); ++j)
            {
                if (solvector[j] == false)
                { // spare row
                    for (int c = 0; c < tmpMatrixSize; ++c)
                    {
                        solmatrix[j][c] = true;
                    }
                }
                else
                { // spare column
                    for (int r = 0; r < tmpMatrixSize; ++r)
                    {
                        solmatrix[r][j] = true;
                    }
                }
            }
            tmpAllSolMatrixsType.push_back(solmatrix);
        }
        solMatForSpares.insert({{r_spare, c_spare}, tmpAllSolMatrixsType});
    }
};


void SolGenerator::genSolForSpares(int r_spare, int c_spare)
{
    genSolVecForSpares(r_spare, c_spare);
    genSolMatForSpares(r_spare, c_spare);
};


void SolGenerator::writeSolVecForSparesToFile(string fileName, int r_spare, int c_spare)
{
    ofstream outFile(fileName);
    if (!outFile.is_open())
    {
        cerr << "Error: Unable to open file " << fileName << " for writing." << endl;
        return;
    }

    cout << "All solution vectors have been written to AllSolVectors.txt" << endl;
    outFile << "=========== solVecForSpares =============" << endl;
    for (size_t i = 0; i < solVecForSpares[{r_spare, c_spare}].size(); ++i)
    {
        outFile << "Solution Vector " << i << ": ";
        for (bool sel : solVecForSpares[{r_spare, c_spare}][i])
        {
            outFile << (sel ? "C " : "R "); // true (C)，false (R)
        }
        outFile << endl;
    }

    outFile.close();
};


void SolGenerator::writeSolMatForSparesToFile(string fileName, int r_spare, int c_spare)
{
    int tmpMatrixSize = r_spare + c_spare;
    ofstream outFile(fileName);
    if (!outFile.is_open())
    {
        cerr << "Error: Unable to open file " << fileName << " for writing." << endl;
        return;
    }

    outFile << "=========== solMatForSpares =============" << endl;
    for (size_t i = 0; i < solMatForSpares[{r_spare, c_spare}].size(); ++i)
    {
        outFile << "Solution Matrix " << i << ":" << endl;
        for (int r = 0; r < tmpMatrixSize; ++r)
        {
            for (int c = 0; c < tmpMatrixSize; ++c)
            {
                outFile << setw(4) << (solMatForSpares[{r_spare, c_spare}][i][r][c] ? " 1 " : " 0 ");
            }
            outFile << endl;
        }
        outFile << endl;
    }

    outFile.close();
    cout << "All solution matrices have been written to AllSolMatrixs.txt" << endl;
};

void SolGenerator::writeAllSolVecForSparesToFile(string fileName)
{
    for (auto &entry : solVecForSpares)
    {
        auto &key = entry.first;
        writeSolVecForSparesToFile(fileName + "_r" + to_string(key.rowCount) + "_c" + to_string(key.columnCount) + ".txt", key.rowCount, key.columnCount);
    }
};

void SolGenerator::writeAllSolMatForSparesToFile(string fileName)
{
    for (auto &entry : solMatForSpares)
    {
        auto &key = entry.first;
        writeSolMatForSparesToFile(fileName + "_r" + to_string(key.rowCount) + "_c" + to_string(key.columnCount) + ".txt", key.rowCount, key.columnCount);
    }
};
