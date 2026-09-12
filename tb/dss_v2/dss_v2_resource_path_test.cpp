#include "Vdss_v2_resource_path_test_top.h"

#include <cstdlib>
#include <iostream>

namespace {
constexpr int A_ROW = 0;
constexpr int B_COL = 2;
constexpr int C_COL = 3;
struct LegacyDecoded {
    unsigned released = 0;
    unsigned borrowed = 0;
    unsigned borrower_id_flat = 0;
};
void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
void tick(Vdss_v2_resource_path_test_top& dut) {
    dut.clk_i = 0; dut.eval(); dut.clk_i = 1; dut.eval();
}
void request(Vdss_v2_resource_path_test_top& dut, int sa, int rows, int cols, bool commit) {
    dut.sa_id_i = sa; dut.sa_valid_i = 1; dut.config_descriptor_i = (rows << 3) | cols;
    dut.commit_i = commit; dut.eval();
}
void reset(Vdss_v2_resource_path_test_top& dut) {
    dut.rst_ni = 0; dut.commit_i = 0; dut.sa_id_i = 0; dut.sa_valid_i = 1;
    dut.config_descriptor_i = (2 << 3) | 2; tick(dut); dut.rst_ni = 1;
}
LegacyDecoded decode_legacy_12bit(unsigned legacy) {
    LegacyDecoded decoded;
    decoded.released = legacy & 0xf;
    for (int resource = 0; resource != 4; ++resource) {
        const unsigned code = (legacy >> (4 + 2 * resource)) & 3;
        if (code == 0) continue;
        require(code != 3, "legacy reserved borrower code");
        decoded.borrowed |= 1u << resource;
        // Legacy resource-local code: row resources 1=B/2=C; column
        // resources 1=A/2=D.  V2 expands it to canonical SA IDs.
        const unsigned sa = resource < B_COL ? (code == 1 ? 1 : 2) : (code == 1 ? 0 : 3);
        decoded.borrower_id_flat |= sa << (2 * resource);
    }
    return decoded;
}
void require_legacy_equivalent(const Vdss_v2_resource_path_test_top& dut, unsigned legacy, const char* name) {
    const LegacyDecoded decoded = decode_legacy_12bit(legacy);
    require(dut.resource_released_o == decoded.released && dut.resource_borrowed_o == decoded.borrowed &&
            dut.borrower_valid_o == decoded.borrowed && dut.borrower_id_flat_o == decoded.borrower_id_flat, name);
}
}  // namespace

int main() {
    Vdss_v2_resource_path_test_top dut;
    reset(dut);

    // B release B_COL, then A borrows it: snapshot -> feasibility -> commit.
    request(dut, 1, 2, 1, true); require(dut.physical_feasible_o && dut.release_required_o, "B release feasible");
    tick(dut); require(dut.resource_released_o == (1 << B_COL) && !dut.resource_borrowed_o, "B_COL released");
    require_legacy_equivalent(dut, 0x004, "legacy B release equivalence");
    request(dut, 0, 2, 3, true); require(dut.physical_feasible_o && dut.selected_donor_resource_o == B_COL, "A primary B_COL");
    tick(dut); require((dut.resource_borrowed_o & (1 << B_COL)) && ((dut.borrower_id_flat_o >> (2 * B_COL)) & 3) == 0,
                       "A owns B_COL borrow");
    require_legacy_equivalent(dut, 0x104, "legacy A borrows B_COL equivalence");

    // Already-borrowed donor cannot be reused; an attempted commit is rejected and stable.
    const unsigned ledger_before_reuse = dut.resource_borrowed_o;
    request(dut, 3, 2, 3, true); require(!dut.physical_feasible_o, "D has no free donor");
    tick(dut); require(dut.commit_error_o && dut.resource_borrowed_o == ledger_before_reuse, "borrow conflict stable");

    // A cannot borrow B_COL before its owner releases it.
    reset(dut); request(dut, 0, 2, 3, true); require(!dut.physical_feasible_o, "borrow before release infeasible");
    tick(dut); require(dut.commit_error_o && dut.resource_released_o == 0, "early borrow no mutation");

    // A falls back from unavailable B_COL to released C_COL.
    reset(dut); request(dut, 2, 2, 1, true); tick(dut);
    request(dut, 0, 2, 3, true); require(dut.physical_feasible_o && dut.selected_donor_resource_o == C_COL, "A fallback C_COL");
    tick(dut); require(dut.resource_borrowed_o == (1 << C_COL), "fallback committed");
    require_legacy_equivalent(dut, 0x408, "legacy A borrows C_COL equivalence");

    // Release+borrow stays invisible before its edge, then both appear after it.
    reset(dut); request(dut, 1, 2, 1, true); tick(dut);
    request(dut, 0, 1, 3, false); require(dut.physical_feasible_o && dut.release_required_o && dut.borrow_required_o,
                                           "A atomic transaction feasible");
    require(dut.resource_released_o == (1 << B_COL) && dut.resource_borrowed_o == 0, "no commit remains invisible");
    dut.commit_i = 1; tick(dut);
    require(dut.resource_released_o == ((1 << A_ROW) | (1 << B_COL)) && dut.resource_borrowed_o == (1 << B_COL),
            "atomic release plus borrow");

    dut.rst_ni = 0; dut.commit_i = 0; tick(dut);
    require(dut.resource_released_o == 0 && dut.resource_borrowed_o == 0 && dut.borrower_valid_o == 0,
            "path reset");
    std::cout << "DSS_V2_PHASE4C_RESOURCE_PATH PASS\n";
    return 0;
}
