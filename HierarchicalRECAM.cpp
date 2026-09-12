#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "inc/DynamicFaultGenerator.hpp"
#include "inc/CanonicalExperiment.hpp"
#include "inc/HierarchicalRecamSimulator.hpp"
#include "inc/SimplifiedFaultLoader.hpp"

namespace
{

using dynamic_spare::FaultGroup;
using dynamic_spare::RepairGroupAddress;
using dynamic_spare::RepairGroupInput;

void printUsage(const char *program)
{
    std::cout
        << "Usage: " << program << " <Rs> <Cs> [options]\n\n"
        << "Device input:\n"
        << "  --simplified-fault-file PATH  Seven-field device fault list\n"
        << "  --fault-file PATH             Count-framed faults.faults input\n"
        << "  --groups N                    Generated repair groups (default: 1)\n"
        << "  --fault-count N               Faults per generated 4-SA group\n"
        << "  --seed N                      Deterministic generator seed\n"
        << "  --fault-model uniform|moderate|strong|hotspot\n"
        << "                                   Default: uniform\n"
        << "  --spatial uniform|mixed|clustered  Default: mixed\n"
        << "  --memory-rows N               Default: 512\n"
        << "  --memory-columns N            Default: 8192\n\n"
        << "Hierarchical RECAM:\n"
        << "  --online-global-reuse-entries N  Override device-wide CAM capacity\n"
        << "                                   Default: RECAM paper Rs+Cs\n"
        << "  --bira-engines N              Shared offline engines (default: 1)\n"
        << "  --data-word-bits N            Default: 256\n"
        << "  --cam-granularity word|cell   Default: word\n\n"
        << "Canonical comparison:\n"
        << "  --canonical-four             Run B0/B1/B2/B3 on one fault set\n"
        << "  --sram-policy serial|chunked:N|wide  Default: chunked:2\n"
        << "  --sram-matrix-policy scan|shadow     Default: shadow\n"
        << "  --registered-sram-search     SRAM read+compare = 2 cycles\n"
        << "  --bist-decoupled-fifo        Enable A/B/C/D serial BIST timeline\n"
        << "  --bist-cycles-per-word N     Default: 1\n\n"
        << "Tier-1 sharing:\n"
        << "  --topology none|directional|edge|global\n"
        << "  --shared-rows N --shared-columns N\n"
        << "  --local-first --max-borrows N\n"
        << "  --solution-take legacy|early|group  Default: legacy\n\n"
        << "Output:\n"
        << "  --output-dir PATH             Default: reports/hierarchical_recam\n"
        << "  --write-fault-corpus          Write resolved seven-field faults\n"
        << "  --write-area-manifest         Canonical B0-B3 resource/bit CSV\n";
}

std::uint64_t parseUnsigned(
    const std::string &text,
    const std::string &option)
{
    if (text.empty() || text.front() == '-')
    {
        throw std::invalid_argument(option + " requires an unsigned integer");
    }
    std::size_t parsed = 0;
    std::uint64_t value = 0;
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
    return value;
}

int parsePositiveInt(const std::string &text, const std::string &option)
{
    const std::uint64_t value = parseUnsigned(text, option);
    if (value == 0 || value > static_cast<std::uint64_t>(
            std::numeric_limits<int>::max()))
    {
        throw std::invalid_argument(option + " must be a positive int");
    }
    return static_cast<int>(value);
}

int parseNonnegativeInt(const std::string &text, const std::string &option)
{
    const std::uint64_t value = parseUnsigned(text, option);
    if (value > static_cast<std::uint64_t>(std::numeric_limits<int>::max()))
    {
        throw std::invalid_argument(option + " exceeds the int range");
    }
    return static_cast<int>(value);
}

std::size_t parseSize(const std::string &text, const std::string &option)
{
    const std::uint64_t value = parseUnsigned(text, option);
    if (value > std::numeric_limits<std::size_t>::max())
    {
        throw std::invalid_argument(option + " exceeds the platform size range");
    }
    return static_cast<std::size_t>(value);
}

dynamic_spare::SharingTopology parseTopology(const std::string &text)
{
    if (text == "none") return dynamic_spare::SharingTopology::NoSharing;
    if (text == "directional")
        return dynamic_spare::SharingTopology::Directional;
    if (text == "edge" || text == "row-pairwise")
        return dynamic_spare::SharingTopology::PairwiseEdge;
    if (text == "global") return dynamic_spare::SharingTopology::GlobalPool;
    throw std::invalid_argument("Unknown topology: " + text);
}

dynamic_spare::SolutionTakePolicy parseSolutionTake(
    const std::string &text)
{
    if (text == "legacy")
        return dynamic_spare::SolutionTakePolicy::Legacy;
    if (text == "early")
        return dynamic_spare::SolutionTakePolicy::Early;
    if (text == "group" || text == "group_compressed")
        return dynamic_spare::SolutionTakePolicy::GroupCompressed;
    throw std::invalid_argument("Unknown solution-take policy: " + text);
}

dynamic_spare::FaultCountModel parseFaultModel(const std::string &text)
{
    if (text == "uniform")
        return dynamic_spare::FaultCountModel::Uniform;
    if (text == "moderate")
        return dynamic_spare::FaultCountModel::ModerateImbalance;
    if (text == "strong")
        return dynamic_spare::FaultCountModel::StrongImbalance;
    if (text == "hotspot")
        return dynamic_spare::FaultCountModel::Hotspot;
    throw std::invalid_argument("Unknown fault model: " + text);
}

dynamic_spare::FaultSpatialModel parseSpatialModel(const std::string &text)
{
    if (text == "uniform")
        return dynamic_spare::FaultSpatialModel::Uniform;
    if (text == "mixed")
        return dynamic_spare::FaultSpatialModel::Mixed;
    if (text == "clustered")
        return dynamic_spare::FaultSpatialModel::Clustered;
    throw std::invalid_argument("Unknown spatial model: " + text);
}

dynamic_spare::CamReuseGranularity parseGranularity(
    const std::string &text)
{
    if (text == "word" || text == "data-word")
        return dynamic_spare::CamReuseGranularity::DataWord;
    if (text == "cell") return dynamic_spare::CamReuseGranularity::Cell;
    throw std::invalid_argument("Unknown CAM granularity: " + text);
}

void applySramPolicy(
    sram_recam::SramRecamConfig &config,
    const std::string &text)
{
    if (text == "serial")
    {
        config.biraSearchPolicy = sram_recam::SearchPolicy::SramSerial;
        config.runtimeSearchPolicy = sram_recam::SearchPolicy::SramSerial;
        config.addressParallelism = 1;
        config.hybridReadParallelism = 1;
        config.runtimeParallelism = 1;
        return;
    }
    if (text == "wide")
    {
        config.biraSearchPolicy = sram_recam::SearchPolicy::SramWide;
        config.runtimeSearchPolicy = sram_recam::SearchPolicy::SramWide;
        config.addressParallelism = 1;
        config.hybridReadParallelism =
            std::numeric_limits<std::uint32_t>::max();
        config.runtimeParallelism = 1;
        return;
    }
    const std::string prefix = "chunked:";
    if (text.size() > prefix.size() &&
        text.substr(0, prefix.size()) == prefix)
    {
        const std::uint64_t value = parseUnsigned(
            text.substr(prefix.size()), "--sram-policy");
        if (value == 0 || value > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::invalid_argument(
                "--sram-policy parallelism is outside uint32 range");
        }
        config.biraSearchPolicy = sram_recam::SearchPolicy::SramChunked;
        config.runtimeSearchPolicy = sram_recam::SearchPolicy::SramChunked;
        config.addressParallelism = static_cast<std::uint32_t>(value);
        config.hybridReadParallelism = static_cast<std::uint32_t>(value);
        config.runtimeParallelism = static_cast<std::uint32_t>(value);
        return;
    }
    throw std::invalid_argument("Unknown SRAM policy: " + text);
}

RepairGroupAddress addressOf(const FaultGroup &faults)
{
    for (const auto &subarrayFaults : faults)
    {
        if (!subarrayFaults.empty())
        {
            const Fault &fault = subarrayFaults.front();
            return {
                fault.HBMID,
                fault.ChannelID,
                fault.BankID,
                fault.SubarrayGroupID};
        }
    }
    throw std::invalid_argument(
        "A file-provided repair group cannot be completely empty");
}

RepairGroupAddress referenceAddress(std::size_t linearGroup)
{
    constexpr std::size_t kBanksPerDomain = 4;
    constexpr std::size_t kGroupsPerBank = 64;
    const std::size_t groupsPerDomain = kBanksPerDomain * kGroupsPerBank;
    return RepairGroupAddress{
        0,
        static_cast<int>(linearGroup / groupsPerDomain),
        static_cast<int>((linearGroup % groupsPerDomain) / kGroupsPerBank),
        static_cast<int>(linearGroup % kGroupsPerBank)};
}

void applyAddress(FaultGroup &faults, const RepairGroupAddress &address)
{
    for (std::size_t subarray = 0;
         subarray < dynamic_spare::kSubarrayCount; ++subarray)
    {
        for (Fault &fault : faults[subarray])
        {
            fault.HBMID = address.device;
            fault.ChannelID = address.domain;
            fault.BankID = address.bank;
            fault.SubarrayGroupID = address.group;
            fault.SubarrayID = static_cast<int>(subarray);
        }
    }
}

void writeResolvedFaultCorpus(
    const std::filesystem::path &path,
    const std::vector<RepairGroupInput> &inputs,
    const dynamic_spare::SimulationConfig &config,
    bool generated)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open())
    {
        throw std::runtime_error(
            "Unable to write resolved fault corpus: " + path.string());
    }
    output
        << "# FAULT_FILE_PHYSICAL_V1\n"
        << "# source=" << (generated ? "generated" : "file_provided")
        << "\n# fault_model="
        << (generated ? dynamic_spare::toString(config.faultCountModel)
                      : "file_provided")
        << "\n# spatial_model="
        << (generated ? dynamic_spare::toString(config.faultSpatialModel)
                      : "file_provided")
        << "\n# seed=" << config.randomSeed
        << "\n# columns=HBMID ChannelID BankID SubarrayGroupID SubarrayID Row CellCol\n";
    for (const RepairGroupInput &input : inputs)
    {
        for (const auto &subarrayFaults : input.faults)
        {
            for (const Fault &fault : subarrayFaults)
            {
                output
                    << fault.HBMID << ' ' << fault.ChannelID << ' '
                    << fault.BankID << ' ' << fault.SubarrayGroupID << ' '
                    << fault.SubarrayID << ' ' << fault.r << ' ' << fault.c
                    << '\n';
            }
        }
    }
    if (!output)
    {
        throw std::runtime_error(
            "Failed while writing resolved fault corpus: " + path.string());
    }
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
        dynamic_spare::SimulationConfig groupConfig;
        groupConfig.spareRows = parsePositiveInt(argv[1], "Rs");
        groupConfig.spareColumns = parsePositiveInt(argv[2], "Cs");
        groupConfig.memoryRows = 512;
        groupConfig.memoryColumns = 8192;
        groupConfig.faultCount = 3;
        groupConfig.simulationRuns = 1;
        groupConfig.topology = dynamic_spare::SharingTopology::NoSharing;
        groupConfig.usePaperCamReuseCapacity = false;
        groupConfig.bufferCamEntries = 0;
        groupConfig.modifiers.localFirst = true;

        dynamic_spare::HierarchicalRecamConfig hierarchicalConfig;
        std::optional<std::filesystem::path> simplifiedFaultFile;
        std::optional<std::filesystem::path> countFramedFaultFile;
        bool canonicalFour = false;
        bool faultModelSpecified = false;
        bool spatialModelSpecified = false;
        bool writeFaultCorpus = false;
        bool writeAreaManifest = false;
        sram_recam::SramRecamConfig sramConfig;
        std::filesystem::path outputDirectory =
            "reports/hierarchical_recam";
        std::size_t generatedGroups = 1;

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

            if (option == "--simplified-fault-file")
                simplifiedFaultFile = requireValue();
            else if (option == "--fault-file")
                countFramedFaultFile = requireValue();
            else if (option == "--groups")
                generatedGroups = parseSize(requireValue(), option);
            else if (option == "--fault-count")
                groupConfig.faultCount = parseUnsigned(requireValue(), option);
            else if (option == "--seed")
                groupConfig.randomSeed = parseUnsigned(requireValue(), option);
            else if (option == "--fault-model")
            {
                groupConfig.faultCountModel =
                    parseFaultModel(requireValue());
                faultModelSpecified = true;
            }
            else if (option == "--spatial")
            {
                groupConfig.faultSpatialModel =
                    parseSpatialModel(requireValue());
                spatialModelSpecified = true;
            }
            else if (option == "--memory-rows" ||
                     option == "--memory-columns")
            {
                const std::uint64_t value = parseUnsigned(requireValue(), option);
                if (value == 0 || value > std::numeric_limits<std::uint32_t>::max())
                    throw std::invalid_argument(option + " is outside uint32 range");
                if (option == "--memory-rows")
                    groupConfig.memoryRows = static_cast<std::uint32_t>(value);
                else
                    groupConfig.memoryColumns = static_cast<std::uint32_t>(value);
            }
            else if (option == "--online-global-reuse-entries")
                hierarchicalConfig.onlineGlobalReuseEntries =
                    parseSize(requireValue(), option);
            else if (option == "--bira-engines")
                hierarchicalConfig.biraEngineCount =
                    parseSize(requireValue(), option);
            else if (option == "--data-word-bits")
            {
                const std::uint64_t value = parseUnsigned(requireValue(), option);
                if (value == 0 || value > std::numeric_limits<std::uint32_t>::max())
                    throw std::invalid_argument(option + " is outside uint32 range");
                hierarchicalConfig.dataWordBits =
                    static_cast<std::uint32_t>(value);
            }
            else if (option == "--cam-granularity")
                hierarchicalConfig.camReuseGranularity =
                    parseGranularity(requireValue());
            else if (option == "--canonical-four")
            {
                canonicalFour = true;
                hierarchicalConfig.modelDecoupledFifo = true;
            }
            else if (option == "--sram-policy")
                applySramPolicy(sramConfig, requireValue());
            else if (option == "--sram-matrix-policy")
            {
                const std::string value = requireValue();
                if (value == "scan")
                    sramConfig.matrixPolicy =
                        sram_recam::MatrixPolicy::SramScanRebuild;
                else if (value == "shadow")
                    sramConfig.matrixPolicy =
                        sram_recam::MatrixPolicy::ShadowMatrix;
                else
                    throw std::invalid_argument(
                        "Unknown SRAM matrix policy: " + value);
            }
            else if (option == "--registered-sram-search")
                sramConfig.useAbstractSearchRoundModel = false;
            else if (option == "--bist-decoupled-fifo")
                hierarchicalConfig.modelDecoupledFifo = true;
            else if (option == "--bist-cycles-per-word")
                hierarchicalConfig.bistCyclesPerWord =
                    parseUnsigned(requireValue(), option);
            else if (option == "--topology")
                groupConfig.topology = parseTopology(requireValue());
            else if (option == "--shared-rows")
                groupConfig.sharedRows =
                    parseNonnegativeInt(requireValue(), option);
            else if (option == "--shared-columns")
                groupConfig.sharedColumns =
                    parseNonnegativeInt(requireValue(), option);
            else if (option == "--local-first")
                groupConfig.modifiers.localFirst = true;
            else if (option == "--max-borrows")
                groupConfig.modifiers.maximumGroupBorrowedSpares =
                    parseNonnegativeInt(requireValue(), option);
            else if (option == "--solution-take")
                groupConfig.solutionTakePolicy =
                    parseSolutionTake(requireValue());
            else if (option == "--output-dir")
                outputDirectory = requireValue();
            else if (option == "--write-fault-corpus")
                writeFaultCorpus = true;
            else if (option == "--write-area-manifest")
                writeAreaManifest = true;
            else if (option == "--help" || option == "-h")
            {
                printUsage(argv[0]);
                return 0;
            }
            else
                throw std::invalid_argument("Unknown option: " + option);
        }

        if (generatedGroups == 0)
        {
            throw std::invalid_argument("--groups must be positive");
        }
        if (writeAreaManifest && !canonicalFour)
        {
            throw std::invalid_argument(
                "--write-area-manifest requires --canonical-four");
        }
        if (simplifiedFaultFile.has_value() &&
            countFramedFaultFile.has_value())
        {
            throw std::invalid_argument(
                "--simplified-fault-file and --fault-file are mutually "
                "exclusive");
        }
        if ((simplifiedFaultFile.has_value() ||
             countFramedFaultFile.has_value()) &&
            (faultModelSpecified || spatialModelSpecified))
        {
            throw std::invalid_argument(
                "--fault-model/--spatial apply only to generated input");
        }
        if (generatedGroups >
            hierarchicalConfig.architectureTotalRepairGroups)
        {
            throw std::invalid_argument(
                "--groups exceeds the 2048-group WoW-v1.0 device geometry");
        }
        if (groupConfig.topology ==
            dynamic_spare::SharingTopology::GlobalPool)
        {
            if (groupConfig.sharedRows > groupConfig.spareRows ||
                groupConfig.sharedColumns > groupConfig.spareColumns)
            {
                throw std::invalid_argument(
                    "Global shared capacity exceeds local Rs/Cs");
            }
            groupConfig.globalPool =
                dynamic_spare::GlobalPoolConfiguration{
                    groupConfig.spareRows - groupConfig.sharedRows,
                    groupConfig.spareColumns - groupConfig.sharedColumns,
                    static_cast<int>(dynamic_spare::kSubarrayCount) *
                        groupConfig.sharedRows,
                    static_cast<int>(dynamic_spare::kSubarrayCount) *
                        groupConfig.sharedColumns};
        }
        groupConfig.dataWidthBits = hierarchicalConfig.dataWordBits;
        hierarchicalConfig.validateDramConfig(groupConfig);

        std::vector<RepairGroupInput> inputs;
        if (simplifiedFaultFile.has_value() ||
            countFramedFaultFile.has_value())
        {
            std::vector<FaultGroup> groups = simplifiedFaultFile.has_value()
                ? dynamic_spare::loadSimplifiedFaultGroups(
                      *simplifiedFaultFile, groupConfig)
                : dynamic_spare::loadCountFramedFaultGroups(
                      *countFramedFaultFile, groupConfig);
            inputs.reserve(groups.size());
            for (FaultGroup &faults : groups)
            {
                const RepairGroupAddress address = addressOf(faults);
                inputs.push_back({address, std::move(faults)});
            }
        }
        else
        {
            groupConfig.simulationRuns = generatedGroups;
            dynamic_spare::DynamicFaultGenerator generator(groupConfig);
            std::vector<FaultGroup> groups =
                generator.generateRuns(generatedGroups);
            inputs.reserve(groups.size());
            for (std::size_t index = 0; index < groups.size(); ++index)
            {
                const RepairGroupAddress address = referenceAddress(index);
                applyAddress(groups[index], address);
                inputs.push_back({address, std::move(groups[index])});
            }
        }

        if (writeFaultCorpus)
        {
            writeResolvedFaultCorpus(
                outputDirectory / "fault_corpus.txt",
                inputs,
                groupConfig,
                !simplifiedFaultFile.has_value() &&
                    !countFramedFaultFile.has_value());
        }

        if (canonicalFour)
        {
            dynamic_spare::CanonicalExperimentConfig experiment;
            experiment.sharingConfig = groupConfig;
            experiment.hierarchicalConfig = hierarchicalConfig;
            experiment.sramConfig = sramConfig;
            const auto results = dynamic_spare::runCanonicalFour(
                inputs, experiment);
            dynamic_spare::CanonicalExperimentReporter::write(
                outputDirectory, inputs, results);
            if (writeAreaManifest)
            {
                dynamic_spare::CanonicalExperimentReporter::writeAreaManifest(
                    outputDirectory, results);
            }
            for (const auto &run : results)
            {
                std::cout
                    << "configuration=" << dynamic_spare::toString(run.id)
                    << " storage="
                    << dynamic_spare::toString(
                           run.device.storageTechnology)
                    << " repairable_groups="
                    << run.device.repairableGroupCount << '/'
                    << run.device.modeledGroupCount
                    << " latency_after_bist_max="
                    << run.device.maximumSolutionLatencyAfterBistCycles
                    << '\n';
            }
            std::cout << "Wrote canonical-four outputs to "
                      << outputDirectory.string() << '\n';
            return 0;
        }

        dynamic_spare::DeviceRepairScheduler scheduler;
        const dynamic_spare::DeviceRepairResult result = scheduler.run(
            inputs, groupConfig, hierarchicalConfig);

        std::filesystem::create_directories(outputDirectory);
        dynamic_spare::HierarchicalCsvReporter::writeDeviceSummary(
            outputDirectory / "device_summary.csv",
            result, groupConfig, hierarchicalConfig);
        dynamic_spare::HierarchicalCsvReporter::writeGroups(
            outputDirectory / "groups.csv", result, hierarchicalConfig);
        dynamic_spare::HierarchicalCsvReporter::writeBiraEngines(
            outputDirectory / "bira_engines.csv", result);

        std::cout
            << "architecture=" << result.architectureVersion << '\n'
            << " simulator=" << result.simulatorVersion << '\n'
            << " cam_scope=" << result.camScope << '\n'
            << " groups=" << result.modeledGroupCount << '\n'
            << " repairable_groups=" << result.repairableGroupCount << '\n'
            << " cam_capacity_source="
            << dynamic_spare::toString(result.onlineCamCapacitySource) << '\n'
            << " global_cam=" << result.globalCam.entriesReserved << '/'
            << result.globalCam.capacity << '\n'
            << " cam_overflow=" << result.globalCam.allocationFailureCount << '\n'
            << " device_success=" << (result.deviceRepairSuccess ? 1 : 0)
            << '\n';
        std::cout << "Wrote hierarchical v2 CSVs to "
                  << outputDirectory.string() << '\n';
    }
    catch (const std::exception &error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
