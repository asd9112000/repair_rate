#include "../inc/SimplifiedFaultLoader.hpp"

#include <array>
#include <fstream>
#include <limits>
#include <map>
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

} // namespace dynamic_spare
