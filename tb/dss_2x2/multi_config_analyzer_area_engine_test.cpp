#include "Vmulti_config_analyzer_area_engine.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

static void require(bool value, const char *message) {
    if (!value) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

static void tick(Vmulti_config_analyzer_area_engine &dut) {
    dut.clk_i=0;
    dut.eval();
    dut.clk_i=1;
    dut.eval();
    dut.clk_i=0;
    dut.eval();
}

static unsigned pattern(const Vmulti_config_analyzer_area_engine &dut,
                        unsigned cfg) {
    return (dut.config_patterns_o >> (cfg*4)) & 0xfU;
}

int main() {
    Vmulti_config_analyzer_area_engine dut;
    dut.rst_ni=0;
    dut.start_i=0;
    dut.cfg_pivot_valid_i=0;
    dut.pivot_rows_flat_i=0;
    dut.pivot_cols_flat_i=0;
    dut.row_must_by_cfg_i=0;
    dut.col_must_by_cfg_i=0;
    dut.hybrid_valid_i=0;
    dut.hybrid_descriptors_i=0;
    dut.hybrid_ptrs_flat_i=0;
    dut.shared_storage_overflow_i=0;
    for(auto &word:dut.hybrid_rows_flat_i) word=0;
    for(auto &word:dut.hybrid_cols_flat_i) word=0;
    for(auto &word:dut.hybrid_cfg_valid_flat_i) word=0;
    tick(dut);
    dut.rst_ni=1;

    const unsigned capacities[7]={4,3,5,4,3,5,4};
    for(unsigned cfg=0;cfg<7;++cfg)
        for(unsigned entry=0;entry<capacities[cfg];++entry)
            dut.cfg_pivot_valid_i |= uint64_t{1} << (entry*7+cfg);
    for(unsigned entry=0;entry<5;++entry) {
        dut.pivot_rows_flat_i |= uint64_t{entry+1} << (entry*10);
        dut.pivot_cols_flat_i |= uint64_t{entry+11} << (entry*10);
    }

    const uint64_t row_must[7]={0x0,0x3,0x7,0x7,0x4,0x18,0x8};
    const uint64_t col_must[7]={0x0,0x4,0x18,0x8,0x3,0x7,0x7};
    for(unsigned cfg=0;cfg<7;++cfg) {
        dut.row_must_by_cfg_i |= row_must[cfg] << (cfg*5);
        dut.col_must_by_cfg_i |= col_must[cfg] << (cfg*5);
    }

    dut.start_i=1;
    tick(dut);
    dut.start_i=0;
    require(dut.busy_o && !dut.done_o,"AREA engine did not start");
    unsigned cycles=0;
    while(dut.busy_o && cycles<45) {
        tick(dut);
        ++cycles;
    }
    require(!dut.busy_o && dut.done_o,
            "AREA engine did not complete after all ConfigID/Pattern views");
    require(cycles==40,"AREA engine candidate schedule is not 40 cycles");
    require(dut.config_valid_o==0x7f,
            "AREA engine did not expose all seven physical ConfigIDs");
    const unsigned expected[7]={1,1,1,1,3,10,4};
    for(unsigned cfg=0;cfg<7;++cfg)
        require(pattern(dut,cfg)==expected[cfg],
                "AREA engine PatternID differs from physical candidate ordering");

    tick(dut);
    require(!dut.done_o,"AREA engine done was not a one-cycle pulse");

    dut.shared_storage_overflow_i=1;
    dut.start_i=1;
    tick(dut);
    dut.start_i=0;
    for(unsigned cycle=0;cycle<40;++cycle) tick(dut);
    require(dut.done_o && dut.config_valid_o==0 && dut.config_patterns_o==0,
            "shared overflow did not invalidate all AREA-mode views");

    std::cout << "multi_config_analyzer_area_engine_test PASS\n";
}
