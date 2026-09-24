#include "Vdss_early_selected_address_mux.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void put_field(WData *bits, int offset, int width, std::uint32_t value)
{
    for (int bit = 0; bit < width; ++bit) {
        const int index = offset + bit;
        bits[index / 32] = (bits[index / 32] & ~(1U << (index % 32))) |
            (((value >> bit) & 1U) << (index % 32));
    }
}

std::uint32_t get_field(const WData *bits, int offset, int width)
{
    std::uint32_t value = 0;
    for (int bit = 0; bit < width; ++bit) {
        const int index = offset + bit;
        value |= ((bits[index / 32] >> (index % 32)) & 1U) << bit;
    }
    return value;
}
} // namespace

int main()
{
    Vdss_early_selected_address_mux dut;
    dut.selected_config_i = 2;
    dut.selected_pattern_id_i = 1;
    dut.pivot_rows_flat_i = 0;
    for (auto &word : dut.pivot_cols_flat_i) word = 0;
    const std::uint32_t columns[5] = {1, 257, 4095, 8191, 256};
    for (int slot = 0; slot < 5; ++slot) {
        dut.pivot_rows_flat_i |= static_cast<QData>(20 + slot) << (slot * 9);
        put_field(dut.pivot_cols_flat_i, slot * 13, 13, columns[slot]);
    }
    dut.eval();

    require(dut.solution_line_valid_o == 0x1f,
            "all five config-2 solution slots must be visible");
    require(get_field(dut.solution_line_address_flat_o, 0, 13) == 1,
            "streamed physical column 1 must be preserved");
    require(get_field(dut.solution_line_address_flat_o, 13, 13) == 257,
            "streamed physical column 257 must be preserved");
    require(get_field(dut.solution_line_address_flat_o, 0, 13) !=
            get_field(dut.solution_line_address_flat_o, 13, 13),
            "streamed physical columns 1 and 257 must not alias");
    require(dut.solution_line_is_row_o == 0x18,
            "config-2 pattern-1 row/column selection must remain canonical");

    std::cout << "EARLY_STREAMING_ADDRESS_MUX=PASS\n";
    std::cout << "EARLY_STREAMING_ADDRESS_NO_ALIAS=PASS\n";
    return 0;
}
