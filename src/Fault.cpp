#include "../inc/Fault.hpp"

void FaultList::classifyFaults()
{
    // cout << endl <<"=========== start pivot classify =============" << endl;
    for ( int i = 0; i < PEFaults.size(); ++i )
    {
        Fault &fault = PEFaults[i];
        // cout << setw(3) << i + 1 << "/" << setw(3) << PEFaults.size() << " Fault - Row: " << setw(4) << fault.r << " Col: " << setw(4) << fault.c << endl;
        // Check if it can be classified as a pivot fault
        bool isPivot = true;
        for (const auto &pf : pivotFaults) {
            if ( fault.r == pf->r || fault.c == pf->c ) {
                isPivot = false;
                break;
            }
        }


        if ( pivotFaults.empty() ){
                pivotFaults.push_back(&fault);
                pivotFaults.back()->isPivot = true;
                // cout << "  -Pivot Fault" << endl;
        }
        else if (isPivot ) {
            if (pivotFaults.size() < matrixSize){

                pivotFaults.push_back(&fault);
                pivotFaults.back()->isPivot = true;
                // cout << "  -Pivot Fault" << endl;
            }
            else if (bufferFaults.size() < Buff_num) {
                bufferFaults.push_back(&fault);
                bufferFaults.back()->isBuffer = true;
                bufferCAM_used = true;
                // cout << "  -Pivot fault in buffer." << endl;
            }
            else {
                // cout << " -Warning: Unrepairable Fault. Num of pivot faults exceeded addressCAM and Buffer capacity." << endl;
                // cout << " -LU:" << fault.LogicUnitID << "Layer:" << fault.LayerID << "Bank:" << fault.BankID << " Row " << fault.r << " Col " << fault.c << ". " << endl;
                fault.isOverflowPivot = true;
                overflowPivotFaults.push_back(&fault);
                bufferCAM_overflow = true;
            }
        }
        else {
            nonPivotFaults.push_back(&fault);
            nonPivotFaults.back()->isNonPivot = true;
            // cout << "  -Non-Pivot Fault" << endl;
        }
    }
}

void FaultList::printFaultList()
{
    cout << "Fault List for a single PE" << endl;
    cout << "Rs: " << Rs << " Cs: " << Cs << " Buffer Num: " << Buff_num << endl;
    cout << "bufferCAM_overflow: " << (bufferCAM_overflow ? "Yes" : "No") << endl;
    if (bufferCAM_overflow)
    {
        cout << "Warning: This PE is unrepairable due to excessive pivot faults." << endl;
    }
    cout << "Total Faults: " << PEFaults.size() << endl;
    cout << "  -pivot faults: " << pivotFaults.size() << " buffer faults: " << bufferFaults.size() << " non-pivot faults: " << nonPivotFaults.size() << " overflow pivot faults: " << overflowPivotFaults.size() << endl;

    cout << "Pivot Faults:" << endl;
    for (const auto &f : pivotFaults)
    {
        cout << "  -Pivot Fault - LU: " << f->LogicUnitID << " Layer: " << f->LayerID << " Bank: " << f->BankID << " Row: " << f->r << " Col: " << f->c << endl;
    }

    cout << endl
         << "Buffer Faults:" << endl;
    for (const auto &f : bufferFaults)
    {
        cout << "  -Buffer Fault - LU: " << f->LogicUnitID << " Layer: " << f->LayerID << " Bank: " << f->BankID << " Row: " << f->r << " Col: " << f->c << endl;
    }

    cout << endl
         << "Non-Pivot Faults:" << endl;
    for (const auto &f : nonPivotFaults)
    {
        cout << "  -Non-Pivot Fault - LU: " << f->LogicUnitID << " Layer: " << f->LayerID << " Bank: " << f->BankID << " Row: " << f->r << " Col: " << f->c << endl;
    }

    cout << endl
         << "Overflow Pivot Faults (exceeding both address CAM and buffer CAM capacity):" << endl;
    for (const auto &f : overflowPivotFaults)
    {
        cout << "  -Overflow Pivot Fault - LU: " << f->LogicUnitID << " Layer: " << f->LayerID << " Bank: " << f->BankID << " Row: " << f->r << " Col: " << f->c << endl;
    }
}





bool FaultLoader :: loadFaults(string filename)
{
    ifstream fault_file(filename);
    if (!fault_file.is_open())
    {
        printf("!file.is_open() %s\n", filename.c_str());
        return false;
    }

    // clean_comments(file, "faults_clear.txt", true);
    string fileContent = clean_comments(fault_file, "fault_generator/faults_clear.faults", true);
    std::istringstream file(fileContent);

    int r = -1, c = -1;
    int LogicUnitID = -1, LayerID = -1, BankID = -1;

    int numFaultArray ;
    int numFaults ;
    file >> numFaultArray;
    // cout << endl
    //      << "======== FaultLoader :: loadFaults ========" << endl;
    // cout << "Number of FaultArray in the file: " << numFaultArray << endl;

    for ( int ii_numFaultArray = 0; ii_numFaultArray < numFaultArray; ++ii_numFaultArray) {
        file >> numFaults;
        // cout << "No." << ii_numFaultArray << " FaultArray ( " << ii_numFaultArray + 1 << "/" << numFaultArray << " ) has " << numFaults << " faults." << endl;
        FaultList faultList(Rs, Cs, Buff_num);
        for ( int ii_numFaults = 0; ii_numFaults < numFaults; ++ii_numFaults) {
            file >> LogicUnitID >> LayerID >> BankID >> r >> c;
            // cout << "Parsed Fault - LU: " << LogicUnitID << " Layer: " << LayerID << " Bank: " << BankID << " Row: " << r << " Col: " << c << endl;
            Fault newFault;
            newFault.r = r;
            newFault.c = c;
            newFault.LogicUnitID = LogicUnitID;
            newFault.LayerID = LayerID;
            newFault.BankID = BankID;
            newFault.isPivot = false;
            newFault.isBuffer = false;
            newFault.isNonPivot = false;
            faultList.addFault(newFault);
        }

        faultLists.push_back(faultList);
        faultLists.back().classifyFaults();
    }

    return true;
}

void FaultLoader::printFaultLoader()
{
    cout << endl
         << "======== FaultLoader::printFaultLoader ========" << endl;
    cout << "Total Fault Lists for PEs: " << faultLists.size() << endl;
    for (int i = 0; i < faultLists.size(); ++i)
    {
        cout << "PE " << i  << ":" << endl;
        faultLists[i].printFaultList();
        cout << endl;
    }
    cout << "===== End of Fault Loader Summary ====" << endl << endl;
}




void FaultLoader::writeFaultMap(string filename)
{
    ofstream outFile(filename);
    if (!outFile.is_open())
    {
        cerr << "Error: Unable to open file " << filename << " for writing." << endl;
        return;
    }

    for (int i_faultLists = 0; i_faultLists < faultLists.size(); ++i_faultLists)
    {
        outFile << endl <<"========== Fault Map with buffer CAM =============" << endl;
        outFile << "PE " << i_faultLists << " Fault List:" << endl;
        outFile << "Pivot fault X, non-pivot fault O, buffer fault B, free ." << endl;

        vector<int> rowAddress, colAddress;
        unordered_map<int, int> rowAddressToIndex, colAddressToIndex;
        for (int i_faults = 0; i_faults < faultLists[i_faultLists].PEFaults.size(); ++i_faults)
        {
            const auto &f = faultLists[i_faultLists].PEFaults[i_faults];
            auto findRow = find(rowAddress.begin(), rowAddress.end(), f.r);
            auto findCol = find(colAddress.begin(), colAddress.end(), f.c);

            if (findRow == rowAddress.end())
            { //  not find in rowAddress
                rowAddress.push_back(f.r);
                rowAddressToIndex[f.r] = rowAddress.size() - 1; // map row address to its index in rowAddress vector
            }

            if (findCol == colAddress.end())
            { //  not find in colAddress
                colAddress.push_back(f.c);
                colAddressToIndex[f.c] = colAddress.size() - 1; // map col address to its index in colAddress vector
            }
        }

        // assign Fault Map with buffer CAM
        // outFile << "Pivot fault X, non-pivot fault O, buffer fault B, free ." << endl;
        //           int           1                  2               3       0
        // vector<vector<bool>> faultMap(rowAddress.size(), vector<bool>(colAddress.size(), false));
        vector<vector<int>> faultMap(rowAddress.size(), vector<int>(colAddress.size(), 0));
        for (int i_faults = 0; i_faults < faultLists[i_faultLists].PEFaults.size(); ++i_faults)
        {
            const auto &f = faultLists[i_faultLists].PEFaults[i_faults];
            int rowIndex = rowAddressToIndex[f.r];
            int colIndex = colAddressToIndex[f.c];
            if (f.isPivot) {

                faultMap[rowIndex][colIndex] = 1;
            }
            else if (f.isNonPivot) {
                faultMap[rowIndex][colIndex] = 2;
            }
            else if (f.isBuffer) {
                faultMap[rowIndex][colIndex] = 3;
            }
        }

        // write Fault Map to file
        outFile << "Fault Map for all fault: " << endl;
        outFile << "    ";
        for (int c = 0; c < colAddress.size(); ++c)
        {
            outFile << setw(4) << colAddress[c];
        }
        outFile << endl;

        for (int r = 0; r < rowAddress.size(); ++r)
        {
            outFile << setw(4) << rowAddress[r];
            for (int c = 0; c < colAddress.size(); ++c)
            {
                char symbol = '.';
                switch (faultMap[r][c]) {
                    case 1: symbol = 'X'; break;
                    case 2: symbol = 'O'; break;
                    case 3: symbol = 'B'; break;
                    default: symbol = '.'; break;
                }
                outFile << setw(4) << symbol;
            }
            outFile << endl;
        }

        outFile << endl
                << endl
                << "========== Fault Map (only CAM) =============" << endl
                << "  - Delete faults that are classified as buffer faults or overflowFault." << endl;
        outFile << "PE " << i_faultLists << " Fault List:" << endl;
        vector<int> rowAddressWoBuff, colAddressWoBuff;
        unordered_map<int, int> rowAddressToIndexWoBuff, colAddressToIndexWoBuff;
        for (int i_faults = 0; i_faults < faultLists[i_faultLists].PEFaults.size(); ++i_faults)
        {
            const auto &f = faultLists[i_faultLists].PEFaults[i_faults];
            if (f.isBuffer || f.isOverflowPivot)
            {
                continue; // skip faults that are classified as buffer faults
            }

            auto findRow = find(rowAddressWoBuff.begin(), rowAddressWoBuff.end(), f.r);
            auto findCol = find(colAddressWoBuff.begin(), colAddressWoBuff.end(), f.c);

            if (findRow == rowAddressWoBuff.end())
            { //  not find in rowAddressWoBuff
                rowAddressWoBuff.push_back(f.r);
                rowAddressToIndexWoBuff[f.r] = rowAddressWoBuff.size() - 1; // map row address to its index in rowAddressWoBuff vector
            }

            if (findCol == colAddressWoBuff.end())
            { //  not find in colAddressWoBuff
                colAddressWoBuff.push_back(f.c);
                colAddressToIndexWoBuff[f.c] = colAddressWoBuff.size() - 1; // map col address to its index in colAddressWoBuff vector
            }
        }

        // assign Fault Map with buffer CAM
        // vector<vector<bool>> faultMapWoBuff(rowAddressWoBuff.size(), vector<bool>(colAddressWoBuff.size(), false));
        vector<vector<int>> faultMapWoBuff(rowAddressWoBuff.size(), vector<int>(colAddressWoBuff.size(), 0));
        for (int i_faults = 0; i_faults < faultLists[i_faultLists].PEFaults.size(); ++i_faults)
        {
            const auto &f = faultLists[i_faultLists].PEFaults[i_faults];
            if (f.isBuffer || f.isOverflowPivot)
            {
                continue; // skip faults that are classified as buffer faults
            }
            int rowIndex = rowAddressToIndexWoBuff[f.r];
            int colIndex = colAddressToIndexWoBuff[f.c];
            if (f.isPivot)
            {

                faultMap[rowIndex][colIndex] = 1;
            }
            else if (f.isNonPivot)
            {
                faultMap[rowIndex][colIndex] = 2;
            }
        }

        // write Fault Map to file
        outFile << "Fault Map (without buffer CAM): " << endl;
        outFile << "    ";
        for (int c = 0; c < colAddressWoBuff.size(); ++c)
        {
            outFile << setw(4) << colAddressWoBuff[c];
        }
        outFile << endl;

        for (int r = 0; r < rowAddressWoBuff.size(); ++r)
        {
            outFile << setw(4) << rowAddressWoBuff[r];
            for (int c = 0; c < colAddressWoBuff.size(); ++c)
            {
                char symbol = '.';
                switch (faultMapWoBuff[r][c]) {
                    case 1: symbol = 'X'; break;
                    case 2: symbol = 'O'; break;
                    case 3: symbol = 'B'; break;
                    default: symbol = '.'; break;
                }
                outFile << setw(4) << symbol;
            }
            outFile << endl;
        }
    }

    outFile.close();
}