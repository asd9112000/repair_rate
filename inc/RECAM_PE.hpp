#ifndef RECAM_PE_HPP
#define RECAM_PE_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip> // for std::setw
#include <limits>

// #include "inc/RECAM_addressCAM.hpp"
#include "./SolGenerator.hpp"
#include "./RECAM_CAM.hpp"
#include "./RECAM_bufferCAM.hpp"

using namespace std;


class RemapTable{
public:
    static constexpr int kDefaultRemapLatency = 99;

    struct AddressEntry
    {
        int channel         = 99;
        int pseudochannel   = 99;
        int bankgroup       = 99;
        int bank            = 99;
        int row             = 99;
        int col             = 99;

    };

    // Entry in the lookup table for remapping addresses.
    struct RemapEntry
    {
        AddressEntry addressEntry;
        bool isSpareRow     = true;
        int newRowColAddr   = 99;
        int Latency = kDefaultRemapLatency;
    };


    struct RemappedResult
    {
        AddressEntry addressEntry;
        int latency = 0;
    };

    std::vector<RemapEntry> RemapEntries;
    void addRemapEntry(const RemapEntry &entry){
        RemapEntries.push_back(entry);
    }

    bool isMatchedThisEntry(const AddressEntry &addrEntry, const RemapEntry &entry) const
    {
        if (addrEntry.channel == entry.addressEntry.channel &&
            addrEntry.pseudochannel == entry.addressEntry.pseudochannel &&
            addrEntry.bankgroup == entry.addressEntry.bankgroup &&
            addrEntry.bank == entry.addressEntry.bank &&
            (entry.isSpareRow
                ? addrEntry.row == entry.addressEntry.row
                : addrEntry.col == entry.addressEntry.col))
        {
            return true;
        }
        return false;
    }


    RemappedResult remapAddress(const AddressEntry &addrEntry) const
    {
        RemappedResult result{addrEntry, 0};
        for (const auto &e : RemapEntries)
        {
            if (isMatchedThisEntry(addrEntry, e))
            {
                if (e.isSpareRow) result.addressEntry.row = e.newRowColAddr;
                else              result.addressEntry.col = e.newRowColAddr;
                result.latency = std::max(result.latency, e.Latency);
            }
        }
        return result;
    }

    // Load one complete four-PE repair option from remapTable.txt.  Call this
    // once after selecting the pattern and option to be used at runtime.
    bool loadFromLog(const std::string &filename, int patternId, int optionId)
    {
        std::ifstream input(filename);
        if (!input.is_open())
        {
            return false;
        }

        RemapEntries.clear();
        std::string tag;
        bool inSelectedOption = false;
        bool foundOption = false;
        while (input >> tag)
        {
            if (tag == "OPTION")
            {
                int filePatternId = -1;
                int fileOptionId = -1;
                int fileConfigIndex = -1;
                input >> filePatternId >> fileOptionId >> fileConfigIndex;
                inSelectedOption = filePatternId == patternId && fileOptionId == optionId;
                foundOption = foundOption || inSelectedOption;
            }
            else if (tag == "MAP")
            {
                RemapEntry entry;
                char remapKind = 'R';
                input >> entry.addressEntry.channel
                      >> entry.addressEntry.pseudochannel
                      >> entry.addressEntry.bankgroup
                      >> entry.addressEntry.bank
                      >> entry.addressEntry.row
                      >> entry.addressEntry.col
                      >> remapKind
                      >> entry.newRowColAddr
                      >> entry.Latency;
                if (inSelectedOption)
                {
                    entry.isSpareRow = remapKind == 'R';
                    RemapEntries.push_back(entry);
                }
            }
            else if (tag == "END_OPTION")
            {
                if (inSelectedOption)
                {
                    return true;
                }
                inSelectedOption = false;
            }
            else
            {
                input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
        RemapEntries.clear();
        return foundOption && !input.bad();
    }

    // Query the option previously selected by loadFromLog().
    RemappedResult remapAddressFromLog(const AddressEntry &addrEntry) const
    {
        return remapAddress(addrEntry);
    }

    int getLatency(const AddressEntry &addrEntry) const
    {
        return remapAddress(addrEntry).latency;
    }

    AddressEntry getRemappedAddr(const AddressEntry &addrEntry) const
    {
        return remapAddress(addrEntry).addressEntry;
    }

};



class RECAM_PE
{
public:
    int Rs ; // number of spare rows
    int Cs ; // number of spare columns
    int buff_num ;
    int matrixSize ;
    bool isRepairable = false;
    bool RepairSuccess = false;

    std::unique_ptr<RECAM_addressCAM>   addressCAM;
    std::unique_ptr<RECAM_hybridCAM>    hybridCAM;
    std::unique_ptr<RECAM_bufferCAM>    bufferCAM;

    using FaultAnalyzeMatrix = vector<vector<bool>>;
    FaultAnalyzeMatrix  faultAnalyzeMatrixHardware;
    std::vector<int>    validSolList; // store the index of valid solutions
    std::vector<RemapTable> remapTableList; // one remap table for each validSolList entry

    RECAM_PE(int r_spare, int c_spare, int buff_num)
        : Rs(r_spare), Cs(c_spare), buff_num(buff_num), matrixSize(r_spare + c_spare),
          addressCAM(std::make_unique<RECAM_addressCAM>(r_spare, c_spare, buff_num)),
          hybridCAM(std::make_unique<RECAM_hybridCAM>(r_spare, c_spare, buff_num)),
          bufferCAM(std::make_unique<RECAM_bufferCAM>(buff_num))
    {
        faultAnalyzeMatrixHardware.resize(matrixSize, vector<bool>(matrixSize, false));
    }




    void loadFaultsToCAMs(FaultList &faultList);
    void printPE() ;
    void genFaultAnalyzeMatrix() ;
    void writeFaultAnalyzeMatrixToFile(const string &filename);
    void printFaultAnalyzeMatrix() ;
    bool checkSolution(const solMatrix &solution, int solIndex);
    void genValidSolList(const vector<solMatrix> &allSolutions);
    void printValidSolList() ;
    void genRepairTable(FaultList &faultList){};

    RemapTable buildRemapTable(const solVector &solutionVector) const;
    RemapTable::AddressEntry buildAddressEntryFromFault(const Fault &fault) const;



};


#endif // RECAM_PE_HPP
