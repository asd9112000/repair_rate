#include "../inc/RECAM_PE.hpp"

#include <set>

void RECAM_PE::loadFaultsToCAMs(FaultList &faultList)
{
    // cout << " =========== Loading faults to CAMs  ===========" << endl;
    addressCAM->addAddressCAMEntryFromList(&faultList);

    // Overflow pivot faults require full-address CAM-reuse entries.  Load
    // those first because a Hybrid-CAM entry cannot represent a pivot that
    // has no Address-CAM pointer.
    const bool pivotBufferLoaded = bufferCAM->addBufferCAMEntryFromList(&faultList);

    // Buffer CAM is reserved for additional pivots. A nonpivot that cannot be
    // represented by Hybrid CAM is unrepairable in every operating mode.
    const std::vector<Fault *> hybridOverflowFaults =
        hybridCAM->addHybridCAMEntryFromList(&faultList, addressCAM.get());

    camStorageOverflow = faultList.bufferCAM_overflow ||
                         !pivotBufferLoaded ||
                         !hybridOverflowFaults.empty();
    isRepairable = !camStorageOverflow;

    if (camStorageOverflow)
    {

        std::ostringstream message;
        message << "RECAM_PE::loadFaultsToCAMs" << endl;
        message << (buff_num == 0
            ? "Result: Unrepairable fault list in buffer-disabled mode"
            : "Warning: Unrepairable fault list") << endl;
        message << "  - CAM collection capacity is exhausted." << endl;
        if (!hybridOverflowFaults.empty())
        {
            message << "  - Hybrid CAM has no entry for a required nonpivot; "
                 << "Buffer CAM accepts additional pivots only." << endl;
        }
        else if (buff_num == 0)
        {
            message << "  - Buffer CAM capacity is 0; an additional pivot cannot "
                 << "fall back to CAM reuse." << endl;
        }
        else
        {
            message << "  - Address CAM and the additional-pivot buffer have no "
                 << "remaining entry." << endl;
        }

        // cout << message.str() << endl;


    }
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

    std::fill(matrixRowAddresses.begin(), matrixRowAddresses.end(), std::nullopt);
    std::fill(matrixColumnAddresses.begin(), matrixColumnAddresses.end(), std::nullopt);

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
    for (size_t index = 0; index < addressCAM->addressCAMEntries.size(); ++index)
    {
        auto &entry = addressCAM->addressCAMEntries[index];
        if (entry.enable && entry.faultPtr != nullptr)
        {
            rowAddrs.push_back(entry.faultPtr->r);
            colAddrs.push_back(entry.faultPtr->c);
            if (index < matrixRowAddresses.size())
            {
                const auto address = buildAddressEntryFromFault(*entry.faultPtr);
                matrixRowAddresses[index] = address;
                matrixColumnAddresses[index] = address;
            }
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
        for (size_t i_row = 0; i_row < rowAddrs.size(); ++i_row)
        {
            rowMatch = (hyEntry.faultPtr->r == rowAddrs[i_row]);
            if (rowMatch)
            {
                rowMatchIndex = static_cast<int>(i_row);
                break;
            }
        }
        for (size_t i_col = 0; i_col < colAddrs.size(); ++i_col)
        {
            colMatch = (hyEntry.faultPtr->c == colAddrs[i_col]);
            if (colMatch)
            {
                colMatchIndex = static_cast<int>(i_col);
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
                    const int extendedColumnIndex =
                        faultAnalyzeMatrixColCnt + faultAnalyzeMatrixExtraCols;
                    faultAnalyzeMatrixHardware[rowMatchIndex][extendedColumnIndex] = true;
                    matrixColumnAddresses[extendedColumnIndex] =
                        buildAddressEntryFromFault(*hyEntry.faultPtr);
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
                    const int extendedRowIndex =
                        faultAnalyzeMatrixRowCnt + faultAnalyzeMatrixExtraRows;
                    faultAnalyzeMatrixHardware[extendedRowIndex][colMatchIndex] = true;
                    matrixRowAddresses[extendedRowIndex] =
                        buildAddressEntryFromFault(*hyEntry.faultPtr);
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
    (void)solIndex;
    if (solution.size() != static_cast<size_t>(matrixSize) ||
        std::any_of(solution.begin(), solution.end(),
                    [this](const std::vector<bool> &row)
                    {
                        return row.size() != static_cast<size_t>(matrixSize);
                    }))
    {
        throw std::invalid_argument(
            "RECAM solution matrix dimensions do not match Rs + Cs");
    }
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
size_t paperSolutionCount(int spareRows, int spareColumns)
{
    const int total = spareRows + spareColumns;
    const int choose = std::min(spareRows, spareColumns);
    size_t result = 1;
    for (int divisor = 1; divisor <= choose; ++divisor)
    {
        result = result * static_cast<size_t>(total - choose + divisor) /
                 static_cast<size_t>(divisor);
    }
    return result;
}

solVector deriveSolVectorFromMatrix(
    const solMatrix &solution,
    int matrixSize,
    int spareRows,
    int spareColumns)
{
    if (spareColumns == 0)
    {
        return solVector(matrixSize, false);
    }
    if (spareRows == 0)
    {
        return solVector(matrixSize, true);
    }

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

bool matchesPaperSolutionMatrix(
    const solMatrix &solution,
    const solVector &solutionVector,
    int matrixSize)
{
    for (int row = 0; row < matrixSize; ++row)
    {
        for (int column = 0; column < matrixSize; ++column)
        {
            const bool expected =
                !solutionVector[static_cast<size_t>(row)] ||
                solutionVector[static_cast<size_t>(column)];
            if (solution[static_cast<size_t>(row)]
                        [static_cast<size_t>(column)] != expected)
            {
                return false;
            }
        }
    }
    return true;
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
    // Keep spare addresses outside the normal 0..1023 address space used by
    // the current fault generator.  The 999-prefix is an output namespace,
    // not a physical normal-memory address.
    int nextSpareRowAddr = 999000;
    int nextSpareColAddr = 999000;

    const size_t matrixAddressCount = std::min(
        solutionVector.size(), matrixRowAddresses.size());
    for (size_t index = 0; index < matrixAddressCount; ++index)
    {
        const bool isSpareColumn = solutionVector[index];
        const auto &physicalAddress = isSpareColumn
            ? matrixColumnAddresses[index]
            : matrixRowAddresses[index];
        if (!physicalAddress.has_value())
        {
            // Paper candidates always assign R or C to every logical index,
            // but an unused matrix slot has no physical line to remap.
            continue;
        }

        RemapTable::RemapEntry remapEntry;
        remapEntry.addressEntry = *physicalAddress;
        remapEntry.isSpareRow = !isSpareColumn;
        remapEntry.newRowColAddr = isSpareColumn ? nextSpareColAddr++ : nextSpareRowAddr++;
        remapEntry.Latency = RemapTable::kDefaultRemapLatency;
        table.addRemapEntry(remapEntry);
    }

    for (const BufferCAMEntry &bufferEntry : bufferCAM->bufferFaults)
    {
        if (bufferEntry.faultPtr == nullptr)
        {
            continue;
        }
        RemapTable::BufferRemapEntry remapEntry;
        remapEntry.addressEntry = buildAddressEntryFromFault(*bufferEntry.faultPtr);
        remapEntry.Latency = RemapTable::kDefaultCamReuseLatency;
        table.addBufferRemapEntry(remapEntry);
    }
    return table;
}

void RECAM_PE::genValidSolList(const vector<solMatrix> &allSolutions)
{
    validSolList.clear();
    remapTableList.clear();
    RepairSuccess = false;

    const size_t expectedSolutionCount = paperSolutionCount(Rs, Cs);
    if (allSolutions.size() != expectedSolutionCount)
    {
        throw std::invalid_argument(
            "RECAM candidate count does not equal C(Rs+Cs,Rs)");
    }

    std::vector<solVector> solutionVectors;
    solutionVectors.reserve(allSolutions.size());
    std::set<solVector> uniqueSolutionVectors;
    for (size_t solIndex = 0; solIndex < allSolutions.size(); ++solIndex)
    {
        const solMatrix &solution = allSolutions[solIndex];
        // checkSolution performs the common KxK dimension validation.
        (void)checkSolution(solution, static_cast<int>(solIndex));
        solVector solutionVector = deriveSolVectorFromMatrix(
            solution, matrixSize, Rs, Cs);
        const size_t selectedColumns = static_cast<size_t>(std::count(
            solutionVector.begin(), solutionVector.end(), true));
        if (selectedColumns != static_cast<size_t>(Cs) ||
            !matchesPaperSolutionMatrix(solution, solutionVector, matrixSize) ||
            !uniqueSolutionVectors.insert(solutionVector).second)
        {
            throw std::invalid_argument(
                "RECAM candidates are not the paper Fig. 8 solution space");
        }
        solutionVectors.push_back(std::move(solutionVector));
    }

    for (size_t solIndex = 0; solIndex < allSolutions.size(); ++solIndex)
    {
        const solMatrix &solution = allSolutions[solIndex];
        if (checkSolution(solution, static_cast<int>(solIndex)))
        {
            validSolList.push_back(static_cast<int>(solIndex));
            remapTableList.push_back(buildRemapTable(solutionVectors[solIndex]));
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
