#include "../inc/CamRecamModel.hpp"

#include <limits>
#include <stdexcept>

namespace dynamic_spare
{
namespace
{

std::uint64_t checkedAdd(
    std::uint64_t left,
    std::uint64_t right,
    const char *description)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
    {
        throw std::overflow_error(description);
    }
    return left + right;
}

std::uint64_t checkedMultiply(
    std::uint64_t left,
    std::uint64_t right,
    const char *description)
{
    if (left != 0 &&
        right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        throw std::overflow_error(description);
    }
    return left * right;
}

} // namespace

BiraLatencyMetrics modelCamBiraLatency(
    const CamLatencyParameters &parameters,
    const CamBiraWorkload &workload)
{
    BiraLatencyMetrics result;
    result.storageTechnology = BiraStorageTechnology::Cam;
    result.faultCollectionWorkCycles = checkedAdd(
        checkedMultiply(
            workload.faultsDetected,
            parameters.insertCyclesPerFault,
            "CAM insert-cycle count overflow"),
        checkedMultiply(
            workload.faultsDetected,
            parameters.lookupCyclesPerFault,
            "CAM lookup-cycle count overflow"),
        "CAM fault-collection cycle count overflow");
    result.repairAnalysisWorkCycles = parameters.ignoreRepairAnalysisLatency
        ? 0
        : checkedAdd(
        checkedMultiply(
            workload.storageEntriesRead,
            parameters.readCyclesPerEntry,
            "CAM read-cycle count overflow"),
        checkedAdd(
            checkedMultiply(
                workload.activeMatrixCells,
                parameters.matrixCyclesPerCell,
                "CAM matrix-cycle count overflow"),
            checkedAdd(
                checkedMultiply(
                    workload.candidatesGenerated,
                    parameters.solutionGenerationCyclesPerCandidate,
                    "CAM solution-generation cycle count overflow"),
                checkedMultiply(
                    workload.candidatesEvaluated,
                    parameters.solutionEvaluationCyclesPerCandidate,
                    "CAM solution-evaluation cycle count overflow"),
                "CAM solution cycle count overflow"),
            "CAM analysis cycle count overflow"),
        "CAM analysis cycle count overflow");
    finalizeBiraWork(result);
    return result;
}

HardwareMetrics deriveCamHardwareMetrics(
    const RecamGeometryConfig &config,
    bool offlineOnlineModeReused)
{
    HardwareMetrics result;
    result.storageTechnology = BiraStorageTechnology::Cam;
    result.geometry = deriveRecamGeometry(config);
    const std::uint64_t addressEntries = std::max(
        result.geometry.addressEntries,
        result.geometry.runtimeEntries);
    const std::uint64_t hybridEntries = std::max(
        result.geometry.hybridEntries,
        result.geometry.runtimeEntries);
    result.addressPhysicalBits = checkedMultiply(
        addressEntries,
        result.geometry.addressEntryBits,
        "CAM Address storage bit count overflow");
    result.hybridPhysicalBits = checkedMultiply(
        hybridEntries,
        result.geometry.hybridEntryBits,
        "CAM Hybrid storage bit count overflow");
    result.temporaryPhysicalBits = result.geometry.tempBufferBits;
    result.runtimePhysicalBits = result.geometry.runtimeCompactBits;
    result.matrixBits = result.geometry.matrixBits;
    // Associative compare is implemented by CAM storage cells; no separate
    // external comparator-bit proxy is added here.
    result.comparatorBits = 0;
    result.offlineOnlineModeReused = offlineOnlineModeReused;
    finalizeHardwareMetrics(result);
    return result;
}

} // namespace dynamic_spare
