#include "Vshared_view_candidate_analyzer.h"
#include <cstdlib>
#include <iostream>
namespace { void require(bool v,const char*m){if(!v){std::cerr<<m<<'\n';std::exit(1);}} }
int main() {
    Vshared_view_candidate_analyzer d;
    d.active_rows_i=2; d.active_cols_i=2; d.pivot_valid_i=1;
    d.pivot_rows_flat_i=17; d.pivot_cols_flat_i=29;
    d.row_must_i=0; d.col_must_i=0; d.hybrid_valid_i=0;
    d.hybrid_rows_flat_i[0]=0; d.hybrid_cols_flat_i[0]=0;
    d.hybrid_ptrs_flat_i=0; d.hybrid_descriptor_row_diff_i=0;
    d.shared_storage_overflow_i=0; d.eval();
    require(d.repairable_o && d.lowest_pattern_id_o != 0 && d.candidate_valid_o != 0,
            "shared-view analyzer did not create a PatternID from PivotPayload");
    d.shared_storage_overflow_i=1; d.eval();
    require(!d.repairable_o && d.lowest_pattern_id_o==0,
            "CAM-reuse/collection overflow must invalidate conventional candidates");
    std::cout<<"shared_view_candidate_analyzer_test PASS\n";
}
