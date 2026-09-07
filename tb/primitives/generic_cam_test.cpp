#include "Vgeneric_cam.h"
#include "verilated.h"

#include <cstdlib>
#include <iostream>

namespace
{
void tick(Vgeneric_cam &dut)
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
        std::cerr << "generic_cam_test: " << message << '\n';
        std::exit(1);
    }
}

void writeEntry(Vgeneric_cam &dut, unsigned index, unsigned key, unsigned data)
{
    dut.write_valid_i = 1;
    dut.write_index_i = index;
    dut.write_key_i = key;
    dut.write_data_i = data;
    tick(dut);
    dut.write_valid_i = 0;
}
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Vgeneric_cam dut;
    dut.rst_ni = 0;
    dut.clear_i = 0;
    dut.write_valid_i = 0;
    dut.invalidate_valid_i = 0;
    dut.read_valid_i = 0;
    dut.search_valid_i = 0;
    tick(dut);
    dut.rst_ni = 1;

    require(dut.valid_vector_o == 0, "reset did not invalidate all entries");
    writeEntry(dut, 1, 0x2a, 0x91);
    writeEntry(dut, 3, 0x2a, 0x73);
    writeEntry(dut, 0, 0x10, 0x55);

    dut.read_valid_i = 1;
    dut.read_index_i = 3;
    dut.eval();
    require(dut.read_entry_valid_o, "written entry is not readable");
    require(dut.read_key_o == 0x2a && dut.read_data_o == 0x73,
            "readback data differs from the write");
    dut.read_valid_i = 0;

    dut.search_valid_i = 1;
    dut.search_key_i = 0x2a;
    dut.eval();
    require(dut.search_match_o, "duplicate key did not match");
    require(dut.search_index_o == 1 && dut.search_data_o == 0x91,
            "priority select did not choose the lowest matching index");
    require((dut.match_vector_o & 0x0aU) == 0x0aU,
            "match vector did not expose both matching entries");

    dut.invalidate_valid_i = 1;
    dut.invalidate_index_i = 1;
    tick(dut);
    dut.invalidate_valid_i = 0;
    dut.eval();
    require(dut.search_match_o && dut.search_index_o == 3,
            "invalidate did not expose the remaining duplicate");

    dut.search_key_i = 0xee;
    dut.eval();
    require(!dut.search_match_o && dut.match_vector_o == 0,
            "missing key produced a false match");

    dut.clear_i = 1;
    tick(dut);
    dut.clear_i = 0;
    require(dut.valid_vector_o == 0, "clear did not invalidate all entries");
    std::cout << "generic_cam_test PASS\n";
    return 0;
}
