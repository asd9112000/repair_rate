#include "Vrecam_dss_v2_early_equivalence_test_top.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>

namespace {
constexpr unsigned kSeed = 20260910;
constexpr unsigned kSaCount = 4;
constexpr unsigned kSlotCount = 4;

struct Candidate {
    bool valid = false;
    bool solution = false;
    bool repairable = false;
    unsigned config = 0;
    unsigned pattern = 0;
    unsigned bitmap = 0;
};
using Maps = std::array<std::array<Candidate, kSlotCount>, kSaCount>;

struct Ledger {
    std::array<bool, 4> released{};
    std::array<bool, 4> borrowed{};
    std::array<unsigned, 4> borrower{};
};

struct Trace {
    bool group_repairable = false;
    int failure_position = -1;
    std::array<bool, 4> committed{};
    std::array<unsigned, 4> config{};
    std::array<unsigned, 4> pattern{};
    std::array<bool, 4> borrow{};
    std::array<unsigned, 4> donor{};
    std::array<bool, 4> release{};
    std::array<Ledger, 4> ledger_after{};
    std::array<bool, 4> physical_fallback{};
};

struct Counters {
    unsigned vectors = 0;
    unsigned vector_mismatches = 0;
    unsigned ledger_mismatches = 0;
    unsigned config_mismatches = 0;
    unsigned pattern_mismatches = 0;
    unsigned donor_release_mismatches = 0;
    unsigned failure_position_mismatches = 0;
    unsigned group_repairable_mismatches = 0;
    unsigned both_pass = 0;
    unsigned both_fail = 0;
    std::array<unsigned, 4> failure_at{};
    unsigned release_transactions = 0;
    unsigned borrow_transactions = 0;
    unsigned release_borrow_transactions = 0;
    unsigned donor_fallbacks = 0;
    unsigned multiple_locally_valid = 0;
    unsigned physical_fallback = 0;
};

unsigned canonical_config(unsigned sa, unsigned slot) {
    static constexpr unsigned order[4][4] = {
        {0, 4, 5, 6}, {0, 2, 1, 3}, {0, 2, 1, 3}, {0, 4, 5, 6}};
    return order[sa][slot];
}
unsigned own_resource(unsigned sa) {
    static constexpr unsigned resource[4] = {0, 2, 3, 1};
    return resource[sa];
}
std::array<unsigned, 2> donor_priority(unsigned sa) {
    static constexpr unsigned priority[4][2] = {{2, 3}, {0, 1}, {1, 0}, {3, 2}};
    return {priority[sa][0], priority[sa][1]};
}
bool requires_release(unsigned sa, unsigned config) {
    return (sa == 0 || sa == 3) ? (config == 4 || config == 6)
                                : (config == 1 || config == 3);
}
bool requires_borrow(unsigned sa, unsigned config) {
    return (sa == 0 || sa == 3) ? (config == 5 || config == 6)
                                : (config == 2 || config == 3);
}

bool same_ledger(const Ledger& a, const Ledger& b) {
    for (unsigned resource = 0; resource != 4; ++resource) {
        if (a.released[resource] != b.released[resource] ||
            a.borrowed[resource] != b.borrowed[resource])
            return false;
        if (a.borrowed[resource] && a.borrower[resource] != b.borrower[resource])
            return false;
    }
    return true;
}
Ledger decode_legacy_ledger(unsigned packed) {
    Ledger ledger;
    for (unsigned resource = 0; resource != 4; ++resource) {
        ledger.released[resource] = (packed >> resource) & 1;
        const unsigned code = (packed >> (4 + resource * 2)) & 3;
        ledger.borrowed[resource] = code != 0;
        if (!ledger.borrowed[resource]) continue;
        if (resource < 2)
            ledger.borrower[resource] = code == 1 ? 1 : 2;
        else
            ledger.borrower[resource] = code == 1 ? 0 : 3;
    }
    return ledger;
}
Ledger v2_ledger(const Vrecam_dss_v2_early_equivalence_test_top& dut) {
    Ledger ledger;
    for (unsigned resource = 0; resource != 4; ++resource) {
        ledger.released[resource] = (dut.v2_canonical_released_o >> resource) & 1;
        ledger.borrowed[resource] = (dut.v2_canonical_borrowed_o >> resource) & 1;
        if (ledger.borrowed[resource])
            ledger.borrower[resource] =
                (dut.v2_canonical_borrower_id_flat_o >> (resource * 2)) & 3;
    }
    return ledger;
}

Trace reference_early(const Maps& maps) {
    Trace trace;
    Ledger ledger;
    for (unsigned sa = 0; sa != kSaCount; ++sa) {
        bool earlier_physical_failure = false;
        unsigned locally_valid = 0;
        for (const Candidate& candidate : maps[sa])
            locally_valid += candidate.valid && candidate.solution && candidate.repairable;
        (void)locally_valid;
        for (unsigned slot = 0; slot != kSlotCount; ++slot) {
            const Candidate& candidate = maps[sa][slot];
            if (!candidate.valid || !candidate.solution || !candidate.repairable)
                continue;
            const bool borrow = requires_borrow(sa, candidate.config);
            const bool release = requires_release(sa, candidate.config);
            unsigned donor = 0;
            bool feasible = true;
            if (borrow) {
                const auto priority = donor_priority(sa);
                if (ledger.released[priority[0]] && !ledger.borrowed[priority[0]])
                    donor = priority[0];
                else if (ledger.released[priority[1]] && !ledger.borrowed[priority[1]])
                    donor = priority[1];
                else
                    feasible = false;
            }
            if (!feasible) {
                earlier_physical_failure = true;
                continue;
            }
            trace.committed[sa] = true;
            trace.config[sa] = candidate.config;
            trace.pattern[sa] = candidate.pattern;
            trace.borrow[sa] = borrow;
            trace.donor[sa] = donor;
            trace.release[sa] = release;
            trace.physical_fallback[sa] = earlier_physical_failure;
            if (release) ledger.released[own_resource(sa)] = true;
            if (borrow) {
                ledger.borrowed[donor] = true;
                ledger.borrower[donor] = sa;
            }
            trace.ledger_after[sa] = ledger;
            break;
        }
        if (!trace.committed[sa]) {
            trace.group_repairable = false;
            trace.failure_position = static_cast<int>(sa);
            return trace;
        }
    }
    trace.group_repairable = true;
    return trace;
}

void write_entry(std::uint32_t* words, unsigned slot, const Candidate& candidate) {
    const std::uint64_t bits = (std::uint64_t(candidate.bitmap & 0x3ff) << 10) |
                               (std::uint64_t(candidate.pattern & 0xf) << 6) |
                               (std::uint64_t(candidate.repairable) << 5) |
                               (std::uint64_t(candidate.solution) << 4) |
                               (std::uint64_t(candidate.config & 7) << 1) |
                               std::uint64_t(candidate.valid);
    const unsigned bit = slot * 20;
    words[bit / 32] |= static_cast<std::uint32_t>(bits << (bit % 32));
    if ((bit % 32) > 12)
        words[bit / 32 + 1] |= static_cast<std::uint32_t>(bits >> (32 - (bit % 32)));
}
std::uint32_t* map_port(Vrecam_dss_v2_early_equivalence_test_top& dut, unsigned sa) {
    switch (sa) {
    case 0: return dut.map_a_i;
    case 1: return dut.map_b_i;
    case 2: return dut.map_c_i;
    default: return dut.map_d_i;
    }
}
void drive_maps(Vrecam_dss_v2_early_equivalence_test_top& dut, const Maps& maps) {
    for (unsigned sa = 0; sa != kSaCount; ++sa) {
        std::uint32_t* port = map_port(dut, sa);
        for (unsigned word = 0; word != 3; ++word) port[word] = 0;
        for (unsigned slot = 0; slot != kSlotCount; ++slot)
            write_entry(port, slot, maps[sa][slot]);
    }
}
void tick(Vrecam_dss_v2_early_equivalence_test_top& dut) {
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

void capture_legacy(const Vrecam_dss_v2_early_equivalence_test_top& dut,
                    Trace& trace, unsigned& previous_commit) {
    const unsigned commits = dut.legacy_sa_commit_valid_o;
    for (unsigned sa = 0; sa != 4; ++sa) {
        if (((commits >> sa) & 1) && !((previous_commit >> sa) & 1)) {
            trace.committed[sa] = true;
            trace.config[sa] = (dut.legacy_selected_config_flat_o >> (3 * sa)) & 7;
            trace.pattern[sa] = (dut.legacy_selected_pattern_flat_o >> (4 * sa)) & 0xf;
            trace.borrow[sa] = (dut.legacy_borrow_flat_o >> sa) & 1;
            trace.donor[sa] = (dut.legacy_selected_donor_flat_o >> (2 * sa)) & 3;
            trace.release[sa] = (dut.legacy_release_flat_o >> sa) & 1;
            trace.ledger_after[sa] = decode_legacy_ledger(dut.legacy_ledger_o);
        }
    }
    previous_commit = commits;
}
void capture_v2(const Vrecam_dss_v2_early_equivalence_test_top& dut,
                Trace& trace, unsigned& previous_commit) {
    const unsigned commits = dut.v2_sa_commit_valid_o;
    for (unsigned sa = 0; sa != 4; ++sa) {
        if (((commits >> sa) & 1) && !((previous_commit >> sa) & 1)) {
            trace.committed[sa] = true;
            trace.config[sa] = (dut.v2_selected_config_flat_o >> (3 * sa)) & 7;
            trace.pattern[sa] = (dut.v2_selected_pattern_flat_o >> (4 * sa)) & 0xf;
            trace.borrow[sa] = (dut.v2_borrow_flat_o >> sa) & 1;
            trace.donor[sa] = (dut.v2_selected_donor_flat_o >> (2 * sa)) & 3;
            trace.release[sa] = (dut.v2_release_flat_o >> sa) & 1;
            trace.ledger_after[sa] = v2_ledger(dut);
        }
    }
    previous_commit = commits;
}

void finalize_trace(const Vrecam_dss_v2_early_equivalence_test_top& dut,
                    Trace& legacy, Trace& v2) {
    legacy.group_repairable = dut.legacy_group_repairable_o;
    if (!legacy.group_repairable)
        for (unsigned sa = 0; sa != 4; ++sa)
            if (!legacy.committed[sa]) { legacy.failure_position = static_cast<int>(sa); break; }
    v2.group_repairable = dut.v2_group_repairable_o;
    v2.failure_position = v2.group_repairable ? -1 : dut.v2_failure_position_o;
}

void dump_maps(const Maps& maps) {
    static const char* name[] = {"A", "B", "C", "D"};
    for (unsigned sa = 0; sa != 4; ++sa) {
        std::cerr << "  SA " << name[sa] << ':';
        for (const Candidate& c : maps[sa])
            std::cerr << " {v=" << c.valid << ",sol=" << c.solution << ",rep=" << c.repairable
                      << ",cfg=" << c.config << ",pat=" << c.pattern << ",bitmap=0x"
                      << std::hex << c.bitmap << std::dec << '}';
        std::cerr << '\n';
    }
}
void dump_trace(const char* label, const Trace& trace) {
    std::cerr << "  " << label << " result=" << trace.group_repairable
              << " failure=" << trace.failure_position << '\n';
    for (unsigned sa = 0; sa != 4; ++sa) if (trace.committed[sa]) {
        const Ledger& ledger = trace.ledger_after[sa];
        std::cerr << "    SA=" << sa << " cfg=" << trace.config[sa]
                  << " pat=" << trace.pattern[sa] << " borrow=" << trace.borrow[sa]
                  << " donor=" << trace.donor[sa] << " release=" << trace.release[sa]
                  << " released=0x" << std::hex
                  << (unsigned(ledger.released[0]) | (unsigned(ledger.released[1]) << 1) |
                      (unsigned(ledger.released[2]) << 2) | (unsigned(ledger.released[3]) << 3))
                  << " borrowed=0x"
                  << (unsigned(ledger.borrowed[0]) | (unsigned(ledger.borrowed[1]) << 1) |
                      (unsigned(ledger.borrowed[2]) << 2) | (unsigned(ledger.borrowed[3]) << 3))
                  << std::dec << '\n';
    }
}

bool compare_trace(const Trace& actual, const Trace& reference, Counters& counters) {
    bool mismatch = false;
    if (actual.group_repairable != reference.group_repairable) {
        ++counters.group_repairable_mismatches; mismatch = true;
    }
    if (actual.failure_position != reference.failure_position) {
        ++counters.failure_position_mismatches; mismatch = true;
    }
    for (unsigned sa = 0; sa != 4; ++sa) {
        if (actual.committed[sa] != reference.committed[sa]) {
            ++counters.config_mismatches; mismatch = true;
            continue;
        }
        if (!reference.committed[sa]) continue;
        if (actual.config[sa] != reference.config[sa]) {
            ++counters.config_mismatches; mismatch = true;
        }
        if (actual.pattern[sa] != reference.pattern[sa]) {
            ++counters.pattern_mismatches; mismatch = true;
        }
        if (actual.borrow[sa] != reference.borrow[sa] ||
            actual.release[sa] != reference.release[sa] ||
            (reference.borrow[sa] && actual.donor[sa] != reference.donor[sa])) {
            ++counters.donor_release_mismatches; mismatch = true;
        }
        if (!same_ledger(actual.ledger_after[sa], reference.ledger_after[sa])) {
            ++counters.ledger_mismatches; mismatch = true;
        }
    }
    return mismatch;
}

Maps random_maps(std::mt19937& rng) {
    Maps maps{};
    for (unsigned sa = 0; sa != 4; ++sa) for (unsigned slot = 0; slot != 4; ++slot) {
        Candidate& c = maps[sa][slot];
        c.valid = (rng() % 100) < 85;
        c.solution = (rng() % 100) < 85;
        c.repairable = (rng() % 100) < 85;
        c.config = canonical_config(sa, slot);
        c.pattern = rng() & 0xf;
        c.bitmap = rng() & 0x3ff;
    }
    return maps;
}

void update_coverage(const Maps& maps, const Trace& reference, Counters& counters) {
    if (reference.group_repairable) ++counters.both_pass;
    else {
        ++counters.both_fail;
        ++counters.failure_at[reference.failure_position];
    }
    for (unsigned sa = 0; sa != 4; ++sa) {
        unsigned locally_valid = 0;
        for (const Candidate& c : maps[sa]) locally_valid += c.valid && c.solution && c.repairable;
        if (locally_valid > 1) ++counters.multiple_locally_valid;
        if (!reference.committed[sa]) continue;
        if (reference.release[sa]) ++counters.release_transactions;
        if (reference.borrow[sa]) {
            ++counters.borrow_transactions;
            if (reference.donor[sa] != donor_priority(sa)[0]) ++counters.donor_fallbacks;
        }
        if (reference.release[sa] && reference.borrow[sa]) ++counters.release_borrow_transactions;
        if (reference.physical_fallback[sa]) ++counters.physical_fallback;
    }
}

bool run_vector(const Maps& maps, unsigned vector, Counters& counters) {
    Vrecam_dss_v2_early_equivalence_test_top dut;
    dut.clk_i = 0; dut.rst_ni = 0; dut.start_i = 0;
    drive_maps(dut, maps);
    tick(dut);
    dut.rst_ni = 1;
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;

    Trace legacy, v2;
    unsigned legacy_commits = 0, v2_commits = 0;
    // done_o is a one-cycle pulse.  The map-based legacy allocator can finish
    // before the streaming core has examined every rejected candidate, so the
    // two pulses must be observed independently rather than simultaneously.
    bool legacy_done_seen = false, v2_done_seen = false;
    for (unsigned cycle = 0; cycle != 32 && !(legacy_done_seen && v2_done_seen); ++cycle) {
        tick(dut);
        capture_legacy(dut, legacy, legacy_commits);
        capture_v2(dut, v2, v2_commits);
        legacy_done_seen = legacy_done_seen || dut.legacy_done_o;
        v2_done_seen = v2_done_seen || dut.v2_done_o;
    }
    if (!legacy_done_seen || !v2_done_seen) {
        std::cerr << "MISMATCH seed=" << kSeed << " vector=" << vector << " timeout\n";
        return false;
    }
    finalize_trace(dut, legacy, v2);
    const Trace reference = reference_early(maps);
    const bool legacy_mismatch = compare_trace(legacy, reference, counters);
    const bool v2_mismatch = compare_trace(v2, reference, counters);
    update_coverage(maps, reference, counters);
    if (legacy_mismatch || v2_mismatch) {
        std::cerr << "RTL_VS_V2_EARLY MISMATCH seed=" << kSeed << " vector=" << vector << '\n';
        dump_maps(maps); dump_trace("REFERENCE", reference); dump_trace("LEGACY", legacy); dump_trace("V2", v2);
        return false;
    }
    return true;
}

int run(unsigned vectors) {
    std::mt19937 rng(kSeed);
    Counters counters;
    counters.vectors = vectors;
    for (unsigned vector = 0; vector != vectors; ++vector) {
        const Maps maps = random_maps(rng);
        if (!run_vector(maps, vector, counters)) ++counters.vector_mismatches;
    }
    std::cout << "EARLY_RANDOM seed=" << kSeed << " vectors=" << vectors
              << " mismatches=" << counters.vector_mismatches << '\n';
    std::cout << "LEDGER_MISMATCHES=" << counters.ledger_mismatches
              << " CONFIG_MISMATCHES=" << counters.config_mismatches
              << " PATTERN_ID_MISMATCHES=" << counters.pattern_mismatches
              << " DONOR_RELEASE_MISMATCHES=" << counters.donor_release_mismatches
              << " FAILURE_POSITION_MISMATCHES=" << counters.failure_position_mismatches
              << " GROUP_REPAIRABLE_MISMATCHES=" << counters.group_repairable_mismatches << '\n';
    std::cout << "COVERAGE both_pass=" << counters.both_pass << " both_fail=" << counters.both_fail
              << " failure_A=" << counters.failure_at[0] << " failure_B=" << counters.failure_at[1]
              << " failure_C=" << counters.failure_at[2] << " failure_D=" << counters.failure_at[3]
              << " release=" << counters.release_transactions << " borrow=" << counters.borrow_transactions
              << " release_borrow=" << counters.release_borrow_transactions
              << " donor_fallback=" << counters.donor_fallbacks
              << " multiple_local=" << counters.multiple_locally_valid
              << " physical_fallback=" << counters.physical_fallback << '\n';
    return counters.vector_mismatches ? 1 : 0;
}
}  // namespace

int main(int argc, char** argv) {
    const unsigned vectors = argc == 2 ? std::strtoul(argv[1], nullptr, 10) : 50;
    if (vectors == 0) return 2;
    return run(vectors);
}
