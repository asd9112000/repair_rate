#include "Vtb_n2_ca_live_group_full_top.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

static void tick(Vtb_n2_ca_live_group_full_top& dut) {
    dut.clk_i=0; dut.eval(); dut.clk_i=1; dut.eval(); dut.clk_i=0; dut.eval();
}

static void reset(Vtb_n2_ca_live_group_full_top& dut) {
    dut.rst_ni=0; dut.canonical_start_i=0; dut.live_state_update_i=0;
    dut.live_test_done_valid_i=0; dut.state_seed_flat_i=0; tick(dut); tick(dut); dut.rst_ni=1;
}

static uint32_t random_next(uint32_t& value) {
    value^=value<<13; value^=value>>17; value^=value<<5; return value;
}

static bool same(const Vtb_n2_ca_live_group_full_top& dut) {
    if (dut.canonical_repairable_o != dut.live_repairable_o || dut.canonical_commit_o != dut.live_commit_o ||
        dut.canonical_config_o != dut.live_config_o || dut.canonical_pattern_o != dut.live_pattern_o ||
        dut.canonical_donor_o != dut.live_donor_o || dut.canonical_borrow_o != dut.live_borrow_o ||
        dut.canonical_release_o != dut.live_release_o || dut.canonical_is_row_o != dut.live_is_row_o ||
        dut.canonical_line_valid_o != dut.live_line_valid_o) return false;
    for (unsigned index=0; index<9; ++index)
        if (dut.canonical_address_o[index] != dut.live_address_o[index]) return false;
    return true;
}

int main(int argc, char** argv) {
    const unsigned vectors=argc==2?std::strtoul(argv[1],nullptr,10):1000U;
    uint32_t random=0x20260928U; Vtb_n2_ca_live_group_full_top dut;
    for (unsigned vector=0; vector<vectors; ++vector) {
        reset(dut); dut.state_seed_flat_i=random_next(random); dut.canonical_start_i=1; tick(dut); dut.canonical_start_i=0;
        for(unsigned cycle=0; cycle<24 && !dut.canonical_done_o; ++cycle) tick(dut);
        if(!dut.canonical_done_o) return 1;
        for(unsigned sa=0; sa<4; ++sa) {
            const bool done_first=(vector&1U)!=0U;
            dut.live_state_update_i=1; dut.live_state_sa_i=sa; dut.live_test_done_valid_i=done_first; dut.live_test_done_sa_i=sa;
            tick(dut); dut.live_state_update_i=0; dut.live_test_done_valid_i=0;
            for(unsigned cycle=0; cycle<8 && ((dut.live_frozen_o>>sa)&1U)==0U; ++cycle) {
                if(!done_first && !dut.live_scan_active_o) { dut.live_test_done_valid_i=1; dut.live_test_done_sa_i=sa; }
                tick(dut); dut.live_test_done_valid_i=0;
            }
            if(((dut.live_frozen_o>>sa)&1U)==0U) return 1;
        }
        for(unsigned cycle=0; cycle<4 && !dut.live_ready_o; ++cycle) tick(dut);
        if(!dut.live_ready_o || !same(dut)) { std::cerr << "full-top mismatch vector=" << vector << '\n'; return 1; }
    }
    std::cout << "N2_CA_LIVE_GROUP_FULL_TOP_PASS vectors=" << vectors << " mismatches=0 address_lockstep=PASS\n";
    return 0;
}
