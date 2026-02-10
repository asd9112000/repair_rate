#ifndef FAULT_HPP
#define FAULT_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip> // for std::setw

#include "./clean_comments.hpp"

using namespace std;


struct Fault
{
    int r, c;
    int LogicUnitID, LayerID, BankID;
    bool isPivot = false;
    bool isBuffer = false;
    bool isNonPivot = false;
    bool isOverflowPivot = false;
};



class FaultList // for a single PE
{
public:

    int Rs, Cs;
    int matrixSize;
    int Buff_num;
    vector<Fault> PEFaults;
    vector<Fault*> pivotFaults;    // pivot faults for address CAM, Rs + Cs pivot faults can be stored in address CAM
    vector<Fault*> bufferFaults;   // pivot faults that exceed the address CAM capacity can be stored in buffer CAM
    vector<Fault*> nonPivotFaults; // non-pivot faults can be stored in hybrid CAM
    vector<Fault*> overflowPivotFaults; // pivot faults that exceed both address CAM and buffer CAM capacity
    bool bufferCAM_used = false;
    bool bufferCAM_overflow = false;
    // bool isRepairable = true;

    FaultList(int r_spare, int c_spare, int buff_num) : Rs(r_spare), Cs(c_spare), Buff_num(buff_num) {
        matrixSize = Rs + Cs;
    };

    void addFault(const Fault &f){
        PEFaults.push_back(f);
    }
    void classifyFaults();
    void printFaultListPE(){
        cout << "Fault List for a single PE" << endl;
        cout << "Total Faults: " << PEFaults.size() << " pivot faults: " << pivotFaults.size() << " buffer faults: " << bufferFaults.size() << " non-pivot faults: " << nonPivotFaults.size() << endl;
        for (const auto &f : PEFaults)
        {
            cout << "Fault - LU: " << f.LogicUnitID << " Layer: " << f.LayerID << " Bank: " << f.BankID << " Row: " << f.r << " Col: " << f.c << endl;
        }
    }
    void printFaultList();

};



class FaultLoader{
public:
    int Rs, Cs;
    int matrixSize;
    int Buff_num;

    vector<FaultList> faultLists; // for multiple PEs

    FaultLoader( int r_spare, int c_spare, int buff_num) : Rs(r_spare), Cs(c_spare), Buff_num(buff_num)
    {
        matrixSize = Rs + Cs;
    };

    bool loadFaults(string filename);
    void printFaultLoader();
    void writeFaultMap(string filename);

};


#endif