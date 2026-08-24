#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "inc/DynamicCsvReporter.hpp"
#include "inc/DynamicFaultGenerator.hpp"
#include "inc/DynamicRemapReporter.hpp"
#include "inc/DynamicRepairSimulator.hpp"
#include "inc/Fault.hpp"
#include "inc/SimplifiedFaultLoader.hpp"
#include "inc/SimulationConfig.hpp"

namespace
{

using dynamic_spare::FaultGroup;
using dynamic_spare::GroupRepairResult;
using dynamic_spare::SimulationBatch;
using FaultGroupIdentity = std::tuple<int, int, int, int>;

void printUsage(const char *program)
{
    std::cout
        << "Usage: " << program << " <Rs> <Cs> [options]\n\n"
        << "Input/output:\n"
        << "  --fault-file PATH        Use ordered fault groups from a file\n"
        << "  --simplified-fault-file PATH\n"
        << "                           Use flat seven-field hardware fault addresses\n"
        << "  --runs N                 Generated runs, or prefix of a file\n"
        << "  --fault-count N          Total faults in A/B/C/D per run\n"
        << "  --seed N                 Deterministic generation seed\n"
        << "  --output-dir PATH        CSV directory (default: reports/dynamic_spare_sharing)\n"
        << "  --write-remap-tables     Also write RemapTable.txt and RemapTable_simplified.txt\n"
        << "  --summary-only           Write summary.csv without attempts/runs CSV\n"
        << "  --sweep                  Run models x topologies x shared=0,1,2\n"
        << "  --repair-rate-sweep      Sweep fault count x symmetric Rs=Cs\n"
        << "  --fault-min N --fault-max N   Fault-count range for repair sweep\n"
        << "  --fault-step N           Fault-count interval (default: 1)\n"
        << "  --spare-min N --spare-max N   Rs=Cs range for repair sweep\n\n"
        << "Fault model:\n"
        << "  --fault-model uniform|moderate|strong|hotspot|user\n"
        << "  --fault-counts A,B,C,D   Required for user model\n"
        << "  --spatial uniform|mixed|clustered\n"
        << "  --memory-rows N --memory-columns N\n\n"
        << "Sharing policy:\n"
        << "  --topology none|directional|global|edge\n"
        << "  --shared-lines N         Set shared rows and columns\n"
        << "  --shared-rows N --shared-columns N\n"
        << "  --global-local-rows N --global-local-columns N\n"
        << "  --global-rows N --global-columns N\n"
        << "  --local-first\n"
        << "  --minimum-row-reserve N --minimum-column-reserve N\n"
        << "  --single-dimension\n"
        << "  --max-borrows N\n\n"
        << "Hardware/latency:\n"
        << "  --paper-cam-reuse        Buffer capacity = Rs+Cs (default)\n"
        << "  --buffer N               Use a fixed buffer capacity instead\n"
        << "  --hybrid-cam-entry-width-bits N\n"
        << "  --data-width N --row-address-width N --column-address-width N\n"
        << "  --storage cam|sram\n"
        << "  --cam-latency insert,lookup,read\n"
        << "  --sram-latency insert,lookup,read\n"
        << "  --analysis-latency matrix,generate,evaluate,allocate\n";
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

int groupCapacityFromPerSubarray(int value, const std::string &description)
{
    if (value < 0 ||
        value > std::numeric_limits<int>::max() /
            static_cast<int>(dynamic_spare::kSubarrayCount))
    {
        throw std::invalid_argument(
            description + " is outside the safe group-capacity range");
    }
    return value * static_cast<int>(dynamic_spare::kSubarrayCount);
}

std::vector<std::uint64_t> parseList(
    const std::string &text,
    std::size_t expected,
    const std::string &option)
{
    std::vector<std::uint64_t> values;
    std::size_t begin = 0;
    while (begin <= text.size())
    {
        const std::size_t comma = text.find(',', begin);
        const std::string field = text.substr(
            begin,
            comma == std::string::npos
                ? std::string::npos
                : comma - begin);
        values.push_back(parseUint64(field, option));
        if (comma == std::string::npos)
        {
            break;
        }
        begin = comma + 1;
    }
    if (values.size() != expected)
    {
        throw std::invalid_argument(
            option + " requires exactly " + std::to_string(expected) +
            " comma-separated values");
    }
    return values;
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
    if (value == "user")
        return dynamic_spare::FaultCountModel::UserDefined;
    throw std::invalid_argument("Unknown fault model: " + value);
}

dynamic_spare::FaultSpatialModel parseSpatialModel(const std::string &value)
{
    if (value == "uniform")
        return dynamic_spare::FaultSpatialModel::Uniform;
    if (value == "mixed")
        return dynamic_spare::FaultSpatialModel::Mixed;
    if (value == "clustered")
        return dynamic_spare::FaultSpatialModel::Clustered;
    throw std::invalid_argument("Unknown spatial model: " + value);
}

dynamic_spare::FaultInformationStorage parseStorage(
    const std::string &value)
{
    if (value == "cam")
        return dynamic_spare::FaultInformationStorage::CAM;
    if (value == "sram")
        return dynamic_spare::FaultInformationStorage::SRAM;
    throw std::invalid_argument("Unknown storage mode: " + value);
}

FaultGroupIdentity identity(const Fault &fault)
{
    return std::make_tuple(
        fault.HBMID,
        fault.ChannelID,
        fault.BankID,
        fault.SubarrayGroupID);
}

void validateFaultGroup(const FaultGroup &group, std::size_t runIndex)
{
    std::optional<FaultGroupIdentity> expected;
    for (std::size_t subarray = 0;
         subarray < dynamic_spare::kSubarrayCount; ++subarray)
    {
        for (const Fault &fault : group[subarray])
        {
            if (fault.SubarrayID != static_cast<int>(subarray))
            {
                throw std::runtime_error(
                    "Fault-list position does not match A/B/C/D SubarrayID "
                    "in run " + std::to_string(runIndex));
            }
            const FaultGroupIdentity current = identity(fault);
            if (!expected.has_value())
                expected = current;
            else if (*expected != current)
                throw std::runtime_error(
                    "A/B/C/D lists cross a hardware group boundary in run " +
                    std::to_string(runIndex));
        }
    }
}

std::vector<FaultGroup> loadFaultGroups(
    const std::filesystem::path &path,
    const dynamic_spare::SimulationConfig &config)
{
    const int bufferEntries = config.usePaperCamReuseCapacity
        ? config.spareRows + config.spareColumns
        : config.bufferCamEntries;
    FaultLoader loader(config.spareRows, config.spareColumns, bufferEntries);
    if (!loader.loadFaults(path.string()))
        throw std::runtime_error("Unable to load fault file: " + path.string());
    if (loader.faultLists.size() % dynamic_spare::kSubarrayCount != 0)
        throw std::runtime_error(
            "Fault input must contain complete A/B/C/D groups");

    const std::size_t runCount =
        loader.faultLists.size() / dynamic_spare::kSubarrayCount;
    std::vector<FaultGroup> groups(runCount);
    for (std::size_t run = 0; run < runCount; ++run)
    {
        for (std::size_t subarray = 0;
             subarray < dynamic_spare::kSubarrayCount; ++subarray)
        {
            groups[run][subarray] = loader.faultLists[
                run * dynamic_spare::kSubarrayCount + subarray].PEFaults;
        }
        validateFaultGroup(groups[run], run);
    }
    return groups;
}

std::size_t groupFaultCount(const FaultGroup &group)
{
    std::size_t count = 0;
    for (const auto &faults : group)
        count += faults.size();
    return count;
}

dynamic_spare::SimulationConfig baselineConfig(
    const dynamic_spare::SimulationConfig &source)
{
    dynamic_spare::SimulationConfig baseline = source;
    baseline.topology = dynamic_spare::SharingTopology::NoSharing;
    baseline.sharedRows = 0;
    baseline.sharedColumns = 0;
    baseline.globalPool.reset();
    baseline.modifiers.localFirst = true;
    baseline.modifiers.singleDimensionBorrowing = false;
    baseline.modifiers.minimumRowReserve = 0;
    baseline.modifiers.minimumColumnReserve = 0;
    baseline.modifiers.maximumGroupBorrowedSpares = 0;
    return baseline;
}

void attachBaseline(
    GroupRepairResult &result,
    const GroupRepairResult &baseline)
{
    result.baselineGroupRepairSuccess = baseline.groupRepairSuccess;
    result.baselineAnalysisCycles = baseline.latency.totalCycles();
    result.sharingGain =
        static_cast<int>(result.groupRepairSuccess) -
        static_cast<int>(baseline.groupRepairSuccess);
    result.sharing.repairSuccessDueToSharing =
        result.groupRepairSuccess && !baseline.groupRepairSuccess;
    result.sharing.repairFailureEvenAfterSharing =
        !result.groupRepairSuccess &&
        result.topology != dynamic_spare::SharingTopology::NoSharing;
}

void discardMappingSnapshots(GroupRepairResult &result)
{
    for (auto &attempts : result.attemptsBySubarray)
    {
        for (auto &attempt : attempts)
        {
            std::vector<dynamic_spare::RepairLineMapping>{}.swap(
                attempt.selectedMappings);
            std::vector<dynamic_spare::CandidateRepairOption>{}.swap(
                attempt.validCandidateOptions);
        }
    }
}

SimulationBatch simulateBatch(
    const dynamic_spare::SimulationConfig &config,
    const std::vector<FaultGroup> &groups,
    const std::vector<GroupRepairResult> *knownBaseline = nullptr,
    bool retainSelectedRemap = false)
{
    dynamic_spare::DynamicRepairSimulator simulator;
    SimulationBatch batch;
    batch.config = config;
    batch.runs.reserve(groups.size());

    std::vector<GroupRepairResult> ownedBaseline;
    const std::vector<GroupRepairResult> *baseline = knownBaseline;
    if (baseline == nullptr &&
        config.topology != dynamic_spare::SharingTopology::NoSharing)
    {
        const auto noSharing = baselineConfig(config);
        ownedBaseline.reserve(groups.size());
        for (std::size_t run = 0; run < groups.size(); ++run)
        {
            GroupRepairResult baselineResult = simulator.run(
                groups[run], noSharing, run);
            for (auto &attempts : baselineResult.attemptsBySubarray)
            {
                std::vector<dynamic_spare::RepairAttemptResult>{}.swap(
                    attempts);
            }
            ownedBaseline.push_back(std::move(baselineResult));
        }
        baseline = &ownedBaseline;
    }

    for (std::size_t run = 0; run < groups.size(); ++run)
    {
        GroupRepairResult result = simulator.run(
            groups[run], config, run, retainSelectedRemap);
        attachBaseline(
            result,
            config.topology == dynamic_spare::SharingTopology::NoSharing
                ? result
                : baseline->at(run));
        discardMappingSnapshots(result);
        batch.runs.push_back(std::move(result));
    }
    return batch;
}

double repairRate(const SimulationBatch &batch)
{
    const std::size_t successes = static_cast<std::size_t>(std::count_if(
        batch.runs.begin(), batch.runs.end(),
        [](const GroupRepairResult &result)
        {
            return result.groupRepairSuccess;
        }));
    return batch.runs.empty()
        ? 0.0
        : static_cast<double>(successes) /
              static_cast<double>(batch.runs.size());
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
    if (argc < 3)
    {
        printUsage(argv[0]);
        return 1;
    }

    try
    {
        dynamic_spare::SimulationConfig config;
        config.spareRows = parseInt(argv[1], "Rs");
        config.spareColumns = parseInt(argv[2], "Cs");

        std::optional<std::filesystem::path> faultFile;
        std::optional<std::filesystem::path> simplifiedFaultFile;
        std::filesystem::path outputDirectory = "reports/dynamic_spare_sharing";
        bool sweep = false;
        bool repairRateSweep = false;
        bool summaryOnly = false;
        bool runsSpecified = false;
        bool faultCountSpecified = false;
        bool faultModelSpecified = false;
        bool spatialModelSpecified = false;
        bool topologySpecified = false;
        bool sharedCapacitySpecified = false;
        bool globalConfigurationSpecified = false;
        bool fixedBufferSpecified = false;
        bool paperCamReuseSpecified = false;
        bool writeRemapTables = false;
        std::optional<std::uint64_t> sweepFaultMin;
        std::optional<std::uint64_t> sweepFaultMax;
        std::optional<std::uint64_t> sweepFaultStep;
        std::optional<int> sweepSpareMin;
        std::optional<int> sweepSpareMax;
        dynamic_spare::GlobalPoolConfiguration globalPool;

        for (int index = 3; index < argc; ++index)
        {
            const std::string option = argv[index];
            const auto requireValue = [&]() -> std::string
            {
                if (index + 1 >= argc)
                    throw std::invalid_argument(option + " requires a value");
                return argv[++index];
            };

            if (option == "--fault-file") faultFile = requireValue();
            else if (option == "--simplified-fault-file")
                simplifiedFaultFile = requireValue();
            else if (option == "--output-dir") outputDirectory = requireValue();
            else if (option == "--write-remap-tables")
                writeRemapTables = true;
            else if (option == "--runs")
            {
                config.simulationRuns = parseUint64(requireValue(), option);
                runsSpecified = true;
            }
            else if (option == "--fault-count")
            {
                config.faultCount = parseUint64(requireValue(), option);
                faultCountSpecified = true;
            }
            else if (option == "--seed")
                config.randomSeed = parseUint64(requireValue(), option);
            else if (option == "--fault-model")
            {
                config.faultCountModel = parseFaultModel(requireValue());
                faultModelSpecified = true;
            }
            else if (option == "--fault-counts")
            {
                const auto values = parseList(requireValue(), 4, option);
                std::uint64_t total = 0;
                for (std::size_t subarray = 0; subarray < 4; ++subarray)
                {
                    if (values[subarray] > static_cast<std::uint64_t>(
                            std::numeric_limits<int>::max()))
                        throw std::invalid_argument(
                            "--fault-counts value exceeds int range");
                    config.userDefinedFaultCounts[subarray] =
                        static_cast<int>(values[subarray]);
                    total += values[subarray];
                }
                config.faultCount = total;
                config.faultCountModel =
                    dynamic_spare::FaultCountModel::UserDefined;
                faultCountSpecified = true;
                faultModelSpecified = true;
            }
            else if (option == "--spatial")
            {
                config.faultSpatialModel = parseSpatialModel(requireValue());
                spatialModelSpecified = true;
            }
            else if (option == "--memory-rows" ||
                     option == "--memory-columns")
            {
                const auto value = parseUint64(requireValue(), option);
                if (value > std::numeric_limits<std::uint32_t>::max())
                    throw std::invalid_argument(option + " is too large");
                if (option == "--memory-rows")
                    config.memoryRows = static_cast<std::uint32_t>(value);
                else
                    config.memoryColumns = static_cast<std::uint32_t>(value);
            }
            else if (option == "--topology")
            {
                config.topology = parseTopology(requireValue());
                topologySpecified = true;
            }
            else if (option == "--shared-lines")
            {
                const int value = parseInt(requireValue(), option);
                config.sharedRows = value;
                config.sharedColumns = value;
                sharedCapacitySpecified = true;
            }
            else if (option == "--shared-rows")
            {
                config.sharedRows = parseInt(requireValue(), option);
                sharedCapacitySpecified = true;
            }
            else if (option == "--shared-columns")
            {
                config.sharedColumns = parseInt(requireValue(), option);
                sharedCapacitySpecified = true;
            }
            else if (option == "--global-local-rows")
            {
                globalPool.localRowsPerSubarray = parseInt(requireValue(), option);
                globalConfigurationSpecified = true;
            }
            else if (option == "--global-local-columns")
            {
                globalPool.localColumnsPerSubarray = parseInt(requireValue(), option);
                globalConfigurationSpecified = true;
            }
            else if (option == "--global-rows")
            {
                globalPool.globalRows = parseInt(requireValue(), option);
                globalConfigurationSpecified = true;
            }
            else if (option == "--global-columns")
            {
                globalPool.globalColumns = parseInt(requireValue(), option);
                globalConfigurationSpecified = true;
            }
            else if (option == "--local-first")
                config.modifiers.localFirst = true;
            else if (option == "--minimum-row-reserve")
                config.modifiers.minimumRowReserve = parseInt(requireValue(), option);
            else if (option == "--minimum-column-reserve")
                config.modifiers.minimumColumnReserve = parseInt(requireValue(), option);
            else if (option == "--single-dimension")
                config.modifiers.singleDimensionBorrowing = true;
            else if (option == "--max-borrows")
                config.modifiers.maximumGroupBorrowedSpares = parseInt(requireValue(), option);
            else if (option == "--buffer")
            {
                if (paperCamReuseSpecified)
                    throw std::invalid_argument(
                        "--buffer and --paper-cam-reuse are mutually exclusive");
                config.bufferCamEntries = parseInt(requireValue(), option);
                config.usePaperCamReuseCapacity = false;
                fixedBufferSpecified = true;
            }
            else if (option == "--paper-cam-reuse")
            {
                if (fixedBufferSpecified)
                    throw std::invalid_argument(
                        "--buffer and --paper-cam-reuse are mutually exclusive");
                config.usePaperCamReuseCapacity = true;
                paperCamReuseSpecified = true;
            }
            else if (option == "--hybrid-cam-entry-width-bits")
            {
                const auto value = parseUint64(requireValue(), option);
                if (value == 0 || value > std::numeric_limits<std::uint32_t>::max())
                    throw std::invalid_argument(
                        "Hybrid CAM entry width is outside uint32 range");
                config.hybridCamEntryWidthBits = static_cast<std::uint32_t>(value);
            }
            else if (option == "--data-width" ||
                     option == "--row-address-width" ||
                     option == "--column-address-width")
            {
                const auto value = parseUint64(requireValue(), option);
                if (value == 0 || value > std::numeric_limits<std::uint32_t>::max())
                    throw std::invalid_argument(option + " is outside uint32 range");
                if (option == "--data-width")
                    config.dataWidthBits = static_cast<std::uint32_t>(value);
                else if (option == "--row-address-width")
                    config.rowAddressWidthBits = static_cast<std::uint32_t>(value);
                else
                    config.columnAddressWidthBits = static_cast<std::uint32_t>(value);
            }
            else if (option == "--storage")
                config.storageMode = parseStorage(requireValue());
            else if (option == "--cam-latency" || option == "--sram-latency")
            {
                const auto values = parseList(requireValue(), 3, option);
                dynamic_spare::StorageLatencyParameters latency{
                    values[0], values[1], values[2]};
                if (option == "--cam-latency") config.latency.cam = latency;
                else config.latency.sram = latency;
            }
            else if (option == "--analysis-latency")
            {
                const auto values = parseList(requireValue(), 4, option);
                config.latency.matrixGenerationCyclesPerCell = values[0];
                config.latency.solutionGenerationCyclesPerCandidate = values[1];
                config.latency.solutionEvaluationCyclesPerCandidate = values[2];
                config.latency.sharingAllocationCyclesPerRequest = values[3];
            }
            else if (option == "--sweep") sweep = true;
            else if (option == "--repair-rate-sweep") repairRateSweep = true;
            else if (option == "--summary-only") summaryOnly = true;
            else if (option == "--fault-min")
                sweepFaultMin = parseUint64(requireValue(), option);
            else if (option == "--fault-max")
                sweepFaultMax = parseUint64(requireValue(), option);
            else if (option == "--fault-step")
                sweepFaultStep = parseUint64(requireValue(), option);
            else if (option == "--spare-min")
                sweepSpareMin = parseInt(requireValue(), option);
            else if (option == "--spare-max")
                sweepSpareMax = parseInt(requireValue(), option);
            else if (option == "--help" || option == "-h")
            {
                printUsage(argv[0]);
                return 0;
            }
            else throw std::invalid_argument("Unknown option: " + option);
        }

        if (sweep && repairRateSweep)
            throw std::invalid_argument(
                "--sweep and --repair-rate-sweep are mutually exclusive");
        if ((sweepFaultMin.has_value() || sweepFaultMax.has_value() ||
             sweepFaultStep.has_value() ||
             sweepSpareMin.has_value() || sweepSpareMax.has_value()) &&
            !repairRateSweep)
            throw std::invalid_argument(
                "--fault-min/--fault-max/--fault-step/--spare-min/"
                "--spare-max require --repair-rate-sweep");
        if (faultFile.has_value() && simplifiedFaultFile.has_value())
            throw std::invalid_argument(
                "--fault-file and --simplified-fault-file are mutually exclusive");
        if ((sweep || repairRateSweep) &&
            (faultFile.has_value() || simplifiedFaultFile.has_value()))
            throw std::invalid_argument(
                "Sweep modes generate faults and cannot use --fault-file or "
                "--simplified-fault-file");
        if ((sweep || repairRateSweep) && writeRemapTables)
            throw std::invalid_argument(
                "--write-remap-tables is available for one policy "
                "configuration, not sweep modes");
        if (repairRateSweep &&
            config.faultCountModel ==
                dynamic_spare::FaultCountModel::UserDefined)
            throw std::invalid_argument(
                "--repair-rate-sweep cannot vary user-defined A/B/C/D counts");
        if (repairRateSweep &&
            (topologySpecified || sharedCapacitySpecified ||
             globalConfigurationSpecified))
            throw std::invalid_argument(
                "--repair-rate-sweep selects its comparison policies; do not "
                "combine it with topology/shared/global capacity options");
        if (config.topology == dynamic_spare::SharingTopology::GlobalPool)
        {
            if (!globalConfigurationSpecified)
            {
                if (config.sharedRows < 0 ||
                    config.sharedRows > config.spareRows ||
                    config.sharedColumns < 0 ||
                    config.sharedColumns > config.spareColumns)
                    throw std::invalid_argument(
                        "Shared capacity must be within local Rs/Cs");
                globalPool.localRowsPerSubarray = config.spareRows - config.sharedRows;
                globalPool.localColumnsPerSubarray = config.spareColumns - config.sharedColumns;
                globalPool.globalRows = groupCapacityFromPerSubarray(
                    config.sharedRows, "Global shared-row capacity");
                globalPool.globalColumns = groupCapacityFromPerSubarray(
                    config.sharedColumns, "Global shared-column capacity");
            }
            else
            {
                if (globalPool.localRowsPerSubarray < 0 ||
                    globalPool.localRowsPerSubarray > config.spareRows ||
                    globalPool.localColumnsPerSubarray < 0 ||
                    globalPool.localColumnsPerSubarray > config.spareColumns)
                    throw std::invalid_argument(
                        "Global local capacity must be within baseline Rs/Cs");
                config.sharedRows =
                    config.spareRows - globalPool.localRowsPerSubarray;
                config.sharedColumns =
                    config.spareColumns -
                    globalPool.localColumnsPerSubarray;
            }
            config.globalPool = globalPool;
        }
        config.validate();

        std::filesystem::create_directories(outputDirectory);
        if (repairRateSweep &&
            (std::filesystem::exists(outputDirectory / "summary.csv") ||
             std::filesystem::exists(outputDirectory / "attempts.csv") ||
             std::filesystem::exists(outputDirectory / "runs.csv")))
            throw std::invalid_argument(
                "--repair-rate-sweep requires a fresh output directory to "
                "avoid mixing experiment data");
        std::size_t writtenBatchCount = 0;
        const auto emitBatch = [&](SimulationBatch batch, bool retainRuns)
        {
            std::vector<SimulationBatch> oneBatch;
            oneBatch.push_back(std::move(batch));
            const bool append = writtenBatchCount != 0;
            if (!summaryOnly)
            {
                dynamic_spare::DynamicCsvReporter::writeAttempts(
                    outputDirectory / "attempts.csv", oneBatch, append);
                dynamic_spare::DynamicCsvReporter::writeRuns(
                    outputDirectory / "runs.csv", oneBatch, append);
            }
            dynamic_spare::DynamicCsvReporter::writeSummary(
                outputDirectory / "summary.csv", oneBatch, append);
            ++writtenBatchCount;
            std::vector<GroupRepairResult> retained;
            if (retainRuns)
                retained = std::move(oneBatch.front().runs);
            return retained;
        };

        std::optional<double> singleRepairRate;
        std::size_t singleSuccessCount = 0;
        std::size_t singleRunCount = 0;
        if (repairRateSweep)
        {
            const std::uint64_t faultMin =
                sweepFaultMin.value_or(config.faultCount);
            const std::uint64_t faultMax =
                sweepFaultMax.value_or(config.faultCount);
            const std::uint64_t faultStep = sweepFaultStep.value_or(1);
            const int spareMin = sweepSpareMin.value_or(config.spareRows);
            const int spareMax = sweepSpareMax.value_or(config.spareRows);
            if (faultMin == 0 || faultMax < faultMin)
                throw std::invalid_argument(
                    "Repair-rate fault range must satisfy 0 < min <= max");
            if (faultStep == 0)
                throw std::invalid_argument(
                    "Repair-rate fault step must be greater than zero");
            if (spareMin <= 0 || spareMax < spareMin)
                throw std::invalid_argument(
                    "Repair-rate spare range must satisfy 0 < min <= max");

            using Topology = dynamic_spare::SharingTopology;
            const std::array<std::pair<Topology, int>, 5> comparisons{{
                {Topology::Directional, 1},
                {Topology::Directional, 2},
                {Topology::PairwiseEdge, 1},
                {Topology::PairwiseEdge, 2},
                {Topology::GlobalPool, 1}}};

            for (std::uint64_t faultCount = faultMin;;)
            {
                dynamic_spare::SimulationConfig generatorConfig = config;
                generatorConfig.faultCount = faultCount;
                generatorConfig.spareRows = spareMin;
                generatorConfig.spareColumns = spareMin;
                generatorConfig.topology = Topology::NoSharing;
                generatorConfig.sharedRows = 0;
                generatorConfig.sharedColumns = 0;
                generatorConfig.globalPool.reset();
                generatorConfig.validate();
                dynamic_spare::DynamicFaultGenerator generator(generatorConfig);
                const auto groups = generator.generateRuns(
                    static_cast<std::size_t>(generatorConfig.simulationRuns));

                for (int spareLines = spareMin;; ++spareLines)
                {
                    dynamic_spare::SimulationConfig baselineConfig =
                        generatorConfig;
                    baselineConfig.spareRows = spareLines;
                    baselineConfig.spareColumns = spareLines;
                    baselineConfig.validate();
                    SimulationBatch baseline = simulateBatch(
                        baselineConfig, groups);
                    const double baselineRepairRate = repairRate(baseline);
                    std::vector<GroupRepairResult> baselineRuns =
                        emitBatch(std::move(baseline), true);
                    for (GroupRepairResult &result : baselineRuns)
                    {
                        for (auto &attempts : result.attemptsBySubarray)
                        {
                            std::vector<dynamic_spare::RepairAttemptResult>{}
                                .swap(attempts);
                        }
                    }
                    std::cout
                        << "completed fault_count=" << faultCount
                        << " spare=" << spareLines
                        << " policy=no_sharing"
                        << " repair_rate=" << baselineRepairRate << '\n';

                    for (const auto &comparison : comparisons)
                    {
                        const Topology topology = comparison.first;
                        const int sharedLines = comparison.second;
                        if (sharedLines > spareLines)
                            continue;
                        dynamic_spare::SimulationConfig policy = baselineConfig;
                        policy.topology = topology;
                        policy.sharedRows = sharedLines;
                        policy.sharedColumns = sharedLines;
                        if (topology == Topology::GlobalPool)
                        {
                            policy.globalPool =
                                dynamic_spare::GlobalPoolConfiguration{
                                    spareLines - sharedLines,
                                    spareLines - sharedLines,
                                    static_cast<int>(
                                        dynamic_spare::kSubarrayCount) *
                                        sharedLines,
                                    static_cast<int>(
                                        dynamic_spare::kSubarrayCount) *
                                        sharedLines};
                        }
                        else
                        {
                            policy.globalPool.reset();
                        }
                        policy.validate();
                        SimulationBatch policyBatch = simulateBatch(
                            policy, groups, &baselineRuns);
                        const double policyRepairRate = repairRate(policyBatch);
                        emitBatch(std::move(policyBatch), false);
                        std::cout
                            << "completed fault_count=" << faultCount
                            << " spare=" << spareLines
                            << " policy="
                            << dynamic_spare::toString(topology)
                            << " shared=" << sharedLines
                            << " repair_rate=" << policyRepairRate << '\n';
                    }
                    if (spareLines == spareMax)
                        break;
                }
                if (faultMax - faultCount < faultStep)
                    break;
                faultCount += faultStep;
            }
        }
        else if (sweep)
        {
            const std::array<dynamic_spare::FaultCountModel, 4> models{{
                dynamic_spare::FaultCountModel::Uniform,
                dynamic_spare::FaultCountModel::ModerateImbalance,
                dynamic_spare::FaultCountModel::StrongImbalance,
                dynamic_spare::FaultCountModel::Hotspot}};
            const std::array<dynamic_spare::SharingTopology, 3> sharing{{
                dynamic_spare::SharingTopology::Directional,
                dynamic_spare::SharingTopology::GlobalPool,
                dynamic_spare::SharingTopology::PairwiseEdge}};
            for (const auto model : models)
            {
                dynamic_spare::SimulationConfig modelConfig = config;
                modelConfig.faultCountModel = model;
                modelConfig.topology = dynamic_spare::SharingTopology::NoSharing;
                modelConfig.sharedRows = 0;
                modelConfig.sharedColumns = 0;
                modelConfig.globalPool.reset();
                modelConfig.validate();
                dynamic_spare::DynamicFaultGenerator generator(modelConfig);
                const auto groups = generator.generateRuns(
                    static_cast<std::size_t>(modelConfig.simulationRuns));
                SimulationBatch baseline = simulateBatch(modelConfig, groups);
                std::vector<GroupRepairResult> baselineRuns =
                    emitBatch(std::move(baseline), true);
                for (GroupRepairResult &result : baselineRuns)
                {
                    for (auto &attempts : result.attemptsBySubarray)
                    {
                        std::vector<dynamic_spare::RepairAttemptResult>{}
                            .swap(attempts);
                    }
                }

                for (const auto topology : sharing)
                {
                    for (int sharedLines = 0; sharedLines <= 2; ++sharedLines)
                    {
                        if (sharedLines > config.spareRows ||
                            sharedLines > config.spareColumns)
                            continue;
                        dynamic_spare::SimulationConfig policy = config;
                        policy.faultCountModel = model;
                        policy.topology = topology;
                        policy.sharedRows = sharedLines;
                        policy.sharedColumns = sharedLines;
                        if (topology == dynamic_spare::SharingTopology::GlobalPool)
                        {
                            policy.globalPool = dynamic_spare::GlobalPoolConfiguration{
                                policy.spareRows - sharedLines,
                                policy.spareColumns - sharedLines,
                                static_cast<int>(dynamic_spare::kSubarrayCount) * sharedLines,
                                static_cast<int>(dynamic_spare::kSubarrayCount) * sharedLines};
                        }
                        else policy.globalPool.reset();
                        policy.validate();
                        SimulationBatch policyBatch = simulateBatch(
                            policy, groups, &baselineRuns);
                        const double policyRepairRate = repairRate(policyBatch);
                        emitBatch(std::move(policyBatch), false);
                        std::cout
                            << "completed model=" << dynamic_spare::toString(model)
                            << " policy=" << dynamic_spare::toString(topology)
                            << " shared=" << sharedLines
                            << " repair_rate=" << policyRepairRate
                            << '\n';
                    }
                }
            }
        }
        else
        {
            std::vector<FaultGroup> groups;
            if (faultFile.has_value() || simplifiedFaultFile.has_value())
            {
                groups = simplifiedFaultFile.has_value()
                    ? dynamic_spare::loadSimplifiedFaultGroups(
                          *simplifiedFaultFile, config)
                    : loadFaultGroups(*faultFile, config);
                for (std::size_t run = 0; run < groups.size(); ++run)
                    validateFaultGroup(groups[run], run);
                if (groups.empty())
                    throw std::runtime_error("Fault file contains no runs");
                const std::size_t observedFaultCount = groupFaultCount(groups.front());
                for (std::size_t run = 0; run < groups.size(); ++run)
                    if (groupFaultCount(groups[run]) != observedFaultCount)
                        throw std::runtime_error(
                            "fault_count must be constant across file runs");
                if (faultCountSpecified && config.faultCount != observedFaultCount)
                    throw std::runtime_error(
                        "Configured fault_count does not match the file group total");
                config.faultCount = observedFaultCount;
                if (!faultModelSpecified)
                    config.faultCountModel =
                        dynamic_spare::FaultCountModel::FileProvided;
                if (!spatialModelSpecified)
                    config.faultSpatialModel =
                        dynamic_spare::FaultSpatialModel::FileProvided;
                if (runsSpecified)
                {
                    if (config.simulationRuns > groups.size())
                        throw std::runtime_error(
                            "--runs exceeds the number of fault-file groups");
                    groups.resize(static_cast<std::size_t>(config.simulationRuns));
                }
                else config.simulationRuns = groups.size();
            }
            else
            {
                config.validate();
                dynamic_spare::DynamicFaultGenerator generator(config);
                groups = generator.generateRuns(
                    static_cast<std::size_t>(config.simulationRuns));
            }
            config.validate();
            SimulationBatch batch = simulateBatch(
                config, groups, nullptr, writeRemapTables);
            singleRepairRate = repairRate(batch);
            singleRunCount = batch.runs.size();
            singleSuccessCount = static_cast<std::size_t>(std::count_if(
                batch.runs.begin(), batch.runs.end(),
                [](const GroupRepairResult &result)
                {
                    return result.groupRepairSuccess;
                }));
            if (writeRemapTables)
            {
                const dynamic_spare::RemapWriteSummary remapSummary =
                    dynamic_spare::DynamicRemapReporter::write(
                        outputDirectory / "RemapTable.txt",
                        outputDirectory / "RemapTable_simplified.txt",
                        config,
                        groups,
                        batch.runs);
                std::cout
                    << "RemapTables: success_groups="
                    << remapSummary.successfulGroups
                    << " failed_groups=" << remapSummary.failedGroups
                    << " map_entries=" << remapSummary.lineMappings
                    << " buffmap_entries=" << remapSummary.bufferMappings
                    << '\n';
            }
            emitBatch(std::move(batch), false);
        }

        if (singleRepairRate.has_value())
        {
            std::cout
                << "RepairRate: " << *singleRepairRate
                << " SuccessGroups: " << singleSuccessCount
                << " TotalGroups: " << singleRunCount
                << " AttemptsCSV: "
                << (outputDirectory / "attempts.csv").string() << '\n';
        }
        std::cout << "Wrote " << writtenBatchCount << " configuration(s) to "
                  << outputDirectory.string() << '\n';
    }
    catch (const std::exception &error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
