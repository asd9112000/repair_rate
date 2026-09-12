#include "Vmust_threshold_decode.h"

#include <cstdlib>
#include <iostream>

namespace { void require(bool value, const char *message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
} }
int main() {
    Vmust_threshold_decode dut;
    dut.row_count_i = 2; dut.col_count_i = 1; dut.eval();
    require((dut.row_must_by_cfg_o & (1U << 1)) != 0, "2R1C RowMust must use row_count > 1");
    require((dut.row_must_by_cfg_o & (1U << 0)) == 0, "2R2C RowMust must not assert for two faults");
    dut.row_count_i = 4; dut.col_count_i = 2; dut.eval();
    require((dut.row_must_by_cfg_o & (1U << 5)) != 0, "2R3C RowMust must use row_count > 3");
    require((dut.col_must_by_cfg_o & (1U << 4)) != 0, "1R2C ColMust must use col_count > 1");
    std::cout << "must_threshold_decode_test PASS\n";
}
