#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip> // for std::setw

// #include "inc/RECAM_addressCAM.hpp"
#include "./SolGenerator.hpp"
#include "./RECAM_CAM.hpp"
#include "./RECAM_bufferCAM.hpp"

using namespace std;

class RECAM_PE
{
public:
    int Rs ; // number of spare rows
    int Cs ; // number of spare columns
    int buff_num ;
    int matrixSize ;
    bool isRepairable = false;
    bool RepairSuccess = false;

    std::unique_ptr<RECAM_addressCAM> addressCAM;
    std::unique_ptr<RECAM_hybridCAM> hybridCAM;
    std::unique_ptr<RECAM_bufferCAM> bufferCAM ;

    using FaultAnalyzeMatrix = vector<vector<bool>>;
    FaultAnalyzeMatrix faultAnalyzeMatrixHardware;
    std::vector<int> validSolList; // store the index of valid solutions

    RECAM_PE(int r_spare, int c_spare, int buff_num)
        : Rs(r_spare), Cs(c_spare), buff_num(buff_num), matrixSize(r_spare + c_spare),
          addressCAM(std::make_unique<RECAM_addressCAM>(r_spare, c_spare, buff_num)),
          hybridCAM(std::make_unique<RECAM_hybridCAM>(r_spare, c_spare, buff_num)),
          bufferCAM(std::make_unique<RECAM_bufferCAM>(buff_num))
    {
        faultAnalyzeMatrixHardware.resize(matrixSize, vector<bool>(matrixSize, false));
    }

    void loadFaultsToCAMs(FaultList &faultList)
    {
        cout << " =========== Loading faults to CAMs  ===========" << endl;
        if (!faultList.bufferCAM_overflow) {
            cout << "Warning: Unrepairable fault list";
            cout << "  -Too many pivot faults exceeding both address CAM and buffer CAM capacity." << endl;
        }
        isRepairable = !faultList.bufferCAM_overflow;
        addressCAM->addAddressCAMEntryFromList(&faultList);
        hybridCAM->addHybridCAMEntryFromList(&faultList, addressCAM.get());
        bufferCAM->addBufferCAMEntryFromList(&faultList);
        cout << " ======= End of Loading faults to CAMs  ========" << endl;
    }

    void printPE() {
        cout << "PE Configuration - Rs: " << Rs << ", Cs: " << Cs << ", Buffer Num: " << buff_num << endl;
        addressCAM->printAddressCAMEntries();
        hybridCAM->printHybridCAMEntries();
        bufferCAM->printBufferCAMEntries();
    }

    void genFaultAnalyzeMatrix() {
        // cout << "Generating fault analyze matrix for hardware..." << endl;
        int addressCAMFullSize = matrixSize;
        int addressCAMEntriesCnt = addressCAM->addressCAMEntries.size();
        bool isAddressCAMEntriesFull = addressCAMEntriesCnt >= addressCAMFullSize;

        bool isfaultAnalyzeMatrixExtendable = !isAddressCAMEntriesFull;
        int faultAnalyzeMatrixRowCnt = addressCAMEntriesCnt;
        int faultAnalyzeMatrixColCnt = addressCAMEntriesCnt;
        int faultAnalyzeMatrixExtraRows = 0;
        int faultAnalyzeMatrixExtraCols = 0;

        for ( int i_row = 0; i_row < matrixSize; ++i_row){
            for ( int i_col = 0; i_col < matrixSize; ++i_col){
                bool rowMust = false;
                bool colMust = false;
                if (i_row <= addressCAMEntriesCnt - 1){
                    rowMust = addressCAM->addressCAMEntries[i_row].rowMust;
                }
                if (i_col <= addressCAMEntriesCnt - 1)
                {
                    colMust = addressCAM->addressCAMEntries[i_col].colMust;
                }

                // bool hasRowPtr = hasRowPtrs[i_row];
                // bool hasColPtr = hasColPtrs[i_col];
                bool diagaonal = (i_row == i_col) && (i_row <= addressCAMEntriesCnt - 1);
                // cout << "Processing cell (" << i_row << ", " << i_col << "): Row Must: " << (rowMust ? "Yes" : "No") << " Col Must: " << (colMust ? "Yes" : "No") << " Diagonal: " << (diagaonal ? "Yes" : "No") << endl;
                faultAnalyzeMatrixHardware[i_row][i_col] = diagaonal || rowMust || colMust; // || hasRowPtr || hasColPtr;
                // printFaultAnalyzeMatrix();
            }
        }

        // printFaultAnalyzeMatrix();

        vector <int> rowAddrs, colAddrs;
        for ( auto &entry : addressCAM->addressCAMEntries){
            if (entry.enable && entry.faultPtr != nullptr){
                rowAddrs.push_back(entry.faultPtr->r);
                colAddrs.push_back(entry.faultPtr->c);
            }
        }
        // cout << "rowAddrs: ";
        // for ( auto &r : rowAddrs){
        //     cout << r << " ";
        // }
        // cout << endl;
        // cout << "colAddrs: ";
        // for ( auto &c : colAddrs){
        //     cout << c << " ";
        // }
        // cout << endl;


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
        for (auto &hyEntry : hybridCAM->hybridCAMEntries){
            bool rowMatch = false, colMatch = false;
            int rowMatchIndex = -1 , colMatchIndex = -1;
            for (int i_row = 0; i_row < rowAddrs.size(); ++i_row)
            {
                rowMatch = (hyEntry.faultPtr->r == rowAddrs[i_row]);
                if (rowMatch){
                    rowMatchIndex = i_row;
                    break;
                }
            }
            for (int i_col = 0; i_col < colAddrs.size(); ++i_col)
            {
                colMatch = (hyEntry.faultPtr->c == colAddrs[i_col]);
                if (colMatch){
                    colMatchIndex = i_col;
                    break;
                }
            }

            // cout <<"  -"  << "Processing Hybrid CAM Entry: Row: " << hyEntry.faultPtr->r << " Col: " << hyEntry.faultPtr->c
            //      << " Related Pivot Fault Pointer: " << hyEntry.pointer
            //      << " descriptorRowIsDiff: " << (hyEntry.descriptorRowIsDiff ? "True" : "False")
            //      << endl;
            // cout << "      - Row Match: " << (rowMatch ? "Yes" : "No") << " (Index: " << rowMatchIndex << ")" << endl
            //      << "      - Col Match: " << (colMatch ? "Yes" : "No") << " (Index: " << colMatchIndex << ")"
            //      << endl;



            if(!isfaultAnalyzeMatrixExtendable ){
                // The original matrix need not to be extend
                if (rowMatch && colMatch)
                {
                 faultAnalyzeMatrixHardware[rowMatchIndex][colMatchIndex] = true;
                }
                else if ( rowMatch){
                    for ( int c = 0; c < matrixSize; ++c){
                        faultAnalyzeMatrixHardware[rowMatchIndex][c] = true;
                    }
                }
                else if ( colMatch){
                    for ( int r = 0; r < matrixSize; ++r){
                        faultAnalyzeMatrixHardware[r][colMatchIndex] = true;
                    }
                }
                else {
                    // This case should not happen
                }

            }
            else{
                if (rowMatch && colMatch)
                {
                 faultAnalyzeMatrixHardware[rowMatchIndex][colMatchIndex] = true;
                }
                else if ( rowMatch){
                    if (faultAnalyzeMatrixColCnt + faultAnalyzeMatrixExtraCols < addressCAMFullSize)
                    {
                        faultAnalyzeMatrixHardware[rowMatchIndex][faultAnalyzeMatrixColCnt + faultAnalyzeMatrixExtraCols] = true;
                        colAddrs.push_back(hyEntry.faultPtr->c); // add the new column address to the list
                        faultAnalyzeMatrixExtraCols++;
                        if  (   (faultAnalyzeMatrixColCnt + faultAnalyzeMatrixExtraCols >= addressCAMFullSize)
                            &&  (faultAnalyzeMatrixRowCnt + faultAnalyzeMatrixExtraRows >= addressCAMFullSize)) {
                            isfaultAnalyzeMatrixExtendable = false; // no more columns can be extended
                        }
                    }
                    else {
                        for ( int c = 0; c < matrixSize; ++c){
                            faultAnalyzeMatrixHardware[rowMatchIndex][c] = true;
                        }
                    }
                }
                else if ( colMatch){
                    if (faultAnalyzeMatrixRowCnt + faultAnalyzeMatrixExtraRows < addressCAMFullSize)
                    {
                        faultAnalyzeMatrixHardware[faultAnalyzeMatrixRowCnt + faultAnalyzeMatrixExtraRows][colMatchIndex] = true;
                        rowAddrs.push_back(hyEntry.faultPtr->r); // add the new row address to the list
                        faultAnalyzeMatrixExtraRows++;
                        if  (   (faultAnalyzeMatrixColCnt + faultAnalyzeMatrixExtraCols >= addressCAMFullSize)
                            &&  (faultAnalyzeMatrixRowCnt + faultAnalyzeMatrixExtraRows >= addressCAMFullSize)) {
                            isfaultAnalyzeMatrixExtendable = false; // no more rows can be extended
                        }
                    }
                    else {
                        for ( int r = 0; r < matrixSize; ++r){
                            faultAnalyzeMatrixHardware[r][colMatchIndex] = true;
                        }
                    }
                }
                else {
                    // This case should not happen
                }
            }
            // printFaultAnalyzeMatrix();
        }
    }

    void writeFaultAnalyzeMatrixToFile(const string &filename) {
        ofstream outFile(filename);
        if (!outFile.is_open()) {
            cerr << "Error: Could not open file " << filename << " for writing." << endl;
            return;
        }

        outFile << " ====== addressCAMEntry: ======" << endl;
        for (const auto& entry : addressCAM->addressCAMEntries) {
            if (entry.enable && entry.faultPtr != nullptr) {
                outFile << "Pivot Fault - LU: " << entry.faultPtr->LogicUnitID
                     << " Layer: " << entry.faultPtr->LayerID
                     << " Bank: " << entry.faultPtr->BankID
                     << " Row: " << entry.faultPtr->r
                     << " Col: " << entry.faultPtr->c
                     << " Row Must: " << (entry.rowMust ? "Yes" : "No")
                     << " Col Must: " << (entry.colMust ? "Yes" : "No")
                     << endl;
            }
        }

        outFile << " ====== hybridCAMEntry: ======" << endl;
        for (const auto& entry : hybridCAM->hybridCAMEntries) {
            if (entry.enable && entry.faultPtr != nullptr) {
                outFile << "Non-Pivot Fault - LU: " << entry.faultPtr->LogicUnitID
                        << " Layer: " << entry.faultPtr->LayerID
                        << " Bank: " << entry.faultPtr->BankID
                        << " Row: " << entry.faultPtr->r
                        << " Col: " << entry.faultPtr->c
                        << " Related Pivot Fault Pointer: " << entry.pointer
                        << " descriptorRowIsDiff: " << (entry.descriptorRowIsDiff ? "True" : "False")
                        << endl;
            }
        }

        outFile << "Fault Analyze Matrix for Hardware (1: Faulty Cell, 0: Non-Faulty Cell)" << endl;
        for (const auto &row : faultAnalyzeMatrixHardware) {
            for (const auto &cell : row) {
                outFile << (cell ? "1 " : "0 ");
            }
            outFile << endl;
        }

        outFile.close();
        cout << "Fault analyze matrix has been written to " << filename << endl;
    }

    void printFaultAnalyzeMatrix() ;
    bool checkSolution(const solMatrix &solution, int solIndex);
    void genValidSolList(const vector<solMatrix> &allSolutions);
    void printValidSolList() ;
};