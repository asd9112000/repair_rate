#include "DirectionalMultiConfigAnalyzer.hpp"
#include "DynamicFaultGenerator.hpp"
#include "FaultAddress.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace
{

constexpr std::uint64_t kSeed = 20260914ULL;
constexpr std::size_t kGroupCount = 10000;

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

dynamic_spare::SimulationConfig frozenConfig()
{
    dynamic_spare::SimulationConfig config;
    config.spareRows = 2;
    config.spareColumns = 2;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    config.dataWidthBits = 256;
    config.rowAddressWidthBits = 9;
    config.columnAddressWidthBits = 13;
    config.faultCount = 28;
    config.simulationRuns = kGroupCount;
    config.randomSeed = kSeed;
    config.memoryRows = 512;
    config.memoryColumns = 8192;
    config.faultCountModel = dynamic_spare::FaultCountModel::Uniform;
    config.faultSpatialModel = dynamic_spare::FaultSpatialModel::Mixed;
    config.layout = dynamic_spare::GroupLayout::Grid2x2;
    config.topology = dynamic_spare::SharingTopology::Directional;
    config.modifiers.maximumGroupBorrowedSpares = 1;
    return config;
}

} // namespace

int main(int argc, char **argv)
{
    try
    {
        if (argc != 2)
        {
            throw std::invalid_argument("Expected one candidate-corpus path");
        }
        const std::filesystem::path outputPath(argv[1]);
        std::filesystem::create_directories(outputPath.parent_path());
        std::ofstream output(outputPath);
        require(output.good(), "Unable to create E0-L candidate corpus");
        output << "transaction_id,seed,fault_count_A,fault_count_B,fault_count_C,"
                  "fault_count_D,group_last_fault_accept_cycle,group_test_done_cycle";
        for (std::size_t sa = 0; sa < 4; ++sa)
        {
            for (std::size_t config = 0;
                 config < dynamic_spare::kDirectionalConfigCount; ++config)
            {
                output << ",pattern_" << static_cast<char>('A' + sa)
                       << "_cfg" << config;
            }
        }
        output << '\n';

        const dynamic_spare::SimulationConfig config = frozenConfig();
        dynamic_spare::DynamicFaultGenerator generator(config);
        const dynamic_spare::DirectionalMultiConfigAnalyzer analyzer;
        const dynamic_spare::SerialBistSchedule schedule;
        require(schedule.geometry.rows == config.memoryRows &&
                    schedule.geometry.cellColumns == config.memoryColumns &&
                    schedule.geometry.wordBits == config.dataWidthBits &&
                    schedule.cyclesPerWord == 1 &&
                    schedule.completionCycle() == 65536,
                "S1D serial-BIST geometry no longer matches the frozen recipe");

        std::size_t emitted = 0;
        for (std::size_t index = 0; index < kGroupCount; ++index)
        {
            const dynamic_spare::FaultGroup faults = generator.generate(index);
            std::optional<std::uint64_t> lastFault;
            output << "E0L-" << index << ',' << kSeed;
            for (std::size_t sa = 0; sa < 4; ++sa)
            {
                require(faults[sa].size() == 7,
                        "Uniform frozen corpus no longer provides lambda_SA=7");
                output << ',' << faults[sa].size();
                for (const Fault &fault : faults[sa])
                {
                    const std::uint64_t arrival = schedule.arrivalCycle(fault);
                    if (!lastFault || arrival > *lastFault)
                    {
                        lastFault = arrival;
                    }
                }
            }
            require(lastFault.has_value(), "Frozen corpus unexpectedly generated zero faults");
            output << ',' << *lastFault << ',' << schedule.completionCycle();
            for (std::size_t sa = 0; sa < 4; ++sa)
            {
                const auto analysis = analyzer.analyze(faults[sa], static_cast<int>(sa));
                for (const auto &candidate : analysis.configs)
                {
                    require(candidate.lowestValidPatternId <= 15,
                            "Candidate PatternID exceeds frozen four-bit RTL boundary");
                    output << ',' << static_cast<unsigned int>(candidate.lowestValidPatternId);
                }
            }
            output << '\n';
            ++emitted;
        }
        require(emitted == kGroupCount, "Candidate corpus row count mismatch");
        std::cout << "E0L_CANDIDATE_CORPUS rows=" << emitted
                  << " seed=" << kSeed << " lambda_sa=7"
                  << " serial_bist_completion=65536 candidate_interface=PASS\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "E0L_CANDIDATE_CORPUS FAIL: " << error.what() << '\n';
        return 1;
    }
}
