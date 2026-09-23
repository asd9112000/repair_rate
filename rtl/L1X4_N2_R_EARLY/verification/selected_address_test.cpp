#include "Vdss_early_selected_address_regs.h"

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

void tick(Vdss_early_selected_address_regs &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}
} // namespace

int main()
{
    Vdss_early_selected_address_regs dut;
    dut.rst_ni = 0;
    dut.commit_enable_i = 0;
    dut.commit_sa_i = 0;
    dut.selected_config_i = 0;
    dut.selected_pattern_id_i = 0;
    dut.pivot_rows_flat_i = 0;
    for (auto &word : dut.pivot_cols_flat_i) word = 0;
    tick(dut);

    dut.rst_ni = 1;
    dut.selected_config_i = 2;
    dut.selected_pattern_id_i = 1;
    const std::uint32_t columns[5] = {1, 257, 4095, 8191, 256};
    for (int slot = 0; slot < 5; ++slot) {
        dut.pivot_rows_flat_i |= static_cast<QData>(20 + slot) << (slot * 9);
        put_field(dut.pivot_cols_flat_i, slot * 13, 13, columns[slot]);
    }
    dut.commit_enable_i = 1;
    tick(dut);
    dut.commit_enable_i = 0;

    require(dut.final_repair_line_valid_flat_o == 0x1f,
            "all five active config-2 slots must be committed");
    require(get_field(dut.final_repair_address_flat_o, 0, 13) == 1,
            "selected physical column 1 must be preserved");
    require(get_field(dut.final_repair_address_flat_o, 13, 13) == 257,
            "selected physical column 257 must be preserved");
    require(get_field(dut.final_repair_address_flat_o, 0, 13) !=
            get_field(dut.final_repair_address_flat_o, 13, 13),
            "physical columns 1 and 257 must not alias after commit");
    require(dut.final_repair_is_row_flat_o == 0x18,
            "config-2 pattern-1 row/column selection must remain canonical");

    std::cout << "EARLY_SELECTED_ADDRESS_COMMIT=PASS\n";
    std::cout << "EARLY_SELECTED_ADDRESS_NO_ALIAS=PASS\n";
    return 0;
}
