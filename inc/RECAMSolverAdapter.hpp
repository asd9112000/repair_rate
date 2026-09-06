#ifndef DYNAMIC_SPARE_SHARING_RECAM_SOLVER_ADAPTER_HPP
#define DYNAMIC_SPARE_SHARING_RECAM_SOLVER_ADAPTER_HPP

#include "Fault.hpp"
#include "RepairAttemptSolver.hpp"
#include "RepairResult.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace dynamic_spare
{

DecodedSolution decodeSolution(
    const TileSolutionState &state,
    std::size_t solutionId);

struct RECAMSolverRequest
{
    std::size_t runIndex = 0;
    int subarrayId = -1;
    int availableRows = 0;
    int availableColumns = 0;
    int bufferCamEntries = 0;
    int provisionedRows = -1;
    int provisionedColumns = -1;
    int stage = 0;
    std::size_t attemptIndex = 0;
    std::optional<std::uint32_t> hybridCamEntryWidthBits;
    std::uint32_t rowAddressWidthBits = 1;
    std::uint32_t columnAddressWidthBits = 1;
};

class RECAMSolverAdapter : public RepairAttemptSolver
{
public:
    // Fault values are copied into a configuration-specific FaultList.  The
    // returned DTO owns no pointer into FaultList, RECAM_PE, or CAM storage.
    RepairAttemptResult solve(
        const std::vector<Fault> &faults,
        const RECAMSolverRequest &request) const override;
};

} // namespace dynamic_spare

#endif
