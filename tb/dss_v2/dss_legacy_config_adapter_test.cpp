#include "Vdss_legacy_config_adapter.h"

#include <cstdlib>
#include <iostream>

static void require(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}

int main() {
    Vdss_legacy_config_adapter dut;
    const unsigned rows[] = {2, 2, 3, 3, 1, 2, 1};
    const unsigned cols[] = {2, 1, 2, 1, 2, 3, 3};
    for (unsigned id = 0; id != 7; ++id) {
        dut.legacy_config_id_i = id;
        dut.config_descriptor_i = (rows[id] << 3) | cols[id];
        dut.eval();
        require(dut.legacy_config_valid_o, "valid legacy ConfigID rejected");
        require(((dut.config_descriptor_o >> 3) & 7) == rows[id], "wrong descriptor row count");
        require((dut.config_descriptor_o & 7) == cols[id], "wrong descriptor col count");
        require(dut.descriptor_valid_o && dut.legacy_config_id_o == id, "legacy round trip failed");
    }
    dut.legacy_config_id_i = 7;
    dut.config_descriptor_i = (4 << 3) | 4;
    dut.eval();
    require(!dut.legacy_config_valid_o, "CFG7 was not invalid");
    require(!dut.descriptor_valid_o && dut.legacy_config_id_o == 7,
            "unsupported descriptor did not explicitly report invalid");
    std::cout << "DSS_V2_PHASE4A_CONFIG_ADAPTER PASS\n";
}
