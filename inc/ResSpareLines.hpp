#ifndef RES_SPARE_LINES_HPP
#define RES_SPARE_LINES_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip>    // for std::setw
#include <filesystem> // for std::filesystem::create_directories

#include "FourWayPE.hpp"
using namespace std;

class ResSpareLines
{
public:
    /*
        +---+---+
        | A | B |
        +---+---+
        | C | D |
        +---+---+

    Block A, B, C, D are arranged in a 2x2 grid, each with dimensions Rs x Cs.

    - **Physical View**:
        - Block A, D:
            - Cs spare columns
            - Rs spare rows (sharedLine_num : Rs - sharedLine_num)
        - Block B, C:
            - Rs spare rows
            - Cs spare columns (sharedLine_num : Cs - sharedLine_num)

    - **Functional View**:
        - Block A, D: max (Rs, Cs + sharedLine_num)
            - Spare columns: Cs ~ (Cs + sharedLine_num)
            - Spare rows: (Rs - sharedLine_num) ~ Rs
        - Block B, C: max (Rs + sharedLine_num, Cs)
            - Spare rows: Rs ~ (Rs + sharedLine_num)
            - Spare columns: (Cs - sharedLine_num) ~ Cs

    */

    int initResRs;
    int initResCs;
    int initSharedLine;

    int resSpareRows[4];
    int resSpareCols[4];
    int resSharedLines[4];
    // int resShareRows[2];
    // int resShareCols[2];
    // resSharedLines[4], PE0 -> 1 -> 2 -> 3
    //  PE0  PE1         3
    //                0     2
    //  PE2  PE3         1

    ResSpareLines(int r_spare, int c_spare, int shared_line) : initResRs(r_spare), initResCs(c_spare), initSharedLine(shared_line)
    {
        resetResSpareLines();
    };

    void resetResSpareLines()
    {
        for (int i = 0; i < 4; ++i)
        {
            if (i == 0 || i == 3)
            { // PE0, PE3
                resSpareRows[i] = initResRs - initSharedLine;
                resSpareCols[i] = initResCs;
            }
            else
            { // PE1, PE2
                resSpareRows[i] = initResRs;
                resSpareCols[i] = initResCs - initSharedLine;
            }

            resSharedLines[i] = initSharedLine;
        }
    };

    bool handleSparLines(int usedRow, int &resRows, int &resSharedLines)
    {
        if (resRows + resSharedLines >= usedRow)
        {
            if (resRows >= usedRow)
            {
                resRows -= usedRow;
            }
            else
            {
                int borrowFromSharedLine = usedRow - resRows;
                resRows = 0;
                resSharedLines -= borrowFromSharedLine;
            }
            return true; // Row 資源足夠
        }
        return false; // Row 資源不足
    }

    bool occupySpareLines(int PE_index, int usedRow, int usedCol)
    {
        // cout << "Attempting to occupy spare lines for PE" << PE_index << " with usedRow: " << usedRow << " and usedCol: " << usedCol << endl;
        bool rowEnough, colEnough;
        bool spareEnough = true;
        // ==========================
        //  PE0  PE1         3
        //                0     2
        //  PE2  PE3         1
        // ==========================

        switch (PE_index)
        {
            case 0:
            {
                rowEnough = handleSparLines(usedRow, resSpareRows[0], resSharedLines[0]);
                colEnough = handleSparLines(usedCol, resSpareCols[0], resSharedLines[3]);
                break;
            }
            case 1:
            {
                rowEnough = handleSparLines(usedRow, resSpareRows[1], resSharedLines[2]);
                colEnough = handleSparLines(usedCol, resSpareCols[1], resSharedLines[3]);
                break;
            }
            case 2:
            {
                rowEnough = handleSparLines(usedRow, resSpareRows[2], resSharedLines[0]);
                colEnough = handleSparLines(usedCol, resSpareCols[2], resSharedLines[1]);
                break;
            }
            case 3:
            {
                rowEnough = handleSparLines(usedRow, resSpareRows[3], resSharedLines[2]);
                colEnough = handleSparLines(usedCol, resSpareCols[3], resSharedLines[1]);
                break;
            }

            default:
            {
                cout << "Warning: Invalid PE index " << PE_index << endl;
                return false;
            }
        }
        spareEnough = rowEnough && colEnough;
        return spareEnough;
    };
};


#endif // RES_SPARE_LINES_HPP