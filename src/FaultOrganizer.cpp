#include "../inc/FaultOrganizer.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace
{
constexpr int kSubarraysPerGroup = 4;

struct FaultAddress
{
    int HBMID;
    int ChannelID;
    int BankID;
    int SubarrayGroupID;
    int SubarrayID;
    int r;
    int c;
};

using SubarrayGroupKey = std::array<int, 4>;
using BankKey = std::array<int, 3>;
using FaultsBySubarray =
    std::map<SubarrayGroupKey, std::array<std::vector<FaultAddress>, kSubarraysPerGroup>>;
}

bool organizeSimplifiedFaults(
    const std::string &inputFilename,
    const std::string &outputFilename,
    int subarrayGroupCount)
{
    if (subarrayGroupCount == 0 || subarrayGroupCount < -1)
    {
        std::cerr << "Error: subarrayGroupCount must be positive or -1 for automatic detection.\n";
        return false;
    }

    std::ifstream input(inputFilename);
    if (!input.is_open())
    {
        std::cerr << "Error: Could not open " << inputFilename << " for reading.\n";
        return false;
    }

    FaultsBySubarray groupedFaults;
    std::set<BankKey> banks;
    std::map<BankKey, int> detectedGroupCounts;
    std::string line;
    int lineNumber = 0;
    while (std::getline(input, line))
    {
        ++lineNumber;

        const std::size_t firstCharacter = line.find_first_not_of(" \t\r");
        if (firstCharacter == std::string::npos ||
            line.compare(firstCharacter, 2, "//") == 0 ||
            line[firstCharacter] == '#')
        {
            continue;
        }

        FaultAddress fault{};
        std::istringstream parser(line);
        if (!(parser >> fault.HBMID
                     >> fault.ChannelID
                     >> fault.BankID
                     >> fault.SubarrayGroupID
                     >> fault.SubarrayID
                     >> fault.r
                     >> fault.c))
        {
            std::cerr << "Error: Invalid fault address at line "
                      << lineNumber << ": " << line << "\n";
            return false;
        }

        std::string extraField;
        if (parser >> extraField)
        {
            std::cerr << "Error: Unexpected field at line "
                      << lineNumber << ": " << extraField << "\n";
            return false;
        }

        if (fault.SubarrayID < 0 || fault.SubarrayID >= kSubarraysPerGroup)
        {
            std::cerr << "Error: SubarrayID must be between 0 and 3 at line "
                      << lineNumber << ", but got " << fault.SubarrayID << ".\n";
            return false;
        }
        if (fault.SubarrayGroupID < 0)
        {
            std::cerr << "Error: SubarrayGroupID cannot be negative at line "
                      << lineNumber << ".\n";
            return false;
        }
        if (subarrayGroupCount > 0 &&
            fault.SubarrayGroupID >= subarrayGroupCount)
        {
            std::cerr << "Error: SubarrayGroupID " << fault.SubarrayGroupID
                      << " at line " << lineNumber
                      << " exceeds the configured group count "
                      << subarrayGroupCount << ".\n";
            return false;
        }

        const SubarrayGroupKey groupKey = {
            fault.HBMID,
            fault.ChannelID,
            fault.BankID,
            fault.SubarrayGroupID};
        const BankKey bankKey = {
            fault.HBMID,
            fault.ChannelID,
            fault.BankID};

        banks.insert(bankKey);
        detectedGroupCounts[bankKey] =
            std::max(detectedGroupCounts[bankKey], fault.SubarrayGroupID + 1);
        groupedFaults[groupKey][fault.SubarrayID].push_back(fault);
    }

    for (const BankKey &bankKey : banks)
    {
        const int groupCount =
            subarrayGroupCount > 0 ? subarrayGroupCount : detectedGroupCounts[bankKey];
        for (int subarrayGroupID = 0;
             subarrayGroupID < groupCount;
             ++subarrayGroupID)
        {
            groupedFaults[{bankKey[0], bankKey[1], bankKey[2], subarrayGroupID}];
        }
    }

    std::ofstream output(outputFilename);
    if (!output.is_open())
    {
        std::cerr << "Error: Could not open " << outputFilename << " for writing.\n";
        return false;
    }

    output << "// HBMID ChannelID BankID SubarrayGroupID SubarrayID Row Col\n";
    output << groupedFaults.size() * kSubarraysPerGroup << "\n";

    for (const auto &[groupKey, subarrays] : groupedFaults)
    {
        for (int subarrayID = 0; subarrayID < kSubarraysPerGroup; ++subarrayID)
        {
            const std::vector<FaultAddress> &faults = subarrays[subarrayID];
            output << faults.size() << "\n";
            for (const FaultAddress &fault : faults)
            {
                output << fault.HBMID << " "
                       << fault.ChannelID << " "
                       << fault.BankID << " "
                       << fault.SubarrayGroupID << " "
                       << fault.SubarrayID << " "
                       << fault.r << " "
                       << fault.c << "\n";
            }
        }
    }

    if (!output)
    {
        std::cerr << "Error: Failed while writing " << outputFilename << ".\n";
        return false;
    }
    return true;
}
