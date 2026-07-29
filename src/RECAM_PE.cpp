#include "../inc/RECAM_PE.hpp"

void RECAM_PE::loadFaultsToCAMs(FaultList &faultList)
{
    // cout << " =========== Loading faults to CAMs  ===========" << endl;
    if (faultList.bufferCAM_overflow)
    {
        cout << "RECAM_PE::loadFaultsToCAMs" << endl;
        cout << "Warning: Unrepairable fault list" << endl;
        cout << "  - bufferCAM_overflow is true." ;
        cout << "  - Too many pivot faults, exceeding both address CAM and buffer CAM capacity." << endl;
    }
    isRepairable = !faultList.bufferCAM_overflow;
    addressCAM->addAddressCAMEntryFromList(&faultList);
    hybridCAM->addHybridCAMEntryFromList(&faultList, addressCAM.get());
    bufferCAM->addBufferCAMEntryFromList(&faultList);
}

void RECAM_PE::printPE()
{
    cout << "PE Configuration - Rs: " << Rs << ", Cs: " << Cs << ", Buffer Num: " << buff_num << endl;
    addressCAM->printAddressCAMEntries();
    hybridCAM->printHybridCAMEntries();
    bufferCAM->printBufferCAMEntries();
}

void RECAM_PE::genFaultAnalyzeMatrix()
{
    // cout << "Generating fault analyze matrix for hardware..." << endl;
    int addressCAMFullSize = matrixSize;
    int addressCAMEntriesCnt = addressCAM->addressCAMEntries.size();
    bool isAddressCAMEntriesFull = addressCAMEntriesCnt >= addressCAMFullSize;

    bool isfaultAnalyzeMatrixExtendable = !isAddressCAMEntriesFull;
    int faultAnalyzeMatrixRowCnt = addressCAMEntriesCnt;
    int faultAnalyzeMatrixColCnt = addressCAMEntriesCnt;
    int faultAnalyzeMatrixExtraRows = 0;
    int faultAnalyzeMatrixExtraCols = 0;

    for (int i_row = 0; i_row < matrixSize; ++i_row)
    {
        for (int i_col = 0; i_col < matrixSize; ++i_col)
        {
            bool rowMust = false;
            bool colMust = false;
            if (i_row <= addressCAMEntriesCnt - 1)
            {
                rowMust = addressCAM->addressCAMEntries[i_row].rowMust;
            }
            if (i_col <= addressCAMEntriesCnt - 1)
            {
                colMust = addressCAM->addressCAMEntries[i_col].colMust;
            }

            bool diagaonal = (i_row == i_col) && (i_row <= addressCAMEntriesCnt - 1);
            faultAnalyzeMatrixHardware[i_row][i_col] = diagaonal || rowMust || colMust; // || hasRowPtr || hasColPtr;
            // printFaultAnalyzeMatrix();
        }
    }


    vector<int> rowAddrs, colAddrs;
    for (auto &entry : addressCAM->addressCAMEntries)
    {
        if (entry.enable && entry.faultPtr != nullptr)
        {
            rowAddrs.push_back(entry.faultPtr->r);
            colAddrs.push_back(entry.faultPtr->c);
        }
    }

    /*
    Case 1: isfaultAnalyzeMatrixExtendable = false
      - Both row and column match: Set a specific location to 1.
      - Either row or column matches: Set the entire line to 1.
      - Neither row nor column matches: This case does not occur.

    Case 2: isfaultAnalyzeMatrixExtendable = true
      - Both row and column match: Set a specific location to 1.
      - Either row or column matches:
          - If a specific row or column can still be extended: Set a specific location to 1.
          - If no specific row or column can be extended: Set the entire line to 1.
      - If all extendable rows and columns are used: Set isfaultAnalyzeMatrixExtendable to false
    */

    // cout << "Updating fault analyze matrix for hardware based on Hybrid CAM entries..." << endl;
    for (auto &hyEntry : hybridCAM->hybridCAMEntries)
    {
        bool rowMatch = false, colMatch = false;
        int rowMatchIndex = -1, colMatchIndex = -1;
        for (int i_row = 0; i_row < rowAddrs.size(); ++i_row)
        {
            rowMatch = (hyEntry.faultPtr->r == rowAddrs[i_row]);
            if (rowMatch)
            {
                rowMatchIndex = i_row;
                break;
            }
        }
        for (int i_col = 0; i_col < colAddrs.size(); ++i_col)
        {
            colMatch = (hyEntry.faultPtr->c == colAddrs[i_col]);
            if (colMatch)
            {
                colMatchIndex = i_col;
                break;
            }
        }

        if (!isfaultAnalyzeMatrixExtendable)
        {
            // The original matrix need not to be extend
            if (rowMatch && colMatch)
            {
                faultAnalyzeMatrixHardware[rowMatchIndex][colMatchIndex] = true;
            }
            else if (rowMatch)
            {
                for (int c = 0; c < matrixSize; ++c)
                {
                    faultAnalyzeMatrixHardware[rowMatchIndex][c] = true;
                }
            }
            else if (colMatch)
            {
                for (int r = 0; r < matrixSize; ++r)
                {
                    faultAnalyzeMatrixHardware[r][colMatchIndex] = true;
                }
            }
            else
            {
                // This case should not happen
            }
        }
        else
        {
            if (rowMatch && colMatch)
            {
                faultAnalyzeMatrixHardware[rowMatchIndex][colMatchIndex] = true;
            }
            else if (rowMatch)
            {
                if (faultAnalyzeMatrixColCnt + faultAnalyzeMatrixExtraCols < addressCAMFullSize)
                {
                    faultAnalyzeMatrixHardware[rowMatchIndex][faultAnalyzeMatrixColCnt + faultAnalyzeMatrixExtraCols] = true;
                    colAddrs.push_back(hyEntry.faultPtr->c); // add the new column address to the list
                    faultAnalyzeMatrixExtraCols++;
                    if ((faultAnalyzeMatrixColCnt + faultAnalyzeMatrixExtraCols >= addressCAMFullSize) && (faultAnalyzeMatrixRowCnt + faultAnalyzeMatrixExtraRows >= addressCAMFullSize))
                    {
                        isfaultAnalyzeMatrixExtendable = false; // no more columns can be extended
                    }
                }
                else
                {
                    for (int c = 0; c < matrixSize; ++c)
                    {
                        faultAnalyzeMatrixHardware[rowMatchIndex][c] = true;
                    }
                }
            }
            else if (colMatch)
            {
                if (faultAnalyzeMatrixRowCnt + faultAnalyzeMatrixExtraRows < addressCAMFullSize)
                {
                    faultAnalyzeMatrixHardware[faultAnalyzeMatrixRowCnt + faultAnalyzeMatrixExtraRows][colMatchIndex] = true;
                    rowAddrs.push_back(hyEntry.faultPtr->r); // add the new row address to the list
                    faultAnalyzeMatrixExtraRows++;
                    if ((faultAnalyzeMatrixColCnt + faultAnalyzeMatrixExtraCols >= addressCAMFullSize) && (faultAnalyzeMatrixRowCnt + faultAnalyzeMatrixExtraRows >= addressCAMFullSize))
                    {
                        isfaultAnalyzeMatrixExtendable = false; // no more rows can be extended
                    }
                }
                else
                {
                    for (int r = 0; r < matrixSize; ++r)
                    {
                        faultAnalyzeMatrixHardware[r][colMatchIndex] = true;
                    }
                }
            }
            else
            {
                // This case should not happen
            }
        }
        // printFaultAnalyzeMatrix();
    }
}

void RECAM_PE::writeFaultAnalyzeMatrixToFile(const string &filename)
{
    ofstream outFile(filename);
    if (!outFile.is_open())
    {
        cerr << "Error: Could not open file " << filename << " for writing." << endl;
        return;
    }

    outFile << " ====== addressCAMEntry: ======" << endl;
    for (const auto &entry : addressCAM->addressCAMEntries)
    {
        if (entry.enable && entry.faultPtr != nullptr)
        {
            outFile << "Pivot Fault - Subarray: " << entry.faultPtr->SubarrayID
                    << " Channel: " << entry.faultPtr->ChannelID
                    << " Bank: " << entry.faultPtr->BankID
                    << " Row: " << entry.faultPtr->r
                    << " Col: " << entry.faultPtr->c
                    << " Row Must: " << (entry.rowMust ? "Yes" : "No")
                    << " Col Must: " << (entry.colMust ? "Yes" : "No")
                    << endl;
        }
    }

    outFile << " ====== hybridCAMEntry: ======" << endl;
    for (const auto &entry : hybridCAM->hybridCAMEntries)
    {
        if (entry.enable && entry.faultPtr != nullptr)
        {
            outFile << "Non-Pivot Fault - Subarray: " << entry.faultPtr->SubarrayID
                    << " Channel: " << entry.faultPtr->ChannelID
                    << " Bank: " << entry.faultPtr->BankID
                    << " Row: " << entry.faultPtr->r
                    << " Col: " << entry.faultPtr->c
                    << " Related Pivot Fault Pointer: " << entry.pointer
                    << " descriptorRowIsDiff: " << (entry.descriptorRowIsDiff ? "True" : "False")
                    << endl;
        }
    }

    outFile << "Fault Analyze Matrix for Hardware (1: Faulty Cell, 0: Non-Faulty Cell)" << endl;
    for (const auto &row : faultAnalyzeMatrixHardware)
    {
        for (const auto &cell : row)
        {
            outFile << (cell ? "1 " : "0 ");
        }
        outFile << endl;
    }

    outFile.close();
    // cout << "Fault analyze matrix has been written to " << filename << endl;
}

void RECAM_PE::printFaultAnalyzeMatrix()
{
    cout << "Fault Analyze Matrix for Hardware (1: Faulty Cell, 0: Non-Faulty Cell)" << endl;
    for (const auto &row : faultAnalyzeMatrixHardware)
    {
        for (const auto &cell : row)
        {
            cout << (cell ? "1 " : "0 ");
        }
        cout << endl;
    }
}

bool RECAM_PE::checkSolution(const solMatrix &solution, int solIndex)
{
    // cout << "  -Checking solution: " << solIndex << endl;
    for ( int i_row = 0; i_row < matrixSize; ++i_row){
        for ( int i_col = 0; i_col < matrixSize; ++i_col){
            if (!solution[i_row][i_col] && faultAnalyzeMatrixHardware[i_row][i_col]){
                // cout << "    -Solution is invalid." << endl;
                // cout << "    -Cell (" << i_row << ", " << i_col << ") is in fault area." << endl;
                return false;
            }
        }
    }
    // cout << "    -Solution: " << solIndex << " is valid." << endl;
    return true;
}

namespace
{
solVector deriveSolVectorFromMatrix(const solMatrix &solution, int matrixSize)
{
    solVector solutionVector(matrixSize, false);
    for (int col = 0; col < matrixSize; ++col)
    {
        bool isSpareColumn = true;
        for (int row = 0; row < matrixSize; ++row)
        {
            if (!solution[row][col])
            {
                isSpareColumn = false;
                break;
            }
        }
        solutionVector[col] = isSpareColumn;
    }
    return solutionVector;
}
}

RemapTable::AddressEntry RECAM_PE::buildAddressEntryFromFault(const Fault &fault) const
{
    RemapTable::AddressEntry addressEntry;
    addressEntry.HBMID = fault.HBMID;
    addressEntry.ChannelID = fault.ChannelID;
    addressEntry.BankID = fault.BankID;
    addressEntry.SubarrayGroupID = fault.SubarrayGroupID;
    addressEntry.SubarrayID = fault.SubarrayID;
    addressEntry.r = fault.r;
    addressEntry.c = fault.c;
    return addressEntry;
}

RemapTable RECAM_PE::buildRemapTable(const solVector &solutionVector) const
{
    RemapTable table;
    int nextSpareRowAddr = 0;
    int nextSpareColAddr = 0;

    const size_t addressCAMCount = std::min(solutionVector.size(), addressCAM->addressCAMEntries.size());
    for (size_t index = 0; index < addressCAMCount; ++index)
    {
        const AddressCAMEntry &camEntry = addressCAM->addressCAMEntries[index];
        if (!camEntry.enable || camEntry.faultPtr == nullptr)
        {
            continue;
        }

        const bool isSpareColumn = solutionVector[index];
        RemapTable::RemapEntry remapEntry;
        remapEntry.addressEntry = buildAddressEntryFromFault(*camEntry.faultPtr);
        remapEntry.isSpareRow = !isSpareColumn;
        remapEntry.newRowColAddr = isSpareColumn ? nextSpareColAddr++ : nextSpareRowAddr++;
        remapEntry.Latency = RemapTable::kDefaultRemapLatency;
        table.addRemapEntry(remapEntry);
    }
    return table;
}

void RECAM_PE::genValidSolList(const vector<solMatrix> &allSolutions)
{
    validSolList.clear();
    remapTableList.clear();
    RepairSuccess = false;

    for (size_t solIndex = 0; solIndex < allSolutions.size(); ++solIndex)
    {
        const solMatrix &solution = allSolutions[solIndex];
        if (checkSolution(solution, static_cast<int>(solIndex)))
        {
            validSolList.push_back(static_cast<int>(solIndex));
            remapTableList.push_back(buildRemapTable(deriveSolVectorFromMatrix(solution, matrixSize)));
        }
    }

    RepairSuccess = isRepairable && !validSolList.empty();
}

void RECAM_PE::printValidSolList()
{
    cout << " ======== RECAM_PE::printValidSolList() ==============" << endl;
    cout << "Have chance to be repaired: " << (isRepairable ? "Yes" : "No") << endl;
    if (isRepairable)
    {
        cout << "Valid Solution List(might be empty): " << endl;
    }
    else
    {
        cout << "Warning: No valid solution due to unrepairable faults." << endl;
        cout << "Valid Solution List is for reference only. ( ignore  the overflow pivot fault)." << endl;
    }

    cout << "  - Solution Index: ";
    for (int solIndex : validSolList)
    {
        cout << solIndex << " ";
    }
    cout << endl;
}
