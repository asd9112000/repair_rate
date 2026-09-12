#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "HierarchicalRecamSimulator.hpp"
#include "SramRecamSolverAdapter.hpp"

namespace
{

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

Fault makeFault(
    const dynamic_spare::RepairGroupAddress &address,
    int subarray,
    int row,
    int column)
{
    Fault fault{};
    fault.HBMID = address.device;
    fault.ChannelID = address.domain;
    fault.BankID = address.bank;
    fault.SubarrayGroupID = address.group;
    fault.SubarrayID = subarray;
    fault.r = row;
    fault.c = column;
    return fault;
}

dynamic_spare::RepairGroupInput oneCamWordGroup(
    int domain,
    int bank,
    int group,
    int subarray = 0)
{
    dynamic_spare::RepairGroupInput input;
    input.address = {0, domain, bank, group};
    input.faults[static_cast<std::size_t>(subarray)] = {
        makeFault(input.address, subarray, 0, 0),
        makeFault(input.address, subarray, 1, 256),
        makeFault(input.address, subarray, 2, 512)};
    return input;
}

dynamic_spare::SimulationConfig baseConfig()
{
    dynamic_spare::SimulationConfig config;
    config.spareRows = 1;
    config.spareColumns = 1;
    config.memoryRows = 512;
    config.memoryColumns = 8192;
    config.topology = dynamic_spare::SharingTopology::NoSharing;
    config.usePaperCamReuseCapacity = false;
    config.bufferCamEntries = 0;
    config.modifiers.localFirst = true;
    config.validate();
    return config;
}

dynamic_spare::HierarchicalRecamConfig hierarchicalConfig(
    std::size_t capacity)
{
    dynamic_spare::HierarchicalRecamConfig config;
    config.onlineGlobalReuseEntries = capacity;
    config.biraEngineCount = 1;
    config.dataWordBits = 256;
    config.camReuseGranularity =
        dynamic_spare::CamReuseGranularity::DataWord;
    return config;
}

void verifyPaperDerivedCapacityAndPhysicalAccounting()
{
    dynamic_spare::SimulationConfig groupConfig = baseConfig();
    groupConfig.spareRows = 2;
    groupConfig.spareColumns = 2;
    groupConfig.validate();

    dynamic_spare::HierarchicalRecamConfig paperConfig;
    paperConfig.biraEngineCount = 1;
    dynamic_spare::DeviceRepairScheduler scheduler;
    const auto paper = scheduler.run({}, groupConfig, paperConfig);

    require(paper.globalCam.capacity == 4 &&
                paper.onlineCamCapacitySource ==
                    dynamic_spare::OnlineCamCapacitySource::PaperRsPlusCs,
            "Default online capacity is not the paper Rs+Cs Address CAM size");
    require(paper.hardware.paperAddressCamEntriesPerStructure == 4 &&
                paper.hardware.paperHybridCamEntriesPerStructure == 4 &&
                paper.hardware.physicalAddressCamEntriesPerStructure == 4 &&
                paper.hardware.physicalHybridCamEntriesPerStructure == 4 &&
                paper.hardware.totalPhysicalAddressCamEntries == 4 &&
                paper.hardware.totalPhysicalHybridCamEntries == 4,
            "Paper CAM entry hardware accounting is incorrect for Rs=Cs=2");

    dynamic_spare::HierarchicalRecamConfig enlargedConfig;
    enlargedConfig.onlineGlobalReuseEntries = 32;
    enlargedConfig.biraEngineCount = 2;
    const auto enlarged = scheduler.run({}, groupConfig, enlargedConfig);
    require(enlarged.globalCam.capacity == 32 &&
                enlarged.onlineCamCapacitySource ==
                    dynamic_spare::OnlineCamCapacitySource::ExplicitOverride,
            "Explicit enlarged-CAM capacity was not retained");
    require(enlarged.hardware.physicalAddressCamEntriesPerStructure == 32 &&
                enlarged.hardware.physicalHybridCamEntriesPerStructure == 32 &&
                enlarged.hardware.totalPhysicalAddressCamEntries == 64 &&
                enlarged.hardware.totalPhysicalHybridCamEntries == 64,
            "Enlarged CAM was not charged to physical Address/Hybrid entries");

    dynamic_spare::HierarchicalRecamConfig restrictedConfig;
    restrictedConfig.onlineGlobalReuseEntries = 0;
    const auto restricted = scheduler.run({}, groupConfig, restrictedConfig);
    require(restricted.globalCam.capacity == 0 &&
                restricted.hardware.physicalAddressCamEntriesPerStructure == 4 &&
                restricted.hardware.physicalHybridCamEntriesPerStructure == 4,
            "Restricting online capacity incorrectly shrank offline RECAM CAM");
}

void verifyDramGeometryValidation()
{
    dynamic_spare::SimulationConfig groupConfig = baseConfig();
    dynamic_spare::HierarchicalRecamConfig hierarchy;
    hierarchy.validateDramConfig(groupConfig);

    groupConfig.memoryColumns = 8191;
    bool rejectedUnalignedWordGeometry = false;
    try
    {
        hierarchy.validateDramConfig(groupConfig);
    }
    catch (const std::invalid_argument &)
    {
        rejectedUnalignedWordGeometry = true;
    }
    require(rejectedUnalignedWordGeometry,
            "DRAM validation accepted a partial data word per row");

    groupConfig = baseConfig();
    hierarchy.architectureTotalRepairGroups = 2047;
    bool rejectedInconsistentHierarchy = false;
    try
    {
        hierarchy.validateDramConfig(groupConfig);
    }
    catch (const std::invalid_argument &)
    {
        rejectedInconsistentHierarchy = true;
    }
    require(rejectedInconsistentHierarchy,
            "DRAM validation accepted inconsistent hierarchy totals");
}

void verifyNoPhysicalCamMultiplication()
{
    dynamic_spare::DeviceRepairScheduler scheduler;
    const auto result = scheduler.run({}, baseConfig(), hierarchicalConfig(32));
    require(result.hardware.totalSubarrays == 8192,
            "WoW hardware accounting lost the 8192-SA reference geometry");
    require(result.hardware.biraEngineCount == 1,
            "Default BIRA engine count is not one");
    require(result.hardware.addressCamStructures == 1 &&
                result.hardware.hybridCamStructures == 1 &&
                result.hardware.matrixAnalyzers == 1,
            "BIRA scratch hardware was multiplied by subarray count");
    require(result.hardware.globalOnlineCamPools == 1 &&
                result.hardware.separatelyCountedOnlineCamPools == 0 &&
                result.hardware.offlineOnlineCamModeReused,
            "Offline and online CAM hardware was double-counted");
}

void verifyScratchReuseAndPersistentSolution()
{
    dynamic_spare::DeviceRepairScheduler scheduler;
    const std::vector<dynamic_spare::RepairGroupInput> groups{
        oneCamWordGroup(0, 0, 0),
        oneCamWordGroup(0, 0, 1)};
    const auto result = scheduler.run(groups, baseConfig(), hierarchicalConfig(4));

    require(result.biraEngines.size() == 1 &&
                result.biraEngines.front().groupsProcessed == 2 &&
                result.biraEngines.front().scratchClearCount == 2,
            "One BIRA engine did not process/clear both groups sequentially");
    require(result.biraEngines.front().transientFaultsInScratch == 0,
            "Transient group faults survived BIRA scratch clear");
    require(result.groups.front().totalBiraCycles ==
                result.groups.front().biraLatency.totalWorkCycles &&
                !result.groups.front().biraLatency.bistTimelineModeled,
            "Hierarchical BIRA work was not exposed through common latency");
    require(result.groups.front().persistentCamAssignments.size() == 1 &&
                result.groups.back().persistentCamAssignments.size() == 1 &&
                result.globalCam.entriesReserved == 2,
            "Committed CAM-reuse assignments did not survive scratch reuse");
}

void verifyGlobalContentionAndFullTags()
{
    dynamic_spare::DeviceRepairScheduler scheduler;
    const std::vector<dynamic_spare::RepairGroupInput> groups{
        oneCamWordGroup(0, 0, 0),
        oneCamWordGroup(0, 0, 1),
        oneCamWordGroup(0, 0, 2)};
    const auto result = scheduler.run(groups, baseConfig(), hierarchicalConfig(2));

    require(result.repairableGroupCount == 2 &&
                result.groups[0].metrics.finalSuccess &&
                result.groups[1].metrics.finalSuccess &&
                !result.groups[2].metrics.finalSuccess,
            "Three groups did not contend for one capacity-2 global pool");
    require(result.globalCam.entriesReserved == 2 &&
                result.globalCam.allocationSuccessCount == 2 &&
                result.globalCam.allocationFailureCount == 1,
            "Global CAM occupancy/overflow accounting is incorrect");
    require(result.groups[0].persistentCamAssignments.front().group !=
                result.groups[1].persistentCamAssignments.front().group,
            "Full global tags did not disambiguate identical local addresses");
}

dynamic_spare::BufferRepairMapping mappingAt(int row, int column)
{
    dynamic_spare::BufferRepairMapping mapping;
    mapping.HBMID = 0;
    mapping.ChannelID = 1;
    mapping.BankID = 2;
    mapping.SubarrayGroupID = 3;
    mapping.SubarrayID = 0;
    mapping.sourceRow = row;
    mapping.sourceColumn = column;
    return mapping;
}

void verifySameWordDeduplication()
{
    dynamic_spare::GlobalOnlineRepairPool pool(
        1, dynamic_spare::CamReuseGranularity::DataWord, 256);
    const auto allocation = pool.reserve({
        mappingAt(7, 10),
        mappingAt(7, 200)});
    require(allocation.wordsRequestedBeforeDedup == 2 &&
                allocation.wordsRequestedAfterDedup == 1 &&
                allocation.entriesNewlyReserved == 1 &&
                allocation.success(),
            "Two cells in one 256-bit repair word consumed two CAM entries");
}

void verifySharingReducesCamDemand()
{
    dynamic_spare::DeviceRepairScheduler scheduler;
    const auto input = oneCamWordGroup(0, 0, 0, 2);

    const auto noSharing = scheduler.run(
        {input}, baseConfig(), hierarchicalConfig(4));
    require(noSharing.groups.front().metrics.camWordsNeeded == 1 &&
                noSharing.groups.front().metrics.repairSource ==
                    dynamic_spare::RepairSource::CamReuseRequired,
            "No-sharing fixture does not require its expected Tier-2 word");

    auto sharingConfig = baseConfig();
    sharingConfig.topology = dynamic_spare::SharingTopology::Directional;
    sharingConfig.sharedRows = 1;
    sharingConfig.sharedColumns = 0;
    sharingConfig.modifiers.maximumGroupBorrowedSpares = 1;
    sharingConfig.modifiers.localFirst = true;
    sharingConfig.validate();
    const auto sharing = scheduler.run(
        {input}, sharingConfig, hierarchicalConfig(4));
    require(sharing.groups.front().metrics.finalSuccess &&
                sharing.groups.front().metrics.sharingUsed &&
                sharing.groups.front().metrics.camWordsNeeded == 0 &&
                sharing.groups.front().metrics.repairSource ==
                    dynamic_spare::RepairSource::SharingRequired,
            "One borrowed row did not reduce Tier-2 demand from one to zero");
}

dynamic_spare::RepairGroupInput fifoFixture()
{
    dynamic_spare::RepairGroupInput input;
    input.address = {0, 0, 0, 0};
    for (int subarray = 0; subarray < 2; ++subarray)
    {
        for (int column = 0; column < 4; ++column)
        {
            input.faults[static_cast<std::size_t>(subarray)].push_back(
                makeFault(input.address, subarray, 0, column));
        }
    }
    return input;
}

void verifySramBackendAndSerialBistTimeline()
{
    dynamic_spare::SimulationConfig camConfig;
    camConfig.spareRows = 2;
    camConfig.spareColumns = 2;
    camConfig.memoryRows = 1;
    camConfig.memoryColumns = 4;
    camConfig.faultCount = 8;
    camConfig.dataWidthBits = 1;
    camConfig.topology = dynamic_spare::SharingTopology::NoSharing;
    camConfig.usePaperCamReuseCapacity = false;
    camConfig.bufferCamEntries = 0;
    camConfig.storageMode = dynamic_spare::FaultInformationStorage::CAM;
    camConfig.validate();

    dynamic_spare::HierarchicalRecamConfig hierarchy;
    hierarchy.onlineGlobalReuseEntries = 4;
    hierarchy.dataWordBits = 1;
    hierarchy.modelDecoupledFifo = true;
    hierarchy.bistCyclesPerWord = 1;

    dynamic_spare::DeviceRepairScheduler camScheduler;
    const auto cam = camScheduler.run(
        {fifoFixture()}, camConfig, hierarchy);

    dynamic_spare::SimulationConfig sramDynamic = camConfig;
    sramDynamic.storageMode = dynamic_spare::FaultInformationStorage::SRAM;
    sram_recam::SramRecamConfig sramConfig;
    sramConfig.rows = 1;
    sramConfig.cols = 4;
    sramConfig.spareRows = 2;
    sramConfig.spareCols = 2;
    sramConfig.channels = 1;
    sramConfig.dataWordBits = 1;
    sramConfig.biraSearchPolicy =
        sram_recam::SearchPolicy::SramChunked;
    sramConfig.runtimeSearchPolicy =
        sram_recam::SearchPolicy::SramChunked;
    sramConfig.addressParallelism = 2;
    sramConfig.hybridReadParallelism = 2;
    sramConfig.runtimeParallelism = 2;
    sramConfig.camReuseEntries = 0;
    sramConfig.minimumAddressEntryBits =
        hierarchy.globalWordAddressEntryBits(sramDynamic);
    auto adapter =
        std::make_shared<dynamic_spare::SramRecamSolverAdapter>(sramConfig);
    dynamic_spare::DeviceRepairScheduler sramScheduler{
        dynamic_spare::DynamicRepairSimulator(adapter),
        sram_recam::deriveCommonHardwareMetrics(sramConfig)};
    const auto sram = sramScheduler.run(
        {fifoFixture()}, sramDynamic, hierarchy);

    require(cam.groups.front().metrics.finalSuccess ==
                sram.groups.front().metrics.finalSuccess &&
                cam.groups.front().metrics.repairSource ==
                    sram.groups.front().metrics.repairSource,
            "SRAM DeviceRepairScheduler backend changed repair semantics");
    require(cam.storageTechnology ==
                dynamic_spare::BiraStorageTechnology::Cam &&
                sram.storageTechnology ==
                    dynamic_spare::BiraStorageTechnology::Sram,
            "DeviceRepairScheduler lost the selected storage backend");

    const auto &camTime = cam.groups.front().biraLatency;
    const auto &sramTime = sram.groups.front().biraLatency;
    require(camTime.bistTimelineModeled && sramTime.bistTimelineModeled &&
                camTime.bistCompletionCycle == 16 &&
                sramTime.bistCompletionCycle == 16,
            "A/B/C/D serial BIST completion cycle is incorrect");
    require(camTime.faultCollectionWorkCycles == 16 &&
                camTime.faultCollectionCompletionCycle == 17 &&
                camTime.faultCollectionLatencyAfterBistCycles == 1 &&
                camTime.totalFaultQueueWaitCycles == 28 &&
                camTime.maximumFaultQueueDepth == 4,
            "CAM decoupled-FIFO timeline is incorrect");
    require(sramTime.faultCollectionWorkCycles == 14 &&
                sramTime.faultCollectionCompletionCycle == 15 &&
                sramTime.faultCollectionLatencyAfterBistCycles == 0 &&
                sramTime.totalFaultQueueWaitCycles == 18 &&
                sramTime.maximumFaultQueueDepth == 3 &&
                sramTime.bistBackpressureCycles == 0,
            "SRAM DeviceRepairScheduler FIFO delay is incorrect");

    for (std::size_t subarray = 0; subarray < 2; ++subarray)
    {
        const auto &attempt = sram.groups.front().tier0Analysis
            .attemptsBySubarray[subarray].front();
        require(attempt.sramRecam.has_value(),
                "SRAM strict occupied-scan metrics are absent");
        const auto &metrics = attempt.sramRecam->bira;
        require(metrics.addressEntryCount == 1 &&
                    metrics.hybridEntryCount == 0 &&
                    metrics.tempEntryCount == 0 &&
                    metrics.addressSearchRounds == 3 &&
                    metrics.hybridBitmapAllocations == 1 &&
                    metrics.hybridFullCount == 0 &&
                    metrics.mustRepairTriggeringFaults == 1 &&
                    metrics.repairAnalysisCycles == 0,
                "SRAM occupancy/bitmap/must statistics are incorrect");
    }
}

} // namespace

int main()
{
    try
    {
        verifyPaperDerivedCapacityAndPhysicalAccounting();
        verifyDramGeometryValidation();
        verifyNoPhysicalCamMultiplication();
        verifyScratchReuseAndPersistentSolution();
        verifyGlobalContentionAndFullTags();
        verifySameWordDeduplication();
        verifySharingReducesCamDemand();
        verifySramBackendAndSerialBistTimeline();
        std::cout << "Hierarchical RECAM simulator tests passed\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "Hierarchical RECAM simulator test failed: "
                  << error.what() << '\n';
        return 1;
    }
    return 0;
}
