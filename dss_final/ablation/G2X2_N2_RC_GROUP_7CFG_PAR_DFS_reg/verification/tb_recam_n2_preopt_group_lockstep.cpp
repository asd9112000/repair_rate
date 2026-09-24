#include "Vtb_recam_n2_preopt_group_lockstep.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

namespace {
using Dut = Vtb_recam_n2_preopt_group_lockstep;
void tick(Dut &d) { d.clk_i = 0; d.eval(); d.clk_i = 1; d.eval(); }
void clear(WData *v, int words) { for (int i = 0; i < words; ++i) v[i] = 0; }
void put(WData *v, int bit, std::uint32_t value) {
    const int word = bit / 32, shift = bit % 32;
    v[word] |= value << shift;
    if (shift > 19) v[word + 1] |= value >> (32 - shift);
}
bool same(const WData *a, const WData *b, int words) {
    for (int i = 0; i < words; ++i) if (a[i] != b[i]) return false;
    return true;
}
void require(bool value, const std::string &message) { if (!value) throw std::runtime_error(message); }
void setVector(Dut &d, unsigned seed) {
    std::mt19937 rng(seed);
    d.pivot_valid_i = (seed % 5 == 0) ? 0x1fU : static_cast<std::uint8_t>(rng() & 0x1fU);
    d.pivot_rows_flat_i = 0;
    clear(d.pivot_cols_flat_i, 3);
    constexpr std::array<std::uint32_t, 8> boundaries{{0,31,32,255,256,257,4095,8191}};
    for (int pivot = 0; pivot < 5; ++pivot) {
        d.pivot_rows_flat_i |= static_cast<QData>((seed + pivot * 17) & 0x1ffU) << (pivot * 9);
        put(d.pivot_cols_flat_i, pivot * 13, boundaries.at((seed + pivot) % boundaries.size()));
    }
    d.row_gt1_i = rng() & 0x1fU; d.row_gt2_i = rng() & 0x1fU; d.row_gt3_i = rng() & 0x1fU;
    d.col_gt1_i = rng() & 0x1fU; d.col_gt2_i = rng() & 0x1fU; d.col_gt3_i = rng() & 0x1fU;
    d.hybrid_valid_i = 0; d.hybrid_pointer_flat_i = 0; d.hybrid_descriptor_i = 0;
    clear(d.hybrid_differing_flat_i, 3); d.conventional_overflow_i = (seed % 17 == 0);
}
void compare(const Dut &d, unsigned index) {
    std::ostringstream why; why << "lockstep mismatch vector=" << index << " canonical(repairable="
        << unsigned(d.canonical_repairable_o) << ", config=" << d.canonical_config_o << ") ablation(repairable="
        << unsigned(d.ablation_repairable_o) << ", config=" << d.ablation_config_o << ')';
    require(d.canonical_repairable_o == d.ablation_repairable_o, why.str() + " field=repairable");
    require(d.canonical_valid_o == d.ablation_valid_o, why.str() + " field=commit-valid");
    require(d.canonical_config_o == d.ablation_config_o, why.str() + " field=config");
    require(d.canonical_pattern_o == d.ablation_pattern_o, why.str() + " field=pattern");
    require(d.canonical_action_o == d.ablation_action_o, why.str() + " field=action");
    require(d.canonical_donor_o == d.ablation_donor_o, why.str() + " field=donor");
    require(d.canonical_borrow_o == d.ablation_borrow_o, why.str() + " field=borrow");
    require(d.canonical_release_o == d.ablation_release_o, why.str() + " field=release");
    require(same(d.canonical_address_o, d.ablation_address_o, 9), why.str() + " field=address");
    require(d.canonical_is_row_o == d.ablation_is_row_o, why.str() + " field=is-row");
    require(d.canonical_line_valid_o == d.ablation_line_valid_o, why.str() + " field=line-valid");
}
void run(Dut &d, unsigned index) {
    d.rst_ni = 0; d.start_i = 0; tick(d); d.rst_ni = 1; d.start_i = 1; tick(d); d.start_i = 0;
    bool canonical_done = false, ablation_done = false;
    for (int cycle = 0; cycle < 256; ++cycle) {
        tick(d); canonical_done |= d.canonical_done_o; ablation_done |= d.ablation_done_o;
        if (canonical_done && ablation_done) { compare(d, index); return; }
    }
    throw std::runtime_error("lockstep timeout vector=" + std::to_string(index) + " canonical_done=" + std::to_string(canonical_done) + " ablation_done=" + std::to_string(ablation_done) + " ablation_busy=" + std::to_string(d.ablation_busy_o) + " commit_error=" + std::to_string(d.ablation_commit_error_o));
}
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv); Dut dut;
    try {
        for (unsigned vector = 0; vector < 1010; ++vector) { setVector(dut, vector); run(dut, vector); }
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
    std::cout << "CANONICAL_LOCKSTEP 1000/1000 PASS\n";
    return 0;
}
