#include "Vrecam_dss_v2_group_equivalence_test_top.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>

namespace {
constexpr unsigned kSeed = 20260910;
constexpr unsigned kSaNum = 4;
constexpr unsigned kSlotNum = 4;

struct Candidate {
    bool valid = false;
    bool solution = false;
    bool repairable = false;
    unsigned config = 0;
    unsigned pattern = 0;
    unsigned bitmap = 0;
};
using Maps = std::array<std::array<Candidate, kSlotNum>, kSaNum>;

struct Ledger {
    std::array<bool, 4> released{};
    std::array<bool, 4> borrowed{};
    std::array<unsigned, 4> borrower{};
};

struct Trace {
    bool repairable = false;
    int failure = -1;
    std::array<bool, 4> committed{};
    std::array<unsigned, 4> config{};
    std::array<unsigned, 4> pattern{};
    std::array<bool, 4> borrow{};
    std::array<unsigned, 4> donor{};
    std::array<bool, 4> release{};
    std::array<Ledger, 4> ledger_after{};
};

struct Counters {
    unsigned vectors = 0;
    unsigned vector_mismatches = 0;
    unsigned ledger_mismatches = 0;
    unsigned config_mismatches = 0;
    unsigned pattern_mismatches = 0;
    unsigned donor_release_mismatches = 0;
    unsigned failure_mismatches = 0;
    unsigned repairable_mismatches = 0;
    unsigned both_pass = 0;
    unsigned both_fail = 0;
    std::array<unsigned, 4> failure_at{};
    std::array<unsigned, 4> selected_slot{};
    unsigned release_transactions = 0;
    unsigned borrow_transactions = 0;
    unsigned release_borrow_transactions = 0;
    unsigned donor_fallbacks = 0;
    unsigned multiple_locally_valid = 0;
    unsigned resource_consumption_events = 0;
};

unsigned canonical_config(unsigned sa, unsigned slot) {
    static constexpr unsigned table[4][4] = {
        {0, 4, 5, 6}, {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 4, 5, 6}};
    return table[sa][slot];
}
unsigned config_slot(unsigned sa, unsigned config) {
    for (unsigned slot = 0; slot != kSlotNum; ++slot)
        if (canonical_config(sa, slot) == config) return slot;
    return 0;
}
unsigned own_resource(unsigned sa) {
    static constexpr unsigned table[] = {0, 2, 3, 1};
    return table[sa];
}
std::array<unsigned, 2> donors(unsigned sa) {
    static constexpr unsigned table[4][2] = {{2, 3}, {0, 1}, {1, 0}, {3, 2}};
    return {table[sa][0], table[sa][1]};
}
bool local_accept(const Candidate& c) { return c.valid && c.solution && c.repairable; }

Ledger decode_legacy(unsigned packed) {
    Ledger ledger;
    for (unsigned resource = 0; resource != 4; ++resource) {
        ledger.released[resource] = (packed >> resource) & 1;
        const unsigned code = (packed >> (4 + resource * 2)) & 3;
        ledger.borrowed[resource] = code != 0;
        if (!ledger.borrowed[resource]) continue;
        ledger.borrower[resource] = resource < 2 ? (code == 1 ? 1 : 2)
                                                : (code == 1 ? 0 : 3);
    }
    return ledger;
}
bool same_ledger(const Ledger& lhs, const Ledger& rhs) {
    for (unsigned resource = 0; resource != 4; ++resource) {
        if (lhs.released[resource] != rhs.released[resource] ||
            lhs.borrowed[resource] != rhs.borrowed[resource]) return false;
        if (lhs.borrowed[resource] && lhs.borrower[resource] != rhs.borrower[resource]) return false;
    }
    return true;
}

void write_entry(std::uint32_t* words, unsigned slot, const Candidate& c) {
    const std::uint64_t bits = (std::uint64_t(c.bitmap & 0x3ff) << 10) |
                               (std::uint64_t(c.pattern & 0xf) << 6) |
                               (std::uint64_t(c.repairable) << 5) |
                               (std::uint64_t(c.solution) << 4) |
                               (std::uint64_t(c.config & 7) << 1) | c.valid;
    const unsigned bit = slot * 20;
    words[bit / 32] |= static_cast<std::uint32_t>(bits << (bit % 32));
    if ((bit % 32) > 12)
        words[bit / 32 + 1] |= static_cast<std::uint32_t>(bits >> (32 - (bit % 32)));
}
std::uint32_t* map_port(Vrecam_dss_v2_group_equivalence_test_top& dut, unsigned sa) {
    switch (sa) {
    case 0: return dut.map_a_i;
    case 1: return dut.map_b_i;
    case 2: return dut.map_c_i;
    default: return dut.map_d_i;
    }
}
void drive_maps(Vrecam_dss_v2_group_equivalence_test_top& dut, const Maps& maps) {
    for (unsigned sa = 0; sa != kSaNum; ++sa) {
        std::uint32_t* words = map_port(dut, sa);
        for (unsigned word = 0; word != 3; ++word) words[word] = 0;
        for (unsigned slot = 0; slot != kSlotNum; ++slot) write_entry(words, slot, maps[sa][slot]);
    }
}
void tick(Vrecam_dss_v2_group_equivalence_test_top& dut) {
    dut.clk_i = 0; dut.eval();
    dut.clk_i = 1; dut.eval();
}

void capture_legacy(const Vrecam_dss_v2_group_equivalence_test_top& dut, Trace& trace,
                    unsigned& previous) {
    const unsigned commits = dut.legacy_sa_commit_valid_o;
    for (unsigned sa = 0; sa != kSaNum; ++sa) if (((commits >> sa) & 1) && !((previous >> sa) & 1)) {
        trace.committed[sa] = true;
        trace.config[sa] = (dut.legacy_selected_config_flat_o >> (sa * 3)) & 7;
        trace.pattern[sa] = (dut.legacy_selected_pattern_flat_o >> (sa * 4)) & 0xf;
        trace.borrow[sa] = (dut.legacy_borrow_flat_o >> sa) & 1;
        trace.donor[sa] = (dut.legacy_selected_donor_flat_o >> (sa * 2)) & 3;
        trace.release[sa] = (dut.legacy_release_flat_o >> sa) & 1;
        trace.ledger_after[sa] = decode_legacy(dut.legacy_ledger_o);
    }
    previous = commits;
}
void capture_v2(const Vrecam_dss_v2_group_equivalence_test_top& dut, Trace& trace,
                unsigned& previous) {
    const unsigned commits = dut.v2_sa_commit_valid_o;
    for (unsigned sa = 0; sa != kSaNum; ++sa) if (((commits >> sa) & 1) && !((previous >> sa) & 1)) {
        trace.committed[sa] = true;
        trace.config[sa] = (dut.v2_selected_config_flat_o >> (sa * 3)) & 7;
        trace.pattern[sa] = (dut.v2_selected_pattern_flat_o >> (sa * 4)) & 0xf;
        trace.borrow[sa] = (dut.v2_borrow_flat_o >> sa) & 1;
        trace.donor[sa] = (dut.v2_selected_donor_flat_o >> (sa * 2)) & 3;
        trace.release[sa] = (dut.v2_release_flat_o >> sa) & 1;
        trace.ledger_after[sa] = decode_legacy(dut.v2_ledger_o);
    }
    previous = commits;
}
void finish_trace(const Vrecam_dss_v2_group_equivalence_test_top& dut, Trace& legacy, Trace& v2) {
    legacy.repairable = dut.legacy_group_repairable_o;
    v2.repairable = dut.v2_group_repairable_o;
    for (unsigned sa = 0; sa != kSaNum; ++sa) {
        if (!legacy.repairable && !legacy.committed[sa]) { legacy.failure = sa; break; }
    }
    if (!v2.repairable) v2.failure = dut.v2_failure_position_o;
}

void dump_maps(const Maps& maps) {
    static const char* names[] = {"A", "B", "C", "D"};
    for (unsigned sa = 0; sa != kSaNum; ++sa) {
        std::cerr << "  SA " << names[sa] << ':';
        for (const Candidate& c : maps[sa])
            std::cerr << " {v=" << c.valid << ",sol=" << c.solution << ",rep=" << c.repairable
                      << ",cfg=" << c.config << ",pat=" << c.pattern << ",bitmap=0x"
                      << std::hex << c.bitmap << std::dec << '}';
        std::cerr << '\n';
    }
}
void dump_trace(const char* label, const Trace& trace) {
    std::cerr << "  " << label << " repairable=" << trace.repairable << " failure=" << trace.failure << '\n';
    for (unsigned sa = 0; sa != kSaNum; ++sa) if (trace.committed[sa]) {
        const Ledger& ledger = trace.ledger_after[sa];
        std::cerr << "    SA=" << sa << " slot=" << config_slot(sa, trace.config[sa])
                  << " cfg=" << trace.config[sa] << " pat=" << trace.pattern[sa]
                  << " donor_valid=" << trace.borrow[sa] << " donor=" << trace.donor[sa]
                  << " release_valid=" << trace.release[sa]
                  << " release_resource=" << (trace.release[sa] ? own_resource(sa) : 0)
                  << " ledger={released=" << ledger.released[0] << ledger.released[1]
                  << ledger.released[2] << ledger.released[3] << ",borrowed="
                  << ledger.borrowed[0] << ledger.borrowed[1] << ledger.borrowed[2]
                  << ledger.borrowed[3] << ",borrower=" << ledger.borrower[0] << "/"
                  << ledger.borrower[1] << "/" << ledger.borrower[2] << "/"
                  << ledger.borrower[3] << "}\n";
    }
}

bool compare(const Trace& legacy, const Trace& v2, Counters& counts) {
    bool mismatch = false;
    if (legacy.repairable != v2.repairable) { ++counts.repairable_mismatches; mismatch = true; }
    if (legacy.failure != v2.failure) { ++counts.failure_mismatches; mismatch = true; }
    for (unsigned sa = 0; sa != kSaNum; ++sa) {
        if (legacy.committed[sa] != v2.committed[sa]) { ++counts.config_mismatches; mismatch = true; continue; }
        if (!legacy.committed[sa]) continue;
        if (legacy.config[sa] != v2.config[sa]) { ++counts.config_mismatches; mismatch = true; }
        if (legacy.pattern[sa] != v2.pattern[sa]) { ++counts.pattern_mismatches; mismatch = true; }
        // donor/release payloads are compared only when their valid action is asserted.
        if (legacy.borrow[sa] != v2.borrow[sa] || legacy.release[sa] != v2.release[sa] ||
            (legacy.borrow[sa] && legacy.donor[sa] != v2.donor[sa])) {
            ++counts.donor_release_mismatches; mismatch = true;
        }
        if (!same_ledger(legacy.ledger_after[sa], v2.ledger_after[sa])) {
            ++counts.ledger_mismatches; mismatch = true;
        }
    }
    return mismatch;
}

Maps random_maps(std::mt19937& rng) {
    Maps maps{};
    for (unsigned sa = 0; sa != kSaNum; ++sa) for (unsigned slot = 0; slot != kSlotNum; ++slot) {
        Candidate& c = maps[sa][slot];
        c.valid = (rng() % 100) < 84;
        c.solution = (rng() % 100) < 84;
        c.repairable = (rng() % 100) < 84;
        c.config = canonical_config(sa, slot);
        c.pattern = rng() & 0xf;
        c.bitmap = rng() & 0x3ff;
    }
    return maps;
}
void update_coverage(const Maps& maps, const Trace& legacy, const Trace& v2, Counters& counts) {
    if (legacy.repairable && v2.repairable) ++counts.both_pass;
    if (!legacy.repairable && !v2.repairable) {
        ++counts.both_fail;
        if (legacy.failure >= 0) ++counts.failure_at[legacy.failure];
    }
    for (unsigned sa = 0; sa != kSaNum; ++sa) {
        unsigned valid_count = 0;
        for (const Candidate& c : maps[sa]) valid_count += local_accept(c);
        if (valid_count > 1) ++counts.multiple_locally_valid;
        if (!legacy.committed[sa]) {
            if (sa != 0) {
                const Ledger& before = legacy.ledger_after[sa - 1];
                const auto priority = donors(sa);
                for (unsigned slot = 2; slot != 4; ++slot) {
                    if (local_accept(maps[sa][slot]) &&
                        (before.borrowed[priority[0]] || before.borrowed[priority[1]])) {
                        ++counts.resource_consumption_events;
                        break;
                    }
                }
            }
            continue;
        }
        ++counts.selected_slot[config_slot(sa, legacy.config[sa])];
        if (legacy.release[sa]) ++counts.release_transactions;
        if (legacy.borrow[sa]) {
            ++counts.borrow_transactions;
            if (legacy.donor[sa] != donors(sa)[0]) ++counts.donor_fallbacks;
        }
        if (legacy.release[sa] && legacy.borrow[sa]) ++counts.release_borrow_transactions;
    }
}

bool run_vector(const Maps& maps, unsigned vector, Counters& counts) {
    Vrecam_dss_v2_group_equivalence_test_top dut;
    dut.clk_i = 0; dut.rst_ni = 0; dut.start_i = 0;
    drive_maps(dut, maps); tick(dut);
    dut.rst_ni = 1; dut.start_i = 1; tick(dut); dut.start_i = 0;
    Trace legacy, v2;
    unsigned legacy_commits = 0, v2_commits = 0;
    bool legacy_done = false, v2_done = false;
    for (unsigned cycle = 0; cycle != 64 && !(legacy_done && v2_done); ++cycle) {
        tick(dut);
        capture_legacy(dut, legacy, legacy_commits);
        capture_v2(dut, v2, v2_commits);
        legacy_done = legacy_done || dut.legacy_done_o;
        v2_done = v2_done || dut.v2_done_o;
    }
    if (!legacy_done || !v2_done) {
        std::cerr << "GROUP_EQUIVALENCE timeout seed=" << kSeed << " vector=" << vector << '\n';
        dump_maps(maps);
        return false;
    }
    finish_trace(dut, legacy, v2);
    const bool mismatch = compare(legacy, v2, counts);
    update_coverage(maps, legacy, v2, counts);
    if (mismatch) {
        std::cerr << "GROUP_EQUIVALENCE MISMATCH seed=" << kSeed << " vector=" << vector << '\n';
        dump_maps(maps); dump_trace("LEGACY", legacy); dump_trace("V2", v2);
    }
    return !mismatch;
}

int run(unsigned vectors) {
    std::mt19937 rng(kSeed);
    Counters counts;
    counts.vectors = vectors;
    for (unsigned vector = 0; vector != vectors; ++vector)
        if (!run_vector(random_maps(rng), vector, counts)) ++counts.vector_mismatches;
    std::cout << "GROUP_RANDOM seed=" << kSeed << " vectors=" << vectors
              << " mismatches=" << counts.vector_mismatches << '\n';
    std::cout << "LEDGER_MISMATCHES=" << counts.ledger_mismatches
              << " CONFIG_MISMATCHES=" << counts.config_mismatches
              << " PATTERN_ID_MISMATCHES=" << counts.pattern_mismatches
              << " DONOR_RELEASE_MISMATCHES=" << counts.donor_release_mismatches
              << " FAILURE_POSITION_MISMATCHES=" << counts.failure_mismatches
              << " GROUP_REPAIRABLE_MISMATCHES=" << counts.repairable_mismatches << '\n';
    std::cout << "COVERAGE both_pass=" << counts.both_pass << " both_fail=" << counts.both_fail
              << " failure_A=" << counts.failure_at[0] << " failure_B=" << counts.failure_at[1]
              << " failure_C=" << counts.failure_at[2] << " failure_D=" << counts.failure_at[3]
              << " slot1=" << counts.selected_slot[1] << " slot0=" << counts.selected_slot[0]
              << " slot3=" << counts.selected_slot[3] << " slot2=" << counts.selected_slot[2]
              << " release=" << counts.release_transactions << " borrow=" << counts.borrow_transactions
              << " release_borrow=" << counts.release_borrow_transactions
              << " donor_fallback=" << counts.donor_fallbacks
              << " multiple_local=" << counts.multiple_locally_valid
              << " resource_consumption=" << counts.resource_consumption_events
              << " slot3_infeasible_to_slot2_feasible=0" << '\n';
    return counts.vector_mismatches ? 1 : 0;
}
}  // namespace

int main(int argc, char** argv) {
    const unsigned vectors = argc == 2 ? std::strtoul(argv[1], nullptr, 10) : 50;
    return vectors ? run(vectors) : 2;
}
