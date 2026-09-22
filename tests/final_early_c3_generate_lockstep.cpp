#include "DynamicFaultGenerator.hpp"
#include "DynamicRepairSimulator.hpp"
#include "SimulationConfig.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
using namespace dynamic_spare;

constexpr std::size_t kVectorsPerLoad = 250;
constexpr std::array<std::uint64_t, 4> kLoads{{16, 20, 24, 28}};

SimulationConfig configFor(std::uint64_t faultCount)
{
    SimulationConfig config;
    config.spareRows = 2;
    config.spareColumns = 2;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    config.layout = GroupLayout::Grid2x2;
    config.topology = SharingTopology::Directional;
    config.solutionTakePolicy = SolutionTakePolicy::DirectionalV2Early;
    config.modifiers.maximumGroupBorrowedSpares = 1;
    config.usePaperCamReuseCapacity = true;
    config.faultCount = faultCount;
    config.simulationRuns = kVectorsPerLoad;
    config.randomSeed = 20260922;
    config.memoryRows = 1024;
    config.memoryColumns = 1024;
    config.faultCountModel = FaultCountModel::ModerateImbalance;
    config.faultSpatialModel = FaultSpatialModel::Mixed;
    return config;
}

void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}

int patternFor(const RepairAttemptResult &attempt)
{
    if (!attempt.repairSuccess || !attempt.successfulCandidateIndex.has_value())
        return 0;
    return static_cast<int>(*attempt.successfulCandidateIndex + 1);
}

void run()
{
    const std::filesystem::path root = "tmp/date2026/final_early_c3";
    std::filesystem::create_directories(root);
    std::ofstream output(root / "model_b2_lockstep.csv");
    require(output.good(), "unable to write C3 Model-B2 lockstep corpus");
    output << "id,success,failure,slots,configs,patterns";
    for (int entry = 0; entry != 16; ++entry)
        output << ",valid" << entry << ",pattern" << entry;
    output << '\n';

    DynamicRepairSimulator simulator;
    std::size_t id = 0;
    for (const std::uint64_t load : kLoads)
    {
        const SimulationConfig config = configFor(load);
        DynamicFaultGenerator generator(config);
        for (std::size_t group = 0; group != kVectorsPerLoad; ++group, ++id)
        {
            const GroupRepairResult result = simulator.run(
                generator.generate(group), config, group, true);
            output << id << ',' << (result.groupRepairSuccess ? 1 : 0) << ','
                   << (result.firstFailureSubarray.has_value()
                       ? static_cast<int>(*result.firstFailureSubarray) : -1)
                   << ',';
            for (std::size_t sa = 0; sa != 4; ++sa)
                output << ':' << (result.selectedAttemptIndices[sa].has_value()
                    ? static_cast<int>(*result.selectedAttemptIndices[sa]) : -1);
            output << ',';
            for (std::size_t sa = 0; sa != 4; ++sa)
                output << ':' << (result.selectedConfigIds[sa].has_value()
                    ? *result.selectedConfigIds[sa] : -1);
            output << ',';
            for (std::size_t sa = 0; sa != 4; ++sa)
                output << ':' << (result.selectedPatternIds[sa].has_value()
                    ? static_cast<int>(*result.selectedPatternIds[sa]) : -1);
            for (std::size_t sa = 0; sa != 4; ++sa)
                for (std::size_t slot = 0; slot != 4; ++slot)
                {
                    const RepairAttemptResult &attempt =
                        result.attemptsBySubarray[sa][slot];
                    output << ',' << (attempt.repairSuccess ? 1 : 0) << ','
                           << patternFor(attempt);
                }
            output << '\n';
        }
    }
    require(id == 1000, "C3 corpus must contain exactly 1000 vectors");
    std::cout << "FINAL_EARLY_C3_MODEL_B2_LOCKSTEP_CORPUS=1000\n";
}
} // namespace

int main()
{
    try { run(); }
    catch (const std::exception &error)
    {
        std::cerr << "FINAL_EARLY_C3_GENERATE FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
