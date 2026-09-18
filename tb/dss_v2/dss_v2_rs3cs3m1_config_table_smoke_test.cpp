#include "Vdss_v2_rs3cs3m1_config_table.h"
#include <iostream>
int main() {
    Vdss_v2_rs3cs3m1_config_table dut;
    int failures = 0;
    const unsigned cfg[4][4] = {{0,1,2,3},{0,4,5,6},{0,4,5,6},{0,1,2,3}};
    const unsigned rows[4][4] = {{3,2,3,2},{3,3,4,4},{3,3,4,4},{3,2,3,2}};
    const unsigned cols[4][4] = {{3,3,4,4},{3,2,3,2},{3,2,3,2},{3,3,4,4}};
    const unsigned action_for_slot[4] = {0,2,1,3};
    for (unsigned sa = 0; sa != 4; ++sa) for (unsigned slot = 0; slot != 4; ++slot) {
        dut.sa_i = sa; dut.slot_i = slot; dut.eval();
        if (!dut.valid_o || dut.config_id_o != cfg[sa][slot] || dut.row_count_o != rows[sa][slot] ||
            dut.col_count_o != cols[sa][slot] || dut.action_o != action_for_slot[slot]) ++failures;
    }
    std::cout << "TARGET_CONFIG_TABLE_7_CONFIGS " << (failures ? "FAIL" : "PASS") << '\n';
    return failures ? 1 : 0;
}
