#include "Vconfig_must_view.h"
#include <cstdlib>
#include <iostream>
static void require(bool v,const char*m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
int main(){
    Vconfig_must_view d;d.pivot_valid_i=1;d.pivot_rows_flat_i=7;d.pivot_cols_flat_i=9;
    d.row_valid_i=1;d.row_addrs_flat_i[0]=7;d.row_counts_flat_i=2;
    d.col_valid_i=1;d.col_addrs_flat_i[0]=9;d.col_counts_flat_i=1;d.eval();
    require((d.row_must_by_cfg_o&(1ULL<<5))!=0,"2R1C did not assert RowMust at row_count=2");
    require((d.row_must_by_cfg_o&(1U<<0))==0,"2R2C incorrectly asserted RowMust at row_count=2");
    require(d.col_must_by_cfg_o==0,"single column fault incorrectly generated ColumnMust");
    std::cout<<"config_must_view_test PASS\n";
}
