#ifndef CAM_RECAM_MODEL_HPP
#define CAM_RECAM_MODEL_HPP

#include "BiraLatency.hpp"
#include "HardwareMetrics.hpp"
#include "RecamGeometry.hpp"

#include <cstdint>

namespace dynamic_spare
{

struct CamLatencyParameters
{
    std::uint64_t insertCyclesPerFault = 1;
    std::uint64_t lookupCyclesPerFault = 1;
    std::uint64_t readCyclesPerEntry = 1;
    std::uint64_t matrixCyclesPerCell = 1;
    std::uint64_t solutionGenerationCyclesPerCandidate = 1;
    std::uint64_t solutionEvaluationCyclesPerCandidate = 1;
    // The current collection-only comparison excludes matrix conversion and
    // fault-free-area analysis from modeled latency.
    bool ignoreRepairAnalysisLatency = true;
};

struct CamBiraWorkload
{
    std::uint64_t faultsDetected = 0;
    std::uint64_t storageEntriesRead = 0;
    std::uint64_t activeMatrixCells = 0;
    std::uint64_t candidatesGenerated = 0;
    std::uint64_t candidatesEvaluated = 0;
};

BiraLatencyMetrics modelCamBiraLatency(
    const CamLatencyParameters &parameters,
    const CamBiraWorkload &workload);

HardwareMetrics deriveCamHardwareMetrics(
    const RecamGeometryConfig &config,
    bool offlineOnlineModeReused = true);

} // namespace dynamic_spare

#endif
