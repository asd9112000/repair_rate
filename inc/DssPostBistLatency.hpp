#ifndef DSS_POST_BIST_LATENCY_HPP
#define DSS_POST_BIST_LATENCY_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dynamic_spare
{

// This timing-only model is intentionally separate from the CAM/SRAM BIRA
// models.  It represents the frozen 2x2 DSS decision boundary: it neither
// reconstructs a repair table nor changes a DSS functional decision.
enum class DssPostBistPolicy
{
    Early,
    GroupNoScratch
};

const char *toString(DssPostBistPolicy policy) noexcept;

struct DssFaultSetEvent
{
    std::uint64_t cycle = 0;
    std::size_t faultId = 0;

    // A duplicate BIST observation does not change the collected fault set
    // and therefore does not invalidate a concurrent DSS decision.
    bool changesFaultSet = true;
};

// candidateEvaluations[SA] is the number of frozen-rank candidate evaluations
// required for that SA (one through four).  The active SAs are A..D for a
// repairable group, or A through failurePosition for a failed group.
//
// EARLY work is the sum of these evaluations.  GROUP-NoScratch adds its
// frozen 16-cycle collection phase before the same sequential allocation work.
// The plan is supplied by a functional DSS trace; this model does not infer
// repairability, candidates, or resource actions from fault coordinates.
struct DssDecisionWorkPlan
{
    DssPostBistPolicy policy = DssPostBistPolicy::Early;
    std::array<unsigned int, 4> candidateEvaluations{{0, 0, 0, 0}};
    bool groupRepairable = true;
    // -1 means all four SAs completed; otherwise A=0, B=1, C=2, D=3.
    int failurePosition = -1;
};

struct DssPostBistRequest
{
    std::vector<DssFaultSetEvent> faultEvents;
    std::uint64_t bistEndCycle = 0;
    DssDecisionWorkPlan finalDecisionPlan;
};

struct DssPostBistLatencyResult
{
    DssPostBistPolicy policy = DssPostBistPolicy::Early;
    std::size_t faultCount = 0;
    std::size_t faultSetChangeCount = 0;
    std::uint64_t lastFaultCycle = 0;
    std::uint64_t bistEndCycle = 0;
    std::uint64_t gapCycles = 0;
    std::uint64_t decisionReadyCycle = 0;
    std::uint64_t latencyFromLastFaultCycles = 0;
    std::uint64_t latencyPostBistCycles = 0;
    std::uint64_t decisionExecutionCycles = 0;
    std::size_t invalidatedDecisionCount = 0;
    bool groupRepairable = false;
    int failurePosition = -1;
};

// Timestamp convention:
// - a fault-set-changing BIST event at rising edge N is visible immediately
//   after that edge;
// - it invalidates any in-flight decision and starts a new final-plan attempt
//   in interval [N,N+1);
// - a W-cycle plan asserts its registered decision boundary at rising edge
//   N+W.  A decision ready at T_BIST_end has zero exposed post-BIST latency.
//
// The function models concurrent analysis by launching/restarting on every
// changing event; it never waits for BIST completion to start the final plan.
DssPostBistLatencyResult modelDssPostBistLatency(
    const DssPostBistRequest &request);

} // namespace dynamic_spare

#endif
