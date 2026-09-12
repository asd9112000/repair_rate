#include "Vrecam_2r2c_analyzer.h"

#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}
}

int main() {
    Vrecam_2r2c_analyzer dut;
    dut.pivot_valid_i = 0;
    dut.pivot_rows_flat_i = 0;
    dut.pivot_cols_flat_i = 0;
    dut.row_must_i = 0;
    dut.col_must_i = 0;
    dut.hybrid_valid_i = 0;
    dut.hybrid_rows_flat_i = 0;
    dut.hybrid_cols_flat_i = 0;
    dut.cam_overflow_i = 0;
    dut.eval();
    require(dut.matrix_flat_o == 0, "empty CAM state must produce an empty matrix");
    require(dut.candidate_valid_o == 0x3f, "all six 2R2C candidates must evaluate in parallel");
    require(dut.repairable_o && dut.pattern_id_o == 1, "encoder must select lowest valid PatternID");

    dut.pivot_valid_i = 1;
    dut.pivot_rows_flat_i = 0x012;
    dut.pivot_cols_flat_i = 0x155;
    dut.eval();
    require((dut.matrix_flat_o & 1) != 0, "pivot must set its diagonal matrix cell");
    require(dut.candidate_valid_o == 0x3f, "diagonal pivot must leave all six candidates valid");
    require(dut.pattern_id_o == 1, "encoder must remain deterministic");

    dut.row_must_i = 1;
    dut.eval();
    require((dut.matrix_flat_o & 0x000f) == 0x000f, "row-must must mark the full matrix row");

    dut.cam_overflow_i = 1;
    dut.eval();
    require(dut.candidate_valid_o == 0 && !dut.repairable_o && dut.pattern_id_o == 0,
            "CAM overflow must invalidate conventional candidates");

    std::cout << "recam_2r2c_analyzer_test PASS\n";
}
