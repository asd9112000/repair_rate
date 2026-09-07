#include "Vpway_search.h"
#include "verilated.h"

#include <cstdlib>
#include <iostream>

namespace
{
void tick(Vpway_search &dut)
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
        std::cerr << "pway_search_test: " << message << '\n';
        std::exit(1);
    }
}

void start(Vpway_search &dut, unsigned key)
{
    dut.search_key_i = key;
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
}

void runToDone(Vpway_search &dut)
{
    for (unsigned guard = 0; guard < 16 && !dut.done_o; ++guard)
        tick(dut);
    require(dut.done_o, "search did not finish");
}
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    const unsigned expectedCycles = argc > 1
        ? static_cast<unsigned>(std::strtoul(argv[1], nullptr, 10))
        : 3;
    Vpway_search dut;
    dut.rst_ni = 0;
    dut.start_i = 0;
    dut.valid_vector_i = 0x1f;
    dut.entries_flat_i =
        (static_cast<unsigned long long>(0x10) << 0) |
        (static_cast<unsigned long long>(0x20) << 8) |
        (static_cast<unsigned long long>(0x30) << 16) |
        (static_cast<unsigned long long>(0x40) << 24) |
        (static_cast<unsigned long long>(0x55) << 32);
    tick(dut);
    dut.rst_ni = 1;

    start(dut, 0x55);
    runToDone(dut);
    require(dut.match_o && dut.match_index_o == 4,
            "late entry match/index is incorrect");
    require(dut.search_cycles_o == expectedCycles,
            "search cycle count does not equal ceil(entries examined / P)");

    tick(dut);
    start(dut, 0xee);
    runToDone(dut);
    require(!dut.match_o && dut.search_cycles_o == expectedCycles,
            "worst-case miss latency/result is incorrect");
    std::cout << "pway_search_test PASS\n";
    return 0;
}
