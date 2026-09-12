#include "Vrecam_dss_v2_phase4i_rtl_timing_top.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "DssPostBistLatency.hpp"
// The Verilator runner compiles one C++ test translation unit.  Include the
// already-validated event-model implementation so every RTL expectation is
// calculated by that same model, not by a duplicate timing formula.
#include "../../src/DssPostBistLatency.cpp"

namespace
{

constexpr std::uint64_t kLastFaultCycle = 20;

enum class RtlPolicy { Early, Group };

struct TimingCase
{
    const char *name = "";
    RtlPolicy policy = RtlPolicy::Early;
    // `true` models an analyzer state without conventional overflow for SA.
    // `false` models that SA's actual conventional-overflow fault state.
    std::array<bool, 4> saLocallyUsable{{true, true, true, true}};
    std::array<unsigned int, 4> candidateEvaluations{{1, 1, 1, 1}};
    bool repairable = true;
    int failurePosition = -1;
    std::uint64_t bistEndCycle = kLastFaultCycle;
};

struct Observation
{
    std::uint64_t rtlReady = 0;
    std::uint64_t rtlPost = 0;
    dynamic_spare::DssPostBistLatencyResult cpp;
    bool match = false;
};

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void tick(Vrecam_dss_v2_phase4i_rtl_timing_top &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

dynamic_spare::DssPostBistLatencyResult cppPrediction(
    const TimingCase &testCase)
{
    dynamic_spare::DssPostBistRequest request;
    request.faultEvents = {{kLastFaultCycle, 0, true}};
    request.bistEndCycle = testCase.bistEndCycle;
    request.finalDecisionPlan.policy = testCase.policy == RtlPolicy::Early
        ? dynamic_spare::DssPostBistPolicy::Early
        : dynamic_spare::DssPostBistPolicy::GroupNoScratch;
    request.finalDecisionPlan.candidateEvaluations =
        testCase.candidateEvaluations;
    request.finalDecisionPlan.groupRepairable = testCase.repairable;
    request.finalDecisionPlan.failurePosition = testCase.failurePosition;
    return dynamic_spare::modelDssPostBistLatency(request);
}

void driveCurrentAnalyzerState(
    Vrecam_dss_v2_phase4i_rtl_timing_top &dut,
    const TimingCase &testCase)
{
    if (testCase.policy == RtlPolicy::Early)
    {
        const unsigned sa = dut.early_current_sa_o;
        require(sa < testCase.saLocallyUsable.size(), "EARLY SA observer is invalid");
        dut.early_conventional_overflow_i = !testCase.saLocallyUsable[sa];
        dut.group_conventional_overflow_i = 1;
    }
    else
    {
        const unsigned sa = dut.group_current_sa_o;
        require(sa < testCase.saLocallyUsable.size(), "GROUP SA observer is invalid");
        dut.group_conventional_overflow_i = !testCase.saLocallyUsable[sa];
        dut.early_conventional_overflow_i = 1;
    }
}

void traceCycle(
    std::ofstream &trace,
    const TimingCase &testCase,
    Vrecam_dss_v2_phase4i_rtl_timing_top &dut,
    std::uint64_t cycle,
    bool lastFault)
{
    const bool early = testCase.policy == RtlPolicy::Early;
    const unsigned sa = early ? dut.early_current_sa_o : dut.group_current_sa_o;
    const unsigned slot = early ? 0U : dut.group_current_slot_o;
    const unsigned cfg = early ? dut.early_current_config_o : dut.group_current_config_o;
    const bool overflow = early ? dut.early_conventional_overflow_i
                                : dut.group_conventional_overflow_i;
    const unsigned commits = early ? dut.early_sa_commit_valid_o
                                   : dut.group_sa_commit_valid_o;
    const unsigned ledger = early ? dut.early_ledger_o : dut.group_ledger_o;
    const bool done = early ? dut.early_done_o : dut.group_done_o;
    trace << testCase.name << ',' << cycle << ',' << (lastFault ? 1 : 0) << ','
          << (cycle == testCase.bistEndCycle ? 1 : 0) << ',' << sa << ','
          << slot << ',' << cfg << ',' << (!overflow ? 1 : 0) << ','
          << commits << ',' << ledger << ',' << (done ? 1 : 0) << '\n';
}

Observation runCase(
    Vrecam_dss_v2_phase4i_rtl_timing_top &dut,
    const TimingCase &testCase,
    std::ofstream &trace)
{
    const auto cpp = cppPrediction(testCase);
    require(cpp.lastFaultCycle == kLastFaultCycle,
            "C++ model changed the frozen last-fault timestamp");

    dut.rst_ni = 0;
    dut.early_start_i = 0;
    dut.group_start_i = 0;
    dut.early_conventional_overflow_i = 1;
    dut.group_conventional_overflow_i = 1;
    tick(dut); // edge 0
    dut.rst_ni = 1;

    std::uint64_t cycle = 0;
    for (cycle = 1; cycle < kLastFaultCycle; ++cycle)
    {
        driveCurrentAnalyzerState(dut, testCase);
        tick(dut);
    }

    driveCurrentAnalyzerState(dut, testCase);
    dut.early_start_i = testCase.policy == RtlPolicy::Early;
    dut.group_start_i = testCase.policy == RtlPolicy::Group;
    tick(dut); // edge 20: testbench's final-fault / start-acceptance point
    traceCycle(trace, testCase, dut, kLastFaultCycle, true);
    dut.early_start_i = 0;
    dut.group_start_i = 0;

    bool done = false;
    std::uint64_t ready = 0;
    for (cycle = kLastFaultCycle + 1; cycle <= kLastFaultCycle + 64; ++cycle)
    {
        driveCurrentAnalyzerState(dut, testCase);
        tick(dut);
        traceCycle(trace, testCase, dut, cycle, false);
        done = testCase.policy == RtlPolicy::Early ? dut.early_done_o : dut.group_done_o;
        if (done)
        {
            ready = cycle;
            break;
        }
    }
    require(done, std::string(testCase.name) + " RTL completion timeout");

    const bool repairable = testCase.policy == RtlPolicy::Early
        ? dut.early_group_repairable_o : dut.group_group_repairable_o;
    const unsigned failure = testCase.policy == RtlPolicy::Early
        ? dut.early_failure_position_o : dut.group_failure_position_o;
    require(repairable == testCase.repairable,
            std::string(testCase.name) + " repairability boundary mismatch");
    if (!testCase.repairable)
    {
        require(failure == static_cast<unsigned>(testCase.failurePosition),
                std::string(testCase.name) + " failure-position mismatch");
    }

    Observation result;
    result.cpp = cpp;
    result.rtlReady = ready;
    result.rtlPost = ready > testCase.bistEndCycle
        ? ready - testCase.bistEndCycle : 0;
    result.match = result.rtlReady == cpp.decisionReadyCycle &&
                   result.rtlPost == cpp.latencyPostBistCycles;
    return result;
}

void writeResult(
    std::ofstream &output,
    const TimingCase &testCase,
    const Observation &observation)
{
    output << testCase.name << ','
           << (testCase.policy == RtlPolicy::Early ? "EARLY" : "GROUP_NO_SCRATCH")
           << ',' << kLastFaultCycle << ',' << testCase.bistEndCycle << ','
           << observation.cpp.decisionReadyCycle << ',' << observation.rtlReady
           << ',' << observation.cpp.latencyPostBistCycles << ','
           << observation.rtlPost << ',' << (observation.match ? "PASS" : "FAIL")
           << '\n';
}

} // namespace

int main()
{
    try
    {
        std::filesystem::create_directories("results/phase4i");
        std::ofstream table("results/phase4i/phase4i3_rtl_timing.csv");
        std::ofstream trace("results/phase4i/phase4i3_rtl_cycle_trace.csv");
        require(table.good() && trace.good(), "Unable to create Phase 4I-3 artifacts");
        table << "case,policy,T_last_fault,T_BIST_end,cpp_T_decision_ready,"
                 "rtl_T_decision_ready,cpp_L_post,rtl_L_post,match\n";
        trace << "case,cycle,last_fault_event,BIST_end_marker,current_sa,current_slot,"
                 "current_config,local_candidate_valid,sa_commit_bitmap,ledger,done\n";

        const std::array<TimingCase, 8> cases{{
            {"E1", RtlPolicy::Early, {{true, true, true, true}}, {{1, 1, 1, 1}}, true, -1, 20},
            {"E2", RtlPolicy::Early, {{true, true, true, true}}, {{1, 1, 1, 1}}, true, -1, 24},
            {"E3", RtlPolicy::Early, {{true, true, true, true}}, {{1, 1, 1, 1}}, true, -1, 22},
            {"E4", RtlPolicy::Early, {{true, false, false, false}}, {{1, 4, 0, 0}}, false, 1, 20},
            {"G1", RtlPolicy::Group, {{true, true, true, true}}, {{1, 1, 1, 1}}, true, -1, 20},
            {"G2", RtlPolicy::Group, {{true, true, true, true}}, {{1, 1, 1, 1}}, true, -1, 40},
            {"G3", RtlPolicy::Group, {{true, true, true, true}}, {{1, 1, 1, 1}}, true, -1, 36},
            {"G4", RtlPolicy::Group, {{true, false, false, false}}, {{1, 4, 0, 0}}, false, 1, 20}
        }};

        Vrecam_dss_v2_phase4i_rtl_timing_top dut;
        unsigned earlyCases = 0;
        unsigned groupCases = 0;
        unsigned earlyMismatches = 0;
        unsigned groupMismatches = 0;
        for (const TimingCase &testCase : cases)
        {
            const Observation observation = runCase(dut, testCase, trace);
            writeResult(table, testCase, observation);
            if (testCase.policy == RtlPolicy::Early)
            {
                ++earlyCases;
                earlyMismatches += observation.match ? 0 : 1;
            }
            else
            {
                ++groupCases;
                groupMismatches += observation.match ? 0 : 1;
            }
            std::cout << "RTL_TIMING " << testCase.name
                      << " cpp_ready=" << observation.cpp.decisionReadyCycle
                      << " rtl_ready=" << observation.rtlReady
                      << " cpp_Lpost=" << observation.cpp.latencyPostBistCycles
                      << " rtl_Lpost=" << observation.rtlPost
                      << " " << (observation.match ? "PASS" : "FAIL") << '\n';
        }
        const unsigned mismatches = earlyMismatches + groupMismatches;
        std::cout << "EARLY_RTL_CASES=" << earlyCases << '\n'
                  << "EARLY_RTL_MISMATCHES=" << earlyMismatches << '\n'
                  << "GROUP_RTL_CASES=" << groupCases << '\n'
                  << "GROUP_RTL_MISMATCHES=" << groupMismatches << '\n'
                  << "RTL_EVENT_MODEL_MISMATCHES=" << mismatches << '\n'
                  << "SAME_EDGE_ZERO_LATENCY_VALIDATED="
                  << (mismatches == 0 ? "YES" : "NO") << '\n'
                  << "NON_DOMINANT_EARLY_PATH_VALIDATED="
                  << (mismatches == 0 ? "YES" : "NO") << '\n'
                  << "LONG_GROUP_PATH_VALIDATED="
                  << (mismatches == 0 ? "YES" : "NO") << '\n';
        require(mismatches == 0, "RTL/C++ timing mismatch");
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Phase 4I-3 RTL timing validation failed: " << error.what() << '\n';
        return 1;
    }
}
