#include "Vrecam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_top.h"
#include "verilated.h"

#include <iostream>
#include <stdexcept>

namespace {
using Dut = Vrecam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_top;
void tick(Dut &dut) { dut.clk_i = 0; dut.eval(); dut.clk_i = 1; dut.eval(); dut.clk_i = 0; dut.eval(); }
void clear(Dut &dut) {
    dut.pivot_valid_flat_i = 0; for (int i=0;i<7;++i) dut.pivot_rows_flat_i[i]=0;
    for (int i=0;i<4;++i) dut.pivot_cols_flat_i[i]=0;
    dut.row_gt1_flat_i=0; dut.row_gt2_flat_i=0; dut.row_gt3_flat_i=0; dut.row_gt4_flat_i=0;
    dut.col_gt1_flat_i=0; dut.col_gt2_flat_i=0; dut.col_gt3_flat_i=0; dut.col_gt4_flat_i=0;
    dut.hybrid_valid_flat_i=0; dut.hybrid_descriptor_flat_i=0;
    for (int i=0;i<4;++i) dut.hybrid_pointer_flat_i[i]=0;
    for (int i=0;i<12;++i) dut.hybrid_differing_flat_i[i]=0;
    dut.conventional_overflow_i=0;
}
void reset(Dut &dut) { clear(dut); dut.rst_ni=0; dut.start_i=0; tick(dut); dut.rst_ni=1; }
int run(Dut &dut) { dut.start_i=1; tick(dut); dut.start_i=0; for(int n=0;n<512;++n){tick(dut);if(dut.done_o)return n+1;} throw std::runtime_error("integrated top timed out"); }
bool bit(const WData *data,int index){return (data[index/32]>>(index%32))&1U;}
int count(const WData *data){int total=0;for(int i=0;i<180;++i) total+=bit(data,i);return total;}
}
int main(int argc,char **argv) {
    Verilated::commandArgs(argc,argv); Dut dut; std::size_t candidate_mismatches=0, local_mismatches=0, end_to_end=0;
    int success_cycles=0, failure_cycles=0; int failure_by_sa[4]={0,0,0,0};
    try {
        reset(dut); success_cycles=run(dut);
        if (!dut.group_repairable_o || dut.selected_valid_o!=15 || dut.selected_attempt_flat_o!=0 ||
            dut.selected_pattern_flat_o!=0x1111 || dut.selected_used_rows_flat_o!=0 || dut.selected_used_cols_flat_o!=0 ||
            count(dut.candidate_valid_debug_o)!=94) ++end_to_end;
        for(int sa=0;sa<4;++sa) for(int attempt=0;attempt<3;++attempt) for(int pattern=0;pattern<15;++pattern) {
            const bool expected = pattern < (attempt==0?6:attempt==1?10:15) && !(sa==0||sa==3 ? attempt==2 : false);
            if (bit(dut.candidate_valid_debug_o,sa*45+attempt*15+pattern)!=expected) ++candidate_mismatches;
        }
        for(int sa=0;sa<4;++sa) { reset(dut); dut.conventional_overflow_i=1U<<sa; failure_by_sa[sa]=run(dut); }
        reset(dut); dut.conventional_overflow_i=4; failure_cycles=run(dut);
        if (dut.group_repairable_o || dut.failure_position_o!=2 || dut.selected_valid_o!=3 || count(dut.candidate_valid_debug_o)!=63)
            ++end_to_end;
        for(int attempt=0;attempt<3;++attempt) for(int pattern=0;pattern<15;++pattern)
            if(bit(dut.candidate_valid_debug_o,2*45+attempt*15+pattern)) ++local_mismatches;
    } catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
    std::cout<<"CANDIDATE_TABLE_MISMATCHES="<<candidate_mismatches<<'\n'
             <<"LOCAL_RECAM_CANDIDATE_MISMATCHES="<<local_mismatches<<'\n'
             <<"END_TO_END_MISMATCHES="<<end_to_end<<'\n'
             <<"INTEGRATED_DIRECTED_SUCCESS_CYCLES="<<success_cycles<<'\n'
             <<"INTEGRATED_DIRECTED_FAILURE_CYCLES="<<failure_cycles<<'\n'
             <<"INTEGRATED_FAILURE_A_CYCLES="<<failure_by_sa[0]<<'\n'
             <<"INTEGRATED_FAILURE_B_CYCLES="<<failure_by_sa[1]<<'\n'
             <<"INTEGRATED_FAILURE_C_CYCLES="<<failure_by_sa[2]<<'\n'
             <<"INTEGRATED_FAILURE_D_CYCLES="<<failure_by_sa[3]<<'\n';
    return candidate_mismatches||local_mismatches||end_to_end;
}
