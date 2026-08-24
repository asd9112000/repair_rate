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
#include <optional>

// #include "inc/RECAM_addressCAM.hpp"
#include "./SolGenerator.hpp"
#include "./RECAM_CAM.hpp"
#include "./RECAM_bufferCAM.hpp"

using namespace std;


class RemapTable{
public:
    static constexpr int kDefaultRemapLatency = 10;
    // The paper reports the online CAM address/data path completing within
    // 2--3 cycles (Sec. IV-D).  Use the conservative endpoint for BUFFMAP.
    static constexpr int kDefaultCamReuseLatency = 3;

    struct AddressEntry
    {
        int HBMID            = 99;
        int ChannelID        = 99;
        int BankID           = 99;
        int SubarrayGroupID  = 99;
        int SubarrayID       = 99;
        int r                = 99;
        int c                = 99;

    };

    // Entry in the lookup table for remapping addresses.
    struct RemapEntry
    {
        AddressEntry addressEntry;
        bool isSpareRow     = true;
        int newRowColAddr   = 99;
        int Latency = kDefaultRemapLatency;
    };

    // Exact-address repair performed by the CAM-reuse storage.  Unlike a
    // MAP entry, this does not consume or name a spare row/column.
    struct BufferRemapEntry
    {
        AddressEntry addressEntry;
        int Latency = kDefaultCamReuseLatency;
    };


    struct RemappedResult
    {
        AddressEntry addressEntry;
        int latency = 0;
        bool servedByBufferCAM = false;
    };

    std::vector<RemapEntry> RemapEntries;
    std::vector<BufferRemapEntry> BufferRemapEntries;
    void addRemapEntry(const RemapEntry &entry){
        RemapEntries.push_back(entry);
    }

    static bool isSamePhysicalAddress(
        const AddressEntry &lhs,
        const AddressEntry &rhs)
    {
        return lhs.HBMID == rhs.HBMID &&
               lhs.ChannelID == rhs.ChannelID &&
               lhs.BankID == rhs.BankID &&
               lhs.SubarrayGroupID == rhs.SubarrayGroupID &&
               lhs.SubarrayID == rhs.SubarrayID &&
               lhs.r == rhs.r &&
               lhs.c == rhs.c;
    }

    void addBufferRemapEntry(const BufferRemapEntry &entry)
    {
        const bool duplicate = std::any_of(
            BufferRemapEntries.begin(), BufferRemapEntries.end(),
            [&entry](const BufferRemapEntry &existing)
            {
                return isSamePhysicalAddress(
                    existing.addressEntry, entry.addressEntry);
            });
        if (!duplicate)
        {
            BufferRemapEntries.push_back(entry);
        }
    }

    bool isMatchedThisEntry(const AddressEntry &addrEntry, const RemapEntry &entry) const
    {
        if (addrEntry.HBMID == entry.addressEntry.HBMID &&
            addrEntry.ChannelID == entry.addressEntry.ChannelID &&
            addrEntry.BankID == entry.addressEntry.BankID &&
            addrEntry.SubarrayGroupID == entry.addressEntry.SubarrayGroupID &&
            addrEntry.SubarrayID == entry.addressEntry.SubarrayID &&
            (entry.isSpareRow
                ? addrEntry.r == entry.addressEntry.r
                : addrEntry.c == entry.addressEntry.c))
        {
            return true;
        }
        return false;
    }


    RemappedResult remapAddress(const AddressEntry &addrEntry) const
    {
        RemappedResult result;
        result.addressEntry = addrEntry;

        // An exact CAM-reuse hit is served by CAM data rather than by a
        // spare line, so it takes precedence over normal line remapping.
        for (const auto &entry : BufferRemapEntries)
        {
            if (isSamePhysicalAddress(addrEntry, entry.addressEntry))
            {
                result.latency = entry.Latency;
                result.servedByBufferCAM = true;
                return result;
            }
        }

        for (const auto &e : RemapEntries)
        {
            if (isMatchedThisEntry(addrEntry, e))
            {
                if (e.isSpareRow) result.addressEntry.r = e.newRowColAddr;
                else              result.addressEntry.c = e.newRowColAddr;
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
        BufferRemapEntries.clear();
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
                input >> entry.addressEntry.HBMID
                      >> entry.addressEntry.ChannelID
                      >> entry.addressEntry.BankID
                      >> entry.addressEntry.SubarrayGroupID
                      >> entry.addressEntry.SubarrayID
                      >> entry.addressEntry.r
                      >> entry.addressEntry.c
                      >> remapKind
                      >> entry.newRowColAddr
                      >> entry.Latency;
                if (inSelectedOption)
                {
                    entry.isSpareRow = remapKind == 'R';
                    RemapEntries.push_back(entry);
                }
            }
            else if (tag == "BUFFMAP")
            {
                BufferRemapEntry entry;
                input >> entry.addressEntry.HBMID
                      >> entry.addressEntry.ChannelID
                      >> entry.addressEntry.BankID
                      >> entry.addressEntry.SubarrayGroupID
                      >> entry.addressEntry.SubarrayID
                      >> entry.addressEntry.r
                      >> entry.addressEntry.c
                      >> entry.Latency;
                if (inSelectedOption)
                {
                    addBufferRemapEntry(entry);
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
        BufferRemapEntries.clear();
        return foundOption && !input.bad();
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
    bool camStorageOverflow = false;

    std::unique_ptr<RECAM_addressCAM>   addressCAM;
    std::unique_ptr<RECAM_hybridCAM>    hybridCAM;
    std::unique_ptr<RECAM_bufferCAM>    bufferCAM;

    using FaultAnalyzeMatrix = vector<vector<bool>>;
    FaultAnalyzeMatrix  faultAnalyzeMatrixHardware;
    // Physical address provenance for every logical matrix row/column.
    // Address-CAM entries initialize the diagonal slots; Hybrid-CAM Case 1
    // may populate otherwise free slots.  Remap generation must use these
    // vectors rather than only the Address CAM, or selected extended lines
    // disappear from RemapTable.txt.
    std::vector<std::optional<RemapTable::AddressEntry>> matrixRowAddresses;
    std::vector<std::optional<RemapTable::AddressEntry>> matrixColumnAddresses;
    std::vector<int>    validSolList; // store the index of valid solutions
    std::vector<RemapTable> remapTableList; // one remap table for each validSolList entry

    RECAM_PE(
        int r_spare,
        int c_spare,
        int buff_num)
        : Rs(r_spare), Cs(c_spare), buff_num(buff_num), matrixSize(r_spare + c_spare),
          addressCAM(std::make_unique<RECAM_addressCAM>(r_spare, c_spare, buff_num)),
          hybridCAM(std::make_unique<RECAM_hybridCAM>(r_spare, c_spare, buff_num)),
          bufferCAM(std::make_unique<RECAM_bufferCAM>(buff_num))
    {
        faultAnalyzeMatrixHardware.resize(matrixSize, vector<bool>(matrixSize, false));
        matrixRowAddresses.resize(matrixSize);
        matrixColumnAddresses.resize(matrixSize);
    }




    void loadFaultsToCAMs(FaultList &faultList);
    void printPE() ;
    void genFaultAnalyzeMatrix() ;
    void writeFaultAnalyzeMatrixToFile(const string &filename);
    void printFaultAnalyzeMatrix() ;
    bool checkSolution(const solMatrix &solution, int solIndex);
    void genValidSolList(const vector<solMatrix> &allSolutions);
    void printValidSolList() ;
    void genRepairTable(FaultList &){};

    RemapTable::AddressEntry buildAddressEntryFromFault(const Fault &fault) const;
    RemapTable buildRemapTable(const solVector &solutionVector) const;



};


#endif // RECAM_PE_HPP
