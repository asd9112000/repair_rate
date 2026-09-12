#include "Vrecam_dss_v2_group_core.h"

#include <array>
#include <cstdlib>
#include <iostream>

struct Candidate { bool valid = false; unsigned pattern = 0; };
using Maps = std::array<std::array<Candidate, 4>, 4>;

static void tick(Vrecam_dss_v2_group_core& dut) {
    dut.clk_i = 0; dut.eval(); dut.clk_i = 1; dut.eval();
}
static void put(Maps& maps, unsigned sa, unsigned slot) { maps[sa][slot] = {true, sa + slot + 1}; }
static unsigned run(const char* name, const Maps& maps, bool expected_repairable,
                    unsigned expected_failure, unsigned expected_cycles) {
    Vrecam_dss_v2_group_core dut;
    dut.rst_ni = 0; dut.start_i = 0; dut.candidate_valid_i = 0; dut.candidate_pattern_id_i = 0;
    tick(dut); dut.rst_ni = 1; dut.start_i = 1; tick(dut); dut.start_i = 0;
    unsigned collection_cycles = 0;
    for (unsigned edge = 1; edge <= 64; ++edge) {
        dut.eval();
        if (dut.collection_active_o) {
            ++collection_cycles;
            const Candidate& candidate = maps[dut.current_sa_o][dut.current_slot_o];
            dut.candidate_valid_i = candidate.valid;
            dut.candidate_pattern_id_i = candidate.pattern;
        } else {
            dut.candidate_valid_i = 0; dut.candidate_pattern_id_i = 0;
        }
        tick(dut);
        if (dut.done_o) {
            if (collection_cycles != 16 || edge != expected_cycles ||
                bool(dut.group_repairable_o) != expected_repairable ||
                (!expected_repairable && dut.failure_position_o != expected_failure)) {
                std::cerr << "FAIL " << name << '\n'; std::exit(1);
            }
            std::cout << "GROUP_LATENCY " << name << " collection_cycles=" << collection_cycles
                      << " allocation_cycles=" << (edge - collection_cycles)
                      << " post_start_cycles=" << edge << '\n';
            return edge;
        }
    }
    std::cerr << "FAIL timeout " << name << '\n'; std::exit(1);
}

int main() {
    Maps all_local{};
    for (unsigned sa = 0; sa != 4; ++sa) for (unsigned slot = 0; slot != 4; ++slot) put(all_local, sa, slot);
    run("ALL_LOCAL", all_local, true, 0, 20);

    run("FAILURE_A", Maps{}, false, 0, 20);
    Maps failure_b{}; put(failure_b, 0, 1);
    run("FAILURE_B", failure_b, false, 1, 21);
    Maps failure_c{}; put(failure_c, 0, 1); put(failure_c, 1, 1);
    run("FAILURE_C", failure_c, false, 2, 22);
    Maps failure_d{}; put(failure_d, 0, 1); put(failure_d, 1, 1); put(failure_d, 2, 1);
    run("FAILURE_D", failure_d, false, 3, 23);

    Maps slot2_last{};
    put(slot2_last, 0, 1); // A release A_ROW.
    put(slot2_last, 1, 2); // B last-ranked borrow A_ROW.
    put(slot2_last, 2, 1); // C release C_COL.
    put(slot2_last, 3, 2); // D last-ranked borrow C_COL.
    run("SLOT2_LAST_SUCCESS", slot2_last, true, 0, 26);

    Maps release_borrow{};
    put(release_borrow, 0, 1); // A releases A_ROW.
    put(release_borrow, 1, 3); // B rank-2 release+borrow.
    put(release_borrow, 2, 1); put(release_borrow, 3, 1);
    run("RELEASE_BORROW", release_borrow, true, 0, 22);
}
