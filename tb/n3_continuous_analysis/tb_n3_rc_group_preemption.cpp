#include "Vrecam_dss_n3_rc_group_live_state_core.h"
#include "verilated.h"

#include <cstdlib>
#include <iostream>

namespace {

void tick(Vrecam_dss_n3_rc_group_live_state_core& dut) {
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
    dut.clk_i = 0;
    dut.eval();
}

void idle_inputs(Vrecam_dss_n3_rc_group_live_state_core& dut) {
    dut.state_update_i = 0;
    dut.state_sa_i = 0;
    dut.test_done_valid_i = 0;
    dut.test_done_sa_i = 0;
    dut.candidate_valid_i = 0;
    dut.candidate_pattern_id_i = 0;
}

void reset(Vrecam_dss_n3_rc_group_live_state_core& dut) {
    dut.rst_ni = 0;
    idle_inputs(dut);
    tick(dut);
    tick(dut);
    dut.rst_ni = 1;
    tick(dut);
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
    idle_inputs(dut);
    return ((dut.sa_result_frozen_o >> sa) & 1U) != 0U;
}

bool candidate_valid_at(const Vrecam_dss_n3_rc_group_live_state_core& dut,
                        unsigned sa, unsigned slot) {
    const unsigned bit = (sa * 4U + slot) * 7U;
    return ((dut.candidate_store_image_o[bit >> 5U] >> (bit & 31U)) & 1U) != 0U;
}

bool test_preemption_at_config(Vrecam_dss_n3_rc_group_live_state_core& dut,
                               unsigned slot) {
    reset(dut);
    if (!complete_sa(dut, 0, 1U)) {
        return false;
    }
    for (unsigned a_slot = 0; a_slot < 4; ++a_slot) {
        if (!candidate_valid_at(dut, 0, a_slot)) {
            return false;
        }
    }

    dut.state_update_i = 1;
    dut.state_sa_i = 1;
    tick(dut);
    dut.state_update_i = 0;
    for (unsigned earlier_slot = 0; earlier_slot < slot; ++earlier_slot) {
        dut.candidate_valid_i = 1;
        dut.candidate_pattern_id_i = 6U + earlier_slot;
        tick(dut);
    }
    if (!dut.scan_active_o || dut.active_sa_o != 1U || dut.scan_slot_o != slot) {
        return false;
    }

    dut.state_update_i = 1;
    dut.state_sa_i = 1;
    dut.candidate_valid_i = 1;
    dut.candidate_pattern_id_i = 35U;
    tick(dut);
    idle_inputs(dut);
    if (!dut.scan_active_o || dut.active_sa_o != 1U || dut.scan_slot_o != 0U ||
        candidate_valid_at(dut, 1, slot) || dut.solution_ready_o ||
        ((dut.sa_result_frozen_o & 3U) != 1U)) {
        return false;
    }
    for (unsigned a_slot = 0; a_slot < 4; ++a_slot) {
        if (!candidate_valid_at(dut, 0, a_slot)) {
            return false;
        }
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vrecam_dss_n3_rc_group_live_state_core dut;
    bool passed = true;
    for (unsigned slot = 0; slot < 4; ++slot) {
        const bool this_case = test_preemption_at_config(dut, slot);
        passed = passed && this_case;
        std::cout << "UPDATE_PREEMPTION_CONFIG_" << (slot + 1U) << ":"
                  << (this_case ? "PASS" : "FAIL") << "\n";
    }
    if (!passed) {
        return EXIT_FAILURE;
    }
    std::cout << "N3_RC_GROUP_PREEMPTION_PASS fault_update_wins=PASS "
              << "same_edge_old_result_suppressed=PASS "
              << "same_edge_old_candidate_write=SUPPRESSED "
              << "partial_scan_finalization=FORBIDDEN_AND_VERIFIED "
              << "earlier_sa_records_preserved=PASS "
              << "earlier_sa_replay=0 stale_generation_mix=0\n";
    return EXIT_SUCCESS;
}
