#ifndef REMAP_VALIDATOR_HPP
#define REMAP_VALIDATOR_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace recam_validation
{

struct PhysicalAddress
{
    int HBMID = -1;
    int ChannelID = -1;
    int BankID = -1;
    int SubarrayGroupID = -1;
    int SubarrayID = -1;
    int row = -1;
    int column = -1;
};

struct ValidationIssue
{
    int patternId = -1;
    int optionId = -1;
    int peId = -1;
    std::optional<PhysicalAddress> fault;
    std::string message;
};

struct RemapValidationReport
{
    bool parseSuccess = true;
    bool allOptionsValid = true;
    std::size_t patternCount = 0;
    std::size_t optionCount = 0;
    std::size_t checkedOptionCount = 0;
    std::size_t validOptionCount = 0;
    std::size_t invalidOptionCount = 0;
    std::size_t checkedFaultCount = 0;
    std::size_t coveredFaultCount = 0;
    std::vector<ValidationIssue> issues;

    bool success() const
    {
        return parseSuccess && allOptionsValid &&
               checkedOptionCount > 0 && invalidOptionCount == 0;
    }
};

// Independent physical-coverage oracle for SharedLine_SRAM output.  It does
// not use RECAM's RepairSuccess, fault matrix, CAM classification, or solution
// list.  MAP covers one complete physical row/column in the same domain;
// BUFFMAP covers one exact seven-field physical address.
RemapValidationReport validateRemapCoverage(
    const std::string &faultFile,
    const std::string &remapFile,
    std::optional<int> patternFilter = std::nullopt);

} // namespace recam_validation

#endif
