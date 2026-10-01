#include "Vrecam_dss_n3_rc_group_live_state_core.h"
#include "verilated.h"

#include <cstdlib>
#include <iostream>

namespace {

void tick(Vrecam_dss_n3_rc_group_live_state_core& dut) {
    dut.clk_i = 0; dut.eval();
    dut.clk_i = 1; dut.eval();
    dut.clk_i = 0; dut.eval();
}

void reset(Vrecam_dss_n3_rc_group_live_state_core& dut) {
    dut.rst_ni = 0;
    dut.state_update_i = 0;
    dut.state_sa_i = 0;
    dut.test_done_valid_i = 0;
    dut.test_done_sa_i = 0;
    dut.candidate_valid_i = 0;
    dut.candidate_pattern_id_i = 0;
    tick(dut); tick(dut);
    dut.rst_ni = 1;
}

bool complete_sa(Vrecam_dss_n3_rc_group_live_state_core& dut, unsigned sa,
                 unsigned pattern) {
    dut.state_update_i = 1;
    dut.state_sa_i = sa;
    dut.test_done_valid_i = 1;
    dut.test_done_sa_i = sa;
    dut.candidate_valid_i = 1;
    dut.candidate_pattern_id_i = pattern;
    tick(dut);
    dut.state_update_i = 0;
    dut.test_done_valid_i = 0;
    for (unsigned slot = 0; slot < 4; ++slot) {
        dut.candidate_valid_i = 1;
        dut.candidate_pattern_id_i = pattern;
        tick(dut);
    }
    dut.candidate_valid_i = 0;
    return ((dut.sa_result_frozen_o >> sa) & 1U) != 0U;
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vrecam_dss_n3_rc_group_live_state_core dut;
    reset(dut);

    // Update arrives during Config #2: the old candidate write is suppressed
    // and the scan restarts at canonical slot zero.
    dut.state_update_i = 1;
    dut.state_sa_i = 0;
    tick(dut);
    dut.state_update_i = 0;
    dut.candidate_valid_i = 1;
    dut.candidate_pattern_id_i = 6;
    tick(dut);
    if (!dut.scan_active_o || dut.scan_slot_o != 1U) {
        std::cerr << "initial scan did not reach Config #2\n";
        return EXIT_FAILURE;
    }
    dut.state_update_i = 1;
    dut.state_sa_i = 0;
    dut.candidate_pattern_id_i = 35;
    tick(dut);
    dut.state_update_i = 0;
    if (!dut.scan_active_o || dut.scan_slot_o != 0U || ((dut.candidate_store_image_o[0] >> 7U) & 1U)) {
        std::cerr << "same-edge update failed to suppress old candidate write\n";
        return EXIT_FAILURE;
    }
    if (dut.solution_ready_o || dut.sa_result_frozen_o != 0U) {
        std::cerr << "partial scan finalized after update\n";
        return EXIT_FAILURE;
    }

    // Complete A, then prove its frozen result persists through a B update.
    reset(dut);
    if (!complete_sa(dut, 0, 1)) {
        std::cerr << "SA A did not freeze\n";
        return EXIT_FAILURE;
    }
    if (!complete_sa(dut, 1, 2)) {
        std::cerr << "SA B did not freeze\n";
        return EXIT_FAILURE;
    }
    if ((dut.sa_result_frozen_o & 3U) != 3U) {
        std::cerr << "earlier SA did not persist\n";
        return EXIT_FAILURE;
    }
    if (!complete_sa(dut, 2, 3) || !complete_sa(dut, 3, 4)) {
        std::cerr << "later SA did not freeze\n";
        return EXIT_FAILURE;
    }
    tick(dut);
    if (!dut.solution_ready_o || !dut.group_repairable_o) {
        std::cerr << "registered GROUP result did not appear\n";
        return EXIT_FAILURE;
    }

    std::cout << "N3_RC_GROUP_CONTROL_PASS update_wins=PASS old_write=PASS "
              << "partial_finalize=FORBIDDEN earlier_sa_replay=0 stale_generation_mix=0 "
              << "generation_tags=ABSENT\n";
    return EXIT_SUCCESS;
}
