#include "Vdss_v2_resource_feasibility.h"

#include <cstdlib>
#include <iostream>

namespace {
constexpr int A_ROW = 0;
constexpr int D_ROW = 1;
constexpr int B_COL = 2;
constexpr int C_COL = 3;

struct Expected {
    bool legal;
    bool feasible;
    bool borrow;
    bool donor_valid;
    int donor;
    bool release;
    int release_resource;
};

void require(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void check(Vdss_v2_resource_feasibility& dut, int sa, int rows, int cols,
           int released, int borrowed, const Expected& expected, const char* name) {
    dut.sa_id_i = sa;
    dut.sa_valid_i = 1;
    dut.config_descriptor_i = (rows << 3) | cols;
    dut.resource_released_i = released;
    dut.resource_borrowed_i = borrowed;
    dut.eval();
    require(dut.resource_state_valid_o, name);
    require(static_cast<bool>(dut.descriptor_legal_o) == expected.legal, name);
    require(static_cast<bool>(dut.physical_feasible_o) == expected.feasible, name);
    require(static_cast<bool>(dut.borrow_required_o) == expected.borrow, name);
    require(static_cast<bool>(dut.selected_donor_valid_o) == expected.donor_valid, name);
    require(!expected.donor_valid || dut.selected_donor_resource_o == expected.donor, name);
    require(static_cast<bool>(dut.release_required_o) == expected.release, name);
    require(!expected.release || dut.release_resource_o == expected.release_resource, name);
    // Feasibility is stateless: all inputs must remain unchanged after eval().
    require(dut.resource_released_i == released && dut.resource_borrowed_i == borrowed, name);
}
}  // namespace

int main() {
    Vdss_v2_resource_feasibility dut;
    constexpr int none = 0;
    constexpr int all = (1 << A_ROW) | (1 << D_ROW) | (1 << B_COL) | (1 << C_COL);

    // A: local, release, primary/secondary column donor, none, and forbidden row borrow.
    check(dut, 0, 2, 2, none, none, {true, true, false, false, B_COL, false, A_ROW}, "A cfg0");
    check(dut, 0, 1, 2, none, none, {true, true, false, false, B_COL, true, A_ROW}, "A release row");
    check(dut, 0, 2, 3, 1 << B_COL, none, {true, true, true, true, B_COL, false, A_ROW}, "A primary donor");
    check(dut, 0, 1, 3, 1 << C_COL, none, {true, true, true, true, C_COL, true, A_ROW}, "A secondary donor and transaction");
    check(dut, 0, 2, 3, none, none, {true, false, true, false, B_COL, false, A_ROW}, "A no donor");
    check(dut, 0, 3, 2, all, none, {false, false, false, false, B_COL, false, A_ROW}, "A forbidden row borrow");

    // B exercises the transposed release/borrow role and primary/secondary rows.
    check(dut, 1, 2, 1, none, none, {true, true, false, false, A_ROW, true, B_COL}, "B release column");
    check(dut, 1, 3, 2, 1 << A_ROW, none, {true, true, true, true, A_ROW, false, B_COL}, "B primary donor");
    check(dut, 1, 3, 1, 1 << D_ROW, none, {true, true, true, true, D_ROW, true, B_COL}, "B secondary donor transaction");
    check(dut, 1, 2, 3, all, none, {false, false, false, false, A_ROW, false, B_COL}, "B forbidden column borrow");

    // C/D donor direction is distinct from A/B and must retain legacy priority.
    check(dut, 2, 3, 2, 1 << D_ROW, none, {true, true, true, true, D_ROW, false, C_COL}, "C D then A priority");
    check(dut, 2, 3, 2, 1 << A_ROW, none, {true, true, true, true, A_ROW, false, C_COL}, "C fallback A");
    check(dut, 3, 2, 3, 1 << C_COL, none, {true, true, true, true, C_COL, false, D_ROW}, "D C then B priority");
    check(dut, 3, 2, 3, 1 << B_COL, none, {true, true, true, true, B_COL, false, D_ROW}, "D fallback B");

    check(dut, 0, 4, 4, all, none, {false, false, false, false, B_COL, false, A_ROW}, "unsupported descriptor");
    check(dut, 0, 2, 3, all, 1 << B_COL, {true, true, true, true, C_COL, false, A_ROW}, "borrowed primary uses secondary");
    dut.resource_released_i = none;
    dut.resource_borrowed_i = 1 << A_ROW;
    dut.sa_id_i = 0;
    dut.sa_valid_i = 1;
    dut.config_descriptor_i = (2 << 3) | 2;
    dut.eval();
    require(!dut.resource_state_valid_o && !dut.physical_feasible_o, "invalid resource state");

    dut.resource_released_i = all;
    dut.resource_borrowed_i = none;
    dut.sa_id_i = 0;
    dut.sa_valid_i = 0;
    dut.config_descriptor_i = (2 << 3) | 2;
    dut.eval();
    require(!dut.descriptor_legal_o && !dut.physical_feasible_o, "invalid SA");

    std::cout << "DSS_V2_PHASE4B_RESOURCE_FEASIBILITY PASS\n";
    return 0;
}
