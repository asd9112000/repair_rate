#include "Vtb_n2_ca_live_early_lockstep_top.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

unsigned selected_rank_counts[5] = {0, 0, 0, 0, 0};

struct Image { uint64_t lo; uint16_t hi; };

void tick(Vtb_n2_ca_live_early_lockstep_top& dut) {
    dut.clk_i = 0; dut.eval();
    dut.clk_i = 1; dut.eval();
    dut.clk_i = 0; dut.eval();
}

void reset(Vtb_n2_ca_live_early_lockstep_top& dut) {
    dut.rst_ni = 0;
    dut.canonical_start_i = 0;
    dut.live_state_update_i = 0;
    dut.live_state_sa_i = 0;
    dut.live_test_done_valid_i = 0;
    dut.live_test_done_sa_i = 0;
    dut.candidate_image_lo_i = 0;
    dut.candidate_image_hi_i = 0;
    tick(dut); tick(dut);
    dut.rst_ni = 1;
}

uint32_t random_next(uint32_t& value) {
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

Image random_image(uint32_t& random) {
    uint64_t lo = 0;
    uint64_t hi = 0;
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

bool run_vector(Vtb_n2_ca_live_early_lockstep_top& dut, Image image,
                unsigned vector_id, bool done_first) {
    reset(dut);
    dut.candidate_image_lo_i = image.lo;
    dut.candidate_image_hi_i = image.hi;
    dut.canonical_start_i = 1;
    tick(dut);
    dut.canonical_start_i = 0;
    for (unsigned cycle = 0; cycle < 24 && !dut.canonical_done_o; ++cycle) tick(dut);
    if (!dut.canonical_done_o) {
        std::cerr << "canonical timeout vector=" << vector_id << '\n';
        return false;
    }

    for (unsigned sa = 0; sa < 4 && !dut.live_done_o; ++sa) {
        dut.live_state_update_i = 1;
        dut.live_state_sa_i = sa;
        dut.live_test_done_valid_i = done_first ? 1 : 0;
        dut.live_test_done_sa_i = sa;
        tick(dut);
        dut.live_state_update_i = 0;
        dut.live_test_done_valid_i = 0;
        for (unsigned cycle = 0; cycle < 8 && ((dut.live_commit_mask_o >> sa) & 1U) == 0U && !dut.live_done_o; ++cycle) {
            if (!done_first && !dut.live_scan_active_o) {
                dut.live_test_done_valid_i = 1;
                dut.live_test_done_sa_i = sa;
            }
            const bool scan_was_active = dut.live_scan_active_o;
            tick(dut);
            dut.live_test_done_valid_i = 0;
            if (scan_was_active && !dut.live_scan_active_o && !dut.live_done_o)
                ++selected_rank_counts[cycle + 1];
        }
        if (((dut.live_commit_mask_o >> sa) & 1U) == 0U && !dut.live_done_o) {
            std::cerr << "live timeout vector=" << vector_id << " sa=" << sa << '\n';
            return false;
        }
    }
    const bool match = dut.canonical_repairable_o == dut.live_repairable_o &&
        dut.canonical_commit_mask_o == dut.live_commit_mask_o &&
        dut.canonical_config_flat_o == dut.live_config_flat_o &&
        dut.canonical_pattern_flat_o == dut.live_pattern_flat_o;
    if (!match) std::cerr << "semantic mismatch vector=" << vector_id << '\n';
    return match;
}

}  // namespace

int main(int argc, char** argv) {
    const unsigned vectors = argc == 2 ? std::strtoul(argv[1], nullptr, 10) : 1000U;
    Vtb_n2_ca_live_early_lockstep_top dut;
    uint32_t random = 0x20260928U;
    for (unsigned vector = 0; vector < vectors; ++vector) {
        if (!run_vector(dut, random_image(random), vector, (vector & 1U) != 0U)) return 1;
    }
    unsigned selected_count = 0;
    unsigned weighted_cycles = 0;
    unsigned cumulative = 0;
    unsigned median = 0;
    unsigned p95 = 0;
    for (unsigned rank = 1; rank <= 4; ++rank) {
        selected_count += selected_rank_counts[rank];
        weighted_cycles += rank * selected_rank_counts[rank];
    }
    for (unsigned rank = 1; rank <= 4; ++rank) {
        cumulative += selected_rank_counts[rank];
        if (median == 0 && cumulative * 2 >= selected_count) median = rank;
        if (p95 == 0 && cumulative * 20 >= selected_count * 19) p95 = rank;
    }
    std::cout << "N2_CA_LIVE_EARLY_LOCKSTEP_PASS vectors=" << vectors
              << " mismatches=0 done_before_solution=PASS solution_before_done=PASS\n"
              << "rank_counts=" << selected_rank_counts[1] << "," << selected_rank_counts[2]
              << "," << selected_rank_counts[3] << "," << selected_rank_counts[4]
              << " selected_n=" << selected_count
              << " mean=" << (static_cast<double>(weighted_cycles) / selected_count)
              << " median=" << median << " p95=" << p95 << " min=1 max=4\n";
    return 0;
}
