#include "Vdss_v2_rs3cs3m1_group_candidate_store.h"
#include <iostream>
int main() {
    Vdss_v2_rs3cs3m1_group_candidate_store dut;
    int failures = 0;
    dut.rst_ni = 0; dut.write_enable_i = 0; dut.write_sa_i = dut.write_slot_i = 0;
    dut.write_valid_i = 0; dut.write_pattern_id_i = 0; dut.read_sa_i = dut.read_slot_i = 0;
    dut.clk_i = 0; dut.eval(); dut.clk_i = 1; dut.eval(); dut.rst_ni = 1;
    for (unsigned sa = 0; sa != 4; ++sa) for (unsigned slot = 0; slot != 4; ++slot) {
        dut.write_enable_i = 1; dut.write_sa_i = sa; dut.write_slot_i = slot;
        dut.write_valid_i = 1; dut.write_pattern_id_i = 20 + sa * 4 + slot;
        dut.clk_i = 0; dut.eval(); dut.clk_i = 1; dut.eval(); dut.write_enable_i = 0;
    }
    for (unsigned sa = 0; sa != 4; ++sa) for (unsigned slot = 0; slot != 4; ++slot) {
        dut.read_sa_i = sa; dut.read_slot_i = slot; dut.eval();
        if (!dut.read_valid_o || dut.read_pattern_id_o != 20 + sa * 4 + slot) ++failures;
    }
    std::cout << "TARGET_GROUP_HISTORY_112_BITS " << (failures ? "FAIL" : "PASS") << '\n';
    return failures ? 1 : 0;
}
