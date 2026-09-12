#include "Vshared_pivot_cam.h"

#include <cstdlib>
#include <iostream>

namespace {
void require(bool value, const char *message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
void tick(Vshared_pivot_cam &dut) {
    dut.clk_i = 0; dut.eval();
    dut.clk_i = 1; dut.eval();
}
void push(Vshared_pivot_cam &dut, unsigned row, unsigned col) {
    dut.pivot_row_i = row; dut.pivot_col_i = col; dut.pivot_write_i = 1;
    tick(dut); dut.pivot_write_i = 0; dut.eval();
}
unsigned cfg_valid(const Vshared_pivot_cam &dut, unsigned entry, unsigned cfg) {
    return (dut.cfg_entry_valid_o >> (entry * 7 + cfg)) & 1U;
}
}
int main() {
    Vshared_pivot_cam dut;
    dut.rst_ni = 0; dut.clear_i = 0; dut.pivot_write_i = 0; tick(dut);
    dut.rst_ni = 1; tick(dut);
    for (unsigned i = 0; i < 5; ++i) push(dut, 10 + i, 20 + i);
    require(dut.occupancy_o == 5 && dut.full_o, "five pivots did not fill shared payload");
    require(cfg_valid(dut, 2, 1) && !cfg_valid(dut, 3, 1), "2R1C view is not a three-entry prefix");
    require(cfg_valid(dut, 3, 0) && !cfg_valid(dut, 4, 0), "2R2C view is not a four-entry prefix");
    require(cfg_valid(dut, 4, 2), "3R2C view is not a five-entry prefix");
    require((dut.rows_flat_o & 0x3ffU) == 10 && (dut.cols_flat_o & 0x3ffU) == 20,
            "pivot payload did not retain arrival-order addresses");
    push(dut, 99, 99);
    require(dut.overflow_o, "sixth pivot did not report shared-payload overflow");
    std::cout << "shared_pivot_cam_test PASS\n";
}
