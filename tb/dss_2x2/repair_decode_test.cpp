#include "Vrepair_decode.h"
#include <cstdlib>
#include <iostream>
static void require(bool v,const char*m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
int main(){
    Vrepair_decode d;d.config_id_i=1;d.pivot_valid_i=1;d.pivot_rows_flat_i=17;d.pivot_cols_flat_i=29;
    d.pattern_id_i=1;d.eval();
    require(d.decode_valid_o&&(d.repair_row_valid_o&1)&&!(d.repair_col_valid_o&1),"PatternID 1 did not reconstruct pivot0 as a row");
    require((d.repair_rows_flat_o&0x3ffU)==17,"row repair address changed during reconstruction");
    d.pattern_id_i=3;d.eval();
    require(d.decode_valid_o&&(d.repair_col_valid_o&1)&&!(d.repair_row_valid_o&1),"PatternID 3 did not reconstruct pivot0 as a column");
    require((d.repair_cols_flat_o&0x3ffU)==29,"column repair address changed during reconstruction");
    d.pattern_id_i=0;d.eval();require(!d.decode_valid_o,"PatternID zero decoded as valid");
    std::cout<<"repair_decode_test PASS\n";
}
