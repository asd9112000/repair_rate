#include "Vrecam_dss_v2_early_latency_test_top.h"
#include <cstdlib>
#include <iostream>

static void tick(Vrecam_dss_v2_early_latency_test_top& d) { d.clk_i=0; d.eval(); d.clk_i=1; d.eval(); }
static void require(bool v, const char* m) { if (!v) { std::cerr << "FAIL: " << m << '\n'; std::exit(1); } }
static void clear(Vrecam_dss_v2_early_latency_test_top& d) {
 d.pivot_valid_i=0; d.pivot_rows_flat_i=0; d.pivot_cols_flat_i=0;
 d.row_gt1_i=d.row_gt2_i=d.row_gt3_i=d.col_gt1_i=d.col_gt2_i=d.col_gt3_i=0;
 d.hybrid_valid_i=0; d.hybrid_pointer_flat_i=0; d.hybrid_descriptor_i=0; d.hybrid_differing_flat_i=0;
}
static void run_case(Vrecam_dss_v2_early_latency_test_top& d, bool overflow, const char* name) {
 d.rst_ni=0; d.start_i=0; d.conventional_overflow_i=overflow; tick(d); d.rst_ni=1;
 d.start_i=1; tick(d); d.start_i=0;
 unsigned legacy=0,v2=0; bool legacy_seen=false,v2_seen=false;
 for(unsigned edge=1;edge<=64 && (!legacy_seen||!v2_seen);++edge) { tick(d); if(d.legacy_done_o&&!legacy_seen){legacy=edge;legacy_seen=true;} if(d.v2_done_o&&!v2_seen){v2=edge;v2_seen=true;} }
 require(legacy_seen&&v2_seen,"both production tops complete");
 if(!overflow) require(d.legacy_group_repairable_o&&d.v2_group_repairable_o,"all-local group succeeds");
 else require(!d.legacy_group_repairable_o&&!d.v2_group_repairable_o,"A failure is reported");
 std::cout<<"LATENCY "<<name<<" legacy_post_start_cycles="<<legacy<<" v2_post_start_cycles="<<v2<<"\n";
}
int main(){ Vrecam_dss_v2_early_latency_test_top d; clear(d); run_case(d,false,"ALL_LOCAL"); run_case(d,true,"FAILURE_A"); }
