#include "CanonicalExperiment.hpp"
#include "DynamicFaultGenerator.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

std::string readFile(const std::filesystem::path &path)
{
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("Unable to read " + path.string());
    std::ostringstream content;
    content << input.rdbuf();
    return content.str();
}

std::string firstTwoLines(const std::filesystem::path &path)
{
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("Unable to read " + path.string());
    std::string header;
    std::string row;
    std::getline(input, header);
    std::getline(input, row);
    return header + '\n' + row + '\n';
}

void writeFile(const std::filesystem::path &path, const std::string &text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::trunc);
    if (!output)
        throw std::runtime_error("Unable to write " + path.string());
    output << text;
}

} // namespace

int main(int argc, char *argv[])
{
    try
    {
        require(argc == 4,
                "Usage: canonical_experiment_test GOLDEN ACTUAL OUTDIR");
        dynamic_spare::SimulationConfig sharing;
        sharing.spareRows = 2;
        sharing.spareColumns = 2;
        sharing.sharedRows = 1;
        sharing.sharedColumns = 0;
        sharing.topology = dynamic_spare::SharingTopology::PairwiseEdge;
        sharing.modifiers.localFirst = true;
        sharing.modifiers.maximumGroupBorrowedSpares = 1;
        sharing.memoryRows = 64;
        sharing.memoryColumns = 64;
        sharing.dataWidthBits = 16;
        sharing.faultCount = 20;
        sharing.simulationRuns = 8;
        sharing.randomSeed = 20260820;
        sharing.usePaperCamReuseCapacity = false;
        sharing.bufferCamEntries = 0;
        sharing.validate();

        dynamic_spare::DynamicFaultGenerator generator(sharing);
        std::vector<dynamic_spare::FaultGroup> generated =
            generator.generateRuns(8);
        std::vector<dynamic_spare::RepairGroupInput> inputs;
        for (std::size_t index = 0; index < generated.size(); ++index)
        {
            dynamic_spare::RepairGroupAddress address{
                0, 0, 0, static_cast<int>(index)};
            for (std::size_t subarray = 0;
                 subarray < dynamic_spare::kSubarrayCount; ++subarray)
            {
                for (Fault &fault : generated[index][subarray])
                {
                    fault.HBMID = address.device;
                    fault.ChannelID = address.domain;
                    fault.BankID = address.bank;
                    fault.SubarrayGroupID = address.group;
                    fault.SubarrayID = static_cast<int>(subarray);
                }
            }
            inputs.push_back({address, std::move(generated[index])});
        }

        dynamic_spare::CanonicalExperimentConfig experiment;
        experiment.sharingConfig = sharing;
        experiment.hierarchicalConfig.dataWordBits = 16;
        experiment.hierarchicalConfig.modelDecoupledFifo = true;
        experiment.hierarchicalConfig.bistCyclesPerWord = 1;
        experiment.sramConfig.biraSearchPolicy =
            sram_recam::SearchPolicy::SramChunked;
        experiment.sramConfig.runtimeSearchPolicy =
            sram_recam::SearchPolicy::SramChunked;
        experiment.sramConfig.addressParallelism = 2;
        experiment.sramConfig.hybridReadParallelism = 2;
        experiment.sramConfig.runtimeParallelism = 2;

        const auto results = dynamic_spare::runCanonicalFour(
            inputs, experiment);
        require(results.size() == 4 &&
                    results[0].id == dynamic_spare::
                        CanonicalConfigurationId::B0CamNoSharing &&
                    results[3].id == dynamic_spare::
                        CanonicalConfigurationId::B3SramSharing,
                "Canonical runner did not return B0/B1/B2/B3 in order");
        require(results[0].device.repairableGroupCount ==
                    results[2].device.repairableGroupCount &&
                    results[1].device.repairableGroupCount ==
                    results[3].device.repairableGroupCount,
                "CAM/SRAM canonical repair results are not equivalent");

        const std::filesystem::path outputDirectory = argv[3];
        dynamic_spare::CanonicalExperimentReporter::write(
            outputDirectory, inputs, results);
        dynamic_spare::CanonicalExperimentReporter::writeAreaManifest(
            outputDirectory, results);
        const std::string manifest = readFile(
            outputDirectory / "hardware_area_manifest.csv");
        require(manifest.find("B0_CAM_NO_SHARING,BIRA_ENGINE,ADDRESS_STORE,CAM") !=
                    std::string::npos &&
                    manifest.find("B3_SRAM_SHARING,BIRA_ENGINE,ADDRESS_STORE,SRAM") !=
                    std::string::npos &&
                    manifest.find("FAULT_COLLECTION_FIFO") != std::string::npos,
                "Canonical area manifest is missing required B0/B3/FIFO rows");
        const std::string actual = firstTwoLines(
            outputDirectory / "canonical_four_summary.csv");
        writeFile(argv[2], actual);
        require(actual == readFile(argv[1]),
                "B0 golden result changed; inspect " +
                std::string(argv[2]));

        std::cout << "Canonical B0/B1/B2/B3 tests passed\n"
                  << "B0 golden: " << argv[1] << '\n'
                  << "B0 actual: " << argv[2] << '\n';
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Canonical experiment test failed: "
                  << error.what() << '\n';
        return 1;
    }
}
