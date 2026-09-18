#include "Vrecam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_candidate_map_producer.h"
#include "verilated.h"

#include <iostream>

namespace
{
using Dut = Vrecam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_candidate_map_producer;

void clearWide(WData *signal, int words) { for (int word = 0; word < words; ++word) signal[word] = 0; }
bool bit(const WData *signal, int index) { return (signal[index / 32] & (1U << (index % 32))) != 0U; }
void tick(Dut &dut) { dut.clk_i = 0; dut.eval(); dut.clk_i = 1; dut.eval(); }
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Dut dut;
    dut.rst_ni = 0; dut.start_i = 0; dut.pivot_valid_flat_i = 0;
    clearWide(dut.pivot_rows_flat_i, 6); clearWide(dut.pivot_cols_flat_i, 4);
    dut.row_gt1_flat_i = 0; dut.row_gt2_flat_i = 0; dut.row_gt3_flat_i = 0;
    dut.col_gt1_flat_i = 0; dut.col_gt2_flat_i = 0; dut.col_gt3_flat_i = 0;
    dut.hybrid_valid_flat_i = 0; clearWide(dut.hybrid_pointer_flat_i, 3);
    dut.hybrid_descriptor_flat_i = 0; clearWide(dut.hybrid_differing_flat_i, 8);
    dut.conventional_overflow_i = 0; tick(dut); dut.rst_ni = 1; dut.start_i = 1; tick(dut); dut.start_i = 0;
    for (int cycle = 0; cycle < 32; ++cycle) {
        tick(dut);
        int count[4] = {0, 0, 0, 0};
        for (int index = 0; index < 160; ++index) count[index / 40] += bit(dut.candidate_valid_o, index);
        std::cout << "cycle=" << cycle << " active_sa=" << static_cast<int>(dut.active_sa_o)
                  << " active_action=" << static_cast<int>(dut.active_action_o)
                  << " config=" << static_cast<int>(dut.active_config_id_o)
                  << " count=" << count[0] << ',' << count[1] << ',' << count[2] << ',' << count[3]
                  << " done=" << static_cast<int>(dut.done_o) << '\n';
        if (dut.done_o) return count[0] == 23 && count[1] == 23 && count[2] == 23 && count[3] == 23 ? 0 : 1;
    }
    return 1;
}
