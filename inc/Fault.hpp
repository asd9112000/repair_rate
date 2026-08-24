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
#include "./PairHash.hpp"

using namespace std;

enum class FaultType
{
    Cell
};

struct Fault
{
    int r, c;
    int HBMID, ChannelID, BankID, SubarrayGroupID, SubarrayID;
    FaultType faultType = FaultType::Cell;

    bool isPivot = false;
    bool isBuffer = false;
    bool isNonPivot = false;
    bool isOverflowPivot = false;
};


/*
    Each FaultList element corresponds to a Fault map for a single subarray.
*/

class FaultList
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

    FaultList(int r_spare, int c_spare, int buff_num) : Rs(r_spare), Cs(c_spare), Buff_num(buff_num) {
        matrixSize = Rs + Cs;
    };
    FaultList(const FaultList &other);
    FaultList &operator=(const FaultList &other);
    FaultList(FaultList &&other) noexcept = default;
    FaultList &operator=(FaultList &&other) noexcept = default;

    void addFault(const Fault &f){
        PEFaults.push_back(f);
    }
    void classifyFaults();
    void printFaultListPE(){
        cout << "Fault List for a single PE" << endl;
        cout << "Total Faults: " << PEFaults.size() << " pivot faults: " << pivotFaults.size() << " buffer faults: " << bufferFaults.size() << " non-pivot faults: " << nonPivotFaults.size() << endl;
        for (const auto &f : PEFaults)
        {
            cout << "Fault - HBM: " << f.HBMID
                 << " Channel: " << f.ChannelID
                 << " Bank: " << f.BankID
                 << " Subarray Group: " << f.SubarrayGroupID
                 << " Subarray: " << f.SubarrayID
                 << " Row: " << f.r
                 << " Col: " << f.c << endl;
        }
    }
    void printFaultList();

};



class FaultLoader{
public:
    int Rs, Cs;
    int matrixSize;
    int Buff_num;

    vector<FaultList> faultLists; // for multiple subarrays

    FaultLoader( int r_spare, int c_spare, int buff_num) : Rs(r_spare), Cs(c_spare), Buff_num(buff_num)
    {
        matrixSize = Rs + Cs;
    };

    bool loadFaults(string filename);
    void printFaultLoader();
    void writeFaultMap(string filename);

};

class FaultLoaderForSpares {
public:
    int Rs, Cs;
    // int RsRuduced;
    // int CsRuduced;
    int matrixSize;
    int Buff_num;
    bool usePaperCamReuseCapacity;

    unordered_map<pair<int, int>,vector<FaultList>, PairHash> faultListsForSpares;

    FaultLoaderForSpares(
        int r_spare,
        int c_spare,
        int buff_num,
        bool use_paper_cam_reuse_capacity = false)
        : Rs(r_spare), Cs(c_spare), Buff_num(buff_num),
          usePaperCamReuseCapacity(use_paper_cam_reuse_capacity) {
        matrixSize = Rs + Cs;
        // RsRuduced = Rs -1;
        // CsRuduced = Cs -1;
    };

    bool generateFaultListsForSpares(int r_spare, int c_spare, string filename = "./fault_generator/faults.faults")
    {
        ifstream fault_file(filename);
        if (!fault_file.is_open())
        {
            printf("!file.is_open() %s\n", filename.c_str());
            return false;
        }
        // cout << "Loading faults from file: " << filename << endl;
        // cout << "faultListsForSpares.find({r_spare, c_spare}) != faultListsForSpares.end()" << (faultListsForSpares.find({r_spare, c_spare}) != faultListsForSpares.end()) << endl;
        if (faultListsForSpares.find({r_spare, c_spare}) == faultListsForSpares.end())
        {
            cout << "Fault lists for spare configuration (Rs: " << r_spare << ", Cs: " << c_spare << ") already generated." << endl;
            const int bufferCapacity = usePaperCamReuseCapacity
                ? r_spare + c_spare
                : Buff_num;
            FaultLoader faultLoader(r_spare, c_spare, bufferCapacity);
            faultLoader.loadFaults(filename);
            faultListsForSpares[{r_spare, c_spare}] = faultLoader.faultLists;
        }

        return true;

    }

    void printFaultListsForSpares() {
        // for ( auto& entry : faultListsForSpares) {
        //     auto& key = entry.first;
        //     auto& faultLists = entry.second;
        //     cout << "Fault Lists for Spare Configuration (Rs: " << key.first << ", Cs: " << key.second << "):" << endl;
        //     for (size_t i = 0; i < faultLists.size(); ++i) {
        //         cout << "  PE " << i << ":" << endl;
        //         faultLists[i].printFaultList();
        //     }
        // }
    }
};

#endif
