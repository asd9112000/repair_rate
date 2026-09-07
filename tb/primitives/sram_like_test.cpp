#include "Vsram_like.h"
#include "verilated.h"

#include <cstdlib>
#include <iostream>

namespace
{
void tick(Vsram_like &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
    dut.clk_i = 0;
    dut.eval();
}

void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << "sram_like_test: " << message << '\n';
        std::exit(1);
    }
}
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Vsram_like dut;
    dut.rst_ni = 0;
    dut.clear_i = 0;
    dut.write_valid_i = 0;
    dut.read_valid_i = 0;
    tick(dut);
    dut.rst_ni = 1;

    dut.write_valid_i = 1;
    dut.write_addr_i = 4;
    dut.write_data_i = 0xbeef;
    dut.write_entry_valid_i = 1;
    tick(dut);
    dut.write_valid_i = 0;

    dut.read_valid_i = 1;
    dut.read_addr_i = 4;
    tick(dut);
    require(dut.read_data_valid_o, "registered read response is missing");
    require(dut.read_entry_valid_o && dut.read_data_o == 0xbeef,
            "registered readback differs from the write");

    dut.read_valid_i = 0;
    dut.write_valid_i = 1;
    dut.write_addr_i = 4;
    dut.write_data_i = 0xbeef;
    dut.write_entry_valid_i = 0;
    tick(dut);
    dut.write_valid_i = 0;
    dut.read_valid_i = 1;
    dut.read_addr_i = 4;
    tick(dut);
    require(dut.read_data_valid_o && !dut.read_entry_valid_o,
            "valid-bit clear was not preserved by the SRAM-like array");

    dut.clear_i = 1;
    tick(dut);
    require(!dut.read_data_valid_o, "clear did not reset the read response");
    std::cout << "sram_like_test PASS\n";
    return 0;
}
