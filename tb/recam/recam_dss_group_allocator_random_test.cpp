#include "Vrecam_dss_group_allocator.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <utility>

// Independent reference model.  It does not instantiate, call, or consume any
// Phase 3G/3I RTL helper or DUT decision/output signal.
struct ConfigPatternMap {
    bool valid = false;
    bool solution = false;
    bool repairable = false;
    unsigned config_id = 0;
    unsigned pattern = 0;
    unsigned candidate = 0;
};

enum ResourceId { A_ROW = 0, D_ROW = 1, B_COL = 2, C_COL = 3 };
struct ResourceLedger { bool released = false; unsigned borrower = 0; };
struct GoldenAllocation {
    bool group_repairable = true;
    int failure_sa = -1;
    std::array<unsigned, 4> selected_config{};
    std::array<unsigned, 4> selected_donor{};
    std::array<ResourceLedger, 4> ledger{};
    std::array<unsigned, 4> ledger_after{};
};

static bool role_legal(unsigned sa, unsigned cfg) {
    return (sa == 0 || sa == 3) ? (cfg == 0 || (cfg >= 4 && cfg <= 6))
                                 : cfg <= 3;
}
static bool releases(unsigned cfg) { return cfg == 4 || cfg == 6 || cfg == 1 || cfg == 3; }
static bool borrows(unsigned cfg) { return cfg == 5 || cfg == 6 || cfg == 2 || cfg == 3; }
static ResourceId own_resource(unsigned sa) {
    static const std::array<ResourceId, 4> table = {A_ROW, B_COL, C_COL, D_ROW};
    return table[sa];
}
static std::array<ResourceId, 2> donor_priority(unsigned sa) {
    static const std::array<std::array<ResourceId, 2>, 4> table = {{
        {{B_COL, C_COL}}, {{A_ROW, D_ROW}}, {{D_ROW, A_ROW}}, {{C_COL, B_COL}}
    }};
    return table[sa];
}
static unsigned resource_local_borrower(ResourceId resource, unsigned sa) {
    // The 12-bit hardware ledger stores a two-bit, resource-local neighbor code.
    return resource < B_COL ? (sa == 1 ? 1 : 2) : (sa == 0 ? 1 : 2);
}
static std::array<unsigned, 4> rank_entries(unsigned sa, bool group_policy) {
    if (!group_policy) return {{0, 1, 2, 3}};
    return (sa == 0 || sa == 3) ? std::array<unsigned, 4>{{1, 0, 3, 2}}
                                 : std::array<unsigned, 4>{{2, 0, 3, 1}};
}
static unsigned pack_ledger(const std::array<ResourceLedger, 4>& ledger) {
    unsigned packed = 0;
    for (unsigned r = 0; r != 4; ++r) {
        packed |= unsigned(ledger[r].released) << r;
        packed |= ledger[r].borrower << (4 + 2 * r);
    }
    return packed;
}
static GoldenAllocation golden_allocate(
    const std::array<std::array<ConfigPatternMap, 4>, 4>& maps, bool group_policy) {
    GoldenAllocation result;
    for (unsigned sa = 0; sa != 4; ++sa) { // Frozen A -> B -> C -> D traversal.
        bool chosen = false;
        for (unsigned index : rank_entries(sa, group_policy)) {
            const ConfigPatternMap& entry = maps[sa][index];
            if (!entry.valid || !entry.solution || !entry.repairable ||
                !role_legal(sa, entry.config_id))
                continue;
            ResourceId donor = A_ROW;
            if (borrows(entry.config_id)) {
                const auto priority = donor_priority(sa);
                if (result.ledger[priority[0]].released && !result.ledger[priority[0]].borrower)
                    donor = priority[0];
                else if (result.ledger[priority[1]].released && !result.ledger[priority[1]].borrower)
                    donor = priority[1];
                else
                    continue;
            }
            // Feasibility was evaluated against the old ledger; commit both actions atomically.
            if (releases(entry.config_id)) result.ledger[own_resource(sa)].released = true;
            if (borrows(entry.config_id))
                result.ledger[donor].borrower = resource_local_borrower(donor, sa);
            result.selected_config[sa] = entry.config_id;
            result.selected_donor[sa] = borrows(entry.config_id) ? donor : 0;
            result.ledger_after[sa] = pack_ledger(result.ledger);
            chosen = true;
            break;
        }
        if (!chosen) {
            result.group_repairable = false;
            result.failure_sa = static_cast<int>(sa);
            break; // First failure; do not roll back prior commits.
        }
    }
    return result;
}

static void clear_maps(Vrecam_dss_group_allocator& dut) {
    for (auto& word : dut.map_a_i) word = 0;
    for (auto& word : dut.map_b_i) word = 0;
    for (auto& word : dut.map_c_i) word = 0;
    for (auto& word : dut.map_d_i) word = 0;
}
static std::uint32_t* map_port(Vrecam_dss_group_allocator& dut, unsigned sa) {
    switch (sa) {
    case 0: return dut.map_a_i;
    case 1: return dut.map_b_i;
    case 2: return dut.map_c_i;
    default: return dut.map_d_i;
    }
}
static void write_entry(Vrecam_dss_group_allocator& dut, unsigned sa, unsigned index,
                        const ConfigPatternMap& entry) {
    const std::uint64_t bits = (std::uint64_t(entry.candidate & 0x3ff) << 10) |
                               (std::uint64_t(entry.pattern & 0xf) << 6) |
                               (std::uint64_t(entry.repairable) << 5) |
                               (std::uint64_t(entry.solution) << 4) |
                               (std::uint64_t(entry.config_id & 7) << 1) |
                               std::uint64_t(entry.valid);
    std::uint32_t* port = map_port(dut, sa);
    const unsigned bit = index * 20, word = bit / 32, offset = bit % 32;
    port[word] |= static_cast<std::uint32_t>(bits << offset);
    if (offset > 12) port[word + 1] |= static_cast<std::uint32_t>(bits >> (32 - offset));
}
static void tick(Vrecam_dss_group_allocator& dut) {
    dut.clk_i = 0; dut.eval();
    dut.clk_i = 1; dut.eval();
}
struct DutTrace {
    bool group_repairable = false;
    int failure_sa = -1;
    std::array<unsigned, 4> selected_config{};
    std::array<unsigned, 4> selected_donor{};
    unsigned ledger = 0;
    bool busy_start_ignored = true;
};
static DutTrace run_dut(const std::array<std::array<ConfigPatternMap, 4>, 4>& maps,
                        bool group_policy, bool repeat_start_while_busy = false) {
    Vrecam_dss_group_allocator dut;
    dut.rst_ni = 0; dut.start_i = 0; dut.policy_i = group_policy;
    clear_maps(dut); tick(dut);
    dut.rst_ni = 1;
    for (unsigned sa = 0; sa != 4; ++sa)
        for (unsigned entry = 0; entry != 4; ++entry) write_entry(dut, sa, entry, maps[sa][entry]);
    dut.start_i = 1; tick(dut);
    bool busy_start_ignored = true;
    if (repeat_start_while_busy) {
        tick(dut);
        busy_start_ignored = dut.busy_o && dut.sa_commit_valid_o == 1;
    }
    dut.start_i = 0;
    for (unsigned cycles = 0; cycles != 5 && !dut.done_o; ++cycles) tick(dut);

    DutTrace trace;
    trace.busy_start_ignored = busy_start_ignored;
    trace.group_repairable = dut.group_repairable_o;
    trace.ledger = dut.ledger_released_borrower_o;
    for (unsigned sa = 0; sa != 4; ++sa) {
        trace.selected_config[sa] = (dut.selected_config_flat_o >> (3 * sa)) & 7;
        trace.selected_donor[sa] = (dut.selected_donor_flat_o >> (2 * sa)) & 3;
    }
    if (!trace.group_repairable)
        for (unsigned sa = 0; sa != 4; ++sa)
            if (((dut.sa_commit_valid_o >> sa) & 1) == 0) { trace.failure_sa = sa; break; }
    return trace;
}

static void dump_maps(const std::array<std::array<ConfigPatternMap, 4>, 4>& maps) {
    static const char* names[] = {"A", "B", "C", "D"};
    for (unsigned sa = 0; sa != 4; ++sa) {
        std::cerr << "  ConfigPatternMap_" << names[sa] << ":";
        for (const auto& e : maps[sa])
            std::cerr << " {v=" << e.valid << ",s=" << e.solution << ",r=" << e.repairable
                      << ",cfg=" << e.config_id << ",pat=" << e.pattern << ",cand=" << e.candidate << "}";
        std::cerr << '\n';
    }
}
static void dump_trace(const char* name, const GoldenAllocation& golden, const DutTrace& dut) {
    std::cerr << "  Golden: cfg=" << golden.selected_config[0] << "/" << golden.selected_config[1]
              << "/" << golden.selected_config[2] << "/" << golden.selected_config[3]
              << " donor=" << golden.selected_donor[0] << "/" << golden.selected_donor[1]
              << "/" << golden.selected_donor[2] << "/" << golden.selected_donor[3]
              << " ledger=0x" << std::hex << pack_ledger(golden.ledger) << std::dec
              << " result=" << golden.group_repairable << " failure=" << golden.failure_sa << '\n';
    std::cerr << "  " << name << ": cfg=" << dut.selected_config[0] << "/" << dut.selected_config[1]
              << "/" << dut.selected_config[2] << "/" << dut.selected_config[3]
              << " donor=" << dut.selected_donor[0] << "/" << dut.selected_donor[1]
              << "/" << dut.selected_donor[2] << "/" << dut.selected_donor[3]
              << " ledger=0x" << std::hex << dut.ledger << std::dec
              << " result=" << dut.group_repairable << " failure=" << dut.failure_sa << '\n';
}
static bool equal(const GoldenAllocation& golden, const DutTrace& dut) {
    return golden.selected_config == dut.selected_config &&
           golden.selected_donor == dut.selected_donor &&
           pack_ledger(golden.ledger) == dut.ledger &&
           golden.group_repairable == dut.group_repairable && golden.failure_sa == dut.failure_sa;
}
static std::array<std::array<ConfigPatternMap, 4>, 4> random_maps(std::mt19937& rng) {
    std::array<std::array<ConfigPatternMap, 4>, 4> maps{};
    for (auto& sa : maps) for (auto& e : sa) {
        e.valid = (rng() & 3) != 0;
        e.solution = (rng() & 3) != 0;
        e.repairable = (rng() & 3) != 0;
        e.config_id = rng() % 7;
        e.pattern = rng() & 0xf;
        e.candidate = rng() & 0x3ff;
    }
    return maps;
}
static ConfigPatternMap usable(unsigned config_id) {
    return {true, true, true, config_id, 0, 1};
}
static std::array<std::array<ConfigPatternMap, 4>, 4> all_local_maps() {
    std::array<std::array<ConfigPatternMap, 4>, 4> maps{};
    for (unsigned sa = 0; sa != 4; ++sa) maps[sa][0] = usable(0);
    return maps;
}
static bool run_directed_case(const char* name, bool policy,
                              const std::array<std::array<ConfigPatternMap, 4>, 4>& maps,
                              bool expected_repairable, const char* expected,
                              bool repeat_start_while_busy = false) {
    const GoldenAllocation golden = golden_allocate(maps, policy);
    const DutTrace dut = run_dut(maps, policy, repeat_start_while_busy);
    const bool pass = equal(golden, dut) && golden.group_repairable == expected_repairable &&
                      (!repeat_start_while_busy || dut.busy_start_ignored);
    std::cout << "DIRECTED " << name << " policy=" << (policy ? "GROUP" : "EARLY")
              << " expected=" << expected
              << " actual_cfg=" << dut.selected_config[0] << "/" << dut.selected_config[1]
              << "/" << dut.selected_config[2] << "/" << dut.selected_config[3]
              << " donor=" << dut.selected_donor[0] << "/" << dut.selected_donor[1]
              << "/" << dut.selected_donor[2] << "/" << dut.selected_donor[3]
              << " repairable=" << dut.group_repairable << " ledger=0x" << std::hex << dut.ledger
              << std::dec << " " << (pass ? "PASS" : "FAIL") << '\n';
    if (!pass) { dump_maps(maps); dump_trace("RTL", golden, dut); }
    return pass;
}
static unsigned run_directed_matrix() {
    unsigned failures = 0;
    auto fail_if = [&](bool pass) { if (!pass) ++failures; };
    auto maps = all_local_maps();
    fail_if(run_directed_case("01_all_local_success", false, maps, true, "all local commits"));

    maps = all_local_maps(); maps[0][1] = usable(4); maps[1][0] = {}; maps[1][1] = usable(2);
    fail_if(run_directed_case("02_release_helps_later_borrower", true, maps, true, "A releases A_ROW; B borrows it"));

    maps = all_local_maps(); maps[0][1] = usable(4); maps[1][0] = {}; maps[1][3] = usable(3); maps[3][0] = {}; maps[3][2] = usable(5);
    fail_if(run_directed_case("03_borrow_release_helps_later_borrower", true, maps, true, "B borrows A_ROW and releases B_COL; D borrows B_COL"));

    maps = all_local_maps(); maps[0][1] = usable(4); maps[1][0] = {}; maps[1][1] = usable(2); maps[2][1] = usable(2); maps[2][0] = {};
    fail_if(run_directed_case("04_donor_conflict", true, maps, false, "B consumes A_ROW; C has no donor"));

    maps = all_local_maps(); maps[1][2] = usable(1); maps[1][0] = {}; maps[3][2] = usable(5); maps[3][0] = {};
    fail_if(run_directed_case("05_alternate_donor_fallback", true, maps, true, "D falls back C_COL to released B_COL"));

    maps = all_local_maps(); maps[0][0] = usable(5); maps[0][1] = usable(0);
    fail_if(run_directed_case("06_config_fallback_after_borrow_failure", false, maps, true, "A borrow fails then local cfg0 selected"));

    maps = all_local_maps(); maps[0][1] = usable(4); maps[1][0] = {}; maps[1][1] = usable(2);
    fail_if(run_directed_case("07_release_to_borrow_transition", true, maps, true, "registered release visible to next SA"));

    maps = all_local_maps(); maps[0][1] = usable(4); maps[1][0] = {}; maps[1][1] = usable(2); maps[2][1] = usable(2); maps[2][0] = {};
    fail_if(run_directed_case("08_no_double_allocation", true, maps, false, "A_ROW cannot have two borrowers"));

    maps = all_local_maps(); maps[0] = {};
    fail_if(run_directed_case("09_first_sa_failure", false, maps, false, "failure position A"));
    maps = all_local_maps(); maps[1] = {};
    fail_if(run_directed_case("10_middle_sa_failure", false, maps, false, "failure position B"));
    maps = all_local_maps(); maps[3] = {};
    fail_if(run_directed_case("11_last_sa_failure", false, maps, false, "failure position D"));

    maps = all_local_maps();
    fail_if(run_directed_case("12_restart_reset", false, maps, true, "fresh reset ledger is empty"));
    fail_if(run_directed_case("13_busy_start_negative", false, maps, true, "second start sampled while busy is ignored", true));

    maps = all_local_maps(); maps[0][1] = usable(4);
    fail_if(run_directed_case("14_early_vs_group_fairness_early", false, maps, true, "EARLY chooses local cfg0"));
    fail_if(run_directed_case("14_early_vs_group_fairness_group", true, maps, true, "GROUP chooses release cfg4"));
    fail_if(run_directed_case("15_early_group_different_final_ledger_early", false, maps, true, "empty ledger"));
    fail_if(run_directed_case("15_early_group_different_final_ledger_group", true, maps, true, "A_ROW released"));
    std::cout << "DIRECTED_MATRIX " << (15 - failures) << "/15 " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
static void write_counterexample(std::ofstream& out,
                                 const std::array<std::array<ConfigPatternMap, 4>, 4>& maps,
                                 const GoldenAllocation& early, const GoldenAllocation& group) {
    static const char* names[] = {"A", "B", "C", "D"};
    out << "seed=20260910\nclassification=EARLY_FAIL_GROUP_PASS\n";
    for (unsigned sa = 0; sa != 4; ++sa) {
        out << "ConfigPatternMap_" << names[sa] << ":";
        for (const auto& e : maps[sa]) out << " {v=" << e.valid << ",s=" << e.solution << ",r=" << e.repairable << ",cfg=" << e.config_id << ",pat=" << e.pattern << ",cand=" << e.candidate << "}";
        out << '\n';
    }
    for (const auto& item : {std::pair<const char*, const GoldenAllocation*>{"EARLY", &early}, {"GROUP", &group}}) {
        const auto& r = *item.second;
        out << item.first << " cfg=" << r.selected_config[0] << "/" << r.selected_config[1] << "/" << r.selected_config[2] << "/" << r.selected_config[3]
            << " donor=" << r.selected_donor[0] << "/" << r.selected_donor[1] << "/" << r.selected_donor[2] << "/" << r.selected_donor[3]
            << " ledger_evolution=0x" << std::hex << r.ledger_after[0] << "/0x" << r.ledger_after[1] << "/0x" << r.ledger_after[2] << "/0x" << r.ledger_after[3]
            << " final_ledger=0x" << pack_ledger(r.ledger) << std::dec << " repairable=" << r.group_repairable << " failure=" << r.failure_sa << '\n';
    }
}
static std::array<unsigned, 3> counterexample_statistics() {
    std::mt19937 rng(20260910);
    std::array<unsigned, 3> counts{};
    bool saved = false;
    std::ofstream artifact("tb/recam/phase3hi_counterexample_early_fail_group_pass.txt");
    for (unsigned vector = 0; vector != 1000; ++vector) {
        const auto maps = random_maps(rng);
        const auto early = golden_allocate(maps, false);
        const auto group = golden_allocate(maps, true);
        if (!early.group_repairable && group.group_repairable) {
            ++counts[0];
            if (!saved) { artifact << "vector=" << vector << '\n'; write_counterexample(artifact, maps, early, group); saved = true; }
        }
        if (early.group_repairable && !group.group_repairable) ++counts[1];
        if (early.group_repairable && group.group_repairable && pack_ledger(early.ledger) != pack_ledger(group.ledger)) ++counts[2];
    }
    if (!saved) artifact << "No EARLY_FAIL_GROUP_PASS vector in fixed 1000-vector corpus.\n";
    std::cout << "COUNTEREXAMPLE_STATS EARLY_FAIL_GROUP_PASS=" << counts[0]
              << " EARLY_PASS_GROUP_FAIL=" << counts[1]
              << " BOTH_PASS_LEDGER_DIFFER=" << counts[2] << '\n';
    return counts;
}
static unsigned run_vectors(bool group_policy, unsigned vectors) {
    constexpr unsigned seed = 20260910;
    std::mt19937 rng(seed);
    unsigned mismatches = 0;
    for (unsigned vector = 0; vector != vectors; ++vector) {
        const auto maps = random_maps(rng);
        const GoldenAllocation golden = golden_allocate(maps, group_policy);
        const DutTrace dut = run_dut(maps, group_policy);
        if (!equal(golden, dut)) {
            ++mismatches;
            std::cerr << "RTL_VS_GOLDEN MISMATCH seed=" << seed << " vector=" << vector
                      << " policy=" << (group_policy ? "GROUP" : "EARLY") << '\n';
            dump_maps(maps); dump_trace("RTL", golden, dut);
        }
    }
    std::cout << "RTL_VS_GOLDEN " << (group_policy ? "GROUP" : "EARLY")
              << " seed=" << seed << " vectors=" << vectors << " mismatches=" << mismatches << '\n';
    return mismatches;
}

int main() {
    unsigned failures = run_directed_matrix();
    unsigned mismatches = run_vectors(false, 50) + run_vectors(true, 50);
    if (mismatches) return 1;
    mismatches = run_vectors(false, 1000) + run_vectors(true, 1000);
    counterexample_statistics();
    return (failures || mismatches) ? 1 : 0;
}
