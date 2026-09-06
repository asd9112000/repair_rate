#include "../inc/SimplifiedFaultLoader.hpp"

#include <array>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace dynamic_spare
{
namespace
{

using GroupIdentity = std::tuple<int, int, int, int>;

int checkedInt(long long value, const char *field, std::size_t lineNumber)
{
    if (value < std::numeric_limits<int>::min() ||
        value > std::numeric_limits<int>::max())
    {
        throw std::runtime_error(
            "Simplified fault " + std::string(field) +
            " is outside the int range at line " +
            std::to_string(lineNumber));
    }
    return static_cast<int>(value);
}

} // namespace

std::vector<FaultGroup> loadSimplifiedFaultGroups(
    const std::filesystem::path &path,
    const SimulationConfig &config)
{
    std::ifstream input(path);
    if (!input.is_open())
    {
        throw std::runtime_error(
            "Unable to open simplified fault file: " + path.string());
    }

    std::map<GroupIdentity, FaultGroup> grouped;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line))
    {
        ++lineNumber;
        const std::size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#' ||
            line.compare(first, 2, "//") == 0)
        {
            continue;
        }

        std::istringstream parser(line);
        std::array<long long, 7> values{};
        for (long long &value : values)
        {
            if (!(parser >> value))
            {
                throw std::runtime_error(
                    "Simplified fault line must contain exactly seven "
                    "integers at line " + std::to_string(lineNumber));
            }
        }
        std::string extra;
        if (parser >> extra)
        {
            throw std::runtime_error(
                "Unexpected field in simplified fault file at line " +
                std::to_string(lineNumber) + ": " + extra);
        }

        Fault fault{};
        fault.HBMID = checkedInt(values[0], "HBMID", lineNumber);
        fault.ChannelID = checkedInt(values[1], "ChannelID", lineNumber);
        fault.BankID = checkedInt(values[2], "BankID", lineNumber);
        fault.SubarrayGroupID = checkedInt(
            values[3], "SubarrayGroupID", lineNumber);
        fault.SubarrayID = checkedInt(values[4], "SubarrayID", lineNumber);
        fault.r = checkedInt(values[5], "Row", lineNumber);
        fault.c = checkedInt(values[6], "Col", lineNumber);

        if (fault.HBMID < 0 || fault.ChannelID < 0 || fault.BankID < 0 ||
            fault.SubarrayGroupID < 0)
        {
            throw std::runtime_error(
                "Simplified fault hardware IDs cannot be negative at line " +
                std::to_string(lineNumber));
        }
        if (fault.SubarrayID < 0 ||
            fault.SubarrayID >= static_cast<int>(kSubarrayCount))
        {
            throw std::runtime_error(
                "Simplified fault SubarrayID must be between 0 and 3 at line " +
                std::to_string(lineNumber));
        }
        if (fault.r < 0 || fault.c < 0 ||
            static_cast<std::uint32_t>(fault.r) >= config.memoryRows ||
            static_cast<std::uint32_t>(fault.c) >= config.memoryColumns)
        {
            throw std::runtime_error(
                "Simplified fault row/column is outside configured memory at "
                "line " + std::to_string(lineNumber));
        }

        const GroupIdentity identity{
            fault.HBMID,
            fault.ChannelID,
            fault.BankID,
            fault.SubarrayGroupID};
        grouped[identity][static_cast<std::size_t>(fault.SubarrayID)]
            .push_back(fault);
    }
    if (!input.eof())
    {
        throw std::runtime_error(
            "Failed while reading simplified fault file: " + path.string());
    }
    if (grouped.empty())
    {
        throw std::runtime_error(
            "Simplified fault file contains no fault addresses: " +
            path.string());
    }

    std::vector<FaultGroup> groups;
    groups.reserve(grouped.size());
    for (auto &entry : grouped)
    {
        groups.push_back(std::move(entry.second));
    }
    return groups;
}

std::vector<FaultGroup> loadCountFramedFaultGroups(
    const std::filesystem::path &path,
    const SimulationConfig &config)
{
    std::ifstream input(path);
    if (!input.is_open())
    {
        throw std::runtime_error(
            "Unable to open count-framed fault file: " + path.string());
    }

    std::ostringstream cleaned;
    std::string line;
    while (std::getline(input, line))
    {
        const std::size_t comment = line.find("//");
        cleaned << line.substr(0, comment) << '\n';
    }
    if (!input.eof())
    {
        throw std::runtime_error(
            "Failed while reading count-framed fault file: " +
            path.string());
    }

    std::istringstream tokens(cleaned.str());
    long long listCountValue = -1;
    if (!(tokens >> listCountValue) || listCountValue < 0)
    {
        throw std::runtime_error(
            "Count-framed fault file has an invalid list count");
    }
    const std::size_t listCount = static_cast<std::size_t>(listCountValue);
    if (listCount == 0 || listCount % kSubarrayCount != 0)
    {
        throw std::runtime_error(
            "Count-framed fault list count must be a positive multiple of 4");
    }

    std::vector<FaultGroup> groups(listCount / kSubarrayCount);
    std::vector<std::optional<GroupIdentity>> identities(groups.size());
    for (std::size_t list = 0; list < listCount; ++list)
    {
        long long faultCountValue = -1;
        if (!(tokens >> faultCountValue) || faultCountValue < 0)
        {
            throw std::runtime_error(
                "Invalid count-framed fault count at list " +
                std::to_string(list));
        }
        const std::size_t group = list / kSubarrayCount;
        const std::size_t subarray = list % kSubarrayCount;
        auto &destination = groups[group][subarray];
        destination.reserve(static_cast<std::size_t>(faultCountValue));
        for (long long index = 0; index < faultCountValue; ++index)
        {
            std::array<long long, 7> values{};
            for (long long &value : values)
            {
                if (!(tokens >> value))
                {
                    throw std::runtime_error(
                        "Invalid seven-field record in count-framed list " +
                        std::to_string(list));
                }
            }
            Fault fault{};
            fault.HBMID = checkedInt(values[0], "HBMID", list + 1);
            fault.ChannelID = checkedInt(values[1], "ChannelID", list + 1);
            fault.BankID = checkedInt(values[2], "BankID", list + 1);
            fault.SubarrayGroupID = checkedInt(
                values[3], "SubarrayGroupID", list + 1);
            fault.SubarrayID = checkedInt(
                values[4], "SubarrayID", list + 1);
            fault.r = checkedInt(values[5], "Row", list + 1);
            fault.c = checkedInt(values[6], "Col", list + 1);
            if (fault.HBMID < 0 || fault.ChannelID < 0 ||
                fault.BankID < 0 || fault.SubarrayGroupID < 0 ||
                fault.SubarrayID != static_cast<int>(subarray))
            {
                throw std::runtime_error(
                    "Count-framed fault hardware identity is invalid at "
                    "list " + std::to_string(list));
            }
            if (fault.r < 0 || fault.c < 0 ||
                static_cast<std::uint32_t>(fault.r) >= config.memoryRows ||
                static_cast<std::uint32_t>(fault.c) >= config.memoryColumns)
            {
                throw std::runtime_error(
                    "Count-framed physical fault is outside configured "
                    "memory at list " + std::to_string(list));
            }
            const GroupIdentity identity{
                fault.HBMID,
                fault.ChannelID,
                fault.BankID,
                fault.SubarrayGroupID};
            if (!identities[group].has_value())
            {
                identities[group] = identity;
            }
            else if (*identities[group] != identity)
            {
                throw std::runtime_error(
                    "A count-framed A/B/C/D group spans multiple hardware "
                    "identities");
            }
            destination.push_back(fault);
        }
    }
    std::string trailing;
    if (tokens >> trailing)
    {
        throw std::runtime_error(
            "Unexpected trailing token in count-framed fault file: " +
            trailing);
    }
    return groups;
}

} // namespace dynamic_spare
