#ifndef RECAM_CAM_HPP
#define RECAM_CAM_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip> // for std::setw
#include <tuple>

using namespace std;

#include "./Fault.hpp"

class HybridCAMEntry
{
public:
    bool enable = false;
    int halfAddr = -1;          // the row/col address which is the same as the related pivot fault
    int pointer = 0;            // index of the pivot fault it relates to
    bool descriptorRowIsDiff = true; // true for row is shared, false for column
    Fault *faultPtr = nullptr;  // pointer to the non-pivot fault itself
};

class AddressCAMEntry
{
public:
    bool enable = false;
    Fault *faultPtr = nullptr;
    int faultInRowCnt = 1; //
    int faultInColCnt = 1;
    bool rowMust = false;
    bool colMust = false;
};


class RECAM_addressCAM // for a single PE
{
public:
    int Rs, Cs;
    int matrixSize;
    vector<AddressCAMEntry> addressCAMEntries;

    RECAM_addressCAM(int r_spare, int c_spare, int buff_num);

    void addAddressCAMEntry(Fault &f);
    void addAddressCAMEntryFromList(FaultList *fPtr);
    std::tuple<bool, bool, int> updateRowColMust(HybridCAMEntry &hybridCAMEntry);

    void printAddressCAMEntries();
};


class RECAM_hybridCAM // for a single PE
{
public:
    int Rs, Cs;
    int matrixSize;
    int hybridCAMSize;
    size_t peakEntries = 0;
    size_t writeOperations = 0;
    bool hybridCAM_overflow = false;
    vector<HybridCAMEntry> hybridCAMEntries;


    RECAM_hybridCAM(int r_spare, int c_spare, int buff_num);

    bool addHybridCAMEntry(Fault &f, RECAM_addressCAM *addressCAM);
    std::vector<Fault *> addHybridCAMEntryFromList(
        FaultList *fPtr,
        RECAM_addressCAM *addressCAM);
    void printHybridCAMEntries() ;

};

#endif
