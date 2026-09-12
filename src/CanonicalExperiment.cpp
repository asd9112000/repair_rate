#include "../inc/CanonicalExperiment.hpp"

#include "../inc/CamRecamModel.hpp"
#include "../inc/DynamicRemapReporter.hpp"
#include "../inc/SramRecamSolverAdapter.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <memory>
#include <limits>
#include <stdexcept>
#include <utility>

namespace dynamic_spare
{
namespace
{

SimulationConfig noSharing(const SimulationConfig &source)
{
    SimulationConfig result = source;
    result.topology = SharingTopology::NoSharing;
    result.sharedRows = 0;
    result.sharedColumns = 0;
    result.globalPool.reset();
    result.modifiers.localFirst = true;
    result.modifiers.singleDimensionBorrowing = false;
    result.modifiers.minimumRowReserve = 0;
    result.modifiers.minimumColumnReserve = 0;
    result.modifiers.maximumGroupBorrowedSpares = 0;
    return result;
}

void requireLogicalEquivalence(
    const CanonicalConfigurationResult &cam,
    const CanonicalConfigurationResult &sram)
{
    if (cam.device.groups.size() != sram.device.groups.size())
    {
        throw std::logic_error(
            "Canonical CAM/SRAM runs produced different group counts");
    }
    for (std::size_t index = 0; index < cam.device.groups.size(); ++index)
    {
        const auto &left = cam.device.groups[index];
        const auto &right = sram.device.groups[index];
        if (!(left.address == right.address) ||
            left.metrics.finalSuccess != right.metrics.finalSuccess ||
            left.metrics.repairSource != right.metrics.repairSource ||
            left.metrics.borrowedRows != right.metrics.borrowedRows ||
            left.metrics.borrowedColumns != right.metrics.borrowedColumns ||
            left.metrics.camWordsNeeded != right.metrics.camWordsNeeded ||
            left.metrics.camWordsAllocated != right.metrics.camWordsAllocated)
        {
            throw std::logic_error(
                "Canonical SRAM backend changed CAM repair semantics at "
                "group " + std::to_string(index));
        }
    }
}

GroupRepairResult selectedAnalysis(const HierarchicalGroupResult &group)
{
    GroupRepairResult selected;
    switch (group.metrics.repairSource)
    {
        case RepairSource::LocalOnly:
            selected = group.tier0Analysis;
            break;
        case RepairSource::SharingRequired:
            if (!group.tier1Analysis.has_value())
                throw std::logic_error("Missing Tier-1 remap analysis");
            selected = *group.tier1Analysis;
            break;
        case RepairSource::CamReuseRequired:
        case RepairSource::SharingAndCamReuseRequired:
            if (!group.tier2Analysis.has_value())
                throw std::logic_error("Missing Tier-2 remap analysis");
            selected = *group.tier2Analysis;
            break;
        case RepairSource::Unrepairable:
            if (group.tier2Analysis.has_value())
                selected = *group.tier2Analysis;
            else if (group.tier1Analysis.has_value())
                selected = *group.tier1Analysis;
            else
                selected = group.tier0Analysis;
            break;
    }
    selected.groupRepairSuccess = group.metrics.finalSuccess;
    return selected;
}

std::ofstream openCsv(const std::filesystem::path &path)
{
    std::ofstream output(path, std::ios::trunc);
    if (!output)
    {
        throw std::runtime_error("Unable to write canonical CSV: " +
                                 path.string());
    }
    output << std::setprecision(12);
    return output;
}

std::uint64_t ceilLog2(std::uint64_t values)
{
    std::uint64_t bits = 0;
    std::uint64_t representable = 1;
    while (representable < values)
    {
        representable <<= 1;
        ++bits;
    }
    return bits;
}

struct SpareEnvelope
{
    std::uint32_t rows = 0;
    std::uint32_t columns = 0;
};

void observeAttempts(const GroupRepairResult &analysis, SpareEnvelope &envelope)
{
    for (const auto &subarrayAttempts : analysis.attemptsBySubarray)
    {
        for (const RepairAttemptResult &attempt : subarrayAttempts)
        {
            envelope.rows = std::max(
                envelope.rows,
                static_cast<std::uint32_t>(attempt.provisionedRows));
            envelope.columns = std::max(
                envelope.columns,
                static_cast<std::uint32_t>(attempt.provisionedColumns));
        }
    }
}

SpareEnvelope conservativeEnvelope(const CanonicalConfigurationResult &run)
{
    SpareEnvelope result{
        static_cast<std::uint32_t>(run.groupConfig.spareRows),
        static_cast<std::uint32_t>(run.groupConfig.spareColumns)};
    for (const HierarchicalGroupResult &group : run.device.groups)
    {
        observeAttempts(group.tier0Analysis, result);
        if (group.tier1Analysis.has_value())
            observeAttempts(*group.tier1Analysis, result);
        if (group.tier2Analysis.has_value())
            observeAttempts(*group.tier2Analysis, result);
    }
    return result;
}

std::uint64_t maximumFifoDepth(const DeviceRepairResult &device)
{
    std::uint64_t result = 0;
    for (const HierarchicalGroupResult &group : device.groups)
    {
        result = std::max<std::uint64_t>(
            result, group.biraLatency.maximumFaultQueueDepth);
    }
    return result;
}

void writeManifestRow(
    std::ofstream &output,
    CanonicalConfigurationId id,
    const char *scope,
    const char *resource,
    const char *technology,
    std::uint64_t instances,
    std::uint64_t logicalEntries,
    std::uint64_t entryBits,
    std::uint64_t physicalDepth,
    std::uint64_t physicalWidth,
    std::uint64_t totalBits,
    const char *sizingRule,
    const char *entryStructure)
{
    output << toString(id) << ',' << scope << ',' << resource << ','
           << technology << ',' << instances << ',' << logicalEntries << ','
           << entryBits << ',' << physicalDepth << ',' << physicalWidth << ','
           << totalBits << ',' << sizingRule << ",\"" << entryStructure
           << "\"\n";
}

} // namespace

const char *toString(CanonicalConfigurationId id) noexcept
{
    switch (id)
    {
        case CanonicalConfigurationId::B0CamNoSharing:
            return "B0_CAM_NO_SHARING";
        case CanonicalConfigurationId::B1CamSharing:
            return "B1_CAM_SHARING";
        case CanonicalConfigurationId::B2SramNoSharing:
            return "B2_SRAM_NO_SHARING";
        case CanonicalConfigurationId::B3SramSharing:
            return "B3_SRAM_SHARING";
    }
    return "UNKNOWN";
}

std::vector<CanonicalConfigurationResult> runCanonicalFour(
    const std::vector<RepairGroupInput> &inputs,
    const CanonicalExperimentConfig &config)
{
    config.sharingConfig.validate();
    config.hierarchicalConfig.validate();
    if (config.sharingConfig.topology == SharingTopology::NoSharing)
    {
        throw std::invalid_argument(
            "Canonical B1/B3 configuration requires a sharing topology");
    }

    SimulationConfig camNoSharing = noSharing(config.sharingConfig);
    camNoSharing.storageMode = FaultInformationStorage::CAM;
    SimulationConfig camSharing = config.sharingConfig;
    camSharing.storageMode = FaultInformationStorage::CAM;
    SimulationConfig sramNoSharing = noSharing(config.sharingConfig);
    sramNoSharing.storageMode = FaultInformationStorage::SRAM;
    SimulationConfig sramSharing = config.sharingConfig;
    sramSharing.storageMode = FaultInformationStorage::SRAM;

    sram_recam::SramRecamConfig sramConfig = config.sramConfig;
    sramConfig.rows = config.sharingConfig.memoryRows;
    sramConfig.cols = config.sharingConfig.memoryColumns;
    sramConfig.spareRows = static_cast<std::uint32_t>(
        config.sharingConfig.spareRows);
    sramConfig.spareCols = static_cast<std::uint32_t>(
        config.sharingConfig.spareColumns);
    sramConfig.dataWordBits = config.hierarchicalConfig.dataWordBits;
    sramConfig.minimumAddressEntryBits =
        config.hierarchicalConfig.globalWordAddressEntryBits(
            config.sharingConfig);
    const std::size_t onlineEntries =
        config.hierarchicalConfig.onlineGlobalReuseEntries.value_or(
            static_cast<std::size_t>(config.sharingConfig.spareRows +
                                     config.sharingConfig.spareColumns));
    if (onlineEntries > std::numeric_limits<std::uint32_t>::max())
    {
        throw std::invalid_argument(
            "Canonical SRAM online capacity exceeds uint32 range");
    }
    sramConfig.camReuseEntries = static_cast<std::uint32_t>(onlineEntries);
    sramConfig.validate();

    std::vector<CanonicalConfigurationResult> results;
    results.reserve(4);

    DeviceRepairScheduler camB0;
    results.push_back({
        CanonicalConfigurationId::B0CamNoSharing,
        camNoSharing,
        camB0.run(inputs, camNoSharing, config.hierarchicalConfig),
        std::nullopt});

    DeviceRepairScheduler camB1;
    results.push_back({
        CanonicalConfigurationId::B1CamSharing,
        camSharing,
        camB1.run(inputs, camSharing, config.hierarchicalConfig),
        std::nullopt});

    auto sramAdapter = std::make_shared<SramRecamSolverAdapter>(sramConfig);
    const HardwareMetrics sramHardware =
        sram_recam::deriveCommonHardwareMetrics(sramConfig);
    DeviceRepairScheduler sramB2(
        DynamicRepairSimulator(sramAdapter), sramHardware);
    results.push_back({
        CanonicalConfigurationId::B2SramNoSharing,
        sramNoSharing,
        sramB2.run(inputs, sramNoSharing, config.hierarchicalConfig),
        sramConfig});

    // A fresh scheduler guarantees an independent FIFO, scratch state, and
    // global online pool for each hypothetical configuration.
    DeviceRepairScheduler sramB3(
        DynamicRepairSimulator(sramAdapter), sramHardware);
    results.push_back({
        CanonicalConfigurationId::B3SramSharing,
        sramSharing,
        sramB3.run(inputs, sramSharing, config.hierarchicalConfig),
        sramConfig});

    requireLogicalEquivalence(results[0], results[2]);
    requireLogicalEquivalence(results[1], results[3]);
    return results;
}

void CanonicalExperimentReporter::writeAreaManifest(
    const std::filesystem::path &outputDirectory,
    const std::vector<CanonicalConfigurationResult> &results)
{
    if (results.size() != 4)
    {
        throw std::invalid_argument(
            "Canonical area manifest requires exactly B0/B1/B2/B3");
    }
    std::filesystem::create_directories(outputDirectory);
    std::ofstream output = openCsv(outputDirectory / "hardware_area_manifest.csv");
    output << "configuration_id,scope,resource,technology,instances,"
              "logical_entries,entry_bits,physical_depth,physical_width,"
              "total_bits,sizing_rule,entry_structure\n";

    for (const CanonicalConfigurationResult &run : results)
    {
        const SpareEnvelope envelope = conservativeEnvelope(run);
        RecamGeometryConfig geometryConfig;
        geometryConfig.rows = run.groupConfig.memoryRows;
        geometryConfig.columns = run.groupConfig.memoryColumns;
        geometryConfig.spareRows = envelope.rows;
        geometryConfig.spareColumns = envelope.columns;
        geometryConfig.dataWordBits = run.groupConfig.dataWidthBits;
        const RecamGeometry geometry = deriveRecamGeometry(geometryConfig);
        const std::uint64_t engines = run.device.hardware.biraEngineCount;
        const bool isSram = run.sramConfig.has_value();

        if (isSram)
        {
            sram_recam::SramRecamConfig sramConfig = *run.sramConfig;
            sramConfig.spareRows = envelope.rows;
            sramConfig.spareCols = envelope.columns;
            const sram_recam::SramRecamGeometry sramGeometry =
                sram_recam::deriveGeometry(sramConfig);
            writeManifestRow(output, run.id, "BIRA_ENGINE", "ADDRESS_STORE",
                "SRAM", engines, sramGeometry.addressSram.logicalEntries,
                sramGeometry.addressSram.entryBits,
                sramGeometry.addressSram.physicalDepth,
                sramGeometry.addressSram.physicalWidth,
                engines * sramGeometry.addressSram.physicalBits,
                "max provisioned Rs/Cs over all evaluated tier attempts",
                "valid,row,col,row_must,col_must,row_fail_count,col_fail_count");
            writeManifestRow(output, run.id, "BIRA_ENGINE", "HYBRID_STORE",
                "SRAM", engines, sramGeometry.hybridSram.logicalEntries,
                sramGeometry.hybridSram.entryBits,
                sramGeometry.hybridSram.physicalDepth,
                sramGeometry.hybridSram.physicalWidth,
                engines * sramGeometry.hybridSram.physicalBits,
                "max provisioned Rs/Cs over all evaluated tier attempts",
                "valid,address_pointer,line_type,line_address");
        }
        else
        {
            writeManifestRow(output, run.id, "BIRA_ENGINE", "ADDRESS_STORE",
                "CAM", engines, geometry.addressEntries,
                geometry.addressOfflineEntryBits, geometry.addressEntries,
                geometry.addressOfflineEntryBits,
                engines * geometry.addressEntries * geometry.addressOfflineEntryBits,
                "max provisioned Rs/Cs over all evaluated tier attempts",
                "valid,row,col,row_must,col_must,row_fail_count,col_fail_count");
            writeManifestRow(output, run.id, "BIRA_ENGINE", "HYBRID_STORE",
                "CAM", engines, geometry.hybridEntries,
                geometry.hybridOfflineEntryBits, geometry.hybridEntries,
                geometry.hybridOfflineEntryBits,
                engines * geometry.hybridEntries * geometry.hybridOfflineEntryBits,
                "max provisioned Rs/Cs over all evaluated tier attempts",
                "valid,address_pointer,line_type,line_address");
        }

        writeManifestRow(output, run.id, "BIRA_ENGINE", "TEMPORARY_FAULT_BUFFER",
            "REGISTER", engines, geometry.tempEntries, geometry.tempEntryBits,
            geometry.tempEntries, geometry.tempEntryBits,
            engines * geometry.tempBufferBits,
            "RECAM K=Rs+Cs at conservative envelope",
            "row,col,channel");
        const std::uint64_t counterBits = 3 * ceilLog2(geometry.k + 1) +
            geometry.addressEntries + geometry.hybridEntries + 2 * geometry.k;
        writeManifestRow(output, run.id, "BIRA_ENGINE", "STANDALONE_COUNTERS",
            "REGISTER", engines, 1, counterBits, 1, counterBits,
            engines * counterBits,
            "RECAM K=Rs+Cs at conservative envelope",
            "address/hybrid/temp occupancy counters; valid/free bitmaps; row/column must flags");

        const std::uint64_t fifoObserved = maximumFifoDepth(run.device);
        const std::uint64_t fifoDepth = std::max<std::uint64_t>(1, fifoObserved);
        const std::uint64_t fifoEntryBits = 1 + 3 + 2 + 6 + 2 +
            ceilLog2(run.groupConfig.memoryRows) +
            ceilLog2(run.groupConfig.memoryColumns);
        writeManifestRow(output, run.id, "BIRA_ENGINE", "FAULT_COLLECTION_FIFO",
            "SRAM", engines, fifoDepth, fifoEntryBits, fifoDepth,
            fifoEntryBits, engines * fifoDepth * fifoEntryBits,
            "max observed FIFO depth; minimum one entry",
            "valid,domain,bank,repair_group,subarray,row,physical_cell_column");

        // A cell-column tag is a conservative upper bound for both supported
        // online granularities: word-granular reuse needs no more bits.
        const std::uint64_t repairColumnValues = run.groupConfig.memoryColumns;
        const std::uint64_t onlineTagBits = 3 + 2 + 6 + 2 +
            ceilLog2(run.groupConfig.memoryRows) +
            ceilLog2(repairColumnValues);
        writeManifestRow(output, run.id, "DEVICE_GLOBAL", "ONLINE_REPAIR_DIRECTORY",
            isSram ? "SRAM" : "CAM", 1, run.device.globalCam.capacity,
            1 + onlineTagBits, run.device.globalCam.capacity,
            1 + onlineTagBits,
            run.device.globalCam.capacity * (1 + onlineTagBits),
            "device-wide online reuse capacity; counted separately from BIRA engines",
            "valid,domain,bank,repair_group,subarray,row,repair_column");
    }
}

void CanonicalExperimentReporter::write(
    const std::filesystem::path &outputDirectory,
    const std::vector<RepairGroupInput> &inputs,
    const std::vector<CanonicalConfigurationResult> &results)
{
    if (results.size() != 4)
    {
        throw std::invalid_argument(
            "Canonical reporter requires exactly B0/B1/B2/B3");
    }
    std::filesystem::create_directories(outputDirectory);
    std::ofstream summary = openCsv(
        outputDirectory / "canonical_four_summary.csv");
    summary
        << "configuration_id,bira_storage,sharing_enabled,topology,Rs,Cs,"
           "modeled_groups,repairable_groups,repair_rate,device_success,"
           "global_reuse_capacity,global_reuse_entries_reserved,"
           "global_reuse_overflow_count,total_bira_completion_cycles,"
           "groups_with_bist_timeline,max_collection_after_bist_cycles,"
           "max_solution_after_bist_cycles,address_entries,hybrid_entries,"
           "online_entries,address_entry_bits,hybrid_entry_bits,"
           "bira_storage_bits_per_engine,bira_modeled_bits_per_engine,"
           "total_bira_storage_bits,total_bira_modeled_bits,"
           "local_only_groups,sharing_required_groups,"
           "reuse_required_groups,sharing_and_reuse_required_groups,"
           "unrepairable_groups\n";

    std::ofstream groups = openCsv(
        outputDirectory / "canonical_four_groups.csv");
    groups
        << "configuration_id,device,domain,bank,group,fault_count,"
           "repair_source,final_success,borrowed_rows,borrowed_columns,"
           "reuse_words_needed,reuse_words_allocated,bira_storage,"
           "fault_collection_work_cycles,repair_analysis_work_cycles,"
           "sharing_allocation_work_cycles,total_work_cycles,"
           "bist_timeline_modeled,bist_completion_cycle,"
           "fault_collection_completion_cycle,solution_ready_cycle,"
           "fault_collection_after_bist_cycles,latency_after_bist_cycles,"
           "maximum_fault_queue_depth,total_fault_queue_wait_cycles,"
           "bist_backpressure_cycles\n";

    std::vector<FaultGroup> faultGroups;
    faultGroups.reserve(inputs.size());
    for (const RepairGroupInput &input : inputs)
        faultGroups.push_back(input.faults);

    for (const CanonicalConfigurationResult &run : results)
    {
        const DeviceRepairResult &device = run.device;
        const HardwareMetrics &hardware = device.hardware.biraPerEngine;
        const double repairRate = device.modeledGroupCount == 0
            ? 0.0
            : static_cast<double>(device.repairableGroupCount) /
                  static_cast<double>(device.modeledGroupCount);
        summary
            << toString(run.id) << ','
            << toString(device.storageTechnology) << ','
            << (run.groupConfig.topology == SharingTopology::NoSharing
                    ? 0 : 1) << ','
            << toString(run.groupConfig.topology) << ','
            << run.groupConfig.spareRows << ','
            << run.groupConfig.spareColumns << ','
            << device.modeledGroupCount << ','
            << device.repairableGroupCount << ',' << repairRate << ','
            << (device.deviceRepairSuccess ? 1 : 0) << ','
            << device.globalCam.capacity << ','
            << device.globalCam.entriesReserved << ','
            << device.globalCam.allocationFailureCount << ','
            << device.totalBiraCompletionCycles() << ','
            << device.groupsWithBistTimeline << ','
            << device.maximumFaultCollectionLatencyAfterBistCycles << ','
            << device.maximumSolutionLatencyAfterBistCycles << ','
            << hardware.geometry.addressEntries << ','
            << hardware.geometry.hybridEntries << ','
            << hardware.geometry.runtimeEntries << ','
            << hardware.geometry.addressEntryBits << ','
            << hardware.geometry.hybridEntryBits << ','
            << hardware.totalStorageBits << ','
            << hardware.totalModeledBits << ','
            << device.hardware.totalBiraStorageBits << ','
            << device.hardware.totalBiraModeledBits;
        for (std::size_t count : device.repairSourceCounts)
            summary << ',' << count;
        summary << '\n';

        std::vector<GroupRepairResult> remapResults;
        remapResults.reserve(device.groups.size());
        for (const HierarchicalGroupResult &group : device.groups)
        {
            const auto &latency = group.biraLatency;
            groups
                << toString(run.id) << ',' << group.address.device << ','
                << group.address.domain << ',' << group.address.bank << ','
                << group.address.group << ',' << group.metrics.faultCount << ','
                << toString(group.metrics.repairSource) << ','
                << (group.metrics.finalSuccess ? 1 : 0) << ','
                << group.metrics.borrowedRows << ','
                << group.metrics.borrowedColumns << ','
                << group.metrics.camWordsNeeded << ','
                << group.metrics.camWordsAllocated << ','
                << toString(latency.storageTechnology) << ','
                << latency.faultCollectionWorkCycles << ','
                << latency.repairAnalysisWorkCycles << ','
                << latency.sharingAllocationWorkCycles << ','
                << latency.totalWorkCycles << ','
                << (latency.bistTimelineModeled ? 1 : 0) << ','
                << latency.bistCompletionCycle << ','
                << latency.faultCollectionCompletionCycle << ','
                << latency.solutionReadyCycle << ','
                << latency.faultCollectionLatencyAfterBistCycles << ','
                << latency.latencyAfterBistCycles << ','
                << latency.maximumFaultQueueDepth << ','
                << latency.totalFaultQueueWaitCycles << ','
                << latency.bistBackpressureCycles << '\n';
            remapResults.push_back(selectedAnalysis(group));
        }

        const std::filesystem::path runDirectory =
            outputDirectory / toString(run.id);
        std::filesystem::create_directories(runDirectory);
        DynamicRemapReporter::write(
            runDirectory / "RemapTable.txt",
            runDirectory / "RemapTable_simplified.txt",
            run.groupConfig,
            faultGroups,
            remapResults);
    }
}

} // namespace dynamic_spare
