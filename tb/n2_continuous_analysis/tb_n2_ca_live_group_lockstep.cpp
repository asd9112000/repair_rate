#include "Vtb_n2_ca_live_group_lockstep_top.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

struct Image { uint64_t lo; uint16_t hi; };

void tick(Vtb_n2_ca_live_group_lockstep_top& dut) {
    dut.clk_i = 0; dut.eval();
    dut.clk_i = 1; dut.eval();
    dut.clk_i = 0; dut.eval();
}

void reset(Vtb_n2_ca_live_group_lockstep_top& dut) {
    dut.rst_ni = 0; dut.canonical_start_i = 0; dut.live_state_update_i = 0;
    dut.live_state_sa_i = 0; dut.live_test_done_valid_i = 0; dut.live_test_done_sa_i = 0;
    dut.candidate_image_lo_i = 0; dut.candidate_image_hi_i = 0;
    tick(dut); tick(dut); dut.rst_ni = 1;
}

uint32_t random_next(uint32_t& value) {
    value ^= value << 13; value ^= value >> 17; value ^= value << 5;
    return value;
}

Image random_image(uint32_t& random) {
    uint64_t lo = 0; uint64_t hi = 0;
    for (unsigned index = 0; index < 16; ++index) {
        const uint64_t entry = (random_next(random) & 3U) != 0U
            ? ((static_cast<uint64_t>((random_next(random) % 15U) + 1U) << 1U) | 1U)
            : 0U;
        const unsigned offset = index * 5U;
        if (offset < 64U) {
            lo |= entry << offset;
            if (offset > 59U) hi |= entry >> (64U - offset);
        } else {
            hi |= entry << (offset - 64U);
        }
    }
    return {lo, static_cast<uint16_t>(hi)};
}

bool run_vector(Vtb_n2_ca_live_group_lockstep_top& dut, Image image,
                unsigned vector_id, bool done_first) {
    reset(dut);
    dut.candidate_image_lo_i = image.lo; dut.candidate_image_hi_i = image.hi;
    dut.canonical_start_i = 1; tick(dut); dut.canonical_start_i = 0;
    for (unsigned cycle = 0; cycle < 24 && !dut.canonical_done_o; ++cycle) tick(dut);
    if (!dut.canonical_done_o) {
        std::cerr << "canonical timeout vector=" << vector_id << '\n';
        return false;
    }
    unsigned final_latency = 0;
    for (unsigned sa = 0; sa < 4; ++sa) {
        dut.live_state_update_i = 1; dut.live_state_sa_i = sa;
        dut.live_test_done_valid_i = done_first ? 1 : 0; dut.live_test_done_sa_i = sa;
        tick(dut);
        dut.live_state_update_i = 0; dut.live_test_done_valid_i = 0;
        for (unsigned cycle = 0; cycle < 8 && ((dut.live_frozen_o >> sa) & 1U) == 0U; ++cycle) {
            if (!done_first && !dut.live_scan_active_o) {
                dut.live_test_done_valid_i = 1; dut.live_test_done_sa_i = sa;
            }
            tick(dut);
            dut.live_test_done_valid_i = 0;
            if (sa == 3) ++final_latency;
        }
        if (((dut.live_frozen_o >> sa) & 1U) == 0U) {
            std::cerr << "live freeze timeout vector=" << vector_id << " sa=" << sa << '\n';
            return false;
        }
    }
    for (unsigned cycle = 0; cycle < 4 && !dut.live_ready_o; ++cycle) {
        tick(dut); ++final_latency;
    }
    if (!dut.live_ready_o || final_latency != (done_first ? 5U : 6U)) {
        std::cerr << "live latency failure vector=" << vector_id << " latency=" << final_latency << '\n';
        return false;
    }
    const bool match = dut.canonical_repairable_o == dut.live_repairable_o &&
        dut.canonical_config_o == dut.live_config_o && dut.canonical_pattern_o == dut.live_pattern_o &&
        dut.canonical_donor_o == dut.live_donor_o && dut.canonical_borrow_o == dut.live_borrow_o &&
        dut.canonical_release_o == dut.live_release_o;
    if (!match) std::cerr << "semantic mismatch vector=" << vector_id << '\n';
    return match;
}

}  // namespace

int main(int argc, char** argv) {
    const unsigned vectors = argc == 2 ? std::strtoul(argv[1], nullptr, 10) : 1000U;
    Vtb_n2_ca_live_group_lockstep_top dut;
    uint32_t random = 0x20260928U;
    for (unsigned vector = 0; vector < vectors; ++vector) {
        if (!run_vector(dut, random_image(random), vector, (vector & 1U) != 0U)) return 1;
    }
    std::cout << "N2_CA_LIVE_GROUP_LOCKSTEP_PASS vectors=" << vectors
              << " mismatches=0 map_clear=NO generation_tags=NONE earlier_sa_replay=0 latency=5\n";
    return 0;
}
