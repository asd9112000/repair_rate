#include "Vtb_n2_ca_live_early_lockstep_top.h"

#include <cstdint>
#include <iostream>

static void tick(Vtb_n2_ca_live_early_lockstep_top& dut) {
    dut.clk_i = 0; dut.eval(); dut.clk_i = 1; dut.eval(); dut.clk_i = 0; dut.eval();
}

static void reset(Vtb_n2_ca_live_early_lockstep_top& dut) {
    dut.rst_ni = 0; dut.canonical_start_i = 0; dut.live_state_update_i = 0;
    dut.live_test_done_valid_i = 0; dut.candidate_image_lo_i = 0; dut.candidate_image_hi_i = 0;
    tick(dut); tick(dut); dut.rst_ni = 1;
}

static uint64_t entry(unsigned index, unsigned pattern) {
    return (static_cast<uint64_t>((pattern << 1U) | 1U)) << (index * 5U);
}

int main() {
    Vtb_n2_ca_live_early_lockstep_top dut;
    reset(dut);
    dut.candidate_image_lo_i = entry(1, 1);
    dut.live_state_update_i = 1; dut.live_state_sa_i = 0;
    dut.live_test_done_valid_i = 1; dut.live_test_done_sa_i = 0;
    tick(dut);
    dut.live_test_done_valid_i = 0; dut.live_state_update_i = 1;
    tick(dut);
    if (dut.live_commit_mask_o != 0U) { std::cerr << "preempt mask=" << unsigned(dut.live_commit_mask_o) << "\n"; return 1; }
    dut.live_state_update_i = 0;
    tick(dut);
    if ((dut.live_commit_mask_o & 1U) == 0U || dut.live_config_flat_o != 4U) { std::cerr << "preempt final mask=" << unsigned(dut.live_commit_mask_o) << " cfg=" << dut.live_config_flat_o << "\n"; return 1; }

    reset(dut);
    dut.candidate_image_lo_i = entry(1, 1) | entry(6, 1) | entry(9, 1) |
        entry(12, 1);
    dut.candidate_image_hi_i = static_cast<uint16_t>(11U << 11U);
    for (unsigned sa = 0; sa < 4; ++sa) {
        dut.live_state_update_i = 1; dut.live_state_sa_i = sa;
        dut.live_test_done_valid_i = 1; dut.live_test_done_sa_i = sa;
        tick(dut);
        dut.live_state_update_i = 0; dut.live_test_done_valid_i = 0;
        for (unsigned cycle = 0; cycle < 5 && ((dut.live_commit_mask_o >> sa) & 1U) == 0U; ++cycle)
            tick(dut);
    }
    if (dut.live_commit_mask_o != 0xFU || ((dut.live_config_flat_o >> 9U) & 7U) != 6U ||
        ((dut.live_pattern_flat_o >> 12U) & 15U) != 5U) { std::cerr << "vector27 mask=" << unsigned(dut.live_commit_mask_o) << " cfg=" << dut.live_config_flat_o << " pat=" << dut.live_pattern_flat_o << "\n"; return 1; }
    std::cout << "N2_CA_LIVE_EARLY_DIRECTED_PASS vector27=CFG6_PATTERN5 fault_update_wins=YES\n";
    return 0;
}
