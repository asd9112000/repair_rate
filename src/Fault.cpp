#include "../inc/Fault.hpp"

FaultList::FaultList(const FaultList &other)
    : Rs(other.Rs),
      Cs(other.Cs),
      matrixSize(other.matrixSize),
      Buff_num(other.Buff_num),
      PEFaults(other.PEFaults),
      bufferCAM_used(other.bufferCAM_used),
      bufferCAM_overflow(other.bufferCAM_overflow)
{
    *this = other;
}

FaultList &FaultList::operator=(const FaultList &other)
{
    if (this == &other)
    {
        return *this;
    }

    Rs = other.Rs;
    Cs = other.Cs;
    matrixSize = other.matrixSize;
    Buff_num = other.Buff_num;
    PEFaults = other.PEFaults;
    bufferCAM_used = other.bufferCAM_used;
    bufferCAM_overflow = other.bufferCAM_overflow;

    // FaultList owns PEFaults. Rebuild every pointer so a copied FaultList
    // points into its own PEFaults rather than into the source object.
    const auto rebind = [&other, this](
                            const std::vector<Fault *> &source,
                            std::vector<Fault *> &destination)
    {
        destination.clear();
        destination.reserve(source.size());
        if (other.PEFaults.empty())
        {
            return;
        }

        for (const Fault *faultPtr : source)
        {
            const auto sourceIt = std::find_if(
                other.PEFaults.begin(),
                other.PEFaults.end(),
                [faultPtr](const Fault &fault) { return &fault == faultPtr; });
            if (sourceIt == other.PEFaults.end())
            {
                throw std::logic_error("FaultList contains a pointer outside PEFaults");
            }
            const size_t index =
                static_cast<size_t>(std::distance(other.PEFaults.begin(), sourceIt));
            destination.push_back(&PEFaults[index]);
        }
    };

    rebind(other.pivotFaults, pivotFaults);
    rebind(other.bufferFaults, bufferFaults);
    rebind(other.nonPivotFaults, nonPivotFaults);
    rebind(other.overflowPivotFaults, overflowPivotFaults);
    return *this;
}

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
                // cout << " -Subarray:" << fault.SubarrayID << " Channel:" << fault.ChannelID << " Bank:" << fault.BankID << " Row " << fault.r << " Col " << fault.c << ". " << endl;
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
    cout << "Fault List for a single Subarray" << endl;
    cout << "Rs: " << Rs << " Cs: " << Cs << " Buffer Num: " << Buff_num << endl;
    cout << "bufferCAM_overflow: " << (bufferCAM_overflow ? "Yes" : "No") << endl;
    if (bufferCAM_overflow)
    {
        cout << "Warning: This Subarray is unrepairable due to excessive pivot faults." << endl;
    }
    cout << "Total Faults: " << PEFaults.size() << endl;
    cout << "  -pivot faults: " << pivotFaults.size() << " buffer faults: " << bufferFaults.size() << " non-pivot faults: " << nonPivotFaults.size() << " overflow pivot faults: " << overflowPivotFaults.size() << endl;

    cout << "Pivot Faults:" << endl;
    for (const auto &f : pivotFaults)
    {
        cout << "  -Pivot Fault - HBM: " << f->HBMID << " Channel: " << f->ChannelID << " Bank: " << f->BankID << " Subarray Group: " << f->SubarrayGroupID << " Subarray: " << f->SubarrayID << " Row: " << f->r << " Col: " << f->c << endl;
    }

    cout << endl
         << "Buffer Faults:" << endl;
    for (const auto &f : bufferFaults)
    {
        cout << "  -Buffer Fault - HBM: " << f->HBMID << " Channel: " << f->ChannelID << " Bank: " << f->BankID << " Subarray Group: " << f->SubarrayGroupID << " Subarray: " << f->SubarrayID << " Row: " << f->r << " Col: " << f->c << endl;
    }

    cout << endl
         << "Non-Pivot Faults:" << endl;
    for (const auto &f : nonPivotFaults)
    {
        cout << "  -Non-Pivot Fault - HBM: " << f->HBMID << " Channel: " << f->ChannelID << " Bank: " << f->BankID << " Subarray Group: " << f->SubarrayGroupID << " Subarray: " << f->SubarrayID << " Row: " << f->r << " Col: " << f->c << endl;
    }

    cout << endl
         << "Overflow Pivot Faults (exceeding both address CAM and buffer CAM capacity):" << endl;
    for (const auto &f : overflowPivotFaults)
    {
        cout << "  -Overflow Pivot Fault - HBM: " << f->HBMID << " Channel: " << f->ChannelID << " Bank: " << f->BankID << " Subarray Group: " << f->SubarrayGroupID << " Subarray: " << f->SubarrayID << " Row: " << f->r << " Col: " << f->c << endl;
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

    // These are default values, but they will be overwritten by the values read from the file.
    int r = -1, c = -1;
    int HBMID = -1, ChannelID = -1, BankID = -1;
    int SubarrayGroupID = -1, SubarrayID = -1;

    int numFaultArray ;
    int numFaults ;
    file >> numFaultArray;

    for ( int ii_numFaultArray = 0; ii_numFaultArray < numFaultArray; ++ii_numFaultArray) {
        file >> numFaults;
        // cout << "No." << ii_numFaultArray << " FaultArray ( " << ii_numFaultArray + 1 << "/" << numFaultArray << " ) has " << numFaults << " faults." << endl;
        FaultList faultList(Rs, Cs, Buff_num);
        for ( int ii_numFaults = 0; ii_numFaults < numFaults; ++ii_numFaults) {
            file >> HBMID >> ChannelID >> BankID >> SubarrayGroupID >> SubarrayID >> r >> c;
            // cout << "Parsed Fault - HBM: " << HBMID << " Channel: " << ChannelID << " Bank: " << BankID << " Subarray Group: " << SubarrayGroupID << " Subarray: " << SubarrayID << " Row: " << r << " Col: " << c << endl;
            Fault newFault;
            newFault.r = r;
            newFault.c = c;
            newFault.HBMID = HBMID;
            newFault.ChannelID = ChannelID;
            newFault.BankID = BankID;
            newFault.SubarrayGroupID = SubarrayGroupID;
            newFault.SubarrayID = SubarrayID;
            newFault.isPivot = false;
            newFault.isBuffer = false;
            newFault.isNonPivot = false;
            faultList.addFault(newFault);
        }

        faultLists.push_back(faultList);
        faultLists.back().classifyFaults();
    }

    // printFaultLoader();
    return true;
}

void FaultLoader::printFaultLoader()
{
    cout << endl
         << "======== FaultLoader::printFaultLoader ========" << endl;
    cout << "Total Fault Lists for subarrays: " << faultLists.size() << endl;
    for (int i = 0; i < faultLists.size(); ++i)
    {
        cout << "subarrays " << i << ":" << endl;
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
        outFile << "Subarray " << i_faultLists << " Fault List:" << endl;
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
        outFile << "Subarray " << i_faultLists << " Fault List:" << endl;
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
