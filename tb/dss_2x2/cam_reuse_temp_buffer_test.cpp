#include "Vcam_reuse_temp_buffer.h"
#include <cstdlib>
#include <iostream>
namespace { void require(bool v,const char*m){if(!v){std::cerr<<m<<'\n';std::exit(1);}} void tick(Vcam_reuse_temp_buffer&d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();} }
int main(){ Vcam_reuse_temp_buffer d;d.rst_ni=0;d.clear_i=0;d.write_valid_i=0;tick(d);d.rst_ni=1;
 d.row_i=33;d.col_i=44;d.cfg_valid_i=0x12;d.write_valid_i=1;tick(d);d.write_valid_i=0;d.eval();
 require(d.occupancy_o==1 && (d.valid_o&1),"CAM-reuse pivot was not buffered");
 require((d.rows_flat_o&0x3ff)==33 && (d.cols_flat_o&0x3ff)==44 && (d.cfg_valid_flat_o&0x7f)==0x12,"CAM-reuse metadata changed");
 std::cout<<"cam_reuse_temp_buffer_test PASS\n"; }
