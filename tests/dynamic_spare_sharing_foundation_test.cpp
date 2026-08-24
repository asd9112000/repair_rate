#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Fault.hpp"
#include "RECAMSolverAdapter.hpp"
#include "RECAM_PE.hpp"
#include "RepairResult.hpp"
#include "ResSpareLines.hpp"
#include "SimulationConfig.hpp"
#include "SolGenerator.hpp"

namespace
{

constexpr std::uint64_t kGoldenSeed = 20260820;
constexpr int kRows = 2;
constexpr int kColumns = 2;
constexpr int kBufferEntries = 2;
constexpr int kSharedLines = 1;
constexpr std::size_t kExpectedRunCount = 32;
constexpr std::size_t kExpectedGroupFaultCount = 28;
constexpr int kLegacyBaselineConfigIndex = 150;
constexpr std::uint64_t kExpectedFaultFixtureFnv1a = 9663152482410307176ULL;
constexpr std::uint64_t kExpectedSharedLineSourceFnv1a =
    5095697116192954848ULL;

const char *kFaultFixture =
    "tests/fixtures/dynamic_sharing_seed_20260820.faults";
const char *kAttemptGolden =
    "tests/golden/dynamic_sharing_seed_20260820_attempts.csv";
const char *kGroupGolden =
    "tests/golden/dynamic_sharing_seed_20260820_groups.csv";

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

std::uint64_t fileFnv1a(const std::string &path)
{
    std::ifstream input(path, std::ios::binary);
    require(input.is_open(), "Unable to open file for hashing: " + path);
    std::uint64_t hash = 14695981039346656037ULL;
    char byte = 0;
    while (input.get(byte))
    {
        hash ^= static_cast<unsigned char>(byte);
        hash *= 1099511628211ULL;
    }
    require(input.eof(), "Unable to finish hashing: " + path);
    return hash;
}

std::vector<std::string> split(const std::string &text, char delimiter)
{
    std::vector<std::string> fields;
    std::istringstream input(text);
    std::string field;
    while (std::getline(input, field, delimiter))
    {
        fields.push_back(field);
    }
    if (!text.empty() && text.back() == delimiter)
    {
        fields.emplace_back();
    }
    return fields;
}

std::vector<std::size_t> parseIndices(const std::string &field)
{
    std::vector<std::size_t> indices;
    if (field == "-" || field.empty())
    {
        return indices;
    }
    for (const std::string &value : split(field, '|'))
    {
        indices.push_back(static_cast<std::size_t>(std::stoull(value)));
    }
    return indices;
}

std::string formatIndices(const std::vector<std::size_t> &indices)
{
    if (indices.empty())
    {
        return "-";
    }
    std::ostringstream output;
    for (std::size_t index = 0; index < indices.size(); ++index)
    {
        if (index != 0)
        {
            output << '|';
        }
        output << indices[index];
    }
    return output.str();
}

struct LegacyCandidateOption
{
    std::size_t candidateIndex = 0;
    std::size_t usedRows = 0;
    std::size_t usedColumns = 0;
    std::size_t bufferCamRemapCount = 0;
    std::vector<std::tuple<bool, int, int>> mappings;
};

struct LegacyAttempt
{
    bool isRepairable = false;
    bool repairSuccess = false;
    bool camStorageOverflow = false;
    std::size_t faultCount = 0;
    std::size_t pivotFaultCount = 0;
    std::size_t nonpivotFaultCount = 0;
    std::size_t bufferedPivotFaultCount = 0;
    std::size_t overflowPivotFaultCount = 0;
    std::size_t addressCamActive = 0;
    std::size_t hybridCamActive = 0;
    std::size_t hybridCamPeak = 0;
    std::size_t candidateSolutions = 0;
    std::optional<std::size_t> selectedCandidateIndex;
    std::vector<std::size_t> validCandidateIndices;
    std::vector<LegacyCandidateOption> validCandidateOptions;
    std::size_t usedRows = 0;
    std::size_t usedColumns = 0;
};

std::string formatCandidateOptions(const LegacyAttempt &attempt)
{
    if (attempt.validCandidateOptions.empty())
    {
        return "-";
    }
    std::ostringstream output;
    for (std::size_t optionIndex = 0;
         optionIndex < attempt.validCandidateOptions.size(); ++optionIndex)
    {
        if (optionIndex != 0)
        {
            output << '|';
        }
        const LegacyCandidateOption &option =
            attempt.validCandidateOptions[optionIndex];
        output
            << option.candidateIndex << ':'
            << option.usedRows << ':'
            << option.usedColumns << ':'
            << option.bufferCamRemapCount << ':';
        if (option.mappings.empty())
        {
            output << '-';
            continue;
        }
        for (std::size_t mappingIndex = 0;
             mappingIndex < option.mappings.size(); ++mappingIndex)
        {
            if (mappingIndex != 0)
            {
                output << '~';
            }
            const auto &mapping = option.mappings[mappingIndex];
            output
                << (std::get<0>(mapping) ? 'R' : 'C')
                << std::get<1>(mapping) << '>'
                << std::get<2>(mapping);
        }
    }
    return output.str();
}

using PhysicalLineKey = std::tuple<int, int, int, int, int, int>;

LegacyAttempt solveLegacyDirect(
    const std::vector<Fault> &orderedFaults,
    int rows,
    int columns)
{
    FaultList faultList(rows, columns, kBufferEntries);
    for (const Fault &source : orderedFaults)
    {
        Fault fault = source;
        fault.isPivot = false;
        fault.isBuffer = false;
        fault.isNonPivot = false;
        fault.isOverflowPivot = false;
        faultList.addFault(fault);
    }
    faultList.classifyFaults();

    RECAM_PE pe(rows, columns, kBufferEntries);
    SolGenerator solutions(rows, columns);
    pe.loadFaultsToCAMs(faultList);
    pe.genFaultAnalyzeMatrix();
    pe.genValidSolList(solutions.allSolMatrixsType);

    LegacyAttempt result;
    result.isRepairable = pe.isRepairable;
    result.repairSuccess = pe.RepairSuccess;
    result.camStorageOverflow = pe.camStorageOverflow;
    result.faultCount = faultList.PEFaults.size();
    result.pivotFaultCount = faultList.pivotFaults.size();
    result.nonpivotFaultCount = faultList.nonPivotFaults.size();
    result.bufferedPivotFaultCount = faultList.bufferFaults.size();
    result.overflowPivotFaultCount = faultList.overflowPivotFaults.size();
    result.addressCamActive = pe.addressCAM->addressCAMEntries.size();
    result.hybridCamActive = pe.hybridCAM->hybridCAMEntries.size();
    result.hybridCamPeak = pe.hybridCAM->peakEntries;
    result.candidateSolutions = solutions.allSolMatrixsType.size();
    require(pe.validSolList.size() == pe.remapTableList.size(),
            "Legacy valid/remap list alignment changed");
    for (std::size_t validIndex = 0;
         validIndex < pe.validSolList.size(); ++validIndex)
    {
        const int candidate = pe.validSolList[validIndex];
        result.validCandidateIndices.push_back(
            static_cast<std::size_t>(candidate));
        const RemapTable &table = pe.remapTableList[validIndex];
        LegacyCandidateOption option;
        option.candidateIndex = static_cast<std::size_t>(candidate);
        option.bufferCamRemapCount = table.BufferRemapEntries.size();
        std::set<PhysicalLineKey> usedRows;
        std::set<PhysicalLineKey> usedColumns;
        for (const RemapTable::RemapEntry &entry : table.RemapEntries)
        {
            const RemapTable::AddressEntry &address = entry.addressEntry;
            PhysicalLineKey key = std::make_tuple(
                address.HBMID,
                address.ChannelID,
                address.BankID,
                address.SubarrayGroupID,
                address.SubarrayID,
                entry.isSpareRow ? address.r : address.c);
            option.mappings.emplace_back(
                entry.isSpareRow,
                entry.isSpareRow ? address.r : address.c,
                entry.newRowColAddr);
            if (entry.isSpareRow)
            {
                usedRows.insert(key);
            }
            else
            {
                usedColumns.insert(key);
            }
        }
        option.usedRows = usedRows.size();
        option.usedColumns = usedColumns.size();
        result.validCandidateOptions.push_back(std::move(option));
    }

    if (pe.RepairSuccess && !pe.validSolList.empty())
    {
        result.selectedCandidateIndex = static_cast<std::size_t>(
            pe.validSolList.front());
        result.usedRows = result.validCandidateOptions.front().usedRows;
        result.usedColumns = result.validCandidateOptions.front().usedColumns;
    }
    return result;
}

void compareAdapterToLegacy(
    const dynamic_spare::RepairAttemptResult &adapter,
    const LegacyAttempt &legacy,
    const std::string &context)
{
    require(adapter.isRepairable == legacy.isRepairable,
            context + ": isRepairable mismatch");
    require(adapter.repairSuccess == legacy.repairSuccess,
            context + ": repairSuccess mismatch");
    require(adapter.camStorageOverflow == legacy.camStorageOverflow,
            context + ": CAM-overflow mismatch");
    require(adapter.faultCount == legacy.faultCount,
            context + ": fault count mismatch");
    require(adapter.pivotFaultCount == legacy.pivotFaultCount,
            context + ": pivot count mismatch");
    require(adapter.nonpivotFaultCount == legacy.nonpivotFaultCount,
            context + ": nonpivot count mismatch");
    require(adapter.bufferedPivotFaultCount ==
                legacy.bufferedPivotFaultCount,
            context + ": buffered-pivot count mismatch");
    require(adapter.overflowPivotFaultCount ==
                legacy.overflowPivotFaultCount,
            context + ": overflow-pivot count mismatch");
    require(adapter.addressCamEntriesActive == legacy.addressCamActive,
            context + ": Address-CAM active count mismatch");
    require(adapter.hybridCamEntriesActive == legacy.hybridCamActive,
            context + ": Hybrid-CAM active count mismatch");
    require(adapter.addressCamEntriesPeak == legacy.addressCamActive,
            context + ": Address-CAM peak count mismatch");
    require(adapter.hybridCamEntriesPeak == legacy.hybridCamPeak,
            context + ": Hybrid-CAM peak count mismatch");
    require(adapter.candidateSolutions == legacy.candidateSolutions,
            context + ": candidate count mismatch");
    require(adapter.candidateSolutionsEvaluated ==
                legacy.candidateSolutions,
            context + ": legacy exhaustive evaluation mismatch");
    require(adapter.validCandidateIndices == legacy.validCandidateIndices,
            context + ": valid candidate indices mismatch");
    require(adapter.validCandidateOptions.size() ==
                legacy.validCandidateOptions.size(),
            context + ": candidate option count mismatch");
    for (std::size_t optionIndex = 0;
         optionIndex < legacy.validCandidateOptions.size(); ++optionIndex)
    {
        const auto &adaptedOption = adapter.validCandidateOptions[optionIndex];
        const auto &legacyOption = legacy.validCandidateOptions[optionIndex];
        require(adaptedOption.candidateIndex == legacyOption.candidateIndex &&
                    adaptedOption.usedRows == legacyOption.usedRows &&
                    adaptedOption.usedColumns == legacyOption.usedColumns &&
                    adaptedOption.bufferCamRemapCount ==
                        legacyOption.bufferCamRemapCount &&
                    adaptedOption.mappings.size() ==
                        legacyOption.mappings.size(),
                context + ": candidate option snapshot mismatch");
        for (std::size_t mappingIndex = 0;
             mappingIndex < legacyOption.mappings.size(); ++mappingIndex)
        {
            const auto &adaptedMapping = adaptedOption.mappings[mappingIndex];
            const auto &legacyMapping = legacyOption.mappings[mappingIndex];
            require(
                (adaptedMapping.dimension ==
                    dynamic_spare::SpareDimension::Row) ==
                        std::get<0>(legacyMapping) &&
                    adaptedMapping.originalAddress ==
                        std::get<1>(legacyMapping) &&
                    adaptedMapping.replacementAddress ==
                        std::get<2>(legacyMapping),
                context + ": candidate mapping snapshot mismatch");
        }
    }
    require(adapter.successfulCandidateIndex ==
                legacy.selectedCandidateIndex,
            context + ": selected candidate mismatch");
    require(adapter.usedRows == legacy.usedRows,
            context + ": used-row count mismatch");
    require(adapter.usedColumns == legacy.usedColumns,
            context + ": used-column count mismatch");
    require(*adapter.addressCamEntriesPeak >=
                adapter.addressCamEntriesActive &&
                *adapter.hybridCamEntriesPeak >=
                    adapter.hybridCamEntriesActive,
            context + ": peak CAM metric is below final active usage");
}

struct GoldenAttempt
{
    std::uint64_t seed = 0;
    std::size_t run = 0;
    std::size_t subarray = 0;
    std::size_t option = 0;
    int availableRows = 0;
    int availableColumns = 0;
    std::string candidateOptionSignature;
    LegacyAttempt attempt;
};

struct GoldenGroup
{
    std::uint64_t seed = 0;
    std::size_t run = 0;
    std::size_t groupFaultCount = 0;
    int baselineConfigIndex = -1;
    bool baselineSuccess = false;
    bool legacySharedSuccess = false;
    std::vector<std::size_t> successfulConfigIndices;
};

void compareLegacyToGolden(
    const LegacyAttempt &actual,
    const LegacyAttempt &golden,
    const std::string &context)
{
    require(actual.faultCount == golden.faultCount,
            context + ": golden fault count mismatch");
    require(actual.pivotFaultCount == golden.pivotFaultCount,
            context + ": golden pivot count mismatch");
    require(actual.nonpivotFaultCount == golden.nonpivotFaultCount,
            context + ": golden nonpivot count mismatch");
    require(actual.bufferedPivotFaultCount == golden.bufferedPivotFaultCount,
            context + ": golden buffered-pivot count mismatch");
    require(actual.overflowPivotFaultCount == golden.overflowPivotFaultCount,
            context + ": golden overflow-pivot count mismatch");
    require(actual.addressCamActive == golden.addressCamActive,
            context + ": golden Address-CAM count mismatch");
    require(actual.hybridCamActive == golden.hybridCamActive,
            context + ": golden Hybrid-CAM count mismatch");
    require(actual.camStorageOverflow == golden.camStorageOverflow,
            context + ": golden CAM-overflow mismatch");
    require(actual.repairSuccess == golden.repairSuccess,
            context + ": golden repair result mismatch");
    require(actual.candidateSolutions == golden.candidateSolutions,
            context + ": golden candidate count mismatch");
    require(actual.usedRows == golden.usedRows &&
                actual.usedColumns == golden.usedColumns,
            context + ": golden used spare count mismatch");
    require(actual.selectedCandidateIndex == golden.selectedCandidateIndex,
            context + ": golden selected candidate mismatch");
    require(actual.validCandidateIndices == golden.validCandidateIndices,
            context + ": golden valid candidate list mismatch");
}

using GoldenAttemptKey =
    std::tuple<std::size_t, std::size_t, std::size_t>;

std::map<GoldenAttemptKey, GoldenAttempt>
loadGoldenAttempts()
{
    std::ifstream input(kAttemptGolden);
    require(input.is_open(), "Unable to open attempt golden CSV");
    std::string line;
    require(static_cast<bool>(std::getline(input, line)),
            "Attempt golden CSV is empty");
    const std::string expectedHeader =
        "seed,run_index,subarray_id,option_index,available_rows,"
        "available_columns,fault_count,pivot_fault_count,"
        "nonpivot_fault_count,buffered_pivot_fault_count,"
        "overflow_pivot_fault_count,address_cam_active,hybrid_cam_active,"
        "cam_storage_overflow,repair_success,candidate_solutions,used_rows,"
        "used_columns,selected_candidate_index,valid_candidate_indices,"
        "candidate_option_signature";
    require(line == expectedHeader, "Unexpected attempt golden CSV header");

    std::map<GoldenAttemptKey, GoldenAttempt> records;
    while (std::getline(input, line))
    {
        if (line.empty())
        {
            continue;
        }
        const std::vector<std::string> fields = split(line, ',');
        require(fields.size() == 21, "Malformed attempt golden CSV row");
        GoldenAttempt record;
        record.seed = std::stoull(fields[0]);
        record.run = std::stoull(fields[1]);
        record.subarray = std::stoull(fields[2]);
        record.option = std::stoull(fields[3]);
        record.availableRows = std::stoi(fields[4]);
        record.availableColumns = std::stoi(fields[5]);
        record.attempt.faultCount = std::stoull(fields[6]);
        record.attempt.pivotFaultCount = std::stoull(fields[7]);
        record.attempt.nonpivotFaultCount = std::stoull(fields[8]);
        record.attempt.bufferedPivotFaultCount = std::stoull(fields[9]);
        record.attempt.overflowPivotFaultCount = std::stoull(fields[10]);
        record.attempt.addressCamActive = std::stoull(fields[11]);
        record.attempt.hybridCamActive = std::stoull(fields[12]);
        record.attempt.camStorageOverflow = std::stoi(fields[13]) != 0;
        record.attempt.repairSuccess = std::stoi(fields[14]) != 0;
        record.attempt.candidateSolutions = std::stoull(fields[15]);
        record.attempt.usedRows = std::stoull(fields[16]);
        record.attempt.usedColumns = std::stoull(fields[17]);
        const long long selected = std::stoll(fields[18]);
        if (selected >= 0)
        {
            record.attempt.selectedCandidateIndex =
                static_cast<std::size_t>(selected);
        }
        record.attempt.validCandidateIndices = parseIndices(fields[19]);
        record.candidateOptionSignature = fields[20];
        const auto key = std::make_tuple(
            record.run, record.subarray, record.option);
        require(records.emplace(key, record).second,
                "Duplicate attempt golden key");
    }
    return records;
}

std::map<std::size_t, GoldenGroup> loadGoldenGroups()
{
    std::ifstream input(kGroupGolden);
    require(input.is_open(), "Unable to open group golden CSV");
    std::string line;
    require(static_cast<bool>(std::getline(input, line)),
            "Group golden CSV is empty");
    const std::string expectedHeader =
        "seed,run_index,group_fault_count,baseline_config_index,"
        "baseline_group_success,legacy_shared_success,"
        "successful_config_indices";
    require(line == expectedHeader, "Unexpected group golden CSV header");

    std::map<std::size_t, GoldenGroup> records;
    while (std::getline(input, line))
    {
        if (line.empty())
        {
            continue;
        }
        const std::vector<std::string> fields = split(line, ',');
        require(fields.size() == 7, "Malformed group golden CSV row");
        GoldenGroup record;
        record.seed = std::stoull(fields[0]);
        record.run = std::stoull(fields[1]);
        record.groupFaultCount = std::stoull(fields[2]);
        record.baselineConfigIndex = std::stoi(fields[3]);
        record.baselineSuccess = std::stoi(fields[4]) != 0;
        record.legacySharedSuccess = std::stoi(fields[5]) != 0;
        record.successfulConfigIndices = parseIndices(fields[6]);
        require(records.emplace(record.run, record).second,
                "Duplicate group golden key");
    }
    return records;
}

using Config = std::pair<int, int>;
using PerSubarrayAttempts = std::array<std::array<LegacyAttempt, 4>, 4>;

const std::array<Config, 4> kLessRowConfigs{{
    {2, 3}, {1, 3}, {2, 2}, {1, 2}}};
const std::array<Config, 4> kLessColumnConfigs{{
    {3, 2}, {2, 2}, {3, 1}, {2, 1}}};

const std::array<Config, 4> &configsForSubarray(std::size_t subarray)
{
    return subarray == 0 || subarray == 3
        ? kLessRowConfigs
        : kLessColumnConfigs;
}

std::vector<std::size_t> evaluateLegacySharedLine(
    const PerSubarrayAttempts &attempts)
{
    std::vector<std::size_t> successfulConfigs;
    for (std::size_t configIndex = 0; configIndex < 256; ++configIndex)
    {
        const std::array<std::size_t, 4> options{{
            configIndex / 64,
            (configIndex / 16) % 4,
            (configIndex / 4) % 4,
            configIndex % 4}};
        ResSpareLines resources(kRows, kColumns, kSharedLines);
        bool success = true;
        for (std::size_t subarray = 0; subarray < 4; ++subarray)
        {
            const Config config =
                configsForSubarray(subarray)[options[subarray]];
            if (!attempts[subarray][options[subarray]].repairSuccess ||
                !resources.occupySpareLines(
                    static_cast<int>(subarray),
                    config.first,
                    config.second))
            {
                success = false;
                break;
            }
        }
        if (success)
        {
            successfulConfigs.push_back(configIndex);
        }
    }
    return successfulConfigs;
}

void printAttemptHeader()
{
    std::cout
        << "seed,run_index,subarray_id,option_index,available_rows,"
           "available_columns,fault_count,pivot_fault_count,"
           "nonpivot_fault_count,buffered_pivot_fault_count,"
           "overflow_pivot_fault_count,address_cam_active,hybrid_cam_active,"
           "cam_storage_overflow,repair_success,candidate_solutions,used_rows,"
           "used_columns,selected_candidate_index,valid_candidate_indices,"
           "candidate_option_signature\n";
}

void printAttempt(
    std::size_t run,
    std::size_t subarray,
    std::size_t option,
    int availableRows,
    int availableColumns,
    const LegacyAttempt &attempt)
{
    std::cout
        << kGoldenSeed << ',' << run << ',' << subarray << ',' << option << ','
        << availableRows << ',' << availableColumns << ','
        << attempt.faultCount << ',' << attempt.pivotFaultCount << ','
        << attempt.nonpivotFaultCount << ','
        << attempt.bufferedPivotFaultCount << ','
        << attempt.overflowPivotFaultCount << ','
        << attempt.addressCamActive << ',' << attempt.hybridCamActive << ','
        << (attempt.camStorageOverflow ? 1 : 0) << ','
        << (attempt.repairSuccess ? 1 : 0) << ','
        << attempt.candidateSolutions << ',' << attempt.usedRows << ','
        << attempt.usedColumns << ','
        << (attempt.selectedCandidateIndex.has_value()
                ? std::to_string(*attempt.selectedCandidateIndex)
                : "-1")
        << ',' << formatIndices(attempt.validCandidateIndices)
        << ',' << formatCandidateOptions(attempt) << '\n';
}

void printGroupHeader()
{
    std::cout
        << "seed,run_index,group_fault_count,baseline_config_index,"
           "baseline_group_success,legacy_shared_success,"
           "successful_config_indices\n";
}

void printGroup(
    std::size_t run,
    std::size_t groupFaultCount,
    bool baselineSuccess,
    const std::vector<std::size_t> &successfulConfigs)
{
    std::cout
        << kGoldenSeed << ',' << run << ',' << groupFaultCount << ','
        << kLegacyBaselineConfigIndex << ','
        << (baselineSuccess ? 1 : 0) << ','
        << (!successfulConfigs.empty() ? 1 : 0) << ','
        << formatIndices(successfulConfigs) << '\n';
}

void verifyConfigContract()
{
    dynamic_spare::SimulationConfig config;
    config.validate();
    require(config.faultCount == 20,
            "fault_count default is not the four-subarray group total");
    require(!config.hybridCamEntryWidthBits.has_value(),
            "Hybrid CAM width was implicitly derived");

    config.faultCountModel = dynamic_spare::FaultCountModel::UserDefined;
    config.userDefinedFaultCounts = {{2, 4, 5, 9}};
    config.validate();

    config.userDefinedFaultCounts = {{2, 4, 5, 8}};
    bool rejectedBadTotal = false;
    try
    {
        config.validate();
    }
    catch (const std::invalid_argument &)
    {
        rejectedBadTotal = true;
    }
    require(rejectedBadTotal,
            "User-defined counts not summing to fault_count were accepted");

    dynamic_spare::SimulationConfig globalConfig;
    globalConfig.topology = dynamic_spare::SharingTopology::GlobalPool;
    globalConfig.globalPool = dynamic_spare::GlobalPoolConfiguration{
        1, 1, 4, 4};
    globalConfig.validate();
    globalConfig.globalPool->globalRows = 5;
    bool rejectedNonConservingPool = false;
    try
    {
        globalConfig.validate();
    }
    catch (const std::invalid_argument &)
    {
        rejectedNonConservingPool = true;
    }
    require(rejectedNonConservingPool,
            "Non-conserving global spare pool was accepted");

    dynamic_spare::SimulationConfig extremePoolConfig;
    extremePoolConfig.topology = dynamic_spare::SharingTopology::GlobalPool;
    extremePoolConfig.globalPool = dynamic_spare::GlobalPoolConfiguration{
        std::numeric_limits<int>::max(),
        std::numeric_limits<int>::max(),
        std::numeric_limits<int>::max(),
        std::numeric_limits<int>::max()};
    bool rejectedExtremePool = false;
    try
    {
        extremePoolConfig.validate();
    }
    catch (const std::invalid_argument &)
    {
        rejectedExtremePool = true;
    }
    require(rejectedExtremePool,
            "Overflow-stress global spare pool was accepted");

    dynamic_spare::SimulationConfig extremeCoreConfig;
    extremeCoreConfig.spareRows = std::numeric_limits<int>::max();
    extremeCoreConfig.spareColumns = std::numeric_limits<int>::max();
    bool rejectedExtremeCore = false;
    try
    {
        extremeCoreConfig.validate();
    }
    catch (const std::invalid_argument &)
    {
        rejectedExtremeCore = true;
    }
    require(rejectedExtremeCore,
            "Capacity outside the legacy RECAM int range was accepted");

    dynamic_spare::GroupRepairResult group;
    group.groupRepairSuccess = true;
    group.successfulGroupBorrowCount = 2;
    require(!group.repairedAfterBorrowCount(1) &&
                group.repairedAfterBorrowCount(2) &&
                !group.repairedAfterBorrowCount(3),
            "Group borrow-stage fields are not derived canonically");

    dynamic_spare::RECAMSolverRequest zeroAxisRequest;
    zeroAxisRequest.subarrayId = 0;
    zeroAxisRequest.availableRows = 0;
    zeroAxisRequest.availableColumns = 0;
    bool rejectedZeroAxis = false;
    try
    {
        dynamic_spare::RECAMSolverAdapter{}.solve({}, zeroAxisRequest);
    }
    catch (const std::invalid_argument &)
    {
        rejectedZeroAxis = true;
    }
    require(rejectedZeroAxis,
            "A RECAM configuration with no row or column capacity was accepted");

    dynamic_spare::RECAMSolverRequest singleZeroAxisRequest;
    singleZeroAxisRequest.subarrayId = 0;
    singleZeroAxisRequest.availableRows = 0;
    singleZeroAxisRequest.availableColumns = 2;
    const auto singleZeroAxis =
        dynamic_spare::RECAMSolverAdapter{}.solve(
            {}, singleZeroAxisRequest);
    require(singleZeroAxis.repairSuccess &&
                singleZeroAxis.hybridCamEntriesProvisioned == 0 &&
                singleZeroAxis.candidateSolutions == 1,
            "The sharedLine=2 single-zero-axis boundary is incorrect");

    dynamic_spare::RECAMSolverRequest invalidSubarrayRequest;
    invalidSubarrayRequest.subarrayId = 4;
    invalidSubarrayRequest.availableRows = 1;
    invalidSubarrayRequest.availableColumns = 1;
    bool rejectedInvalidSubarray = false;
    try
    {
        dynamic_spare::RECAMSolverAdapter{}.solve(
            {}, invalidSubarrayRequest);
    }
    catch (const std::invalid_argument &)
    {
        rejectedInvalidSubarray = true;
    }
    require(rejectedInvalidSubarray,
            "RECAM request accepted a subarray ID outside A/B/C/D");

    Fault wrongSubarray{};
    wrongSubarray.SubarrayID = 1;
    dynamic_spare::RECAMSolverRequest identityRequest;
    identityRequest.subarrayId = 0;
    identityRequest.availableRows = 1;
    identityRequest.availableColumns = 1;
    bool rejectedWrongSubarray = false;
    try
    {
        dynamic_spare::RECAMSolverAdapter{}.solve(
            {wrongSubarray}, identityRequest);
    }
    catch (const std::invalid_argument &)
    {
        rejectedWrongSubarray = true;
    }
    require(rejectedWrongSubarray,
            "Faults from a different subarray were silently relabeled");

    Fault firstDomain{};
    firstDomain.SubarrayID = 0;
    Fault secondDomain = firstDomain;
    secondDomain.HBMID = 1;
    bool rejectedMixedDomain = false;
    try
    {
        dynamic_spare::RECAMSolverAdapter{}.solve(
            {firstDomain, secondDomain}, identityRequest);
    }
    catch (const std::invalid_argument &)
    {
        rejectedMixedDomain = true;
    }
    require(rejectedMixedDomain,
            "One RECAM attempt accepted faults from different groups");

    dynamic_spare::RECAMSolverRequest extremeProvisioningRequest;
    extremeProvisioningRequest.subarrayId = 0;
    extremeProvisioningRequest.availableRows = 1;
    extremeProvisioningRequest.availableColumns = 1;
    extremeProvisioningRequest.provisionedRows =
        std::numeric_limits<int>::max();
    extremeProvisioningRequest.provisionedColumns =
        std::numeric_limits<int>::max();
    extremeProvisioningRequest.hybridCamEntryWidthBits = 64;
    bool rejectedBitCapacityOverflow = false;
    try
    {
        dynamic_spare::RECAMSolverAdapter{}.solve(
            {}, extremeProvisioningRequest);
    }
    catch (const std::overflow_error &)
    {
        rejectedBitCapacityOverflow = true;
    }
    require(rejectedBitCapacityOverflow,
            "Overflowing provisioned Hybrid-CAM bit count was accepted");
}

} // namespace

int main(int argc, char *argv[])
{
    try
    {
        const bool dumpAttempts =
            argc == 2 && std::string(argv[1]) == "--dump-attempts";
        const bool dumpGroups =
            argc == 2 && std::string(argv[1]) == "--dump-groups";
        require(argc == 1 || dumpAttempts || dumpGroups,
                "Usage: dynamic_spare_sharing_foundation_test "
                "[--dump-attempts|--dump-groups]");

        verifyConfigContract();

        const std::uint64_t fixtureHash = fileFnv1a(kFaultFixture);
        require(
            fixtureHash == kExpectedFaultFixtureFnv1a,
            "Ordered fault fixture FNV-1a changed; actual=" +
                std::to_string(fixtureHash));
        const std::uint64_t sharedLineSourceHash = fileFnv1a("SharedLine.cpp");
        require(
            sharedLineSourceHash == kExpectedSharedLineSourceFnv1a,
            "SharedLine.cpp changed; review/recapture the legacy golden "
            "before continuing. actual=" +
                std::to_string(sharedLineSourceHash));

        FaultLoader loader(kRows, kColumns, kBufferEntries);
        require(loader.loadFaults(kFaultFixture),
                "Unable to load fixed-seed fault fixture");
        require(loader.faultLists.size() ==
                    kExpectedRunCount * dynamic_spare::kSubarrayCount,
                "Unexpected fixture subarray count");

        const auto goldenAttempts = dumpAttempts || dumpGroups
            ? std::map<GoldenAttemptKey, GoldenAttempt>{}
            : loadGoldenAttempts();
        const auto goldenGroups = dumpAttempts || dumpGroups
            ? std::map<std::size_t, GoldenGroup>{}
            : loadGoldenGroups();
        if (!dumpAttempts && !dumpGroups)
        {
            require(goldenAttempts.size() ==
                        kExpectedRunCount * dynamic_spare::kSubarrayCount * 4,
                    "Unexpected golden attempt count");
            require(goldenGroups.size() == kExpectedRunCount,
                    "Unexpected golden group count");
        }

        if (dumpAttempts)
        {
            printAttemptHeader();
        }
        if (dumpGroups)
        {
            printGroupHeader();
        }

        dynamic_spare::RECAMSolverAdapter adapter;
        for (std::size_t run = 0; run < kExpectedRunCount; ++run)
        {
            PerSubarrayAttempts attempts;
            std::size_t groupFaultCount = 0;
            std::array<LegacyAttempt, 4> baselineAttempts;

            for (std::size_t subarray = 0; subarray < 4; ++subarray)
            {
                const FaultList &loaded = loader.faultLists[run * 4 + subarray];
                groupFaultCount += loaded.PEFaults.size();
                const auto &configs = configsForSubarray(subarray);
                for (std::size_t option = 0; option < configs.size(); ++option)
                {
                    const Config config = configs[option];
                    LegacyAttempt legacy = solveLegacyDirect(
                        loaded.PEFaults, config.first, config.second);
                    attempts[subarray][option] = legacy;

                    dynamic_spare::RECAMSolverRequest request;
                    request.subarrayId = static_cast<int>(subarray);
                    request.availableRows = config.first;
                    request.availableColumns = config.second;
                    request.bufferCamEntries = kBufferEntries;
                    request.provisionedRows = config.first;
                    request.provisionedColumns = config.second;
                    request.hybridCamEntryWidthBits = 64;
                    const auto adapted = adapter.solve(
                        loaded.PEFaults, request);
                    const std::string context =
                        "run=" + std::to_string(run) +
                        " subarray=" + std::to_string(subarray) +
                        " option=" + std::to_string(option);
                    compareAdapterToLegacy(adapted, legacy, context);
                    require(adapted.hybridCamBitsActive ==
                                adapted.hybridCamEntriesActive * 64 &&
                                adapted.hybridCamBitsProvisioned ==
                                adapted.hybridCamEntriesProvisioned * 64,
                            context + ": direct Hybrid-CAM width not applied");

                    if (config == Config{kRows, kColumns})
                    {
                        baselineAttempts[subarray] = legacy;
                    }
                }
            }
            require(groupFaultCount == kExpectedGroupFaultCount,
                    "Golden fixture no longer has group fault_count=28");

            if (run == 0)
            {
                // This non-selected legacy configuration deliberately has
                // multiple valid candidates with different actual row use.
                // The adapter must retain both so the future group allocator
                // can minimize borrowing before applying candidate-index
                // tie-breaking.
                const FaultList &loaded = loader.faultLists[2];
                dynamic_spare::RECAMSolverRequest request;
                request.subarrayId = 2;
                request.availableRows = 2;
                request.availableColumns = 3;
                request.bufferCamEntries = kBufferEntries;
                const auto alternatives = adapter.solve(
                    loaded.PEFaults, request);
                const auto candidate0 = std::find_if(
                    alternatives.validCandidateOptions.begin(),
                    alternatives.validCandidateOptions.end(),
                    [](const dynamic_spare::CandidateRepairOption &option)
                    {
                        return option.candidateIndex == 0;
                    });
                const auto candidate6 = std::find_if(
                    alternatives.validCandidateOptions.begin(),
                    alternatives.validCandidateOptions.end(),
                    [](const dynamic_spare::CandidateRepairOption &option)
                    {
                        return option.candidateIndex == 6;
                    });
                require(
                    candidate0 != alternatives.validCandidateOptions.end() &&
                        candidate6 !=
                            alternatives.validCandidateOptions.end() &&
                        candidate0->usedRows == 2 &&
                        candidate0->usedColumns == 3 &&
                        candidate6->usedRows == 1 &&
                        candidate6->usedColumns == 3,
                    "Adapter did not retain borrow-relevant candidate "
                    "alternatives");
            }

            const bool baselineSuccess = std::all_of(
                baselineAttempts.begin(), baselineAttempts.end(),
                [](const LegacyAttempt &attempt)
                {
                    return attempt.repairSuccess;
                });
            const std::vector<std::size_t> successfulConfigs =
                evaluateLegacySharedLine(attempts);
            const bool baselineConfigSucceeded = std::binary_search(
                successfulConfigs.begin(), successfulConfigs.end(),
                static_cast<std::size_t>(kLegacyBaselineConfigIndex));
            require(baselineSuccess == baselineConfigSucceeded,
                    "Legacy config 150 no longer equals four local baselines");

            if (dumpAttempts)
            {
                for (std::size_t subarray = 0; subarray < 4; ++subarray)
                {
                    const auto &configs = configsForSubarray(subarray);
                    for (std::size_t option = 0;
                         option < configs.size(); ++option)
                    {
                        const Config config = configs[option];
                        printAttempt(
                            run, subarray, option,
                            config.first, config.second,
                            attempts[subarray][option]);
                    }
                }
            }
            else if (dumpGroups)
            {
                printGroup(
                    run, groupFaultCount, baselineSuccess,
                    successfulConfigs);
            }
            else
            {
                for (std::size_t subarray = 0; subarray < 4; ++subarray)
                {
                    const auto &configs = configsForSubarray(subarray);
                    for (std::size_t option = 0;
                         option < configs.size(); ++option)
                    {
                        const auto key = std::make_tuple(
                            run, subarray, option);
                        const auto found = goldenAttempts.find(key);
                        require(found != goldenAttempts.end(),
                                "Missing attempt golden record");
                        const GoldenAttempt &golden = found->second;
                        const Config config = configs[option];
                        const std::string context =
                            "run=" + std::to_string(run) +
                            " subarray=" + std::to_string(subarray) +
                            " option=" + std::to_string(option);
                        require(
                            golden.seed == kGoldenSeed &&
                                golden.availableRows == config.first &&
                                golden.availableColumns == config.second,
                            context + ": golden identity changed");
                        compareLegacyToGolden(
                            attempts[subarray][option], golden.attempt,
                            context);
                        require(
                            formatCandidateOptions(
                                attempts[subarray][option]) ==
                                golden.candidateOptionSignature,
                            context +
                                ": golden candidate option signature mismatch");
                    }
                }

                const auto foundGroup = goldenGroups.find(run);
                require(foundGroup != goldenGroups.end(),
                        "Missing group golden record");
                const GoldenGroup &golden = foundGroup->second;
                require(golden.seed == kGoldenSeed &&
                            golden.groupFaultCount == groupFaultCount &&
                            golden.baselineConfigIndex ==
                                kLegacyBaselineConfigIndex &&
                            golden.baselineSuccess == baselineSuccess &&
                            golden.legacySharedSuccess ==
                                !successfulConfigs.empty() &&
                            golden.successfulConfigIndices == successfulConfigs,
                        "Legacy SharedLine group golden mismatch at run=" +
                            std::to_string(run));
            }
        }

        if (!dumpAttempts && !dumpGroups)
        {
            std::cout
                << "DynamicSpareSharing foundation golden regression passed\n";
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "Foundation regression failed: "
                  << error.what() << '\n';
        return 1;
    }
    return 0;
}
