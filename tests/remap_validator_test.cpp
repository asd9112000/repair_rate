#include <iostream>
#include <stdexcept>

#include "RemapValidator.hpp"
#include "RECAM_PE.hpp"

namespace
{

void require(bool condition, const char *message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

} // namespace

int main()
{
    const std::string faultFile = "tests/remap_validator_faults.faults";

    const auto valid = recam_validation::validateRemapCoverage(
        faultFile, "tests/remap_validator_valid.txt");
    require(valid.success(), "valid MAP+BUFFMAP fixture was rejected");
    require(valid.checkedOptionCount == 1 && valid.validOptionCount == 1,
            "valid option counts are incorrect");
    require(valid.checkedFaultCount == 4 && valid.coveredFaultCount == 4,
            "valid fixture did not cover all four faults");

    const auto invalid = recam_validation::validateRemapCoverage(
        faultFile, "tests/remap_validator_invalid.txt");
    require(!invalid.success(), "missing BUFFMAP was not detected");
    require(invalid.invalidOptionCount == 1,
            "invalid option count is incorrect");
    require(invalid.checkedFaultCount == 4 && invalid.coveredFaultCount == 3,
            "invalid fixture coverage counts are incorrect");
    require(!invalid.issues.empty() && invalid.issues.back().fault.has_value(),
            "uncovered physical fault was not reported");

    const auto missingPattern = recam_validation::validateRemapCoverage(
        faultFile, "tests/remap_validator_valid.txt", 9);
    require(!missingPattern.success() && missingPattern.checkedOptionCount == 0,
            "missing pattern filter was incorrectly accepted");

    RemapTable runtimeTable;
    require(runtimeTable.loadFromLog(
                "tests/remap_validator_valid.txt", 0, 0),
            "runtime RemapTable parser rejected BUFFMAP fixture");
    require(runtimeTable.BufferRemapEntries.size() == 1,
            "runtime RemapTable parser omitted BUFFMAP");
    RemapTable::AddressEntry bufferAddress;
    bufferAddress.HBMID = 0;
    bufferAddress.ChannelID = 0;
    bufferAddress.BankID = 0;
    bufferAddress.SubarrayGroupID = 0;
    bufferAddress.SubarrayID = 1;
    bufferAddress.r = 5;
    bufferAddress.c = 5;
    const auto bufferResult = runtimeTable.remapAddress(bufferAddress);
    require(bufferResult.servedByBufferCAM && bufferResult.latency == 3,
            "runtime exact-address BUFFMAP lookup failed");

    std::cout << "Remap coverage validator tests passed\n";
}
