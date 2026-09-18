#include "Vrecam_dss_v2_retained_collector_lockstep_top.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>

double sc_time_stamp()
{
    return 0.0;
}

namespace {
struct Statistics {
    unsigned states = 0;
    unsigned config_evaluations = 0;
    unsigned post_must_nine = 0;
    unsigned membership_gt_nine = 0;
    unsigned reuse_states = 0;
    unsigned hybrid_states = 0;
};

void tick(Vrecam_dss_v2_retained_collector_lockstep_top& dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

[[noreturn]] void fail(const char* what, unsigned scenario, unsigned step, unsigned config)
{
    std::cerr << "S1GA2G_LOCKSTEP_FAIL " << what << " scenario=" << scenario
              << " step=" << step << " config=" << config << '\n';
    throw std::runtime_error(what);
}

void compare_all_configs(Vrecam_dss_v2_retained_collector_lockstep_top& dut,
                         Statistics& stats, unsigned scenario, unsigned step)
{
    ++stats.states;
    for (unsigned config = 0; config < 7; ++config) {
        dut.config_id_i = config;
        dut.eval();
        ++stats.config_evaluations;
        if (!dut.packing_match_o)
            fail("packing", scenario, step, config);
        if (!dut.must_match_o)
            fail("must", scenario, step, config);
        if (!dut.view_match_o)
            fail("view", scenario, step, config);
        if (!dut.analyzer_match_o)
            fail("analyzer", scenario, step, config);
        if (dut.projected_count_o == 9)
            ++stats.post_must_nine;
        if (dut.membership_count_o > 9)
            ++stats.membership_gt_nine;
    }
    if (dut.retained_reuse_count_o != 0)
        ++stats.reuse_states;
    if (dut.retained_hybrid_count_o != 0)
        ++stats.hybrid_states;
}

void reset_case(Vrecam_dss_v2_retained_collector_lockstep_top& dut)
{
    dut.clear_i = 1;
    dut.fault_valid_i = 0;
    tick(dut);
    dut.clear_i = 0;
}

void drive_fault(Vrecam_dss_v2_retained_collector_lockstep_top& dut,
                 unsigned row, unsigned col)
{
    dut.fault_valid_i = 1;
    dut.fault_row_i = row;
    dut.fault_col_i = col;
    tick(dut);
    dut.fault_valid_i = 0;
}
} // namespace

int main()
{
    try {
        Vrecam_dss_v2_retained_collector_lockstep_top dut;
        dut.rst_ni = 0;
        dut.clear_i = 0;
        dut.fault_valid_i = 0;
        dut.fault_row_i = 0;
        dut.fault_col_i = 0;
        dut.config_id_i = 0;
        tick(dut);
        dut.rst_ni = 1;

        Statistics stats;
        std::mt19937 random(0xA2C2026U);

        // Mandatory membership-visible-11 boundary: one Pivot followed by
        // eleven row-related faults.  H10 is the final reachable Hybrid.
        reset_case(dut);
        for (unsigned step = 0; step < 12; ++step) {
            drive_fault(dut, 7, 3 + step);
            compare_all_configs(dut, stats, 0, step);
        }
        if (dut.retained_hybrid_count_o != 11 || dut.retained_fault_count_o != 12)
            throw std::runtime_error("MAX_FAULTS/H10 boundary did not materialize");

        // The two A2F post-Must=9 witnesses: three independent pivots,
        // followed by either (row=1,col=2) or (row=2,col=1) star extras.
        for (unsigned witness = 0; witness < 2; ++witness) {
            unsigned step = 0;
            reset_case(dut);
            for (unsigned pivot = 0; pivot < 3; ++pivot) {
                drive_fault(dut, 100 + pivot, 200 + pivot);
                compare_all_configs(dut, stats, 100 + witness, step++);
            }
            for (unsigned pivot = 0; pivot < 3; ++pivot) {
                const unsigned extras = witness == 0 ? 1 : 2;
                for (unsigned extra = 0; extra < extras; ++extra) {
                    drive_fault(dut, 100 + pivot, 1000 + pivot * 10 + extra);
                    compare_all_configs(dut, stats, 100 + witness, step++);
                }
            }
            for (unsigned pivot = 0; pivot < 3; ++pivot) {
                const unsigned extras = witness == 0 ? 2 : 1;
                for (unsigned extra = 0; extra < extras; ++extra) {
                    drive_fault(dut, 1100 + pivot * 10 + extra, 200 + pivot);
                    compare_all_configs(dut, stats, 100 + witness, step++);
                }
            }
        }

        // 1,500 independently cleared reachable traces give 18,000 checked
        // post-transition states.  The four shapes deliberately include
        // Pivot prefixes, same-row and same-column Hybrid writes, and the
        // pointer-four/reuse case after all five pivots have been occupied.
        for (unsigned scenario = 1; scenario <= 1500; ++scenario) {
            reset_case(dut);
            for (unsigned step = 0; step < 12; ++step) {
                unsigned row = 0;
                unsigned col = 0;
                switch (scenario & 3U) {
                case 0:
                    row = 100 + step;
                    col = 300 + step;
                    break;
                case 1:
                    if (step < 5) {
                        row = 200 + step;
                        col = 400 + step;
                    } else {
                        row = 204; // Pivot 4: Hybrid cfg membership plus reuse.
                        col = 450 + step;
                    }
                    break;
                case 2:
                    row = 500;
                    col = 600 + step;
                    break;
                default:
                    row = 700 + (random() % 8);
                    col = 800 + (random() % 8);
                    break;
                }
                drive_fault(dut, row, col);
                compare_all_configs(dut, stats, scenario, step);
            }
        }
        if (stats.states < 10000 || stats.config_evaluations < 70000)
            throw std::runtime_error("lockstep campaign did not reach required coverage floor");
        std::cout << "S1GA2G_LOCKSTEP_WITNESSES post_must_9=" << stats.post_must_nine
                  << " membership_gt9=" << stats.membership_gt_nine
                  << " reuse_states=" << stats.reuse_states
                  << " hybrid_states=" << stats.hybrid_states << '\n';
        if (stats.reuse_states == 0 || stats.hybrid_states == 0 || stats.post_must_nine == 0 ||
            stats.membership_gt_nine == 0)
            throw std::runtime_error("required retained-state witness class was not observed");
        std::cout << "S1GA2G_LOCKSTEP PASS states=" << stats.states
                  << " config_evaluations=" << stats.config_evaluations
                  << " post_must_9=" << stats.post_must_nine
                  << " membership_gt9=" << stats.membership_gt_nine
                  << " reuse_states=" << stats.reuse_states
                  << " hybrid_states=" << stats.hybrid_states << '\n';
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "S1GA2G_LOCKSTEP FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
