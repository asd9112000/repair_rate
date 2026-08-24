#include "../inc/RemapValidator.hpp"

#include <array>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace recam_validation
{
namespace
{

using FaultLists = std::vector<std::vector<PhysicalAddress>>;

struct LineMap
{
    PhysicalAddress address;
    char kind = '?';
    int destination = -1;
    int latency = -1;
};

struct BufferMap
{
    PhysicalAddress address;
    int latency = -1;
};

struct PeRemaps
{
    bool seen = false;
    int spareRows = -1;
    int spareColumns = -1;
    int solutionIndex = -1;
    std::vector<LineMap> lineMaps;
    std::vector<BufferMap> bufferMaps;
};

struct RemapOption
{
    int patternId = -1;
    int optionId = -1;
    int configIndex = -1;
    bool syntaxValid = true;
    bool ended = false;
    std::array<PeRemaps, 4> pes;
};

std::string trim(const std::string &input)
{
    const std::size_t first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        return {};
    }
    const std::size_t last = input.find_last_not_of(" \t\r\n");
    return input.substr(first, last - first + 1);
}

std::string stripComment(const std::string &line, const std::string &marker)
{
    const std::size_t position = line.find(marker);
    return position == std::string::npos ? line : line.substr(0, position);
}

bool sameDomain(const PhysicalAddress &lhs, const PhysicalAddress &rhs)
{
    return lhs.HBMID == rhs.HBMID &&
           lhs.ChannelID == rhs.ChannelID &&
           lhs.BankID == rhs.BankID &&
           lhs.SubarrayGroupID == rhs.SubarrayGroupID &&
           lhs.SubarrayID == rhs.SubarrayID;
}

bool sameAddress(const PhysicalAddress &lhs, const PhysicalAddress &rhs)
{
    return sameDomain(lhs, rhs) &&
           lhs.row == rhs.row &&
           lhs.column == rhs.column;
}

void addIssue(
    RemapValidationReport &report,
    int patternId,
    int optionId,
    int peId,
    const std::string &message,
    std::optional<PhysicalAddress> fault = std::nullopt)
{
    report.issues.push_back({
        patternId, optionId, peId, std::move(fault), message});
}

FaultLists loadFaultLists(
    const std::string &filename,
    RemapValidationReport &report)
{
    std::ifstream input(filename);
    if (!input.is_open())
    {
        throw std::runtime_error("cannot open fault file: " + filename);
    }

    std::ostringstream cleaned;
    std::string line;
    while (std::getline(input, line))
    {
        cleaned << stripComment(line, "//") << '\n';
    }

    std::istringstream tokens(cleaned.str());
    int listCount = -1;
    if (!(tokens >> listCount) || listCount < 0)
    {
        throw std::runtime_error("invalid fault-list count");
    }

    FaultLists faultLists(static_cast<std::size_t>(listCount));
    for (int listIndex = 0; listIndex < listCount; ++listIndex)
    {
        int faultCount = -1;
        if (!(tokens >> faultCount) || faultCount < 0)
        {
            throw std::runtime_error(
                "invalid fault count for list " + std::to_string(listIndex));
        }

        auto &faults = faultLists[static_cast<std::size_t>(listIndex)];
        faults.reserve(static_cast<std::size_t>(faultCount));
        for (int faultIndex = 0; faultIndex < faultCount; ++faultIndex)
        {
            PhysicalAddress fault;
            if (!(tokens >> fault.HBMID
                         >> fault.ChannelID
                         >> fault.BankID
                         >> fault.SubarrayGroupID
                         >> fault.SubarrayID
                         >> fault.row
                         >> fault.column))
            {
                throw std::runtime_error(
                    "invalid fault record in list " +
                    std::to_string(listIndex));
            }
            faults.push_back(fault);
        }
    }

    std::string trailing;
    if (tokens >> trailing)
    {
        throw std::runtime_error(
            "unexpected trailing token in fault file: " + trailing);
    }

    if (faultLists.size() % 4 != 0)
    {
        throw std::runtime_error(
            "fault-list count is not divisible by four SharedLine PEs");
    }

    report.patternCount = faultLists.size() / 4;
    for (std::size_t listIndex = 0; listIndex < faultLists.size(); ++listIndex)
    {
        const int peId = static_cast<int>(listIndex % 4);
        const auto &faults = faultLists[listIndex];
        if (faults.empty())
        {
            continue;
        }
        const PhysicalAddress &domain = faults.front();
        for (const PhysicalAddress &fault : faults)
        {
            if (!sameDomain(domain, fault))
            {
                throw std::runtime_error(
                    "fault list " + std::to_string(listIndex) +
                    " contains multiple physical domains");
            }
            if (fault.SubarrayID != peId)
            {
                throw std::runtime_error(
                    "fault list " + std::to_string(listIndex) +
                    " does not match its PE/SubarrayID");
            }
        }
    }
    return faultLists;
}

std::vector<RemapOption> loadRemapOptions(
    const std::string &filename,
    RemapValidationReport &report)
{
    std::ifstream input(filename);
    if (!input.is_open())
    {
        throw std::runtime_error("cannot open remap file: " + filename);
    }

    std::vector<RemapOption> options;
    RemapOption *currentOption = nullptr;
    int currentPe = -1;
    std::string rawLine;
    int lineNumber = 0;

    const auto syntaxIssue = [&](const std::string &message)
    {
        report.parseSuccess = false;
        if (currentOption != nullptr)
        {
            currentOption->syntaxValid = false;
        }
        addIssue(
            report,
            currentOption == nullptr ? -1 : currentOption->patternId,
            currentOption == nullptr ? -1 : currentOption->optionId,
            currentPe,
            "remap line " + std::to_string(lineNumber) + ": " + message);
    };

    while (std::getline(input, rawLine))
    {
        ++lineNumber;
        const std::string line = trim(stripComment(rawLine, "#"));
        if (line.empty())
        {
            continue;
        }

        std::istringstream fields(line);
        std::string tag;
        fields >> tag;

        if (tag == "OPTION")
        {
            if (currentOption != nullptr)
            {
                syntaxIssue("new OPTION before END_OPTION");
            }
            RemapOption option;
            if (!(fields >> option.patternId >> option.optionId >> option.configIndex))
            {
                syntaxIssue("malformed OPTION");
                continue;
            }
            options.push_back(option);
            currentOption = &options.back();
            currentPe = -1;
        }
        else if (tag == "PE")
        {
            int peId = -1;
            int spareRows = -1;
            int spareColumns = -1;
            int solutionIndex = -1;
            if (currentOption == nullptr ||
                !(fields >> peId >> spareRows >> spareColumns >> solutionIndex) ||
                peId < 0 || peId >= 4 || spareRows < 0 || spareColumns < 0)
            {
                syntaxIssue("malformed or out-of-context PE");
                continue;
            }
            if (currentPe != -1)
            {
                syntaxIssue("nested PE without END_PE");
            }
            PeRemaps &pe = currentOption->pes[static_cast<std::size_t>(peId)];
            if (pe.seen)
            {
                syntaxIssue("duplicate PE " + std::to_string(peId));
            }
            pe.seen = true;
            pe.spareRows = spareRows;
            pe.spareColumns = spareColumns;
            pe.solutionIndex = solutionIndex;
            currentPe = peId;
        }
        else if (tag == "MAP")
        {
            LineMap map;
            if (currentOption == nullptr || currentPe == -1 ||
                !(fields >> map.address.HBMID
                         >> map.address.ChannelID
                         >> map.address.BankID
                         >> map.address.SubarrayGroupID
                         >> map.address.SubarrayID
                         >> map.address.row
                         >> map.address.column
                         >> map.kind
                         >> map.destination
                         >> map.latency) ||
                (map.kind != 'R' && map.kind != 'C'))
            {
                syntaxIssue("malformed or out-of-context MAP");
                continue;
            }
            currentOption->pes[static_cast<std::size_t>(currentPe)]
                .lineMaps.push_back(map);
        }
        else if (tag == "BUFFMAP")
        {
            BufferMap map;
            if (currentOption == nullptr || currentPe == -1 ||
                !(fields >> map.address.HBMID
                         >> map.address.ChannelID
                         >> map.address.BankID
                         >> map.address.SubarrayGroupID
                         >> map.address.SubarrayID
                         >> map.address.row
                         >> map.address.column
                         >> map.latency))
            {
                syntaxIssue("malformed or out-of-context BUFFMAP");
                continue;
            }
            currentOption->pes[static_cast<std::size_t>(currentPe)]
                .bufferMaps.push_back(map);
        }
        else if (tag == "END_PE")
        {
            if (currentOption == nullptr || currentPe == -1)
            {
                syntaxIssue("END_PE without PE");
                continue;
            }
            currentPe = -1;
        }
        else if (tag == "END_OPTION")
        {
            if (currentOption == nullptr || currentPe != -1)
            {
                syntaxIssue("END_OPTION before a complete OPTION/PE");
                continue;
            }
            currentOption->ended = true;
            currentOption = nullptr;
        }
        else
        {
            syntaxIssue("unknown tag " + tag);
        }
    }

    if (currentPe != -1 || currentOption != nullptr)
    {
        syntaxIssue("file ended before END_PE/END_OPTION");
    }

    report.optionCount = options.size();
    return options;
}

bool isCovered(
    const PhysicalAddress &fault,
    const PeRemaps &pe)
{
    for (const BufferMap &map : pe.bufferMaps)
    {
        if (sameAddress(fault, map.address))
        {
            return true;
        }
    }
    for (const LineMap &map : pe.lineMaps)
    {
        if (!sameDomain(fault, map.address))
        {
            continue;
        }
        if ((map.kind == 'R' && fault.row == map.address.row) ||
            (map.kind == 'C' && fault.column == map.address.column))
        {
            return true;
        }
    }
    return false;
}

using SourceKey = std::tuple<int, int, int, int, int, int>;
using DestinationKey = std::tuple<int, int, int, int, int, char, int>;

SourceKey sourceKey(const LineMap &map)
{
    return {
        map.address.HBMID,
        map.address.ChannelID,
        map.address.BankID,
        map.address.SubarrayGroupID,
        map.address.SubarrayID,
        map.kind == 'R' ? map.address.row : map.address.column};
}

DestinationKey destinationKey(const LineMap &map)
{
    return {
        map.address.HBMID,
        map.address.ChannelID,
        map.address.BankID,
        map.address.SubarrayGroupID,
        map.address.SubarrayID,
        map.kind,
        map.destination};
}

bool validatePeResources(
    const RemapOption &option,
    int peId,
    RemapValidationReport &report)
{
    const PeRemaps &pe = option.pes[static_cast<std::size_t>(peId)];
    bool valid = true;
    std::set<SourceKey> rowSources;
    std::set<SourceKey> columnSources;
    std::map<std::pair<char, SourceKey>, int> sourceDestinations;
    std::map<DestinationKey, SourceKey> destinationSources;

    for (const LineMap &map : pe.lineMaps)
    {
        if (map.latency < 0)
        {
            addIssue(report, option.patternId, option.optionId, peId,
                     "MAP latency is negative");
            valid = false;
        }
        const SourceKey source = sourceKey(map);
        (map.kind == 'R' ? rowSources : columnSources).insert(source);

        const auto sourceDestinationKey = std::make_pair(map.kind, source);
        const auto sourceIt = sourceDestinations.find(sourceDestinationKey);
        if (sourceIt != sourceDestinations.end() &&
            sourceIt->second != map.destination)
        {
            addIssue(report, option.patternId, option.optionId, peId,
                     "one source line maps to conflicting destinations");
            valid = false;
        }
        sourceDestinations[sourceDestinationKey] = map.destination;

        const DestinationKey destination = destinationKey(map);
        const auto destinationIt = destinationSources.find(destination);
        if (destinationIt != destinationSources.end() &&
            destinationIt->second != source)
        {
            addIssue(report, option.patternId, option.optionId, peId,
                     "multiple source lines share one destination");
            valid = false;
        }
        destinationSources[destination] = source;
    }

    for (const BufferMap &map : pe.bufferMaps)
    {
        if (map.latency < 0)
        {
            addIssue(report, option.patternId, option.optionId, peId,
                     "BUFFMAP latency is negative");
            valid = false;
        }
    }

    if (rowSources.size() > static_cast<std::size_t>(pe.spareRows))
    {
        addIssue(report, option.patternId, option.optionId, peId,
                 "distinct row MAP count exceeds PE spare_rows");
        valid = false;
    }
    if (columnSources.size() > static_cast<std::size_t>(pe.spareColumns))
    {
        addIssue(report, option.patternId, option.optionId, peId,
                 "distinct column MAP count exceeds PE spare_cols");
        valid = false;
    }
    return valid;
}

} // namespace

RemapValidationReport validateRemapCoverage(
    const std::string &faultFile,
    const std::string &remapFile,
    std::optional<int> patternFilter)
{
    RemapValidationReport report;
    FaultLists faultLists;
    std::vector<RemapOption> options;
    try
    {
        faultLists = loadFaultLists(faultFile, report);
        options = loadRemapOptions(remapFile, report);
    }
    catch (const std::exception &error)
    {
        report.parseSuccess = false;
        report.allOptionsValid = false;
        addIssue(report, -1, -1, -1, error.what());
        return report;
    }

    for (const RemapOption &option : options)
    {
        if (patternFilter.has_value() && option.patternId != *patternFilter)
        {
            continue;
        }

        ++report.checkedOptionCount;
        bool optionValid = option.syntaxValid && option.ended;
        if (option.patternId < 0 ||
            static_cast<std::size_t>(option.patternId) >= report.patternCount)
        {
            addIssue(report, option.patternId, option.optionId, -1,
                     "OPTION pattern_id is outside faults.faults");
            optionValid = false;
        }

        for (int peId = 0; peId < 4; ++peId)
        {
            const PeRemaps &pe = option.pes[static_cast<std::size_t>(peId)];
            if (!pe.seen)
            {
                addIssue(report, option.patternId, option.optionId, peId,
                         "OPTION is missing this PE section");
                optionValid = false;
                continue;
            }
            optionValid = validatePeResources(option, peId, report) && optionValid;

            if (option.patternId < 0 ||
                static_cast<std::size_t>(option.patternId) >= report.patternCount)
            {
                continue;
            }
            const std::size_t faultListIndex =
                static_cast<std::size_t>(option.patternId) * 4 +
                static_cast<std::size_t>(peId);
            for (const PhysicalAddress &fault : faultLists[faultListIndex])
            {
                ++report.checkedFaultCount;
                if (isCovered(fault, pe))
                {
                    ++report.coveredFaultCount;
                }
                else
                {
                    addIssue(report, option.patternId, option.optionId, peId,
                             "fault is not covered by MAP or BUFFMAP", fault);
                    optionValid = false;
                }
            }
        }

        if (optionValid)
        {
            ++report.validOptionCount;
        }
        else
        {
            ++report.invalidOptionCount;
        }
    }

    if (report.checkedOptionCount == 0)
    {
        addIssue(
            report,
            patternFilter.value_or(-1), -1, -1,
            patternFilter.has_value()
                ? "no remap OPTION exists for the requested pattern"
                : "remap file contains no OPTION to validate");
    }

    report.allOptionsValid = report.parseSuccess &&
                             report.checkedOptionCount > 0 &&
                             report.invalidOptionCount == 0;
    return report;
}

} // namespace recam_validation
