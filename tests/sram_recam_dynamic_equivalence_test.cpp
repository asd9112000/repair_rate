#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "DynamicRepairSimulator.hpp"
#include "Fault.hpp"
#include "RepairResult.hpp"
#include "SimulationConfig.hpp"
#include "SramRecamModel.hpp"
#include "SramRecamSolverAdapter.hpp"

namespace
{

using dynamic_spare::FaultGroup;
using dynamic_spare::GroupRepairResult;
using dynamic_spare::RepairAttemptResult;

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

Fault makeFault(int subarray, int row, int column)
{
    Fault fault;
    fault.r = row;
    fault.c = column;
    fault.HBMID = 0;
    fault.ChannelID = 0;
    fault.BankID = 0;
    fault.SubarrayGroupID = 0;
    fault.SubarrayID = subarray;
    fault.faultType = FaultType::Cell;
    return fault;
}

FaultGroup deterministicFaultGroup()
{
    FaultGroup faults;

    // Five independent pivots exceed the local K=4 Address structure.  With
    // buffer=0, PE0 needs its directional extra column.  With paper K reuse,
    // the fifth pivot is served by the additional-pivot buffer instead.
    faults[0] = {
        makeFault(0, 0, 0),
        makeFault(0, 1, 1),
        makeFault(0, 2, 2),
        makeFault(0, 3, 3),
        makeFault(0, 4, 4)};

    // The last fault matches the first pivot row and the second pivot column,
    // exercising the requirement that an Address-SRAM scan find both.
    faults[1] = {
        makeFault(1, 8, 8),
        makeFault(1, 9, 9),
        makeFault(1, 8, 9)};

    faults[2] = {
        makeFault(2, 16, 16),
        makeFault(2, 16, 17)};
    faults[3] = {
        makeFault(3, 24, 24),
        makeFault(3, 25, 24)};
    return faults;
}

dynamic_spare::SimulationConfig simulationConfig(bool paperCapacity)
{
    dynamic_spare::SimulationConfig config;
    config.spareRows = 2;
    config.spareColumns = 2;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    config.topology = dynamic_spare::SharingTopology::Directional;
    config.modifiers.maximumGroupBorrowedSpares = 1;
    config.modifiers.localFirst = false;

    config.memoryRows = 64;
    config.memoryColumns = 64;
    config.dataWidthBits = 16;
    config.hybridCamEntryWidthBits = 16;
    config.simulationRuns = 1;
    config.randomSeed = paperCapacity ? 91002 : 91001;

    config.usePaperCamReuseCapacity = paperCapacity;
    config.bufferCamEntries = 0;

    // Functional-equivalence selection must not be perturbed by the legacy
    // storage-latency proxy.  SRAM-specific timing is carried independently
    // in RepairAttemptResult::sramRecam.
    config.latency.cam = {1, 1, 1};
    config.latency.sram = config.latency.cam;
    return config;
}

struct PolicyCase
{
    const char *name;
    sram_recam::SearchPolicy policy;
    std::uint32_t parallelism;
};

const std::array<PolicyCase, 4> kPolicies{{
    {"serial", sram_recam::SearchPolicy::SramSerial, 1},
    {"chunk2", sram_recam::SearchPolicy::SramChunked, 2},
    {"chunk4", sram_recam::SearchPolicy::SramChunked, 4},
    {"wide", sram_recam::SearchPolicy::SramWide, 1}}};

sram_recam::SramRecamConfig sramConfig(
    const dynamic_spare::SimulationConfig &simulation,
    const PolicyCase &policy)
{
    sram_recam::SramRecamConfig config;
    config.rows = simulation.memoryRows;
    config.cols = simulation.memoryColumns;
    config.spareRows = static_cast<std::uint32_t>(simulation.spareRows);
    config.spareCols = static_cast<std::uint32_t>(
        simulation.spareColumns);
    config.channels = 1;
    config.dataWordBits = simulation.dataWidthBits;
    config.biraSearchPolicy = policy.policy;
    config.runtimeSearchPolicy = policy.policy;
    config.addressParallelism = policy.parallelism;
    config.hybridReadParallelism = policy.parallelism;
    config.runtimeParallelism = policy.parallelism;
    config.matrixPolicy = sram_recam::MatrixPolicy::ShadowMatrix;
    config.analyzeAfterEachFault = false;
    config.camReuseEntries = simulation.usePaperCamReuseCapacity
        ? std::optional<std::uint32_t>{}
        : std::optional<std::uint32_t>{
              static_cast<std::uint32_t>(simulation.bufferCamEntries)};
    return config;
}

void compareAttempt(
    const RepairAttemptResult &cam,
    const RepairAttemptResult &sram,
    const PolicyCase &policy,
    std::size_t subarray,
    std::size_t attemptIndex)
{
    const std::string context = std::string(policy.name) + " PE" +
        std::to_string(subarray) + " attempt " +
        std::to_string(attemptIndex) + ": ";

    require(sram.runIndex == cam.runIndex &&
                sram.attemptIndex == cam.attemptIndex &&
                sram.stage == cam.stage &&
                sram.subarrayId == cam.subarrayId &&
                sram.availableRows == cam.availableRows &&
                sram.availableColumns == cam.availableColumns,
            context + "attempt identity/capacity differs from CAM");
    require(sram.repairSuccess == cam.repairSuccess,
            context + "repairSuccess differs from CAM");
    require(sram.isRepairable == cam.isRepairable,
            context + "isRepairable differs from CAM");
    require(sram.camStorageOverflow == cam.camStorageOverflow,
            context + "CAM overflow state differs from CAM");
    require(sram.faultCount == cam.faultCount,
            context + "fault count differs from CAM");
    require(sram.pivotFaultCount == cam.pivotFaultCount,
            context + "pivot count differs from CAM");
    require(sram.nonpivotFaultCount == cam.nonpivotFaultCount,
            context + "non-pivot count differs from CAM");
    require(sram.bufferedPivotFaultCount == cam.bufferedPivotFaultCount,
            context + "buffered-pivot count differs from CAM");
    require(sram.overflowPivotFaultCount == cam.overflowPivotFaultCount,
            context + "overflow-pivot count differs from CAM");
    require(sram.validCandidateIndices == cam.validCandidateIndices,
            context + "valid candidate indices differ from CAM");

    const bool nonzeroCapacity =
        sram.availableRows != 0 || sram.availableColumns != 0;
    if (nonzeroCapacity)
    {
        require(sram.sramRecam.has_value(),
                context + "SRAM_RECAM metrics are absent");
        const auto &metrics = *sram.sramRecam;
        require(metrics.addressClassificationMatchesCam,
                context + "classificationMatchesCam is false");
        require(metrics.biraPolicy == policy.policy,
                context + "reported BIRA policy is incorrect");
        require(metrics.geometry.k == static_cast<std::uint64_t>(
                    sram.availableRows + sram.availableColumns),
                context + "geometry K does not match attempt capacity");
        require(metrics.geometry.tempEntries ==
                    sram.bufferCamEntriesProvisioned,
                context + "temporary-buffer geometry differs from CAM");
        require(metrics.bira.faultsDetected == sram.faultCount,
                context + "BIRA fault count differs from the attempt");
        require(sram.biraLatency.storageTechnology ==
                    dynamic_spare::BiraStorageTechnology::Sram &&
                    sram.biraLatency.faultCollectionWorkCycles ==
                        metrics.bira.faultCollectionCycles &&
                    sram.biraLatency.repairAnalysisWorkCycles ==
                        metrics.bira.repairAnalysisCycles &&
                    sram.biraLatency.totalWorkCycles ==
                        metrics.bira.modeledCycles,
                context + "common BIRA latency differs from SRAM metrics");
    }
}

void compareGroup(
    const GroupRepairResult &cam,
    const GroupRepairResult &sram,
    const PolicyCase &policy)
{
    require(sram.groupRepairSuccess == cam.groupRepairSuccess,
            std::string(policy.name) +
                ": groupRepairSuccess differs from CAM");
    require(sram.selectedAttemptIndices == cam.selectedAttemptIndices,
            std::string(policy.name) +
                ": selectedAttemptIndices differ from CAM");
    require(sram.selectedCandidateIndices == cam.selectedCandidateIndices,
            std::string(policy.name) +
                ": selectedCandidateIndices differ from CAM");

    for (std::size_t subarray = 0;
         subarray < dynamic_spare::kSubarrayCount; ++subarray)
    {
        const auto &camAttempts = cam.attemptsBySubarray[subarray];
        const auto &sramAttempts = sram.attemptsBySubarray[subarray];
        require(sramAttempts.size() == camAttempts.size(),
                std::string(policy.name) + " PE" +
                    std::to_string(subarray) +
                    ": attempt count differs from CAM");
        for (std::size_t attempt = 0;
             attempt < camAttempts.size(); ++attempt)
        {
            compareAttempt(
                camAttempts[attempt], sramAttempts[attempt], policy,
                subarray, attempt);
        }
    }
}

void verifyBufferScenario(bool paperCapacity)
{
    const FaultGroup faults = deterministicFaultGroup();
    dynamic_spare::SimulationConfig camConfig =
        simulationConfig(paperCapacity);
    camConfig.storageMode = dynamic_spare::FaultInformationStorage::CAM;

    const dynamic_spare::DynamicRepairSimulator camSimulator;
    const GroupRepairResult cam = camSimulator.run(faults, camConfig, 0);

    require(cam.biraLatency.storageTechnology ==
                dynamic_spare::BiraStorageTechnology::Cam &&
                cam.biraLatency.repairAnalysisWorkCycles == 0,
            "CAM collection-only model included repair-analysis latency");

    require(cam.groupRepairSuccess,
            paperCapacity
                ? "Paper-K CAM scenario unexpectedly failed"
                : "buffer=0 CAM scenario was not repaired by sharing");
    require(!cam.attemptsBySubarray[0].empty(),
            "PE0 has no local CAM attempt");
    const RepairAttemptResult &local = cam.attemptsBySubarray[0].front();
    if (paperCapacity)
    {
        require(local.bufferCamEntriesProvisioned == 4 &&
                    local.bufferedPivotFaultCount == 1 &&
                    local.overflowPivotFaultCount == 0,
                "Paper-K trace did not exercise additional-pivot buffering");
    }
    else
    {
        require(local.bufferCamEntriesProvisioned == 0 &&
                    local.bufferedPivotFaultCount == 0 &&
                    local.overflowPivotFaultCount == 1,
                "buffer=0 trace did not exercise pivot overflow");
        require(cam.attemptsBySubarray[0].size() > 1 &&
                    cam.selectedAttemptIndices[0].has_value() &&
                    *cam.selectedAttemptIndices[0] != 0,
                "buffer=0 trace did not exercise a borrowed-capacity attempt");
    }

    for (const PolicyCase &policy : kPolicies)
    {
        dynamic_spare::SimulationConfig dynamicSramConfig = camConfig;
        dynamicSramConfig.storageMode =
            dynamic_spare::FaultInformationStorage::SRAM;
        const auto adapter =
            std::make_shared<dynamic_spare::SramRecamSolverAdapter>(
                sramConfig(dynamicSramConfig, policy));
        const dynamic_spare::DynamicRepairSimulator sramSimulator(adapter);
        const GroupRepairResult sram =
            sramSimulator.run(faults, dynamicSramConfig, 0);
        compareGroup(cam, sram, policy);
    }
}

} // namespace

int main()
{
    try
    {
        verifyBufferScenario(false);
        verifyBufferScenario(true);
        std::cout << "SRAM_RECAM dynamic equivalence tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "SRAM_RECAM dynamic equivalence test failed: "
                  << error.what() << '\n';
        return 1;
    }
}
