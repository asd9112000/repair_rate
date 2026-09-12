#include "Vrc_transpose_wrapper.h"
#include <cstdlib>
#include <iostream>
static void require(bool v, const char *m) { if (!v) { std::cerr << m << '\n'; std::exit(1); } }
int main() {
    Vrc_transpose_wrapper d;
    d.physical_rows_i=1; d.physical_cols_i=3;
    d.physical_pivot_rows_i=0x12; d.physical_pivot_cols_i=0x34;
    d.physical_row_counts_i=0x2; d.physical_col_counts_i=0x5;
    d.physical_row_must_i=0x1; d.physical_col_must_i=0x4;
    d.transpose_i=1; d.eval();
    require(d.canonical_rows_o==3 && d.canonical_cols_o==1, "R/C envelope was not transposed");
    require((d.canonical_pivot_rows_o & 0x3ffU)==0x34 && (d.canonical_pivot_cols_o & 0x3ffU)==0x12,
            "pivot addresses were not transposed");
    require((d.canonical_row_counts_o & 0xfU)==5 && (d.canonical_col_counts_o & 0xfU)==2,
            "fault counts were not transposed");
    require(d.canonical_row_must_o==4 && d.canonical_col_must_o==1, "Must masks were not transposed");
    std::cout << "rc_transpose_wrapper_test PASS\n";
}
