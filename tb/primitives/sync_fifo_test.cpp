#include "Vsync_fifo.h"
#include "verilated.h"

#include <cstdlib>
#include <iostream>

namespace
{
void tick(Vsync_fifo &dut)
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
        std::cerr << "sync_fifo_test: " << message << '\n';
        std::exit(1);
    }
}

void push(Vsync_fifo &dut, unsigned value)
{
    require(dut.push_ready_o, "FIFO refused a legal push");
    dut.push_valid_i = 1;
    dut.push_data_i = value;
    tick(dut);
    dut.push_valid_i = 0;
}

void popAndExpect(Vsync_fifo &dut, unsigned value)
{
    require(dut.pop_valid_o && dut.pop_data_o == value,
            "FIFO pop order/data mismatch");
    dut.pop_ready_i = 1;
    tick(dut);
    dut.pop_ready_i = 0;
}
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Vsync_fifo dut;
    dut.rst_ni = 0;
    dut.clear_i = 0;
    dut.push_valid_i = 0;
    dut.pop_ready_i = 0;
    tick(dut);
    dut.rst_ni = 1;
    require(dut.empty_o && !dut.pop_valid_o, "reset FIFO is not empty");

    push(dut, 0x11);
    push(dut, 0x22);
    push(dut, 0x33);
    require(dut.full_o && !dut.push_ready_o, "full flag/backpressure mismatch");
    popAndExpect(dut, 0x11);
    push(dut, 0x44);
    popAndExpect(dut, 0x22);
    popAndExpect(dut, 0x33);
    popAndExpect(dut, 0x44);
    require(dut.empty_o && dut.count_o == 0, "FIFO did not return to empty");

    dut.clear_i = 1;
    tick(dut);
    require(dut.empty_o, "clear did not empty the FIFO");
    std::cout << "sync_fifo_test PASS\n";
    return 0;
}
