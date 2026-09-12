#include "Vrepair_decode_with_hybrid.h"

#include <cstdlib>
#include <iostream>

static void require(bool value, const char* message) {
    if (!value) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

int main() {
    Vrepair_decode_with_hybrid dut;

    dut.config_id_i = 0;  // CFG0: 2R2C
    dut.pattern_id_i = 1; // RRCC, for the ordered reconstructed dictionary.
    dut.pivot_valid_i = 1;
    dut.pivot_rows_flat_i = 3;
    dut.pivot_cols_flat_i = 9;
    dut.hybrid_valid_i = 1;
    dut.hybrid_descriptors_i = 1; // same-column Hybrid: add a row dictionary entry.
    dut.hybrid_rows_flat_i[0] = 4;
    dut.hybrid_cols_flat_i[0] = 9;
    dut.hybrid_ptrs_flat_i = 0;
    dut.hybrid_cfg_valid_flat_i[0] = 1; // Hybrid record 0 is active for CFG0.
    dut.row_must_by_cfg_i = 0;
    dut.col_must_by_cfg_i = 0;
    dut.eval();

    require(dut.decode_valid_o, "Hybrid-aware decoder rejected a valid PatternID");
    require((dut.repair_row_valid_o & 0x3) == 0x3,
            "Pattern RRCC did not select two reconstructed row entries");
    require((dut.repair_rows_flat_o & 0x3ff) == 3,
            "First reconstructed row differs from PivotPayload");
    require(((dut.repair_rows_flat_o >> 10) & 0x3ff) == 4,
            "Retained Hybrid delta did not reconstruct the second row");
    require(!dut.repair_col_valid_o,
            "Pattern RRCC unexpectedly selected a conventional column repair");

    std::cout << "repair_decode_with_hybrid_test PASS\n";
}
