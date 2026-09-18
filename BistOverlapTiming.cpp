#include "BistOverlapTimingModel.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

std::vector<std::string> splitCsvLine(const std::string &line)
{
    std::vector<std::string> fields;
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ','))
    {
        fields.push_back(field);
    }
    return fields;
}

std::uint64_t parseUnsigned(const std::string &field, const char *description)
{
    std::size_t consumed = 0;
    const std::uint64_t value = std::stoull(field, &consumed);
    if (consumed != field.size())
    {
        throw std::invalid_argument(std::string("Invalid ") + description);
    }
    return value;
}

Fault makePhysicalFault(
    std::uint64_t subarray,
    std::uint64_t row,
    std::uint64_t physicalColumn)
{
    if (subarray >= dynamic_spare::kSubarrayCount || row >= 512 ||
        physicalColumn >= 8192)
    {
        throw std::invalid_argument(
            "Fault CSV address is outside the frozen 2x2 BIST geometry");
    }
    Fault fault{};
    fault.SubarrayID = static_cast<int>(subarray);
    fault.r = static_cast<int>(row);
    fault.c = static_cast<int>(physicalColumn);
    return fault;
}

dynamic_spare::FaultGroup loadFaultGroup(const std::filesystem::path &path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("Cannot open T1 fault CSV: " + path.string());
    }

    dynamic_spare::FaultGroup faults;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line))
    {
        ++lineNumber;
        if (line.empty())
        {
            continue;
        }
        const std::vector<std::string> fields = splitCsvLine(line);
        if (lineNumber == 1 && fields.size() == 3 && fields[0] == "sa_id" &&
            fields[1] == "row" && fields[2] == "physical_cell_column")
        {
            continue;
        }
        if (fields.size() != 3)
        {
            throw std::invalid_argument(
                "T1 fault CSV requires sa_id,row,physical_cell_column at line " +
                std::to_string(lineNumber));
        }
        const std::uint64_t subarray = parseUnsigned(fields[0], "SA ID");
        const std::uint64_t row = parseUnsigned(fields[1], "row");
        const std::uint64_t physicalColumn = parseUnsigned(
            fields[2], "physical cell column");
        faults[subarray].push_back(
            makePhysicalFault(subarray, row, physicalColumn));
    }
    return faults;
}

std::string optionalCsv(const std::optional<std::uint64_t> &value)
{
    return value ? std::to_string(*value) : "N/A";
}

void writeTimingCsv(
    const std::filesystem::path &outputDirectory,
    const std::string &sampleId,
    const dynamic_spare::BistOverlapTimingResult &result)
{
    std::filesystem::create_directories(outputDirectory);
    const std::filesystem::path saPath = outputDirectory / "t1_sa_timing.csv";
    const std::filesystem::path groupPath = outputDirectory / "t1_group_timing.csv";
    std::ofstream saOutput(saPath);
    std::ofstream groupOutput(groupPath);
    if (!saOutput || !groupOutput)
    {
        throw std::runtime_error("Cannot create T1 timing CSV output files");
    }

    saOutput << "sample_id,sa_id,fault_count,last_fault_accept_cycle,"
             << "candidate_ready_cycle,candidate_catchup_latency,"
             << "ownership_wait_cycles,hidden_analysis_slack,"
             << "provisional_post_bist_latency,fully_hidden\n";
    for (std::size_t sa = 0; sa < dynamic_spare::kSubarrayCount; ++sa)
    {
        const auto &timing = result.subarrays[sa];
        saOutput << sampleId << ',' << sa << ',' << timing.physicalFaultCount << ','
                 << optionalCsv(timing.lastFaultAcceptCycle) << ','
                 << timing.candidateReadyCycle << ','
                 << optionalCsv(timing.candidateCatchupLatency) << ','
                 << optionalCsv(timing.ownershipWaitCycles) << ','
                 << timing.hiddenAnalysisSlack << ','
                 << timing.provisionalPostBistLatency << ','
                 << (timing.fullyHidden ? "true" : "false") << '\n';
    }

    groupOutput << "sample_id,group_test_done_cycle,latest_candidate_ready_cycle,"
                << "group_hidden_analysis_slack,"
                << "group_provisional_post_bist_latency,"
                << "all_candidates_ready_before_test_done\n";
    groupOutput << sampleId << ',' << result.group.groupTestDoneCycle << ','
                << result.group.latestCandidateReadyCycle << ','
                << result.group.hiddenAnalysisSlack << ','
                << result.group.provisionalPostBistLatency << ','
                << (result.group.allCandidatesReadyBeforeTestDone ? "true" : "false")
                << '\n';
}

void validateSampleId(const std::string &sampleId)
{
    if (sampleId.empty() || sampleId.find(',') != std::string::npos)
    {
        throw std::invalid_argument(
            "T1 sample ID must be nonempty and cannot contain a comma");
    }
}

} // namespace

int main(int argc, char *argv[])
{
    try
    {
        if (argc != 3 && argc != 4)
        {
            throw std::invalid_argument(
                "usage: BistOverlapTiming <fault_csv> <output_directory> [sample_id]");
        }
        const std::filesystem::path faultPath = argv[1];
        const std::filesystem::path outputDirectory = argv[2];
        const std::string sampleId = argc == 4 ? argv[3] : "0";
        validateSampleId(sampleId);

        const dynamic_spare::FaultGroup faults = loadFaultGroup(faultPath);
        const dynamic_spare::SerialBistSchedule schedule;
        const dynamic_spare::BistOverlapTimingModel model;
        const auto result = model.evaluateGroup(faults, schedule);
        writeTimingCsv(outputDirectory, sampleId, result);
        std::cout << "T1_GROUP_TIMING sample_id=" << sampleId
                  << " group_test_done=" << result.group.groupTestDoneCycle
                  << " latest_candidate_ready="
                  << result.group.latestCandidateReadyCycle
                  << " provisional_post_bist_analyze_latency="
                  << result.group.provisionalPostBistLatency << '\n';
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "T1_GROUP_TIMING FAIL: " << error.what() << '\n';
        return 1;
    }
}
