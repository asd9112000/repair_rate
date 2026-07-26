#include "../inc/SolGenerator.hpp"

void SolGenerator::initialize_allSolVectorsType()
{
    solVector solvector = solVector(Rs + Cs, false);
    fill(solvector.begin(), solvector.begin() + Cs, true); // 0~Cs-1  are Col (true), the rest are Row (false)
    do
    {
        allSolVectorsType.push_back(solvector);
    } while (prev_permutation(solvector.begin(), solvector.end())); // gerate all combinations
};


void SolGenerator::initialize_allSolMatrixsType()
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
};


void SolGenerator::genSolVecForSpares(int r_spare, int c_spare)
{
    // check if the solution has already been generated
    if (solVecForSpares.find({r_spare, c_spare}) == solVecForSpares.end())
    {
        solVector solvector = solVector(r_spare + c_spare, false);
        AllSolVectorsType tmpAllSolVectorsType;
        fill(solvector.begin(), solvector.begin() + c_spare, true); // 0~c_spare-1  are Col (true), the rest are Row (false)
        do
        {
            tmpAllSolVectorsType.push_back(solvector);
        } while (prev_permutation(solvector.begin(), solvector.end())); // gerate all combinations
        solVecForSpares.insert({{r_spare, c_spare}, tmpAllSolVectorsType});
    }
};


void SolGenerator::genSolMatForSpares(int r_spare, int c_spare)
{
    // check if the solution has already been generated
    if (solMatForSpares.find({r_spare, c_spare}) == solMatForSpares.end())
    {
        int tmpMatrixSize = r_spare + c_spare;
        AllSolMatrixsType tmpAllSolMatrixsType;
        for (int i = 0; i < solVecForSpares[{r_spare, c_spare}].size(); ++i)
        {
            solMatrix solmatrix(tmpMatrixSize, vector<bool>(tmpMatrixSize, false));
            solVector &solvector = solVecForSpares[{r_spare, c_spare}][i];
            for (int j = 0; j < solvector.size(); ++j)
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
    for (int i = 0; i < solVecForSpares[{r_spare, c_spare}].size(); ++i)
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
    for (int i = 0; i < solMatForSpares[{r_spare, c_spare}].size(); ++i)
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
        auto &value = entry.second;
        writeSolVecForSparesToFile(fileName + "_r" + to_string(key.rowCount) + "_c" + to_string(key.columnCount) + ".txt", key.rowCount, key.columnCount);
    }
};

void SolGenerator::writeAllSolMatForSparesToFile(string fileName)
{
    for (auto &entry : solMatForSpares)
    {
        auto &key = entry.first;
        auto &value = entry.second;
        writeSolMatForSparesToFile(fileName + "_r" + to_string(key.rowCount) + "_c" + to_string(key.columnCount) + ".txt", key.rowCount, key.columnCount);
    }
};
