#include "Vshared_fault_counter.h"
#include <cstdlib>
#include <iostream>
namespace { void require(bool value, const char *message) { if (!value) { std::cerr << message << '\n'; std::exit(1); } }
void tick(Vshared_fault_counter &d) { d.clk_i=0; d.eval(); d.clk_i=1; d.eval(); }
void push(Vshared_fault_counter &d, unsigned r, unsigned c) { d.fault_row_i=r; d.fault_col_i=c; d.fault_valid_i=1; tick(d); d.fault_valid_i=0; d.eval(); }
unsigned bits(uint32_t value, unsigned offset, unsigned width) { return (value >> offset) & ((1U << width) - 1U); } }
int main() {
    Vshared_fault_counter d; d.rst_ni=0; d.clear_i=0; d.fault_valid_i=0; tick(d); d.rst_ni=1;
    push(d, 7, 1); push(d, 7, 2); push(d, 8, 1);
    require(bits(d.row_counts_flat_o, 0, 4) == 2, "row 7 count was not shared/incremented");
    require(bits(d.col_counts_flat_o, 0, 4) == 2, "column 1 count was not shared/incremented");
    require(bits(d.row_addrs_flat_o[0], 0, 10) == 7 && bits(d.col_addrs_flat_o[0], 0, 10) == 1,
            "counter address provenance changed");
    std::cout << "shared_fault_counter_test PASS\n";
}
