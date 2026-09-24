#include "Vrecam_n2_preopt_candidate_map_producer.h"
#include "verilated.h"

#include <array>
#include <iostream>
#include <stdexcept>

namespace {
using Dut = Vrecam_n2_preopt_candidate_map_producer;
void clear(WData *v, int words) { for (int i = 0; i < words; ++i) v[i] = 0; }
bool bit(const WData *v, int i) { return (v[i / 32] >> (i % 32)) & 1U; }
void tick(Dut &d) { d.clk_i = 0; d.eval(); d.clk_i = 1; d.eval(); }
void require(bool v, const char *why) { if (!v) throw std::runtime_error(why); }
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Dut d; d.rst_ni = 0; d.start_i = 0; d.pivot_valid_flat_i = 0;
    clear(d.pivot_rows_flat_i, 6); clear(d.pivot_cols_flat_i, 9);
    d.row_gt1_flat_i = d.row_gt2_flat_i = d.row_gt3_flat_i = 0;
    d.col_gt1_flat_i = d.col_gt2_flat_i = d.col_gt3_flat_i = 0;
    d.hybrid_valid_flat_i = 0; clear(d.hybrid_pointer_flat_i, 3);
    d.hybrid_descriptor_flat_i = 0; clear(d.hybrid_differing_flat_i, 12);
    d.conventional_overflow_i = 0; tick(d); d.rst_ni = 1; d.start_i = 1; tick(d); d.start_i = 0;
    const std::array<int, 16> expected_config{{0,4,5,6,0,1,2,3,0,1,2,3,0,4,5,6}};
    for (int cycle = 0; cycle < 16; ++cycle) {
        require(d.active_config_id_o == expected_config.at(cycle), "canonical role-to-config selection mismatch");
        tick(d);
    }
    require(d.done_o, "producer did not complete after 16 map writes");
    for (int sa = 0; sa < 4; ++sa) {
        int valid = 0, release = 0, borrow = 0;
        for (int index = sa * 40; index < (sa + 1) * 40; ++index) {
            valid += bit(d.candidate_valid_o, index); release += bit(d.candidate_release_o, index); borrow += bit(d.candidate_borrow_o, index);
        }
        require(valid == 23 && release == 7 && borrow == 14, "map or effect replication mismatch");
    }
    for (int base : {0, 40, 80, 120}) {
        require(bit(d.candidate_valid_o, base), "first map index is not valid");
        require(!bit(d.candidate_valid_o, base + 39), "last map index should be an unused pattern slot");
        require(bit(d.candidate_release_o, base + 10), "R slot release did not map to expected index");
        require(bit(d.candidate_borrow_o, base + 20), "B slot borrow did not map to expected index");
        require(bit(d.candidate_release_o, base + 30) && bit(d.candidate_borrow_o, base + 30), "RB effects did not map to expected index");
    }
    std::cout << "N2 pre-opt dense-map indexing and seven-lane selection PASS\n";
    return 0;
}
