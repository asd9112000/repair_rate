#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip> // for std::setw


#include "inc/clean_comments.hpp"
using namespace std;


// struct Config
// {
//     int num_logic_units = 16;
//     int stack_height = 1;
//     int rows = 1024;
//     int cols = 1024;
//     double prob_cluster = 0.2;
//     double prob_same_line = 0.3;
//     int fixed_faults = 0;
//     std::string output_file = "faults.txt";
//     int seed = 42;
// };

struct Fault {
    int r, c;
    int LogicUnitID, LayerID, BankID;
    bool isPivot = false;
};

struct HybridCAMEntry {
    bool descriptor_row = true; // true for row, false for column
    int  pointer = 0;           // index of the pivot fault it relates to
};


class RECAM_Analyzer {
private:
    int LU_num, Layer_num, Bank_num, Buff_num;
    int Rs, Cs; // 備用行與列的數量
    int matrixSize, hybridCAMSize;
    vector<Fault*> allFaults; // 儲存所有 Faults


    // index list: [LogicUnitID] [LayerID] [BankID]
    std::vector<std::vector<std::vector<std::vector<Fault >>>> FaultMap;
    std::vector<std::vector<std::vector<std::vector<Fault*>>>> addressCAM;
    std::vector<std::vector<std::vector<std::vector< int   >>>> aCAM_colCnt;
    std::vector<std::vector<std::vector<std::vector< int   >>>> aCAM_rowCnt;
    std::vector<std::vector<std::vector<std::vector<bool   >>>> aCAM_colMust;
    std::vector<std::vector<std::vector<std::vector<bool   >>>> aCAM_rowMust;
    std::vector<std::vector<std::vector<std::vector<int>>>> pivotCol;
    std::vector<std::vector<std::vector<std::vector<int>>>> pivotRow;

    std::vector<std::vector<std::vector<std::vector<Fault *>>>> hybridCAM;
    std::vector<std::vector<std::vector<std::vector<HybridCAMEntry>>>> hybridCAMEntry;
    std::vector<std::vector<std::vector<std::vector<Fault *>>>> bufferCAM;
    std::vector<std::vector<std::vector<bool>>> bufferCAM_used;
    std::vector<std::vector<std::vector<bool>>> bufferCAM_overflow;

    using faultMatrix = vector<vector<bool>>; // [Row][Col]
    using AllFaultMatrix = std::vector < std::vector < std::vector < faultMatrix >>>;
    AllFaultMatrix allFaultMatrix;

    // ======  generate all combinations of solutions =============
    // 1.generate all combinations of selecting Rs rows from Rs + Cs
    //  - Must generate the first solvector in dictionary order !!!!!
    // 2. for each combination, generate the solution matrix
    using solVector = std::vector<bool>;
    using solMatrix = std::vector<std::vector<bool>>;                     // [Row][Col]
    std::vector<solVector> allSolVectorsType;  // Col (true),  Row (false)
    std::vector<solMatrix> allSolMatrixsType;                             // allSolutions[solIndex][Row][Col]
    std::vector<std::vector<std::vector<std::vector<int>>>> validSolList; // [LogicUnitID][LayerID][BankID][solIndex]
    // std::vector<std::vector<std::vector<std::ve78687687686ctor<bool>>>> repairList; // [LogicUnitID][LayerID][BankID][solIndex]


public:
    RECAM_Analyzer(int r_spare, int c_spare, int lu_num, int layer_num, int bank_num, int buff_num)
        : Rs(r_spare), Cs(c_spare), LU_num(lu_num), Layer_num(layer_num), Bank_num(bank_num), Buff_num(buff_num)
    {
        matrixSize = Rs + Cs;
        hybridCAMSize = Rs * (Cs - 1) + Cs * (Rs - 1);
        initializeVector(FaultMap, LU_num, Layer_num, Bank_num);
        initializeVector(addressCAM, LU_num, Layer_num, Bank_num);
        initializeVector(hybridCAM, LU_num, Layer_num, Bank_num);
        initializeVector(hybridCAMEntry, LU_num, Layer_num, Bank_num);
        initializeVector(bufferCAM, LU_num, Layer_num, Bank_num);
        initializeVector(bufferCAM_used, LU_num, Layer_num, Bank_num,  false);
        initializeVector(bufferCAM_overflow, LU_num, Layer_num, Bank_num,  false);
        initializeVector(allFaultMatrix, LU_num, Layer_num, Bank_num, matrixSize, vector<bool>(matrixSize, false));

        initializeVector(aCAM_colCnt, LU_num, Layer_num, Bank_num, matrixSize, 0);
        initializeVector(aCAM_rowCnt, LU_num, Layer_num, Bank_num, matrixSize, 0);
        initializeVector(aCAM_colMust, LU_num, Layer_num, Bank_num, matrixSize, false);
        initializeVector(aCAM_rowMust, LU_num, Layer_num, Bank_num, matrixSize, false);
        initializeVector(pivotCol, LU_num, Layer_num, Bank_num, matrixSize, -1);
        initializeVector(pivotRow, LU_num, Layer_num, Bank_num, matrixSize, -1);
        initializeVector(validSolList, LU_num, Layer_num, Bank_num);

        initialize_allSolVectorsType();
        initialize_allSolMatrixsType();
        // printAllSolVectors(true);
        // printAllSolMatrixs(true);
        writeAllSolVectorsToFile(true);
        writeAllSolMatrixsToFile(true);
    }

    void initialize_allSolVectorsType(){
        solVector solvector = solVector(Rs + Cs, false);
        fill(solvector.begin(), solvector.begin() + Cs, true); // 0~Cs-1  are Col (true), the rest are Row (false)
        do
        {
            allSolVectorsType.push_back(solvector);
        } while (prev_permutation(solvector.begin(), solvector.end())); // gerate all combinations
    }

    void initialize_allSolMatrixsType(){
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

    // init vector for three-dimensional vector
    template <typename T>
    void initializeVector(std::vector<std::vector<std::vector<T>>> &vec, int dim1, int dim2, int dim3)
    {
        vec.resize(dim1);
        for (int i = 0; i < dim1; ++i)
        {
            vec[i].resize(dim2);
            for (int j = 0; j < dim2; ++j)
            {
                vec[i][j].resize(dim3);
            }
        }
    }

    template <typename T>
    void initializeVector(std::vector<std::vector<std::vector<T>>> &vec, int dim1, int dim2, int dim3, const T &defaultValue)
    {
        vec.resize(dim1);
        for (int i = 0; i < dim1; ++i)
        {
            vec[i].resize(dim2);
            for (int j = 0; j < dim2; ++j)
            {
                vec[i][j].resize(dim3, defaultValue);
            }
        }
    }

        // init vector for four-dimensional vector
    template <typename T>
    void initializeVector(std::vector<std::vector<std::vector<std::vector<T>>>> &vec, int dim1, int dim2, int dim3, int dim4, const T &defaultValue)
    {
        vec.resize(dim1);
        for (int i = 0; i < dim1; ++i)
        {
            vec[i].resize(dim2);
            for (int j = 0; j < dim2; ++j)
            {
                vec[i][j].resize(dim3);
                for (int k = 0; k < dim3; ++k)
                {
                    vec[i][j][k].resize(dim4, defaultValue);
                }
            }
        }
    }
    // =======================================================
    //                            work
    // =======================================================
    bool loadFaults(string filename) {
        ifstream fault_file(filename);
        if (!fault_file.is_open()) {
            printf("!file.is_open() %s\n", filename.c_str());
            return false;
        }

        // clean_comments(file, "faults_clear.txt", true);
        string fileContent = clean_comments(fault_file, "fault_generator/faults_clear.txt", true);
        std::istringstream file(fileContent);

        int r = -1, c = -1;
        int LogicUnitID = -1, LayerID = -1, BankID = -1;

        while (file >> LogicUnitID >> LayerID >> BankID >> r >> c)
        {
            if (LogicUnitID >= LU_num || LayerID >= Layer_num || BankID >= Bank_num)
            {
                cout << "Fault[" << LogicUnitID << "][" << LayerID << "][" << BankID << "] is out of range." << endl;
                continue;
            }
            // cout << "Parsed Fault - LU: " << LogicUnitID << " Layer: " << LayerID << " Bank: " << BankID << " Row: " << r << " Col: " << c << endl;
            Fault newFault ;
            newFault.r = r ;
            newFault.c = c ;
            newFault.LogicUnitID = LogicUnitID ;
            newFault.LayerID = LayerID ;
            newFault.BankID = BankID ;
            FaultMap[LogicUnitID][LayerID][BankID].push_back(newFault);
        }

        for (int lu = 0; lu < LU_num; ++lu)
        {
            for (int ly = 0; ly < Layer_num; ++ly)
            {
                for (int bk = 0; bk < Bank_num; ++bk)
                {
                    for (auto &f : FaultMap[lu][ly][bk])
                    {
                        allFaults.push_back(&f);
                    }
                }
            }
        }



        // ========= Debug: Print all loaded faults ==========
        bool printallFaults = true;
        bool printFaultMap = true;
        if (printallFaults){
            cout << "Total Faults Loaded: " << allFaults.size() << endl;
            for (const auto &f : allFaults)
            {
                cout << "Fault - LU: " << f->LogicUnitID << " Layer: " << f->LayerID << " Bank: " << f->BankID << " Row: " << f->r << " Col: " << f->c << endl;
            }
        }

        if (printFaultMap)
        {
            for (int lu = 0; lu < LU_num; ++lu)
            {
                for (int ly = 0; ly < Layer_num; ++ly)
                {
                    for (int bk = 0; bk < Bank_num; ++bk)
                    {
                        cout << "Faults in LU " << lu << " Layer " << ly << " Bank " << bk << ": " << FaultMap[lu][ly][bk].size() << endl;
                        for (const auto &f : FaultMap[lu][ly][bk])
                        {
                            cout << "  - Row: " << f.r << " Col: " << f.c << endl;
                        }
                    }
                }
            }
        }
        return true;
    }

    void pivotClassify(){
        cout << endl <<"=========== start pivot classify =============" << endl;
        for (int lu = 0; lu < LU_num; ++lu)
        {
            for (int ly = 0; ly < Layer_num; ++ly)
            {
                for (int bk = 0; bk < Bank_num; ++bk)
                {
                    cout << "Faults in LU " << lu << " Layer " << ly << " Bank " << bk << ": " << FaultMap[lu][ly][bk].size() << endl;
                    for (auto &fault : FaultMap[lu][ly][bk])
                    {
                        bool isPivot = true;
                        cout << "  - Row: " << fault.r << " Col: " << fault.c << endl;
                        if ( addressCAM[lu][ly][bk].empty() ){
                            isPivot = true;
                            if (addressCAM[lu][ly][bk].size() < matrixSize)
                            {
                                addressCAM[lu][ly][bk].push_back(&fault);
                            }
                            else if (bufferCAM[lu][ly][bk].size() < Buff_num)
                            {
                                bufferCAM_used[lu][ly][bk] = true;
                                bufferCAM[lu][ly][bk].push_back(&fault);
                            }
                            else
                            {
                                cout << "Not repairable Fault at LU " << lu << " Layer " << ly << " Bank " << bk << ". Exceeded CAM and Buffer capacity." << endl;
                                cout << "Exceeded CAM and Buffer capacity at LU " << lu << " Layer " << ly << " Bank " << bk << ". Early termination." << endl;
                                bufferCAM_overflow[lu][ly][bk] = true;
                                // return;
                            }
                        }
                        else {
                            int index = 0;
                            for (const auto &f : addressCAM[lu][ly][bk]) {
                                cout << "    - Comparing with Pivot Row: " << f->r << " Col: " << f->c << endl;
                                if (fault.r == f->r || fault.c == f->c){
                                    isPivot = false;
                                    cout << "    - isPivot: false" << endl;
                                    bool rowMatch = (fault.r == f->r);
                                    if (rowMatch)
                                    {
                                        aCAM_rowCnt[lu][ly][bk][index]++;
                                        cout << "    - aCAM_rowCnt[" << lu << "][" << ly << "][" << bk << "][" << index << "] = " << aCAM_rowCnt[lu][ly][bk][index] << endl;
                                        if (aCAM_rowCnt[lu][ly][bk][index] + 1> Cs ){ // cnt = n : n+1 of faults on this row. n+1 > spare cols is must row.
                                            aCAM_rowMust[lu][ly][bk][index] = true;
                                            cout << "    - aCAM_rowMust[" << lu << "][" << ly << "][" << bk << "][" << index << "] = true" << endl;
                                            cout << "    - clean hybridCAM for row " << f->r << " because it's must row now." << endl;
                                            cout << "    - (It will reduce the non-pivot fault tist because it's catched from hybridCAM)" << endl;
                                        }
                                    }
                                    else {
                                        aCAM_colCnt[lu][ly][bk][index]++;
                                        cout << "    - aCAM_colCnt[" << lu << "][" << ly << "][" << bk << "][" << index << "] = " << aCAM_colCnt[lu][ly][bk][index] << endl;
                                        if (aCAM_colCnt[lu][ly][bk][index] + 1> Rs ){ // cnt = n :   n+1 of faults on this col. n+1 > spare rows is must col.
                                            aCAM_colMust[lu][ly][bk][index] = true;
                                            cout << "    - aCAM_colMust[" << lu << "][" << ly << "][" << bk << "][" << index << "] = true" << endl;
                                            cout << "    - will clean hybridCAM for col " << f->c << " because it's must col now. " << endl;
                                            cout << "    - (It will reduce the non-pivot fault tist because it's catched from hybridCAM)" << endl;
                                        }
                                    }


                                    if ( aCAM_rowMust[lu][ly][bk][index] ){
                                        // remove the element from hybridCAM if it's row match f->r
                                        for (int k = hybridCAM[lu][ly][bk].size() - 1; k >= 0; k--)
                                        {
                                            if (hybridCAM[lu][ly][bk][k]->r == f->r)
                                            {
                                                hybridCAM[lu][ly][bk].erase(hybridCAM[lu][ly][bk].begin() + k);
                                                hybridCAMEntry[lu][ly][bk].erase(hybridCAMEntry[lu][ly][bk].begin() + k);
                                            }
                                        }
                                    }

                                    if ( aCAM_colMust[lu][ly][bk][index] ){
                                        // remove the element from hybridCAM if it's col match f->c
                                        for (int k = hybridCAM[lu][ly][bk].size() - 1; k >= 0; k--)
                                        {
                                            if (hybridCAM[lu][ly][bk][k]->c == f->c)
                                            {
                                                hybridCAM[lu][ly][bk].erase(hybridCAM[lu][ly][bk].begin() + k);
                                                hybridCAMEntry[lu][ly][bk].erase(hybridCAMEntry[lu][ly][bk].begin() + k);
                                            }
                                        }
                                    }

                                    if ( aCAM_rowMust[lu][ly][bk][index] || aCAM_colMust[lu][ly][bk][index] ){
                                        // skip adding to hybridCAM, it's have been tagged as must
                                        cout << "    - skip adding to hybridCAM, it's have been tagged as must" << endl;
                                        // print fault which is sikpped
                                        cout << "    - Skipped Fault Row: " << fault.r << " Col: " << fault.c << endl;
                                    }
                                    else if (hybridCAM[lu][ly][bk].size() < hybridCAMSize)
                                    {
                                        hybridCAM[lu][ly][bk].push_back(&fault);
                                        hybridCAMEntry[lu][ly][bk].push_back( HybridCAMEntry{ rowMatch, index } );
                                    }
                                    else
                                    {
                                        cout << "Not repairable Fault at LU " << lu << " Layer " << ly << " Bank " << bk << ". Exceeded Hybrid CAM capacity." << endl;
                                        cout << "Exceeded Hybrid CAM capacity at LU " << lu << " Layer " << ly << " Bank " << bk << ". Early termination." << endl;
                                        // return;
                                    }

                                    break;
                                }
                                index++;
                            }

                            if ( isPivot){
                                if (addressCAM[lu][ly][bk].size() < matrixSize)
                                {
                                    addressCAM[lu][ly][bk].push_back(&fault);
                                }
                                else if (bufferCAM[lu][ly][bk].size() < Buff_num)
                                {
                                    bufferCAM_used[lu][ly][bk] = true;
                                    bufferCAM[lu][ly][bk].push_back(&fault);
                                }
                                else
                                {
                                    cout << "Not repairable Fault at LU " << lu << " Layer " << ly << " Bank " << bk << ". Exceeded CAM and Buffer capacity." << endl;
                                    cout << "Exceeded CAM and Buffer capacity at LU " << lu << " Layer " << ly << " Bank " << bk << ". Early termination." << endl;
                                    bufferCAM_overflow[lu][ly][bk] = true;
                                    // return;
                                }
                            }
                        }
                        cout << "    - isPivot: " << (isPivot ? "true" : "false") << endl;
                    }

                    // print addressCAM
                    cout << "Fault Classification Results for LU " << lu << " Layer " << ly << " Bank " << bk << ":" << endl;
                    cout << "  - addressCAM size: " << addressCAM[lu][ly][bk].size() << endl;
                    for (const auto &f : addressCAM[lu][ly][bk]) {
                        cout << "    - Pivot Row: " << f->r << " Col: " << f->c << endl;
                    }
                    // print bufferCAM
                    cout << "  - bufferCAM size: " << bufferCAM[lu][ly][bk].size() << endl;
                    for (const auto &f : bufferCAM[lu][ly][bk]) {
                        cout << "    - (Buffer) Pivot Row: " << f->r << " Col: " << f->c << endl;
                    }
                    // print hybridCAM
                    cout << "  - hybridCAM size: " << hybridCAM[lu][ly][bk].size() << endl;
                    for (int h = 0; h < hybridCAM[lu][ly][bk].size(); ++h) {
                        const auto &f = hybridCAM[lu][ly][bk][h];
                        const auto &entry = hybridCAMEntry[lu][ly][bk][h];
                        cout << "    - Non-Pivot Row: " << f->r << " Col: " << f->c
                             << " (relates to Pivot Index: " << entry.pointer
                             << ", Descriptor: " << (entry.descriptor_row ? "Row" : "Col") << ")" << endl;
                    }
                }
            }
        }

        /* ========  Debug print pivot information ==========*/
        bool printPivot = true;
        bool printNonPivot = true;
        if (printPivot){
            cout << endl;
            cout << "=========== start print Pivot =============" << endl;
            for (int lu = 0; lu < LU_num; ++lu)
            {
                for (int ly = 0; ly < Layer_num; ++ly)
                {
                    for (int bk = 0; bk < Bank_num; ++bk)
                    {
                        cout << "Pivot Faults in LU " << lu << " Layer " << ly << " Bank " << bk << ": " << addressCAM[lu][ly][bk].size() << endl;

                        if (!addressCAM[lu][ly][bk].empty()){
                            for ( auto &f : addressCAM[lu][ly][bk])
                            {
                                cout << "  - Row: " << f->r << " Col: " << f->c << endl;
                            }
                        }

                        if (!bufferCAM[lu][ly][bk].empty()){
                            for ( auto &f : bufferCAM[lu][ly][bk])
                            {
                                cout << "  - (Buffer) Row: " << f->r << " Col: " << f->c << endl;
                            }
                        }
                    }
                }
            }
        }
        cout << endl;

        if (printNonPivot){
            cout << endl;
            cout << "=========== start print Non-Pivot =============" << endl;
            for (int lu = 0; lu < LU_num; ++lu)
            {
                for (int ly = 0; ly < Layer_num; ++ly)
                {
                    for (int bk = 0; bk < Bank_num; ++bk)
                    {
                        cout << "Non-Pivot Faults in LU " << lu << " Layer " << ly << " Bank " << bk << ": " << hybridCAM[lu][ly][bk].size() << endl;
                        if (!hybridCAM[lu][ly][bk].empty()) {
                            for ( auto &f : hybridCAM[lu][ly][bk])
                            {
                                cout << "  - Row: " << f->r << " Col: " << f->c << endl;
                            }
                        }
                    }
                }
            }
        }
        cout << endl;

        return ;
    }


    void solve() {
        cout << endl;
        cout << "=========== start to solve =============" << endl;
        cout << "=========== transfer Faults to allFaultMatrix =============" << endl;
        for (int i = 0; i < LU_num; ++i)
        {
            for (int j = 0; j < Layer_num; ++j)
            {
                for (int k = 0; k < Bank_num; ++k)
                {
                    cout << " -trasnfer fault in LU " << i << " Layer " << j << " Bank " << k << " to fault matrix "<< endl;
                    cout << " -addressCAM size: " << addressCAM[i][j][k].size() << " hybridCAM size: " << hybridCAM[i][j][k].size() << " buffer " << bufferCAM[i][j][k].size() << endl;

                    // ====== Now, start to transfer Faults to allFaultMatrix ============
                    // -1. pivot faults in addressCAM, this will be marked as diagonal
                    bool isAddressCAMFULL = (addressCAM[i][j][k].size() >= matrixSize);
                    int extraHybridRows = 0;
                    int extraHybridCols = 0;
                    for (int l = 0; l < addressCAM[i][j][k].size(); ++l)
                    {
                        allFaultMatrix[i][j][k][l][l] = true; // Pivot點
                        pivotRow[i][j][k][l] = addressCAM[i][j][k][l]->r;
                        pivotCol[i][j][k][l] = addressCAM[i][j][k][l]->c;
                    }

                    // -2.1 row must faults in addressCAM
                    for (int r = 0; r < aCAM_rowMust[i][j][k].size(); ++r)
                    {
                        if (aCAM_rowMust[i][j][k][r]) {
                            for (int c = 0; c < matrixSize; ++c) {
                                allFaultMatrix[i][j][k][r][c] = true;
                            }
                        }
                    }

                    // -2.2 col must faults in addressCAM
                    for (int c = 0; c < aCAM_colMust[i][j][k].size(); ++c)
                    {
                        if (aCAM_colMust[i][j][k][c]) {
                            for (int r = 0; r < matrixSize; ++r) {
                                allFaultMatrix[i][j][k][r][c] = true;
                            }
                        }
                    }

                    // ==== deBug print row/col must in this bank =======
                    bool printRowColMust = true;
                    if (printRowColMust){
                        cout << "  - aCAM_rowMust: ";
                        for (int r = 0; r < aCAM_rowMust[i][j][k].size(); ++r)
                        {
                            cout << (aCAM_rowMust[i][j][k][r] ? "1 " : "0 ");
                        }
                        cout << endl;
                        cout << "  - aCAM_colMust: ";
                        for (int c = 0; c < aCAM_colMust[i][j][k].size(); ++c)
                        {
                            cout << (aCAM_colMust[i][j][k][c] ? "1 " : "0 ");
                        }
                        cout << endl;
                    }

                    // -3. non-pivot faults in hybridCAM
                    // -3.1 addressCAM isn't full(isAddressCAMFULL != true), set one location true
                    // if full, need to check row and col mapping
                    // -3.2 row and col are both in addressCAM ( distribute to in different pivot faults), set 1 location true
                    // -3.3 one of row or col is in addressCAM ( cannot be mapped, skip ), set whole line true
                    for (const auto &hf : hybridCAM[i][j][k]) {
                        int rowIdx = -1, colIdx = -1;
                        for (int m = 0; m < addressCAM[i][j][k].size(); ++m) {
                            if (addressCAM[i][j][k][m]->r == hf->r) rowIdx = m;
                            if (addressCAM[i][j][k][m]->c == hf->c) colIdx = m;
                            if (rowIdx != -1 && colIdx != -1) break;
                        }
                        // case 3.1
                        if (!isAddressCAMFULL) { // match case r/c, r ,c
                            cout << "  - addressCAM is not full." << endl;
                            cout << "  - hybrid Fault Row: " << hf->r << " Col: " << hf->c << endl;

                            if (rowIdx != -1){
                                if (colIdx != -1)
                                { // case r/c, need not to expand
                                    allFaultMatrix[i][j][k][rowIdx][colIdx] = true;
                                }
                                else {
                                    // case r, need to expand col
                                    extraHybridCols++;
                                    cout << "extraHybridCols: " << extraHybridCols << endl;
                                    cout << "addressCAM size: " << addressCAM[i][j][k].size() << endl;
                                    cout << "set pivotCol[" << i << "][" << j << "][" << k << "][" << addressCAM[i][j][k].size() - 1 + extraHybridCols << "] = " << hf->c << endl;
                                    pivotCol[i][j][k][addressCAM[i][j][k].size() - 1 + extraHybridCols] = hf->c;
                                    // cout << "fuck"<<endl;
                                    allFaultMatrix[i][j][k][rowIdx][addressCAM[i][j][k].size() - 1 + extraHybridCols] = true;
                                }

                            }
                            else if (colIdx != -1){
                                extraHybridRows++;
                                cout << "extraHybridRows: " << extraHybridRows << endl;
                                cout << "addressCAM size: " << addressCAM[i][j][k].size() << endl;
                                cout << "set pivotRow[" << i << "][" << j << "][" << k << "][" << addressCAM[i][j][k].size() - 1 + extraHybridRows << "] = " << hf->r << endl;
                                pivotRow[i][j][k][addressCAM[i][j][k].size() - 1 + extraHybridRows] = hf->r;
                                // cout << "fuck" << endl;
                                cout << "allFaultMatrix size: " << allFaultMatrix[i][j][k].size() << endl ;//<< " X " << allFaultMatrix[i][j][k][0].size() << endl;
                                cout << "set allFaultMatrix[" << i << "][" << j << "][" << k << "][" << addressCAM[i][j][k].size() - 1 + extraHybridRows << "][" << colIdx << "] = true" << endl;

                                allFaultMatrix[i][j][k][addressCAM[i][j][k].size() - 1 + extraHybridRows][colIdx] = true;
                            }

                        }
                        // case 3.2 rowIdx != -1 or colIdx == -1
                        if (rowIdx != -1 && colIdx != -1) {
                            allFaultMatrix[i][j][k][rowIdx][colIdx] = true;
                        }
                        // case 3.3
                        else if (rowIdx != -1 ) { // multiple faults on this row, set whole row true
                            for (int c = 0; c < matrixSize; ++c) {
                                allFaultMatrix[i][j][k][rowIdx][c] = true;
                            }
                        }
                        else if (colIdx != -1 ) { // multiple faults on this col, set whole col true
                            for (int r = 0; r < matrixSize; ++r) {
                                allFaultMatrix[i][j][k][r][colIdx] = true;
                            }
                        }
                    }
                }
            }
        }

        //  ========= Debug print allFaultMatrix ==========
        bool printAllFaultMatrix = true;
        if (printAllFaultMatrix)
        {
            cout << endl;
            cout << "=========== Print All Fault Matrix =============" << endl;
            for (int lu = 0; lu < LU_num; ++lu)
            {
                for (int ly = 0; ly < Layer_num; ++ly)
                {
                    for (int bk = 0; bk < Bank_num; ++bk)
                    {
                        cout << "Fault Matrix in LU " << lu << " Layer " << ly << " Bank " << bk << ":" << endl;

                        // Print column headers (pivotCol)
                        cout << "     "; // Offset for row headers
                        for (int c = 0; c < matrixSize; ++c)
                        {
                            if (c < pivotCol[lu][ly][bk].size())
                            {
                                cout << setw(4) << pivotCol[lu][ly][bk][c] << " ";
                            }
                            else
                            {
                                cout << setw(4) << "-" << " "; // Placeholder for missing pivotCol
                            }
                        }
                        cout << endl;

                        // Print matrix rows with row headers (pivotRow)
                        for (int r = 0; r < matrixSize; ++r)
                        {
                            if (r < pivotRow[lu][ly][bk].size())
                            {
                                cout << setw(4) << pivotRow[lu][ly][bk][r] << " | "; // Row header
                            }
                            else
                            {
                                cout << setw(4) << "-" << " | "; // Placeholder for missing pivotRow
                            }

                            for (int c = 0; c < matrixSize; ++c)
                            {
                                cout << setw(4) << (allFaultMatrix[lu][ly][bk][r][c] ? " 1 " : " 0 ");
                            }
                            cout << endl;
                        }
                        cout << endl;
                    }
                }
            }
        }

        // We try to find a combination of Rs spare rows and Cs spare columns
        // There will be less than or equal to matrixSize(Rs+Cs) pivot faults in addressCAM
        // 1. Each pivot fault will choose to be repaired by either a spare row or a spare column
        //   - solNum = C(Rs+Cs, Rs) combinations
        //   - row_selection: 0 means spare row, 1 means spare column
        //   - solvector    : 0 means spare row, 1 means spare column
        vector<int> indices(matrixSize);
        for(int i=0; i<matrixSize; ++i) indices[i] = i;

        int solNum = 1 ; // C(Rs+Cs, Rs)
        for ( int i = 0; i < Rs; ++i ) {
            solNum *= (matrixSize - i);
            solNum /= (i + 1);
        }





        cout << "=========== Fault Free region analyze =============" << endl;
        cout << "  - compare solutions and fault matrices" << endl;
        std::vector<std::vector<std::vector<bool>>> repairList;
        std::vector<std::vector<std::vector<std::vector<int>>>> solList;
        repairList.resize(LU_num);
        solList.resize(LU_num);
        for (int i = 0; i < LU_num; ++i)
        {
            repairList[i].resize(Layer_num);
            solList[i].resize(Layer_num);
            for (int j = 0; j < Layer_num; ++j)
            {
                repairList[i][j].resize(Bank_num, false);
                solList[i][j].resize(Bank_num);
            }
        }

        for (int i = 0; i < LU_num; ++i)
        {
            for (int j = 0; j < Layer_num; ++j)
            {
                for (int k = 0; k < Bank_num; ++k)
                {
                    cout << "allFaultMatrix in LU " << i << " Layer " << j << " Bank " << k << ": " << hybridCAM[i][j][k].size() << endl;
                    bool repairable = false;
                    for (int s = 0; s < allSolMatrixsType.size(); ++s)
                    {
                        bool match = true;
                        bool faultInFreeRegion = false;
                        for (int r = 0; r < matrixSize; ++r)
                        {
                            for (int c = 0; c < matrixSize; ++c)
                            {
                                if (allSolMatrixsType[s][r][c]){
                                    // pass
                                }
                                else {
                                    if (allFaultMatrix[i][j][k][r][c]){
                                        faultInFreeRegion = true;
                                        break;
                                    }
                                }
                            }
                            if (faultInFreeRegion){
                                break;
                            }
                        }
                        if (!faultInFreeRegion)
                        {
                            repairable = true;
                            repairList[i][j][k] = true;
                            solList[i][j][k].push_back(s) ;
                            cout << "  - Found repairable solution at index " << s << endl;
                            // cout << "  - Solution " << s << " is not repairable." << endl;
                        }
                    }
                    if (!repairable){
                        cout << "  - No repairable solution found." << endl;
                    }
                }
            }
        }

    }

    void solveHardware(){
        cout <<endl << "=========== start to solve Hardware =============" << endl;

        for (int solvector_dix = 0; solvector_dix < allSolVectorsType.size(); ++solvector_dix)
        {
            solVector &solvector = allSolVectorsType[solvector_dix];
            cout << "Solution Vector " << solvector_dix << ": ";
            for (bool sel : solvector)
            {
                cout << (sel ? "C " : "R "); // true (C)，false  (R)
            }
            cout << endl;
        }

            std::vector<std::vector<std::vector<std::vector<std::vector<bool>>>>> RepairMatrix; // for block analysis
            std::vector<std::vector<std::vector<std::vector<bool>>>> hasRowPtr;
            std::vector<std::vector<std::vector<std::vector<bool>>>> hasColPtr;

            // innitialize RepairMatrix, hasRowPtr, hasColPtr
            RepairMatrix.resize(LU_num);
            hasRowPtr.resize(LU_num);
            hasColPtr.resize(LU_num);
            for (int i = 0; i < LU_num; ++i)
            {
                RepairMatrix[i].resize(Layer_num);
                hasRowPtr[i].resize(Layer_num);
                hasColPtr[i].resize(Layer_num);
                for (int j = 0; j < Layer_num; ++j)
                {
                    RepairMatrix[i][j].resize(Bank_num);
                    hasRowPtr[i][j].resize(Bank_num);
                    hasColPtr[i][j].resize(Bank_num);
                    for (int k = 0; k < Bank_num; ++k)
                    {
                        RepairMatrix[i][j][k].resize(matrixSize);
                        hasRowPtr[i][j][k].resize(matrixSize, false);
                        hasColPtr[i][j][k].resize(matrixSize, false);
                        for (int r = 0; r < matrixSize; ++r)
                        {
                            RepairMatrix[i][j][k][r].resize(matrixSize, false);
                        }
                    }
                }
            }



            // compute hasRowPtr and hasColPtr from hybridCAMEntry
            for (int lu = 0; lu < LU_num; ++lu)
            {
                for (int ly = 0; ly < Layer_num; ++ly)
                {
                    for (int bk = 0; bk < Bank_num; ++bk)
                    {
                        cout << "Faults in LU " << lu << " Layer " << ly << " Bank " << bk << ": " << FaultMap[lu][ly][bk].size() << endl;
                        cout << " ====== assign  hasRowPtr/hasColPtr ========" << endl;
                        for ( int ii = 0; ii  < hybridCAM[lu][ly][bk].size(); ++ii )
                        {
                            cout << "hybridCAMEntry[" << lu << "][" << ly << "][" << bk << "][" << ii << "]: " << (hybridCAMEntry[lu][ly][bk][ii].descriptor_row ? "Row" : "Col")
                            << " Descriptor_row: " << hybridCAMEntry[lu][ly][bk][ii].descriptor_row
                            << " Pointer: " << hybridCAMEntry[lu][ly][bk][ii].pointer << endl;
                            if ( hybridCAMEntry[lu][ly][bk][ii].descriptor_row ){
                                if (hybridCAMEntry[lu][ly][bk][ii].pointer >= hasRowPtr[lu][ly][bk].size())
                                {
                                    cerr << "Error: hybridCAMEntry[" << lu << "][" << ly << "][" << bk << "][" << ii << "].pointer ("
                                         << hybridCAMEntry[lu][ly][bk][ii].pointer << ") exceeds hasRowPtr[" << lu << "][" << ly << "][" << bk << "] size ("
                                         << hasRowPtr[lu][ly][bk].size() << ")." << endl;
                                    throw std::out_of_range("Size mismatch detected in hasRowPtr.");
                                }
                                else
                                {
                                    hasRowPtr[lu][ly][bk][hybridCAMEntry[lu][ly][bk][ii].pointer] = true;
                                }
                            }
                            else {

                                if (hybridCAMEntry[lu][ly][bk][ii].pointer >= hasColPtr[lu][ly][bk].size())
                                {
                                    cerr << "Error: hybridCAMEntry[" << lu << "][" << ly << "][" << bk << "][" << ii << "].pointer ("
                                         << hybridCAMEntry[lu][ly][bk][ii].pointer << ") exceeds hasColPtr[" << lu << "][" << ly << "][" << bk << "] size ("
                                         << hasColPtr[lu][ly][bk].size() << ")." << endl;
                                    throw std::out_of_range("Size mismatch detected in hasColPtr.");
                                }
                                else
                                {
                                    hasColPtr[lu][ly][bk][hybridCAMEntry[lu][ly][bk][ii].pointer] = true;
                                }
                            }
                        }

                        cout << endl << " ====== generate RepairMatrix ========" << endl;
                        for ( int matrix_row = 0 ; matrix_row < matrixSize; matrix_row++){
                            for ( int matrix_col = 0; matrix_col < matrixSize; ++matrix_col){
                                cout << "Generating RepairMatrix[" << lu << "][" << ly << "][" << bk << "][" << matrix_row << "][" << matrix_col << "]" << endl;
                                // print all aCAM_rowMust, aCAM_colMust, hasRowPtr, hasColPtr
                                cout << "  -aCAM_rowmust: " ;
                                for (int r = 0; r < aCAM_rowMust[lu][ly][bk].size(); ++r)
                                {
                                    cout << (aCAM_rowMust[lu][ly][bk][r] ? "1 " : "0 ");
                                }
                                cout << endl;
                                cout << "  -aCAM_colmust: " ;
                                for (int c = 0; c < aCAM_colMust[lu][ly][bk].size(); ++c)
                                {
                                    cout << (aCAM_colMust[lu][ly][bk][c] ? "1 " : "0 ");
                                }
                                cout << endl;
                                cout << "  - hasRowPtr: " ;
                                for (int r = 0; r < hasRowPtr[lu][ly][bk].size(); ++r)
                                {
                                    cout << (hasRowPtr[lu][ly][bk][r] ? "1 " : "0 ");
                                }
                                cout << endl;
                                cout << "  - hasColPtr: " ;
                                for (int c = 0; c < hasColPtr[lu][ly][bk].size(); ++c)
                                {
                                    cout << (hasColPtr[lu][ly][bk][c] ? "1 " : "0 ");
                                }
                                cout << endl;
                                cout << "  - aCAM_rowMust[" << lu << "][" << ly << "][" << bk << "][" << matrix_row << "] = "
                                     << (aCAM_rowMust[lu][ly][bk][matrix_row] ? "true" : "false") << endl;
                                cout << "  - aCAM_colMust[" << lu << "][" << ly << "][" << bk << "][" << matrix_col << "] = "
                                     << (aCAM_colMust[lu][ly][bk][matrix_col] ? "true" : "false") << endl;
                                cout << "  - hasRowPtr[" << lu << "][" << ly << "][" << bk << "][" << matrix_row << "] = "
                                     << (hasRowPtr[lu][ly][bk][matrix_row] ? "true" : "false") << endl;
                                cout << "  - hasColPtr[" << lu << "][" << ly << "][" << bk << "][" << matrix_col << "] = "
                                     << (hasColPtr[lu][ly][bk][matrix_col] ? "true" : "false") << endl;
                                RepairMatrix[lu][ly][bk][matrix_row][matrix_col] =  aCAM_rowMust[lu][ly][bk][matrix_row] ||
                                                                                    aCAM_colMust[lu][ly][bk][matrix_col] || hasRowPtr[lu][ly][bk][matrix_row] ||
                                                                                    hasColPtr[lu][ly][bk][matrix_col];
                            }
                        }

                        // Debug : print RepairMatrix
                        bool printReparMatrix = true;
                        if (printReparMatrix) {
                            cout << endl;
                            cout << "=========== Print Repair Matrix =============" << endl;
                            cout << "Repair Matrix in LU " << lu << " Layer " << ly << " Bank " << bk << ":" << endl;

                            // Print matrix rows with row headers (pivotRow)
                            for (int r = 0; r < matrixSize; ++r)
                            {
                                if (r < pivotRow[lu][ly][bk].size())
                                {
                                    // cout << setw(4) << pivotRow[lu][ly][bk][r] << " | "; // Row header
                                }
                                else
                                {
                                    cout << setw(4) << "-" << " | "; // Placeholder for missing pivotRow
                                }

                                for (int c = 0; c < matrixSize; ++c)
                                {
                                    cout << setw(4) << (RepairMatrix[lu][ly][bk][r][c] ? " 1 " : " 0 ");
                                }
                                cout << endl;
                            }
                            cout << endl;
                        }

                        cout << " ====== compare solMatrix and RepairMatrix ========" << endl;
                        for ( int solMatrix_idx = 0 ; solMatrix_idx < allSolMatrixsType.size(); ++solMatrix_idx ){
                            cout << " - Comparing with Solution Matrix index " << solMatrix_idx << endl;
                            bool findFaultInFA = false;
                            for ( int matrix_row = 0 ; matrix_row < matrixSize; matrix_row++){
                                for ( int matrix_col = 0; matrix_col < matrixSize; matrix_col++){
                                    // cout << "Comparing SolMatrix[" << solMatrix_idx << "][" << matrix_row << "][" << matrix_col << "] = "
                                    //      << (allSolMatrixsType[solMatrix_idx][matrix_row][matrix_col] ? "1" : "0")
                                    //      << " with RepairMatrix[" << lu << "][" << ly << "][" << bk << "][" << matrix_row << "][" << matrix_col << "] = "
                                    //      << (RepairMatrix[lu][ly][bk][matrix_row][matrix_col] ? "1" : "0") << endl;
                                    // cout << "  - findFaultInFA before: " << (findFaultInFA ? "true" : "false") << endl;

                                    if ( allSolMatrixsType[solMatrix_idx][matrix_row][matrix_col] == false ){
                                        findFaultInFA = RepairMatrix[lu][ly][bk][matrix_row][matrix_col] || findFaultInFA;
                                    }
                                    // cout << "  - findFaultInFA after: " << (findFaultInFA ? "true" : "false") << endl;
                                    if (findFaultInFA){
                                        break;
                                    }
                                }
                                if (findFaultInFA){
                                    break;
                                }
                            }
                            if (!findFaultInFA && bufferCAM_overflow[lu][ly][bk] == false){
                                cout << "  - For LU " << lu << " Layer " << ly << " Bank " << bk << ": " << endl;
                                cout << "  - Found repairable solution vector at index " << solMatrix_idx << endl;
                                validSolList[lu][ly][bk].push_back(solMatrix_idx);
                            }
                        }

                        cout << " Valid Solutions found so far in LU " << lu << " Layer " << ly << " Bank " << bk << ": " << validSolList[lu][ly][bk].size() << endl;
                        for (int idx : validSolList[lu][ly][bk])
                        {
                            cout << "  - Solution index: " << idx << endl;
                        }
                    }
            }

        }


        cout << "=========== print all validSolList =============" << endl;
        for (int lu = 0; lu < LU_num; ++lu)
        {
            for (int ly = 0; ly < Layer_num; ++ly)
            {
                for (int bk = 0; bk < Bank_num; ++bk)                {
                    cout << "Valid Solutions in LU " << lu << " Layer " << ly << " Bank " << bk << ": " << validSolList[lu][ly][bk].size() << endl;
                    for (int idx : validSolList[lu][ly][bk])
                    {
                        cout << "  - Solution index: " << idx << endl;
                    }
                }
            }
        }




        // ======= Debug print almost information =======
        bool debugPrint = true;
        if (debugPrint){
            for (int lu = 0; lu < LU_num; ++lu)
            {
                for (int ly = 0; ly < Layer_num; ++ly)
                {
                    for (int bk = 0; bk < Bank_num; ++bk)
                    {
                        cout << "================================================================" << endl;
                        cout << "================= all information ==============================" << endl;
                        cout << "================================================================" << endl;

                        cout << "Faults in LU " << lu << " Layer " << ly << " Bank " << bk << ": " << FaultMap[lu][ly][bk].size() << endl;
                        // print FaultMap
                        cout << "  - FaultMap: " << endl;
                        for ( const auto &f : FaultMap[lu][ly][bk] ){
                            cout << "  - Fault Row: " << f.r << " Col: " << f.c << endl;
                        }

                        // fault in addressCAM
                        cout << "  - addressCAM: " << endl;
                        for ( const auto &f : addressCAM[lu][ly][bk] ){
                            cout << "    - Pivot Fault Row: " << f->r << " Col: " << f->c << endl;
                        }

                        // fault in bufferCAM
                        cout << "  - bufferCAM: " << endl;
                        for ( const auto &f : bufferCAM[lu][ly][bk] ){
                            cout << "    - (Buffer) Pivot Fault Row: " << f->r << " Col: " << f->c << endl;
                        }

                        cout << "  - bufferCAM_used: " << (bufferCAM_used[lu][ly][bk] ? "true" : "false") << endl;
                        cout << "  - bufferCAM_overflow: " << (bufferCAM_overflow[lu][ly][bk] ? "true" : "false") << endl;


                        // fault in hybridCAM
                        cout << "  - hybridCAM: " << endl;
                        for ( int h = 0; h < hybridCAM[lu][ly][bk].size(); ++h ){
                            const auto &f = hybridCAM[lu][ly][bk][h];
                            const auto &entry = hybridCAMEntry[lu][ly][bk][h];
                            cout << "    - Non-Pivot Fault Row: " << f->r << " Col: " << f->c
                                 << " (relates to Pivot Index: " << entry.pointer
                                 << ", Descriptor: " << (entry.descriptor_row ? "Row" : "Col") << ")" << endl;
                        }

                        // cout << repairMAtrix
                        cout << "  - RepairMatrix: " << endl;
                        for ( int matrix_row = 0 ; matrix_row < matrixSize; matrix_row++){
                            for ( int matrix_col = 0; matrix_col < matrixSize; ++matrix_col){
                                cout << setw(4) << (RepairMatrix[lu][ly][bk][matrix_row][matrix_col] ? " 1 " : " 0 ");
                            }
                            cout << endl;
                        }

                        // print row/col must
                        cout << "  - aCAM_rowMust: ";
                        for (int r = 0; r < aCAM_rowMust[lu][ly][bk].size(); ++r)
                        {
                            cout << (aCAM_rowMust[lu][ly][bk][r] ? "1 " : "0 ");
                        }
                        cout << endl;
                        cout << "  - aCAM_colMust: ";
                        for (int c = 0; c < aCAM_colMust[lu][ly][bk].size(); ++c)
                        {
                            cout << (aCAM_colMust[lu][ly][bk][c] ? "1 " : "0 ");
                        }
                        cout << endl;

                        // print hasRowPtr
                        cout << "  - hasRowPtr: ";
                        for ( int r = 0; r < hasRowPtr[lu][ly][bk].size(); ++r){
                            cout << (hasRowPtr[lu][ly][bk][r] ? "1 " : "0 ");
                        }
                        cout << endl;
                        // print hasColPtr
                        cout << "  - hasColPtr: ";
                        for ( int c = 0; c < hasColPtr[lu][ly][bk].size(); ++c){
                            cout << (hasColPtr[lu][ly][bk][c] ? "1 " : "0 ");
                        }
                        cout << endl;

                        // print validSolList
                        cout << "  - validSolList: ";
                        for ( int v = 0; v < validSolList[lu][ly][bk].size(); ++v){
                            cout << validSolList[lu][ly][bk][v] << " ";
                        }
                        cout << endl;

                    }
                }
            }
        }

        // return;
    }

};

int main(int argc, char *argv[])
{
    int paramNum = argc;    // program name + Rs + Cs
    int Rs = atoi(argv[1]); // number of spare rows
    int Cs = atoi(argv[2]); // number of spare columns
    cout << "Input Rs: " << Rs << ", Cs: " << Cs << endl;
    RECAM_Analyzer analyzer(Rs, Cs, 1, 1, 1,2);
    // RECAM_Analyzer(int r_spare, int c_spare, int lu_num, int layer_num, int bank_num, int buff_num)

    // if (!analyzer.loadFaults("./fault_generator/faults.txt"))
        if (!analyzer.loadFaults("./faults.txt"))
        {
            cout << "load failed, could not open file or too many faults." << endl;
            return 0;
        }

    // analyzer.printAllSolVectors( true );
    analyzer.pivotClassify();

    // analyzer.solve();
    analyzer.solveHardware();
    return 0;
}