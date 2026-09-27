#include "Vtb_n2_ca_live_group_lockstep_top.h"

#include <cstdint>
#include <iostream>

static void tick(Vtb_n2_ca_live_group_lockstep_top& dut) {
    dut.clk_i = 0; dut.eval(); dut.clk_i = 1; dut.eval(); dut.clk_i = 0; dut.eval();
}

static void reset(Vtb_n2_ca_live_group_lockstep_top& dut) {
    dut.rst_ni = 0; dut.canonical_start_i = 0; dut.live_state_update_i = 0;
    dut.live_test_done_valid_i = 0; dut.candidate_image_lo_i = 0; dut.candidate_image_hi_i = 0;
    tick(dut); tick(dut); dut.rst_ni = 1;
}

static uint64_t entry(unsigned index, unsigned pattern) {
    return (static_cast<uint64_t>((pattern << 1U) | 1U)) << (index * 5U);
}

int main() {
    Vtb_n2_ca_live_group_lockstep_top dut;
    reset(dut);
    dut.candidate_image_lo_i = entry(0, 1) | entry(1, 2);
    dut.live_state_update_i = 1; dut.live_state_sa_i = 0;
    tick(dut);
    dut.live_state_update_i = 0;
    tick(dut);
    if ((dut.live_candidate_store_o[0] & 0x1FU) == 0U) return 1;
    dut.live_state_update_i = 1;
    tick(dut);
    dut.live_state_update_i = 0;
    if ((dut.live_candidate_store_o[0] & 0x1FU) == 0U ||
        ((dut.live_candidate_store_o[0] >> 5U) & 0x1FU) != 0U) return 1;
    std::cout << "N2_CA_LIVE_GROUP_DIRECTED_PASS fault_update_wins=YES "
              << "old_candidate_write=NO map_clear=NO\n";
    return 0;
}
