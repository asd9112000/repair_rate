#include "Vrecam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_core.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using Dut = Vrecam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_core;
constexpr int kSa = 4;
constexpr int kAttempts = 3;
constexpr int kPatterns = 15;

struct Candidate { bool valid = false; unsigned rows = 0; unsigned cols = 0; };
using Map = std::array<Candidate, kSa * kAttempts * kPatterns>;
int index(int sa, int attempt, int pattern) { return sa * 45 + attempt * 15 + pattern; }
void tick(Dut &dut) { dut.clk_i = 0; dut.eval(); dut.clk_i = 1; dut.eval(); dut.clk_i = 0; dut.eval(); }
void clear(Dut &dut) {
    for (int i = 0; i < 6; ++i) dut.candidate_valid_i[i] = 0;
    for (int i = 0; i < 17; ++i) dut.candidate_used_rows_flat_i[i] = 0;
    for (int i = 0; i < 12; ++i) dut.candidate_used_cols_flat_i[i] = 0;
}
void drive(Dut &dut, const Map &map) {
    clear(dut);
    for (int sa = 0; sa < kSa; ++sa) for (int attempt = 0; attempt < kAttempts; ++attempt)
        for (int pattern = 0; pattern < kPatterns; ++pattern) {
            const Candidate c = map[index(sa, attempt, pattern)];
            const int candidate = index(sa, attempt, pattern);
            if (c.valid) dut.candidate_valid_i[candidate / 32] |= 1U << (candidate % 32);
            const int row_bit = candidate * 3;
            dut.candidate_used_rows_flat_i[row_bit / 32] |= c.rows << (row_bit % 32);
            if (row_bit % 32 > 29) dut.candidate_used_rows_flat_i[row_bit / 32 + 1] |= c.rows >> (32 - row_bit % 32);
            const int col_bit = candidate * 2;
            dut.candidate_used_cols_flat_i[col_bit / 32] |= c.cols << (col_bit % 32);
            if (col_bit % 32 > 30) dut.candidate_used_cols_flat_i[col_bit / 32 + 1] |= c.cols >> (32 - col_bit % 32);
        }
}
void add(Map &map, int sa, int attempt, int pattern, unsigned rows, unsigned cols = 2) {
    map[index(sa, attempt, pattern)] = {true, rows, cols};
}
void run(Dut &dut, const Map &map) {
    dut.rst_ni = 0; dut.start_i = 0; drive(dut, map); tick(dut);
    dut.rst_ni = 1; dut.start_i = 1; tick(dut); dut.start_i = 0;
    for (int cycle = 0; cycle < 256 && !dut.done_o; ++cycle) { drive(dut, map); tick(dut); }
    if (!dut.done_o) throw std::runtime_error("core timed out");
}
unsigned assignment(const Dut &dut, int row) { return (dut.row_assignment_flat_o >> (row * 3)) & 7U; }
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Dut dut;
    std::size_t adjacency_mismatches = 0, priority_mismatches = 0;
    std::size_t forwarding_violations = 0, early_priority_mismatches = 0, rollback_events = 0;
    std::size_t oracle_mismatches = 0;
    try {
        Map local{};
        for (int sa = 0; sa < kSa; ++sa) add(local, sa, 0, 0, 2);
        run(dut, local);
        if (!dut.group_repairable_o || dut.selected_attempt_flat_o != 0 || dut.selected_pattern_flat_o != 0x1111 ||
            dut.selected_valid_o != 0xf || assignment(dut, 0) != 0 || assignment(dut, 7) != 3) ++oracle_mismatches;

        Map b_left{};
        add(b_left, 0, 0, 0, 1); add(b_left, 1, 1, 0, 3);
        add(b_left, 2, 0, 0, 2); add(b_left, 3, 0, 0, 2);
        run(dut, b_left);
        if (!dut.group_repairable_o || ((dut.selected_donor_flat_o >> 4) & 3U) != 0U) ++priority_mismatches;

        Map c_left{};
        add(c_left, 0, 0, 0, 2); add(c_left, 1, 0, 0, 1);
        add(c_left, 2, 1, 0, 3); add(c_left, 3, 0, 0, 2);
        run(dut, c_left);
        if (!dut.group_repairable_o || ((dut.selected_donor_flat_o >> 8) & 3U) != 1U) ++priority_mismatches;

        Map c_right{};
        add(c_right, 0, 0, 0, 2); add(c_right, 1, 0, 0, 2);
        add(c_right, 2, 1, 0, 3); add(c_right, 3, 0, 0, 1);
        run(dut, c_right);
        if (!dut.group_repairable_o || ((dut.selected_donor_flat_o >> 8) & 3U) != 3U) ++priority_mismatches;

        Map no_forward{};
        add(no_forward, 0, 0, 0, 1); add(no_forward, 1, 1, 0, 3);
        add(no_forward, 2, 1, 0, 3); add(no_forward, 3, 0, 0, 1);
        run(dut, no_forward);
        if (!dut.group_repairable_o || ((dut.selected_donor_flat_o >> 8) & 3U) != 3U || assignment(dut, 1) != 1U)
            ++forwarding_violations;

        Map priority{};
        add(priority, 0, 1, 5, 3); add(priority, 0, 1, 2, 3);
        add(priority, 1, 0, 0, 1); add(priority, 2, 0, 0, 1); add(priority, 3, 0, 0, 1);
        run(dut, priority);
        if ((dut.selected_attempt_flat_o & 3U) != 1U || (dut.selected_pattern_flat_o & 15U) != 3U)
            ++early_priority_mismatches;

        Map rollback{};
        add(rollback, 0, 0, 0, 1); add(rollback, 1, 0, 0, 1);
        run(dut, rollback);
        if (dut.group_repairable_o || dut.failure_position_o != 2U || dut.selected_valid_o != 3U ||
            assignment(dut, 0) != 0U || assignment(dut, 2) != 1U) ++rollback_events;

        if (argc < 2) throw std::runtime_error("shared C++ oracle corpus path is required");
        std::ifstream corpus(argv[1]); unsigned cases = 0, seed = 0;
        if (!(corpus >> cases >> seed) || cases != 1000) throw std::runtime_error("invalid shared oracle corpus header");
        std::size_t final_owner_mismatches = 0, failure_mismatches = 0;
        std::array<std::size_t, 9> class_mismatches{};
        for (unsigned vector = 0; vector < cases; ++vector) {
            unsigned expected_success, expected_failure, expected_borrows;
            std::array<unsigned,4> attempts{}, patterns{}, rows{}, cols{}, donors{};
            std::array<unsigned,8> owners{}; Map random{};
            corpus >> expected_success >> expected_failure >> expected_borrows;
            for (int sa=0;sa<4;++sa) corpus >> attempts[sa] >> patterns[sa] >> rows[sa] >> cols[sa];
            for (unsigned &donor:donors) corpus >> donor;
            for (unsigned &owner:owners) corpus >> owner;
            for (int candidate=0;candidate<180;++candidate) { unsigned valid; corpus >> valid >> random[candidate].rows >> random[candidate].cols; random[candidate].valid=valid!=0; }
            if (!corpus) throw std::runtime_error("truncated shared oracle corpus");
            run(dut, random);
            bool mismatch = false;
            if (unsigned(dut.group_repairable_o) != expected_success) { ++class_mismatches[2]; mismatch=true; }
            if (unsigned(dut.failure_position_o) != expected_failure) { ++failure_mismatches; ++class_mismatches[7]; mismatch=true; }
            for (int sa=0;sa<4;++sa) {
                if (patterns[sa] == 0) continue;
                if (((dut.selected_attempt_flat_o>>(sa*2))&3U)!=attempts[sa] || ((dut.selected_pattern_flat_o>>(sa*4))&15U)!=patterns[sa]) { ++class_mismatches[2]; mismatch=true; }
                if (((dut.selected_used_rows_flat_o>>(sa*3))&7U)!=rows[sa] || ((dut.selected_used_cols_flat_o>>(sa*2))&3U)!=cols[sa]) { ++class_mismatches[1]; mismatch=true; }
                if (((dut.selected_donor_flat_o>>(sa*4))&15U)!=donors[sa]) { ++class_mismatches[4]; mismatch=true; }
            }
            unsigned actual_borrows=0;
            for (int line=0;line<8;++line) { const unsigned got=assignment(dut,line); if(got!=owners[line]) { ++final_owner_mismatches; ++class_mismatches[5]; mismatch=true; } if(got!=4U && got!=unsigned(line/2)) ++actual_borrows; }
            if(actual_borrows!=expected_borrows) { ++class_mismatches[5]; mismatch=true; }
            if(mismatch) {
                if (oracle_mismatches == 0) {
                    std::cout << "FIRST_ORACLE_MISMATCH_CASE=" << vector << " expected_success=" << expected_success
                              << " observed_success=" << unsigned(dut.group_repairable_o) << " expected_failure=" << expected_failure
                              << " observed_failure=" << unsigned(dut.failure_position_o) << '\n';
                    for (int sa=0;sa<4;++sa) std::cout << "  SA" << sa << " expected=" << attempts[sa] << ',' << patterns[sa] << ',' << rows[sa] << ',' << cols[sa] << ',' << donors[sa]
                        << " observed=" << ((dut.selected_attempt_flat_o>>(sa*2))&3U) << ',' << ((dut.selected_pattern_flat_o>>(sa*4))&15U) << ','
                        << ((dut.selected_used_rows_flat_o>>(sa*3))&7U) << ',' << ((dut.selected_used_cols_flat_o>>(sa*2))&3U) << ',' << ((dut.selected_donor_flat_o>>(sa*4))&15U) << '\n';
                }
                ++oracle_mismatches;
            }
        }
        std::cout << "CPP_RTL_SHARED_CORPUS=YES\nCPP_ORACLE_RANDOM_CASES=" << cases << "\nCPP_ORACLE_SEED=" << seed << '\n'
                  << "FINAL_OWNER_MISMATCHES=" << final_owner_mismatches << '\n'
                  << "FAILURE_POSITION_MISMATCHES=" << failure_mismatches << '\n'
                  << "ORACLE_CLASS_A_CANDIDATE_GENERATION=" << class_mismatches[0] << '\n'
                  << "ORACLE_CLASS_B_LOCAL_RECAM=" << class_mismatches[1] << '\n'
                  << "ORACLE_CLASS_C_PRIORITY=" << class_mismatches[2] << '\n'
                  << "ORACLE_CLASS_D_ADJACENCY=" << class_mismatches[3] << '\n'
                  << "ORACLE_CLASS_E_DONOR_PRIORITY=" << class_mismatches[4] << '\n'
                  << "ORACLE_CLASS_F_OWNERSHIP=" << class_mismatches[5] << '\n'
                  << "ORACLE_CLASS_G_COMMIT=" << class_mismatches[6] << '\n'
                  << "ORACLE_CLASS_H_FAILURE=" << class_mismatches[7] << '\n'
                  << "ORACLE_CLASS_I_ENCODING=" << class_mismatches[8] << '\n';
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
    std::cout << "ADJACENCY_MISMATCHES=" << adjacency_mismatches << '\n'
              << "MIDDLE_DONOR_PRIORITY_MISMATCHES=" << priority_mismatches << '\n'
              << "BORROWED_LINE_FORWARDING_VIOLATIONS=" << forwarding_violations << '\n'
              << "EARLY_PRIORITY_MISMATCHES=" << early_priority_mismatches << '\n'
              << "ROLLBACK_EVENTS=" << rollback_events << '\n'
              << "CPP_ORACLE_MISMATCHES=" << oracle_mismatches << '\n';
    return adjacency_mismatches || priority_mismatches || forwarding_violations || early_priority_mismatches || rollback_events || oracle_mismatches;
}
