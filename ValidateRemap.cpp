#include <iostream>
#include <optional>
#include <string>

#include "inc/RemapValidator.hpp"

int main(int argc, char *argv[])
{
    std::string faultFile = "./fault_generator/faults.faults";
    std::string remapFile = "./reports/SharedLine_SRAM/RemapTable.txt";
    std::optional<int> patternFilter;

    if (argc != 1 && argc != 3 && argc != 4)
    {
        std::cerr
            << "Usage: " << argv[0]
            << " [<faults.faults> <RemapTable.txt> [pattern_id]]\n";
        return 2;
    }
    if (argc >= 3)
    {
        faultFile = argv[1];
        remapFile = argv[2];
    }
    if (argc == 4)
    {
        try
        {
            patternFilter = std::stoi(argv[3]);
        }
        catch (const std::exception &)
        {
            std::cerr << "Invalid pattern_id: " << argv[3] << '\n';
            return 2;
        }
    }

    const auto report = recam_validation::validateRemapCoverage(
        faultFile, remapFile, patternFilter);

    std::cout << "Remap coverage validation\n"
              << "  patterns in fault file : " << report.patternCount << '\n'
              << "  options in remap file  : " << report.optionCount << '\n'
              << "  options checked        : " << report.checkedOptionCount << '\n'
              << "  valid / invalid        : " << report.validOptionCount
              << " / " << report.invalidOptionCount << '\n'
              << "  faults covered         : " << report.coveredFaultCount
              << " / " << report.checkedFaultCount << '\n';

    for (const auto &issue : report.issues)
    {
        std::cerr << "  issue pattern=" << issue.patternId
                  << " option=" << issue.optionId
                  << " pe=" << issue.peId << ": " << issue.message;
        if (issue.fault.has_value())
        {
            const auto &fault = *issue.fault;
            std::cerr << " ["
                      << fault.HBMID << ' '
                      << fault.ChannelID << ' '
                      << fault.BankID << ' '
                      << fault.SubarrayGroupID << ' '
                      << fault.SubarrayID << ' '
                      << fault.row << ' '
                      << fault.column << ']';
        }
        std::cerr << '\n';
    }

    if (!report.success())
    {
        std::cerr << "REMAP VALIDATION FAILED\n";
        return 1;
    }

    std::cout << "REMAP VALIDATION PASSED\n";
    return 0;
}
