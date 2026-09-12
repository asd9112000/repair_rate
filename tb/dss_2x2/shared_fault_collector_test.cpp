#include "Vshared_fault_collector.h"
#include <cstdlib>
#include <iostream>
static void require(bool v,const char*m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
static void tick(Vshared_fault_collector&d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();}
static void push(Vshared_fault_collector&d,unsigned r,unsigned c){require(d.fault_ready_o,"collector backpressured legal fault");d.fault_row_i=r;d.fault_col_i=c;d.fault_valid_i=1;tick(d);d.fault_valid_i=0;d.eval();}
int main(){
    Vshared_fault_collector d;d.rst_ni=0;d.clear_i=0;d.fault_valid_i=0;tick(d);d.rst_ni=1;
    for(unsigned i=0;i<5;++i)push(d,10+i,20+i);
    require(d.pivot_occupancy_o==5,"five independent faults did not form the shared PivotPayload");
    for(unsigned entry=0;entry<3;++entry)require(((d.cfg_pivot_valid_o>>(entry*7+1))&1U)!=0,"2R1C lost a prefix pivot");
    require(((d.cfg_pivot_valid_o>>(3*7+1))&1U)==0,"2R1C incorrectly activated the fourth pivot");
    require(d.cam_reuse_occupancy_o==2,"config-dependent additional pivots were not buffered");
    require((d.cam_reuse_cfg_valid_flat_o[0]&0x7fU)==0x12U,"fourth pivot has wrong CAM-reuse ConfigID mask");
    d.clear_i=1;tick(d);d.clear_i=0;push(d,7,1);push(d,7,2);
    require(d.pivot_occupancy_o==1&&d.hybrid_occupancy_o==1,"related fault was not tagged as one shared Hybrid record");
    require((d.hybrid_cfg_valid_flat_o[0]&0x7fU)==0x7fU,"shared Hybrid record lost ConfigID membership");
    require((d.hybrid_rows_flat_o[0]&0x3ffU)==7&&(d.hybrid_cols_flat_o[0]&0x3ffU)==2,"Hybrid payload address differs from fault stream");
    std::cout<<"shared_fault_collector_test PASS\n";
}
