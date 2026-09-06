#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "inc/DynamicFaultGenerator.hpp"
#include "inc/DynamicRepairSimulator.hpp"
#include "inc/SimulationConfig.hpp"
#include "inc/SramRecamModel.hpp"
#include "inc/SramRecamSolverAdapter.hpp"

namespace
{

using dynamic_spare::FaultGroup;
using dynamic_spare::GroupRepairResult;
using dynamic_spare::RepairAttemptResult;
using dynamic_spare::SimulationConfig;
using sram_recam::AreaProxyParameters;
using sram_recam::BiraLatencyStats;
using sram_recam::HardwareCostDescriptor;
using sram_recam::MatrixPolicy;
using sram_recam::RuntimeLatencyStats;
using sram_recam::SearchPolicy;
using sram_recam::SramRecamConfig;
using sram_recam::SramRecamGeometry;

struct PolicySpec
{
    std::string label;
    SearchPolicy policy = SearchPolicy::SramSerial;
    std::uint32_t parallelism = 1;
    bool wide = false;
};

struct Options
{
    SimulationConfig dynamic;
    SramRecamConfig sram;
    AreaProxyParameters area;
    std::vector<PolicySpec> policies;
    std::filesystem::path outputDirectory =
        "reports/dynamic_spare_sharing_sram_recam";
    std::uint64_t runtimeRequests = 0;
    double runtimeHitRate = 0.0;
    std::optional<std::uint64_t> runtimeOccupiedEntries;
    bool overwrite = false;
    bool sharedLinesSpecified = false;
    bool fixedBufferSpecified = false;
    bool paperCamReuseSpecified = false;
};

struct GroupHardwareCost
{
    std::array<SramRecamConfig, dynamic_spare::kSubarrayCount> configs;
    std::array<HardwareCostDescriptor, dynamic_spare::kSubarrayCount>
        descriptors;
    double normalizedSramArea = 0.0;
    double normalizedComparatorArea = 0.0;
    double normalizedAreaProxy = 0.0;
    std::uint64_t addressPhysicalBits = 0;
    std::uint64_t hybridPhysicalBits = 0;
    std::uint64_t runtimePhysicalBits = 0;
    std::uint64_t temporaryBufferBits = 0;
    std::uint64_t matrixBits = 0;
    std::uint64_t comparatorBits = 0;
};

struct PolicyResult
{
    PolicySpec policy;
    SramRecamConfig config;
    SramRecamGeometry geometry;
    BiraLatencyStats bira;
    RuntimeLatencyStats runtime;
    GroupHardwareCost groupCost;
    std::uint64_t runtimeOccupiedEntries = 0;
    std::uint64_t successCount = 0;
    std::uint64_t attemptCount = 0;
    std::uint64_t attemptsWithSramMetrics = 0;
    std::uint64_t logicalSignature = 0;
    bool equivalent = true;
};

void printUsage(const char *program)
{
    std::cout
        << "Usage: " << program << " <Rs> <Cs> [options]\n\n"
        << "Fault experiment:\n"
        << "  --runs N --fault-count N --seed N\n"
        << "  --fault-model uniform|moderate|strong|hotspot\n"
        << "  --spatial uniform|mixed|clustered\n"
        << "  --memory-rows N --memory-columns N\n\n"
        << "Dynamic spare sharing:\n"
        << "  --topology none|directional|global|edge\n"
        << "  --shared-lines N --local-first --max-borrows N\n"
        << "  --solution-take legacy|early|group (default: legacy)\n"
        << "  --buffer N | --paper-cam-reuse\n\n"
        << "SRAM_RECAM architecture:\n"
        << "  --policies serial,chunk2,chunk4,wide,chunked:N\n"
        << "  --matrix-policy scan|shadow (default: shadow)\n"
        << "      shadow: strict occupied scan; matrix/analyzer latency ignored\n"
        << "  --registered-search       Model SRAM read and compare separately\n"
        << "  --analysis-per-fault      Rebuild/analyze after every fault\n"
        << "  --channels N --word-bits N\n"
        << "  --normal-dram-latency N\n"
        << "  --runtime-requests N --runtime-hit-rate FRACTION\n"
        << "  --runtime-occupied N\n"
        << "  --area-proxy-coefficients SRAM,COMPARATOR,CONTROLLER\n"
        << "  --sram-bit-area-units F\n"
        << "  --comparator-bit-area-units F\n"
        << "  --fixed-controller-area-units F\n\n"
        << "Output:\n"
        << "  --output-dir PATH --overwrite\n";
}

long long parseSigned(const std::string &text, const std::string &option)
{
    std::size_t parsed = 0;
    long long value = 0;
    try
    {
        value = std::stoll(text, &parsed);
    }
    catch (const std::exception &)
    {
        throw std::invalid_argument(option + " requires an integer");
    }
    if (parsed != text.size())
    {
        throw std::invalid_argument(option + " requires an integer");
    }
    return value;
}

int parseInt(const std::string &text, const std::string &option)
{
    const long long value = parseSigned(text, option);
    if (value < std::numeric_limits<int>::min() ||
        value > std::numeric_limits<int>::max())
    {
        throw std::invalid_argument(option + " is outside the int range");
    }
    return static_cast<int>(value);
}

std::uint64_t parseUint64(
    const std::string &text,
    const std::string &option)
{
    if (!text.empty() && text.front() == '-')
    {
        throw std::invalid_argument(option + " cannot be negative");
    }
    std::size_t parsed = 0;
    unsigned long long value = 0;
    try
    {
        value = std::stoull(text, &parsed);
    }
    catch (const std::exception &)
    {
        throw std::invalid_argument(option + " requires an unsigned integer");
    }
    if (parsed != text.size())
    {
        throw std::invalid_argument(option + " requires an unsigned integer");
    }
    return static_cast<std::uint64_t>(value);
}

std::uint32_t parseUint32(
    const std::string &text,
    const std::string &option,
    bool allowZero = false)
{
    const std::uint64_t value = parseUint64(text, option);
    if ((!allowZero && value == 0) ||
        value > std::numeric_limits<std::uint32_t>::max())
    {
        throw std::invalid_argument(option + " is outside its uint32 range");
    }
    return static_cast<std::uint32_t>(value);
}

double parseDouble(const std::string &text, const std::string &option)
{
    std::size_t parsed = 0;
    double value = 0.0;
    try
    {
        value = std::stod(text, &parsed);
    }
    catch (const std::exception &)
    {
        throw std::invalid_argument(option + " requires a number");
    }
    if (parsed != text.size() || !std::isfinite(value))
    {
        throw std::invalid_argument(option + " requires a finite number");
    }
    return value;
}

std::vector<std::string> splitCommaList(
    const std::string &text,
    const std::string &option)
{
    std::vector<std::string> fields;
    std::size_t begin = 0;
    while (begin <= text.size())
    {
        const std::size_t comma = text.find(',', begin);
        const std::string field = text.substr(
            begin,
            comma == std::string::npos ? std::string::npos : comma - begin);
        if (field.empty())
        {
            throw std::invalid_argument(option + " contains an empty field");
        }
        fields.push_back(field);
        if (comma == std::string::npos)
        {
            break;
        }
        begin = comma + 1;
    }
    return fields;
}

dynamic_spare::SharingTopology parseTopology(const std::string &value)
{
    if (value == "none" || value == "no-sharing")
        return dynamic_spare::SharingTopology::NoSharing;
    if (value == "directional")
        return dynamic_spare::SharingTopology::Directional;
    if (value == "global")
        return dynamic_spare::SharingTopology::GlobalPool;
    if (value == "edge")
        return dynamic_spare::SharingTopology::PairwiseEdge;
    throw std::invalid_argument("Unknown topology: " + value);
}

dynamic_spare::SolutionTakePolicy parseSolutionTake(
    const std::string &value)
{
    if (value == "legacy")
        return dynamic_spare::SolutionTakePolicy::Legacy;
    if (value == "early")
        return dynamic_spare::SolutionTakePolicy::Early;
    if (value == "group" || value == "group_compressed")
        return dynamic_spare::SolutionTakePolicy::GroupCompressed;
    throw std::invalid_argument("Unknown solution-take policy: " + value);
}

dynamic_spare::FaultCountModel parseFaultModel(const std::string &value)
{
    if (value == "uniform")
        return dynamic_spare::FaultCountModel::Uniform;
    if (value == "moderate")
        return dynamic_spare::FaultCountModel::ModerateImbalance;
    if (value == "strong")
        return dynamic_spare::FaultCountModel::StrongImbalance;
    if (value == "hotspot")
        return dynamic_spare::FaultCountModel::Hotspot;
    throw std::invalid_argument("Unknown fault model: " + value);
}

dynamic_spare::FaultSpatialModel parseSpatialModel(
    const std::string &value)
{
    if (value == "uniform")
        return dynamic_spare::FaultSpatialModel::Uniform;
    if (value == "mixed")
        return dynamic_spare::FaultSpatialModel::Mixed;
    if (value == "clustered")
        return dynamic_spare::FaultSpatialModel::Clustered;
    throw std::invalid_argument("Unknown spatial model: " + value);
}

PolicySpec parsePolicy(const std::string &value)
{
    if (value == "serial")
    {
        return {"serial", SearchPolicy::SramSerial, 1, false};
    }
    if (value == "wide")
    {
        return {"wide", SearchPolicy::SramWide,
                std::numeric_limits<std::uint32_t>::max(), true};
    }
    static const std::string chunkedPrefix = "chunked:";
    if (value.size() > chunkedPrefix.size() &&
        value.substr(0, chunkedPrefix.size()) == chunkedPrefix)
    {
        const std::uint32_t parallelism = parseUint32(
            value.substr(chunkedPrefix.size()),
            "chunked policy parallelism");
        return {"chunk" + std::to_string(parallelism),
                SearchPolicy::SramChunked, parallelism, false};
    }
    if (value.size() > 5 && value.substr(0, 5) == "chunk")
    {
        const std::uint32_t parallelism = parseUint32(
            value.substr(5), "chunk policy parallelism");
        return {"chunk" + std::to_string(parallelism),
                SearchPolicy::SramChunked, parallelism, false};
    }
    throw std::invalid_argument("Unknown SRAM_RECAM policy: " + value);
}

std::vector<PolicySpec> parsePolicies(const std::string &value)
{
    std::vector<PolicySpec> policies;
    std::set<std::string> labels;
    for (const std::string &field : splitCommaList(value, "--policies"))
    {
        PolicySpec policy = parsePolicy(field);
        if (!labels.insert(policy.label).second)
        {
            throw std::invalid_argument(
                "Duplicate SRAM_RECAM policy: " + field);
        }
        policies.push_back(std::move(policy));
    }
    return policies;
}

std::uint32_t addressWidthForLegacyConfig(std::uint32_t entries)
{
    std::uint32_t bits = 0;
    std::uint64_t represented = 1;
    while (represented < entries)
    {
        represented <<= 1;
        ++bits;
    }
    return std::max<std::uint32_t>(1, bits);
}

Options parseOptions(int argc, char *argv[])
{
    if (argc < 3)
    {
        throw std::invalid_argument("Rs and Cs are required");
    }

    Options options;
    options.dynamic.spareRows = parseInt(argv[1], "Rs");
    options.dynamic.spareColumns = parseInt(argv[2], "Cs");
    options.dynamic.storageMode =
        dynamic_spare::FaultInformationStorage::CAM;

    for (int index = 3; index < argc; ++index)
    {
        const std::string option = argv[index];
        const auto requireValue = [&]() -> std::string
        {
            if (index + 1 >= argc)
            {
                throw std::invalid_argument(option + " requires a value");
            }
            return argv[++index];
        };

        if (option == "--runs")
            options.dynamic.simulationRuns = parseUint64(requireValue(), option);
        else if (option == "--fault-count")
            options.dynamic.faultCount = parseUint64(requireValue(), option);
        else if (option == "--seed")
            options.dynamic.randomSeed = parseUint64(requireValue(), option);
        else if (option == "--fault-model")
            options.dynamic.faultCountModel = parseFaultModel(requireValue());
        else if (option == "--spatial")
            options.dynamic.faultSpatialModel = parseSpatialModel(requireValue());
        else if (option == "--memory-rows")
        {
            options.dynamic.memoryRows = parseUint32(requireValue(), option);
        }
        else if (option == "--memory-columns")
        {
            options.dynamic.memoryColumns = parseUint32(requireValue(), option);
        }
        else if (option == "--topology")
            options.dynamic.topology = parseTopology(requireValue());
        else if (option == "--shared-lines")
        {
            const int value = parseInt(requireValue(), option);
            options.dynamic.sharedRows = value;
            options.dynamic.sharedColumns = value;
            options.sharedLinesSpecified = true;
        }
        else if (option == "--local-first")
            options.dynamic.modifiers.localFirst = true;
        else if (option == "--max-borrows")
            options.dynamic.modifiers.maximumGroupBorrowedSpares =
                parseInt(requireValue(), option);
        else if (option == "--solution-take")
            options.dynamic.solutionTakePolicy =
                parseSolutionTake(requireValue());
        else if (option == "--buffer")
        {
            if (options.paperCamReuseSpecified)
            {
                throw std::invalid_argument(
                    "--buffer and --paper-cam-reuse are mutually exclusive");
            }
            options.dynamic.bufferCamEntries = parseInt(requireValue(), option);
            options.dynamic.usePaperCamReuseCapacity = false;
            options.fixedBufferSpecified = true;
        }
        else if (option == "--paper-cam-reuse")
        {
            if (options.fixedBufferSpecified)
            {
                throw std::invalid_argument(
                    "--buffer and --paper-cam-reuse are mutually exclusive");
            }
            options.dynamic.usePaperCamReuseCapacity = true;
            options.paperCamReuseSpecified = true;
        }
        else if (option == "--policies")
            options.policies = parsePolicies(requireValue());
        else if (option == "--matrix-policy")
        {
            const std::string value = requireValue();
            if (value == "scan")
                options.sram.matrixPolicy = MatrixPolicy::SramScanRebuild;
            else if (value == "shadow")
                options.sram.matrixPolicy = MatrixPolicy::ShadowMatrix;
            else
                throw std::invalid_argument("Unknown matrix policy: " + value);
        }
        else if (option == "--registered-search")
            options.sram.useAbstractSearchRoundModel = false;
        else if (option == "--analysis-per-fault")
            options.sram.analyzeAfterEachFault = true;
        else if (option == "--channels")
            options.sram.channels = parseUint32(requireValue(), option);
        else if (option == "--word-bits")
            options.sram.dataWordBits = parseUint32(requireValue(), option);
        else if (option == "--normal-dram-latency")
            options.sram.normalDramReadyCycles =
                parseUint32(requireValue(), option, true);
        else if (option == "--runtime-requests")
            options.runtimeRequests = parseUint64(requireValue(), option);
        else if (option == "--runtime-hit-rate")
            options.runtimeHitRate = parseDouble(requireValue(), option);
        else if (option == "--runtime-occupied")
            options.runtimeOccupiedEntries = parseUint64(requireValue(), option);
        else if (option == "--area-proxy-coefficients")
        {
            const auto fields = splitCommaList(requireValue(), option);
            if (fields.size() != 3)
            {
                throw std::invalid_argument(
                    option + " requires SRAM,COMPARATOR,CONTROLLER");
            }
            options.area.sramBitAreaUnits = parseDouble(fields[0], option);
            options.area.comparatorBitAreaUnits = parseDouble(fields[1], option);
            options.area.fixedControllerAreaUnits = parseDouble(fields[2], option);
        }
        else if (option == "--sram-bit-area-units" ||
                 option == "--sram-bit-area")
            options.area.sramBitAreaUnits = parseDouble(requireValue(), option);
        else if (option == "--comparator-bit-area-units" ||
                 option == "--comparator-bit-area")
            options.area.comparatorBitAreaUnits =
                parseDouble(requireValue(), option);
        else if (option == "--fixed-controller-area-units" ||
                 option == "--controller-area")
            options.area.fixedControllerAreaUnits =
                parseDouble(requireValue(), option);
        else if (option == "--output-dir")
            options.outputDirectory = requireValue();
        else if (option == "--overwrite")
            options.overwrite = true;
        else if (option == "--help" || option == "-h")
        {
            printUsage(argv[0]);
            std::exit(0);
        }
        else
            throw std::invalid_argument("Unknown option: " + option);
    }

    if (options.policies.empty())
    {
        options.policies = parsePolicies("serial,chunk2,chunk4,wide");
    }
    if (options.runtimeHitRate < 0.0 || options.runtimeHitRate > 1.0)
    {
        throw std::invalid_argument(
            "--runtime-hit-rate must be within [0,1]");
    }
    if (options.outputDirectory.empty())
    {
        throw std::invalid_argument("--output-dir cannot be empty");
    }
    if (options.dynamic.simulationRuns >
        static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
    {
        throw std::invalid_argument("--runs exceeds the platform size range");
    }
    if (options.dynamic.topology ==
            dynamic_spare::SharingTopology::NoSharing &&
        options.sharedLinesSpecified &&
        (options.dynamic.sharedRows != 0 ||
         options.dynamic.sharedColumns != 0))
    {
        throw std::invalid_argument(
            "No-sharing topology requires --shared-lines 0");
    }

    if (options.dynamic.topology ==
        dynamic_spare::SharingTopology::GlobalPool)
    {
        if (options.dynamic.sharedRows < 0 ||
            options.dynamic.sharedRows > options.dynamic.spareRows ||
            options.dynamic.sharedColumns < 0 ||
            options.dynamic.sharedColumns > options.dynamic.spareColumns)
        {
            throw std::invalid_argument(
                "Global shared capacity must be within Rs/Cs");
        }
        const int subarrays = static_cast<int>(dynamic_spare::kSubarrayCount);
        options.dynamic.globalPool = dynamic_spare::GlobalPoolConfiguration{
            options.dynamic.spareRows - options.dynamic.sharedRows,
            options.dynamic.spareColumns - options.dynamic.sharedColumns,
            subarrays * options.dynamic.sharedRows,
            subarrays * options.dynamic.sharedColumns};
    }
    else
    {
        options.dynamic.globalPool.reset();
    }

    options.dynamic.dataWidthBits = options.sram.dataWordBits;
    options.dynamic.rowAddressWidthBits =
        addressWidthForLegacyConfig(options.dynamic.memoryRows);
    options.dynamic.columnAddressWidthBits =
        addressWidthForLegacyConfig(options.dynamic.memoryColumns);
    options.dynamic.validate();

    options.sram.rows = options.dynamic.memoryRows;
    options.sram.cols = options.dynamic.memoryColumns;
    options.sram.spareRows = static_cast<std::uint32_t>(
        options.dynamic.spareRows);
    options.sram.spareCols = static_cast<std::uint32_t>(
        options.dynamic.spareColumns);
    const int bufferEntries = options.dynamic.usePaperCamReuseCapacity
        ? options.dynamic.spareRows + options.dynamic.spareColumns
        : options.dynamic.bufferCamEntries;
    if (bufferEntries < 0)
    {
        throw std::invalid_argument("Buffer capacity cannot be negative");
    }
    options.sram.camReuseEntries = static_cast<std::uint32_t>(bufferEntries);
    options.sram.validate();

    // Validate area coefficients through the public model API.
    (void)sram_recam::deriveHardwareCost(options.sram, options.area);
    return options;
}

[[noreturn]] void equivalenceFailure(
    std::size_t run,
    std::optional<std::size_t> subarray,
    std::optional<std::size_t> attempt,
    const std::string &field)
{
    std::ostringstream message;
    message << "SRAM_RECAM equivalence failure at run " << run;
    if (subarray.has_value())
        message << ", PE " << *subarray;
    if (attempt.has_value())
        message << ", attempt " << *attempt;
    message << ": " << field;
    throw std::logic_error(message.str());
}

void compareAttempts(
    const RepairAttemptResult &cam,
    const RepairAttemptResult &sram,
    std::size_t run,
    std::size_t subarray,
    std::size_t attempt)
{
#define CHECK_ATTEMPT(field) \
    if (cam.field != sram.field) \
        equivalenceFailure(run, subarray, attempt, #field)
    CHECK_ATTEMPT(runIndex);
    CHECK_ATTEMPT(attemptIndex);
    CHECK_ATTEMPT(stage);
    CHECK_ATTEMPT(subarrayId);
    CHECK_ATTEMPT(availableRows);
    CHECK_ATTEMPT(availableColumns);
    CHECK_ATTEMPT(provisionedRows);
    CHECK_ATTEMPT(provisionedColumns);
    CHECK_ATTEMPT(isRepairable);
    CHECK_ATTEMPT(repairSuccess);
    CHECK_ATTEMPT(camStorageOverflow);
    CHECK_ATTEMPT(faultCount);
    CHECK_ATTEMPT(pivotFaultCount);
    CHECK_ATTEMPT(nonpivotFaultCount);
    CHECK_ATTEMPT(bufferedPivotFaultCount);
    CHECK_ATTEMPT(overflowPivotFaultCount);
    CHECK_ATTEMPT(usedRows);
    CHECK_ATTEMPT(usedColumns);
    CHECK_ATTEMPT(candidateSolutions);
    CHECK_ATTEMPT(candidateSolutionsEvaluated);
    CHECK_ATTEMPT(failedCandidates);
    CHECK_ATTEMPT(successfulCandidateIndex);
    CHECK_ATTEMPT(validCandidateIndices);
#undef CHECK_ATTEMPT

    if ((sram.availableRows != 0 || sram.availableColumns != 0) &&
        !sram.sramRecam.has_value())
    {
        equivalenceFailure(
            run, subarray, attempt, "missing SRAM_RECAM attempt metrics");
    }
    if (sram.sramRecam.has_value() &&
        !sram.sramRecam->addressClassificationMatchesCam)
    {
        equivalenceFailure(
            run, subarray, attempt,
            "Address-SRAM classification does not match CAM");
    }
}

void compareGroups(
    const GroupRepairResult &cam,
    const GroupRepairResult &sram,
    std::size_t run)
{
#define CHECK_GROUP(field) \
    if (cam.field != sram.field) \
        equivalenceFailure(run, std::nullopt, std::nullopt, #field)
    CHECK_GROUP(groupRepairSuccess);
    CHECK_GROUP(localRepairSuccess);
    CHECK_GROUP(repairSuccess);
    CHECK_GROUP(selectedAttemptIndices);
    CHECK_GROUP(selectedCandidateIndices);
    CHECK_GROUP(successfulGroupBorrowCount);
    CHECK_GROUP(usedRows);
    CHECK_GROUP(usedColumns);
    CHECK_GROUP(unusedPhysicalRows);
    CHECK_GROUP(unusedPhysicalColumns);
#undef CHECK_GROUP

    for (std::size_t subarray = 0;
         subarray < dynamic_spare::kSubarrayCount; ++subarray)
    {
        if (cam.attemptsBySubarray[subarray].size() !=
            sram.attemptsBySubarray[subarray].size())
        {
            equivalenceFailure(
                run, subarray, std::nullopt, "attempt count");
        }
        for (std::size_t attempt = 0;
             attempt < cam.attemptsBySubarray[subarray].size(); ++attempt)
        {
            compareAttempts(
                cam.attemptsBySubarray[subarray][attempt],
                sram.attemptsBySubarray[subarray][attempt],
                run,
                subarray,
                attempt);
        }
    }
}

void discardLargeSnapshots(GroupRepairResult &group)
{
    for (auto &attempts : group.attemptsBySubarray)
    {
        for (RepairAttemptResult &attempt : attempts)
        {
            std::vector<dynamic_spare::RepairLineMapping>{}.swap(
                attempt.selectedMappings);
            std::vector<dynamic_spare::CandidateRepairOption>{}.swap(
                attempt.validCandidateOptions);
        }
    }
}

void signatureMix(std::uint64_t &signature, std::uint64_t value)
{
    constexpr std::uint64_t kPrime = 1099511628211ULL;
    for (unsigned int byte = 0; byte < 8; ++byte)
    {
        signature ^= (value >> (byte * 8)) & 0xffU;
        signature *= kPrime;
    }
}

void signatureMixOptional(
    std::uint64_t &signature,
    const std::optional<std::size_t> &value)
{
    signatureMix(signature, value.has_value() ? 1 : 0);
    signatureMix(signature, value.value_or(0));
}

std::uint64_t logicalSignature(
    const std::vector<GroupRepairResult> &groups)
{
    std::uint64_t signature = 1469598103934665603ULL;
    for (const GroupRepairResult &group : groups)
    {
        signatureMix(signature, group.runIndex);
        signatureMix(signature, group.groupRepairSuccess);
        for (std::size_t subarray = 0;
             subarray < dynamic_spare::kSubarrayCount; ++subarray)
        {
            signatureMix(signature, group.localRepairSuccess[subarray]);
            signatureMix(signature, group.repairSuccess[subarray]);
            signatureMixOptional(
                signature, group.selectedAttemptIndices[subarray]);
            signatureMixOptional(
                signature, group.selectedCandidateIndices[subarray]);
            const auto &attempts = group.attemptsBySubarray[subarray];
            signatureMix(signature, attempts.size());
            for (const RepairAttemptResult &attempt : attempts)
            {
                signatureMix(signature, attempt.availableRows);
                signatureMix(signature, attempt.availableColumns);
                signatureMix(signature, attempt.repairSuccess);
                signatureMix(signature, attempt.camStorageOverflow);
                signatureMix(signature, attempt.validCandidateIndices.size());
                for (const std::size_t index : attempt.validCandidateIndices)
                    signatureMix(signature, index);
            }
        }
    }
    return signature;
}

std::uint64_t successCount(const std::vector<GroupRepairResult> &groups)
{
    return static_cast<std::uint64_t>(std::count_if(
        groups.begin(), groups.end(),
        [](const GroupRepairResult &group)
        {
            return group.groupRepairSuccess;
        }));
}

std::uint64_t roundedHitCount(std::uint64_t requests, double hitRate)
{
    const long double exact =
        static_cast<long double>(requests) * hitRate;
    if (exact > static_cast<long double>(
            std::numeric_limits<std::uint64_t>::max()))
    {
        throw std::overflow_error("Runtime hit count exceeds uint64 range");
    }
    const std::uint64_t hits = static_cast<std::uint64_t>(
        std::floor(exact + 0.5L));
    return std::min(hits, requests);
}

SramRecamConfig configForPolicy(
    const SramRecamConfig &base,
    const PolicySpec &policy)
{
    SramRecamConfig config = base;
    config.biraSearchPolicy = policy.policy;
    config.runtimeSearchPolicy = policy.policy;
    config.addressParallelism = policy.parallelism;
    config.hybridReadParallelism = policy.parallelism;
    config.runtimeParallelism = policy.parallelism;
    config.validate();
    return config;
}

std::array<std::pair<std::uint32_t, std::uint32_t>,
           dynamic_spare::kSubarrayCount>
provisionedCapacities(const GroupRepairResult &group)
{
    std::array<std::pair<std::uint32_t, std::uint32_t>,
               dynamic_spare::kSubarrayCount> capacities;
    for (std::size_t subarray = 0;
         subarray < dynamic_spare::kSubarrayCount; ++subarray)
    {
        const auto &attempts = group.attemptsBySubarray[subarray];
        if (attempts.empty())
        {
            throw std::logic_error("CAM baseline contains no PE attempt");
        }
        int rows = 0;
        int columns = 0;
        for (const RepairAttemptResult &attempt : attempts)
        {
            rows = std::max(rows, attempt.provisionedRows);
            columns = std::max(columns, attempt.provisionedColumns);
        }
        if (rows < 0 || columns < 0 || (rows == 0 && columns == 0))
        {
            throw std::logic_error(
                "Invalid provisioned capacity in CAM baseline");
        }
        capacities[subarray] = {
            static_cast<std::uint32_t>(rows),
            static_cast<std::uint32_t>(columns)};
    }
    return capacities;
}

std::uint64_t checkedAdd(
    std::uint64_t left,
    std::uint64_t right,
    const char *description)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error(description);
    return left + right;
}

GroupHardwareCost deriveGroupHardwareCost(
    const SramRecamConfig &policyConfig,
    const AreaProxyParameters &area,
    const std::array<std::pair<std::uint32_t, std::uint32_t>,
                     dynamic_spare::kSubarrayCount> &capacities)
{
    GroupHardwareCost result;
    for (std::size_t subarray = 0;
         subarray < dynamic_spare::kSubarrayCount; ++subarray)
    {
        SramRecamConfig config = policyConfig;
        config.spareRows = capacities[subarray].first;
        config.spareCols = capacities[subarray].second;
        config.validate();
        result.configs[subarray] = config;
        result.descriptors[subarray] =
            sram_recam::deriveHardwareCost(config, area);
        const HardwareCostDescriptor &descriptor =
            result.descriptors[subarray];
        const SramRecamGeometry &geometry = descriptor.geometry;
        result.normalizedSramArea += descriptor.normalizedSramArea;
        result.normalizedComparatorArea +=
            descriptor.normalizedComparatorArea;
        result.normalizedAreaProxy += descriptor.normalizedAreaProxy;
        result.addressPhysicalBits = checkedAdd(
            result.addressPhysicalBits,
            geometry.addressSram.physicalBits,
            "Four-PE Address-SRAM bit total overflow");
        result.hybridPhysicalBits = checkedAdd(
            result.hybridPhysicalBits,
            geometry.hybridSram.physicalBits,
            "Four-PE Hybrid-SRAM bit total overflow");
        result.runtimePhysicalBits = checkedAdd(
            result.runtimePhysicalBits,
            geometry.runtimeSram.physicalBits,
            "Four-PE Runtime-SRAM bit total overflow");
        result.temporaryBufferBits = checkedAdd(
            result.temporaryBufferBits,
            geometry.tempBufferBits,
            "Four-PE temporary-buffer bit total overflow");
        result.matrixBits = checkedAdd(
            result.matrixBits,
            geometry.matrixBits,
            "Four-PE matrix bit total overflow");
        result.comparatorBits = checkedAdd(
            result.comparatorBits,
            geometry.comparatorBitCount,
            "Four-PE comparator bit total overflow");
    }
    return result;
}

std::string signatureText(std::uint64_t signature)
{
    std::ostringstream output;
    output << std::hex << std::setw(16) << std::setfill('0') << signature;
    return output.str();
}

void ensureOutputTargetsAvailable(
    const Options &options,
    const std::vector<PolicyResult> &results)
{
    std::vector<std::filesystem::path> targets{
        options.outputDirectory / "sram_recam_metrics.csv"};
    for (const PolicyResult &result : results)
    {
        targets.push_back(options.outputDirectory /
            ("hardware_cost_" + result.policy.label + ".json"));
    }
    if (!options.overwrite)
    {
        for (const auto &target : targets)
        {
            if (std::filesystem::exists(target))
            {
                throw std::invalid_argument(
                    "Output already exists; use --overwrite: " +
                    target.string());
            }
        }
    }
    std::filesystem::create_directories(options.outputDirectory);
}

void writeGroupHardwareJson(
    const std::filesystem::path &path,
    const PolicyResult &result,
    const AreaProxyParameters &parameters)
{
    std::ofstream output(path, std::ios::trunc);
    if (!output)
    {
        throw std::runtime_error(
            "Unable to write hardware-cost JSON: " + path.string());
    }
    output << std::setprecision(12)
        << "{\n"
        << "  \"architecture\": \"SRAM_RECAM_dynamic_spare_sharing\",\n"
        << "  \"policy\": \"" << result.policy.label << "\",\n"
        << "  \"search_policy\": \""
        << sram_recam::toString(result.config.biraSearchPolicy) << "\",\n"
        << "  \"matrix_policy\": \""
        << sram_recam::toString(result.config.matrixPolicy) << "\",\n"
        << "  \"address_search_scope\": \"occupied_entries\",\n"
        << "  \"address_match_policy\": \"strict_row_and_column\",\n"
        << "  \"empty_search_cycles\": 0,\n"
        << "  \"matrix_latency_mode\": \""
        << (result.config.matrixPolicy == MatrixPolicy::ShadowMatrix
                ? "ignored" : "modeled_scan") << "\",\n"
        << "  \"analyzer_latency_mode\": \""
        << (result.config.matrixPolicy == MatrixPolicy::ShadowMatrix
                ? "ignored" : "modeled") << "\",\n"
        << "  \"area_proxy_is_physical_area\": false,\n"
        << "  \"coefficients\": {\"sram_bit\": "
        << parameters.sramBitAreaUnits
        << ", \"comparator_bit\": "
        << parameters.comparatorBitAreaUnits
        << ", \"controller_per_pe\": "
        << parameters.fixedControllerAreaUnits << "},\n"
        << "  \"four_pe_group\": {\n"
        << "    \"address_physical_bits\": "
        << result.groupCost.addressPhysicalBits << ",\n"
        << "    \"hybrid_physical_bits\": "
        << result.groupCost.hybridPhysicalBits << ",\n"
        << "    \"runtime_physical_bits\": "
        << result.groupCost.runtimePhysicalBits << ",\n"
        << "    \"temporary_buffer_bits\": "
        << result.groupCost.temporaryBufferBits << ",\n"
        << "    \"matrix_bits\": " << result.groupCost.matrixBits << ",\n"
        << "    \"comparator_bits\": "
        << result.groupCost.comparatorBits << ",\n"
        << "    \"normalized_sram_area\": "
        << result.groupCost.normalizedSramArea << ",\n"
        << "    \"normalized_comparator_area\": "
        << result.groupCost.normalizedComparatorArea << ",\n"
        << "    \"normalized_total_area_proxy\": "
        << result.groupCost.normalizedAreaProxy << "\n"
        << "  },\n"
        << "  \"processing_elements\": [\n";
    for (std::size_t subarray = 0;
         subarray < dynamic_spare::kSubarrayCount; ++subarray)
    {
        const SramRecamConfig &config = result.groupCost.configs[subarray];
        const HardwareCostDescriptor &descriptor =
            result.groupCost.descriptors[subarray];
        const SramRecamGeometry &g = descriptor.geometry;
        output
            << "    {\"pe\": " << subarray
            << ", \"Rs_provisioned\": " << config.spareRows
            << ", \"Cs_provisioned\": " << config.spareCols
            << ", \"address_sram\": {\"depth\": "
            << g.addressSram.physicalDepth << ", \"width\": "
            << g.addressSram.physicalWidth << ", \"bits\": "
            << g.addressSram.physicalBits << "}"
            << ", \"hybrid_sram\": {\"depth\": "
            << g.hybridSram.physicalDepth << ", \"width\": "
            << g.hybridSram.physicalWidth << ", \"bits\": "
            << g.hybridSram.physicalBits << "}"
            << ", \"runtime_sram\": {\"depth\": "
            << g.runtimeSram.physicalDepth << ", \"width\": "
            << g.runtimeSram.physicalWidth << ", \"bits\": "
            << g.runtimeSram.physicalBits << "}"
            << ", \"row_comparators\": " << g.rowComparatorCount
            << ", \"column_comparators\": "
            << g.columnComparatorCount
            << ", \"runtime_comparators\": "
            << g.runtimeComparatorCount
            << ", \"normalized_area_proxy\": "
            << descriptor.normalizedAreaProxy << "}"
            << (subarray + 1 == dynamic_spare::kSubarrayCount ? "\n" : ",\n");
    }
    output << "  ]\n}\n";
    if (!output)
    {
        throw std::runtime_error(
            "Failed while writing hardware-cost JSON: " + path.string());
    }
}

void writeMetricsCsv(
    const std::filesystem::path &path,
    const Options &options,
    const std::vector<PolicyResult> &results,
    std::uint64_t baselineSuccesses,
    std::uint64_t baselineSignature)
{
    std::ofstream output(path, std::ios::trunc);
    if (!output)
    {
        throw std::runtime_error("Unable to write metrics CSV: " + path.string());
    }
    output << std::setprecision(12);
    output
        << "architecture,policy,search_policy,matrix_policy,search_timing_model,"
           "address_search_scope,address_match_policy,empty_search_cycles,"
           "matrix_latency_mode,analyzer_latency_mode,"
           "analyze_after_each_fault,M,N,Rs,Cs,channels,word_bits,"
           "topology,solution_take_policy,shared_lines,max_group_borrows,runs,fault_count,seed,"
           "buffer_entries,address_entries,hybrid_entries,runtime_entries,"
           "address_entry_bits,hybrid_entry_bits,temp_entry_bits,runtime_entry_bits,"
           "address_logical_bits,hybrid_logical_bits,temp_buffer_bits,"
           "total_storage_bits,runtime_compact_bits,matrix_bits,solution_units,"
           "address_physical_depth,address_physical_width,address_physical_bits,"
           "address_padded_bits,hybrid_physical_depth,hybrid_physical_width,"
           "hybrid_physical_bits,hybrid_padded_bits,runtime_physical_depth,"
           "runtime_physical_width,runtime_physical_bits,runtime_padded_bits,"
           "P_A,P_H,P_R,P_SOL,row_comparator_count,column_comparator_count,"
           "runtime_comparator_count,comparator_bit_count,analysis_attempts,"
           "attempts_with_sram_metrics,faults_detected,address_entry_count,"
           "hybrid_entry_count,temp_entry_count,address_search_rounds_total,"
           "address_search_rounds_per_fault,address_search_cycles_total,"
           "address_search_cycles_avg,address_search_cycles_max,"
           "address_sram_reads,address_writes,"
           "hybrid_writes,temp_buffer_writes,matrix_address_read_rounds,"
           "matrix_hybrid_read_rounds,analysis_rounds,comparator_evaluations,"
           "comparator_bit_comparisons,early_termination_hit_count,"
           "hybrid_bitmap_allocations,hybrid_full_count,pivot_faults,"
           "pivot_search_cycles_total,pivot_search_cycles_avg,nonpivot_faults,"
           "nonpivot_search_cycles_total,nonpivot_search_cycles_avg,"
           "must_repair_triggering_faults,must_repair_search_cycles_total,"
           "must_repair_search_cycles_avg,bira_fault_collection_cycles,"
           "fault_collection_cycles_total,fault_collection_cycles_avg,"
           "bira_repair_analysis_cycles,bira_cycles_total,bira_cycles_per_fault,"
           "runtime_requests,runtime_hits,runtime_misses,runtime_occupied_entries,"
           "runtime_lookup_rounds_total,runtime_lookup_rounds_avg,"
           "runtime_hit_rounds_avg,runtime_miss_rounds_avg,"
           "runtime_extra_latency_total,runtime_extra_latency_avg,"
           "repair_successes,cam_baseline_successes,repair_rate,"
           "cam_baseline_repair_rate,repair_result_matches_cam_baseline,"
           "cam_baseline_signature,sram_logical_signature,"
           "group_address_physical_bits,group_hybrid_physical_bits,"
           "group_runtime_physical_bits,group_temp_buffer_bits,group_matrix_bits,"
           "group_comparator_bits,sram_bit_area_units,comparator_bit_area_units,"
           "fixed_controller_area_units,group_normalized_sram_area,"
           "group_normalized_comparator_area,group_normalized_total_area_proxy\n";

    const double baselineRate = options.dynamic.simulationRuns == 0
        ? 0.0
        : static_cast<double>(baselineSuccesses) /
              static_cast<double>(options.dynamic.simulationRuns);
    for (const PolicyResult &result : results)
    {
        const SramRecamGeometry &g = result.geometry;
        const BiraLatencyStats &b = result.bira;
        const RuntimeLatencyStats &runtime = result.runtime;
        const double repairRate = options.dynamic.simulationRuns == 0
            ? 0.0
            : static_cast<double>(result.successCount) /
                  static_cast<double>(options.dynamic.simulationRuns);
        const double searchRoundsPerFault = b.faultsDetected == 0
            ? 0.0
            : static_cast<double>(b.addressSearchRounds) /
                  static_cast<double>(b.faultsDetected);
        const double cyclesPerFault = b.faultsDetected == 0
            ? 0.0
            : static_cast<double>(b.modeledCycles) /
                  static_cast<double>(b.faultsDetected);
        const double searchCyclesPerFault = b.faultsDetected == 0
            ? 0.0
            : static_cast<double>(b.addressSearchCycles) /
                  static_cast<double>(b.faultsDetected);
        const double collectionCyclesPerFault = b.faultsDetected == 0
            ? 0.0
            : static_cast<double>(b.faultCollectionCycles) /
                  static_cast<double>(b.faultsDetected);
        const double pivotSearchCyclesAverage = b.pivotFaults == 0
            ? 0.0
            : static_cast<double>(b.pivotSearchCycles) /
                  static_cast<double>(b.pivotFaults);
        const double nonpivotSearchCyclesAverage = b.nonpivotFaults == 0
            ? 0.0
            : static_cast<double>(b.nonpivotSearchCycles) /
                  static_cast<double>(b.nonpivotFaults);
        const double mustSearchCyclesAverage =
            b.mustRepairTriggeringFaults == 0
                ? 0.0
                : static_cast<double>(b.mustRepairSearchCycles) /
                      static_cast<double>(b.mustRepairTriggeringFaults);
        output
            << "SRAM_RECAM," << result.policy.label << ','
            << sram_recam::toString(result.config.biraSearchPolicy) << ','
            << sram_recam::toString(result.config.matrixPolicy) << ','
            << (result.config.useAbstractSearchRoundModel
                    ? "abstract_round" : "registered_read_compare") << ','
            << "occupied_entries,strict_row_and_column,0,"
            << (result.config.matrixPolicy == MatrixPolicy::ShadowMatrix
                    ? "ignored" : "modeled_scan") << ','
            << (result.config.matrixPolicy == MatrixPolicy::ShadowMatrix
                    ? "ignored" : "modeled") << ','
            << (result.config.analyzeAfterEachFault ? 1 : 0) << ','
            << result.config.rows << ',' << result.config.cols << ','
            << result.config.spareRows << ',' << result.config.spareCols << ','
            << result.config.channels << ',' << result.config.dataWordBits << ','
            << dynamic_spare::toString(options.dynamic.topology) << ','
            << dynamic_spare::toString(options.dynamic.solutionTakePolicy) << ','
            << options.dynamic.sharedRows << ','
            << options.dynamic.modifiers.maximumGroupBorrowedSpares << ','
            << options.dynamic.simulationRuns << ','
            << options.dynamic.faultCount << ','
            << options.dynamic.randomSeed << ','
            << g.tempEntries << ',' << g.addressEntries << ','
            << g.hybridEntries << ',' << g.runtimeEntries << ','
            << g.addressEntryBits << ',' << g.hybridEntryBits << ','
            << g.tempEntryBits << ',' << g.runtimeEntryBits << ','
            << g.addressLogicalBits << ',' << g.hybridLogicalBits << ','
            << g.tempBufferBits << ',' << g.totalStorageBits << ','
            << g.runtimeCompactBits << ',' << g.matrixBits << ','
            << g.solutionUnits << ','
            << g.addressSram.physicalDepth << ','
            << g.addressSram.physicalWidth << ','
            << g.addressSram.physicalBits << ','
            << g.addressSram.paddedBits << ','
            << g.hybridSram.physicalDepth << ','
            << g.hybridSram.physicalWidth << ','
            << g.hybridSram.physicalBits << ','
            << g.hybridSram.paddedBits << ','
            << g.runtimeSram.physicalDepth << ','
            << g.runtimeSram.physicalWidth << ','
            << g.runtimeSram.physicalBits << ','
            << g.runtimeSram.paddedBits << ','
            << g.addressSram.parallelEntries << ','
            << g.hybridSram.parallelEntries << ','
            << g.runtimeSram.parallelEntries << ','
            << (result.config.solutionParallelism == 0
                    ? g.solutionUnits
                    : std::min<std::uint64_t>(
                          result.config.solutionParallelism, g.solutionUnits))
            << ',' << g.rowComparatorCount << ','
            << g.columnComparatorCount << ','
            << g.runtimeComparatorCount << ','
            << g.comparatorBitCount << ','
            << result.attemptCount << ','
            << result.attemptsWithSramMetrics << ','
            << b.faultsDetected << ',' << b.addressEntryCount << ','
            << b.hybridEntryCount << ',' << b.tempEntryCount << ','
            << b.addressSearchRounds << ',' << searchRoundsPerFault << ','
            << b.addressSearchCycles << ',' << searchCyclesPerFault << ','
            << b.addressSearchCyclesMaximum << ',' << b.addressSramReads << ','
            << b.addressWrites << ',' << b.hybridWrites << ','
            << b.tempBufferWrites << ',' << b.matrixAddressReadRounds << ','
            << b.matrixHybridReadRounds << ',' << b.analysisRounds << ','
            << b.comparatorEvaluations << ','
            << b.comparatorBitComparisons << ','
            << b.earlyTerminationHitCount << ','
            << b.hybridBitmapAllocations << ',' << b.hybridFullCount << ','
            << b.pivotFaults << ',' << b.pivotSearchCycles << ','
            << pivotSearchCyclesAverage << ',' << b.nonpivotFaults << ','
            << b.nonpivotSearchCycles << ',' << nonpivotSearchCyclesAverage
            << ',' << b.mustRepairTriggeringFaults << ','
            << b.mustRepairSearchCycles << ',' << mustSearchCyclesAverage << ','
            << b.faultCollectionCycles << ',' << b.faultCollectionCycles << ','
            << collectionCyclesPerFault << ','
            << b.repairAnalysisCycles << ',' << b.modeledCycles << ','
            << cyclesPerFault << ','
            << runtime.requests << ',' << runtime.hits << ','
            << runtime.misses << ',' << result.runtimeOccupiedEntries << ','
            << runtime.lookupRounds << ',' << runtime.lookupRoundsAverage << ','
            << runtime.hitRoundsAverage << ',' << runtime.missRoundsAverage << ','
            << runtime.exposedExtraCycles << ','
            << runtime.exposedExtraCyclesAverage << ','
            << result.successCount << ',' << baselineSuccesses << ','
            << repairRate << ',' << baselineRate << ','
            << (result.equivalent ? 1 : 0) << ','
            << signatureText(baselineSignature) << ','
            << signatureText(result.logicalSignature) << ','
            << result.groupCost.addressPhysicalBits << ','
            << result.groupCost.hybridPhysicalBits << ','
            << result.groupCost.runtimePhysicalBits << ','
            << result.groupCost.temporaryBufferBits << ','
            << result.groupCost.matrixBits << ','
            << result.groupCost.comparatorBits << ','
            << options.area.sramBitAreaUnits << ','
            << options.area.comparatorBitAreaUnits << ','
            << options.area.fixedControllerAreaUnits << ','
            << result.groupCost.normalizedSramArea << ','
            << result.groupCost.normalizedComparatorArea << ','
            << result.groupCost.normalizedAreaProxy << '\n';
    }
    if (!output)
    {
        throw std::runtime_error("Failed while writing metrics CSV: " + path.string());
    }
}

PolicyResult simulatePolicy(
    const Options &options,
    const PolicySpec &policy,
    const std::vector<FaultGroup> &faultGroups,
    const std::vector<GroupRepairResult> &camBaseline,
    const std::array<std::pair<std::uint32_t, std::uint32_t>,
                     dynamic_spare::kSubarrayCount> &capacities)
{
    PolicyResult result;
    result.policy = policy;
    result.config = configForPolicy(options.sram, policy);
    result.geometry = sram_recam::deriveGeometry(result.config);
    result.groupCost = deriveGroupHardwareCost(
        result.config, options.area, capacities);

    const std::uint64_t runtimeCapacity = result.geometry.runtimeEntries;
    result.runtimeOccupiedEntries = options.runtimeOccupiedEntries.value_or(
        runtimeCapacity);
    if (result.runtimeOccupiedEntries > runtimeCapacity)
    {
        throw std::invalid_argument(
            "--runtime-occupied exceeds runtime capacity for policy " +
            policy.label);
    }
    const std::uint64_t runtimeHits = roundedHitCount(
        options.runtimeRequests, options.runtimeHitRate);
    if (runtimeHits != 0 && result.runtimeOccupiedEntries == 0)
    {
        throw std::invalid_argument(
            "A nonzero runtime hit rate requires occupied runtime entries");
    }
    result.runtime = sram_recam::modelRuntimeLatency(
        result.config,
        options.runtimeRequests,
        runtimeHits,
        result.runtimeOccupiedEntries);

    auto adapter = std::make_shared<dynamic_spare::SramRecamSolverAdapter>(
        result.config);
    dynamic_spare::DynamicRepairSimulator simulator(adapter);
    std::vector<GroupRepairResult> logicalResults;
    logicalResults.reserve(faultGroups.size());
    for (std::size_t run = 0; run < faultGroups.size(); ++run)
    {
        GroupRepairResult sram = simulator.run(
            faultGroups[run], options.dynamic, run);
        compareGroups(camBaseline[run], sram, run);
        result.successCount += sram.groupRepairSuccess ? 1 : 0;
        for (const auto &attempts : sram.attemptsBySubarray)
        {
            result.attemptCount += attempts.size();
            for (const RepairAttemptResult &attempt : attempts)
            {
                if (attempt.sramRecam.has_value())
                {
                    ++result.attemptsWithSramMetrics;
                    sram_recam::accumulate(
                        result.bira, attempt.sramRecam->bira);
                }
            }
        }
        discardLargeSnapshots(sram);
        logicalResults.push_back(std::move(sram));
    }
    result.logicalSignature = logicalSignature(logicalResults);
    return result;
}

} // namespace

int main(int argc, char *argv[])
{
    if (argc == 2 &&
        (std::string(argv[1]) == "--help" ||
         std::string(argv[1]) == "-h"))
    {
        printUsage(argv[0]);
        return 0;
    }

    try
    {
        const Options options = parseOptions(argc, argv);

        dynamic_spare::DynamicFaultGenerator generator(options.dynamic);
        const std::vector<FaultGroup> faultGroups = generator.generateRuns(
            static_cast<std::size_t>(options.dynamic.simulationRuns));

        dynamic_spare::DynamicRepairSimulator camSimulator;
        std::vector<GroupRepairResult> camBaseline;
        camBaseline.reserve(faultGroups.size());
        for (std::size_t run = 0; run < faultGroups.size(); ++run)
        {
            GroupRepairResult group = camSimulator.run(
                faultGroups[run], options.dynamic, run);
            discardLargeSnapshots(group);
            camBaseline.push_back(std::move(group));
        }
        const std::uint64_t baselineSuccesses = successCount(camBaseline);
        const std::uint64_t baselineSignature = logicalSignature(camBaseline);
        const auto capacities = provisionedCapacities(camBaseline.front());

        std::vector<PolicyResult> results;
        results.reserve(options.policies.size());
        for (const PolicySpec &policy : options.policies)
        {
            std::cout << "Simulating SRAM_RECAM policy "
                      << policy.label << "...\n";
            PolicyResult result = simulatePolicy(
                options, policy, faultGroups, camBaseline, capacities);
            if (result.logicalSignature != baselineSignature)
            {
                throw std::logic_error(
                    "SRAM_RECAM strict logical signature mismatch for " +
                    policy.label);
            }
            results.push_back(std::move(result));
        }

        ensureOutputTargetsAvailable(options, results);
        writeMetricsCsv(
            options.outputDirectory / "sram_recam_metrics.csv",
            options,
            results,
            baselineSuccesses,
            baselineSignature);
        for (const PolicyResult &result : results)
        {
            writeGroupHardwareJson(
                options.outputDirectory /
                    ("hardware_cost_" + result.policy.label + ".json"),
                result,
                options.area);
        }

        const double repairRate = options.dynamic.simulationRuns == 0
            ? 0.0
            : static_cast<double>(baselineSuccesses) /
                  static_cast<double>(options.dynamic.simulationRuns);
        std::cout
            << "CAM baseline signature: "
            << signatureText(baselineSignature) << '\n'
            << "RepairRate: " << repairRate
            << " SuccessGroups: " << baselineSuccesses
            << " TotalGroups: " << options.dynamic.simulationRuns << '\n'
            << "All " << results.size()
            << " SRAM_RECAM policies are logically equivalent to CAM.\n"
            << "Wrote "
            << (options.outputDirectory / "sram_recam_metrics.csv").string()
            << '\n';
    }
    catch (const std::exception &error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
