#ifndef SRAM_RECAM_SOLVER_ADAPTER_HPP
#define SRAM_RECAM_SOLVER_ADAPTER_HPP

#include "RECAMSolverAdapter.hpp"
#include "RepairAttemptSolver.hpp"
#include "SramRecamModel.hpp"

namespace dynamic_spare
{

// Functional SRAM_RECAM adapter for DynamicSpareSharing.  The existing RECAM
// solver remains the golden repair algorithm.  This wrapper independently
// replays the complete Address-SRAM search (including both row and column
// matches), checks classification equivalence, and attaches SRAM timing/cost
// metrics without changing the golden repair state or candidate ordering.
class SramRecamSolverAdapter : public RepairAttemptSolver
{
public:
    explicit SramRecamSolverAdapter(
        sram_recam::SramRecamConfig config);

    RepairAttemptResult solve(
        const std::vector<Fault> &faults,
        const RECAMSolverRequest &request) const override;

    const sram_recam::SramRecamConfig &config() const noexcept;

private:
    sram_recam::SramRecamConfig config_;
    RECAMSolverAdapter golden_;
};

} // namespace dynamic_spare

#endif
