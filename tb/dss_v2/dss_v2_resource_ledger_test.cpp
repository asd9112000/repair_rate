#include "Vdss_v2_resource_ledger.h"

#include <cstdlib>
#include <iostream>

namespace {
constexpr int A_ROW = 0;
constexpr int B_COL = 2;
constexpr int C_COL = 3;

void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
void tick(Vdss_v2_resource_ledger& dut) {
    dut.clk_i = 0; dut.eval();
    dut.clk_i = 1; dut.eval();
}
void set_no_actions(Vdss_v2_resource_ledger& dut) {
    dut.transaction_valid_i = 1;
    dut.release_required_i = 0; dut.release_resource_valid_i = 0; dut.release_resource_id_i = 0;
    dut.borrow_required_i = 0; dut.selected_donor_valid_i = 0; dut.selected_donor_resource_id_i = 0;
}
}  // namespace

int main() {
    Vdss_v2_resource_ledger dut;
    dut.rst_ni = 0; dut.commit_i = 0; dut.requester_sa_i = 0; set_no_actions(dut); tick(dut);
    require(dut.resource_released_o == 0 && dut.resource_borrowed_o == 0 &&
            dut.borrower_valid_o == 0 && dut.borrower_id_flat_o == 0, "canonical reset");
    dut.rst_ni = 1;

    // Commit a release; a repeated release is rejected without mutation.
    dut.commit_i = 1; dut.release_required_i = 1; dut.release_resource_valid_i = 1;
    dut.release_resource_id_i = B_COL; dut.borrow_required_i = 0; tick(dut);
    require(dut.commit_accepted_o && (dut.resource_released_o & (1 << B_COL)), "release commit");
    const unsigned after_release = dut.resource_released_o;
    tick(dut);
    require(dut.commit_error_o && dut.resource_released_o == after_release, "duplicate release rejected");

    // Borrow identity is canonical SA ID, not a resource-local magic code.
    dut.requester_sa_i = 0; dut.release_required_i = 0; dut.release_resource_valid_i = 0;
    dut.borrow_required_i = 1; dut.selected_donor_valid_i = 1; dut.selected_donor_resource_id_i = B_COL;
    tick(dut);
    require(dut.commit_accepted_o && (dut.resource_borrowed_o & (1 << B_COL)) &&
            (dut.borrower_valid_o & (1 << B_COL)) && ((dut.borrower_id_flat_o >> (2 * B_COL)) & 3) == 0,
            "borrow commit and identity");
    const unsigned released_after_borrow = dut.resource_released_o;
    const unsigned borrowed_after_borrow = dut.resource_borrowed_o;
    tick(dut);
    require(dut.commit_error_o && dut.resource_released_o == released_after_borrow &&
            dut.resource_borrowed_o == borrowed_after_borrow, "second borrower rejected");

    // A malformed simultaneous use of one resource, an infeasible transaction, and no commit are stable.
    dut.commit_i = 0; dut.borrow_required_i = 0; dut.release_required_i = 1; dut.release_resource_id_i = C_COL;
    dut.release_resource_valid_i = 1; tick(dut);
    require(dut.resource_released_o == released_after_borrow, "commit low is stable");
    dut.commit_i = 1; dut.transaction_valid_i = 0; tick(dut);
    require(dut.commit_error_o && dut.resource_released_o == released_after_borrow, "invalid transaction stable");
    dut.transaction_valid_i = 1; dut.release_required_i = 1; dut.release_resource_id_i = A_ROW;
    dut.borrow_required_i = 1; dut.selected_donor_valid_i = 1; dut.selected_donor_resource_id_i = A_ROW;
    tick(dut);
    require(dut.commit_error_o && !(dut.resource_released_o & (1 << A_ROW)), "same release donor rejected");

    dut.rst_ni = 0; dut.commit_i = 0; tick(dut);
    require(dut.resource_released_o == 0 && dut.resource_borrowed_o == 0 &&
            dut.borrower_valid_o == 0 && dut.borrower_id_flat_o == 0, "reset after commits");
    std::cout << "DSS_V2_PHASE4C_RESOURCE_LEDGER PASS\n";
    return 0;
}
