#include "Vtb_recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_equivalence.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using Dut = Vtb_recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_equivalence;
constexpr int kCandidates = 180;
using CandidateMap = std::array<std::array<unsigned, 2>, kCandidates>;

struct Output {
    unsigned repairable;
    unsigned valid;
    unsigned attempt;
    unsigned pattern;
    unsigned rows;
    unsigned columns;
    unsigned donor;
    unsigned assignment;
    unsigned borrows;
    unsigned usedRows;
};

int index(int subarray, int attempt, int pattern) {
    return subarray * 45 + attempt * 15 + pattern;
}

void tick(Dut &dut) {
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
    dut.clk_i = 0;
    dut.eval();
}

void clear(Dut &dut) {
    for (int word = 0; word < 6; ++word)
        dut.candidate_valid_i[word] = 0;
    for (int word = 0; word < 17; ++word)
        dut.candidate_used_rows_flat_i[word] = 0;
    for (int word = 0; word < 12; ++word)
        dut.candidate_used_cols_flat_i[word] = 0;
}

void setBits(WData *target, int offset, unsigned value, int width) {
    target[offset / 32] |= value << (offset % 32);
    if (offset % 32 + width > 32)
        target[offset / 32 + 1] |= value >> (32 - offset % 32);
}

void drive(Dut &dut, const CandidateMap &map) {
    clear(dut);
    for (int candidate = 0; candidate < kCandidates; ++candidate) {
        if (map[candidate][0])
            dut.candidate_valid_i[candidate / 32] |= 1U << (candidate % 32);
        setBits(dut.candidate_used_rows_flat_i, candidate * 3, map[candidate][1] >> 2, 3);
        setBits(dut.candidate_used_cols_flat_i, candidate * 2, map[candidate][1] & 3U, 2);
    }
}

void add(CandidateMap &map, int subarray, int attempt, int pattern,
         unsigned rows, unsigned columns) {
    map[index(subarray, attempt, pattern)] = {{1, (rows << 2) | columns}};
}

Output readOpt0(const Dut &dut) {
    return {unsigned(dut.opt0_repairable_o), unsigned(dut.opt0_selected_valid_o),
            unsigned(dut.opt0_selected_attempt_flat_o), unsigned(dut.opt0_selected_pattern_flat_o),
            unsigned(dut.opt0_selected_used_rows_flat_o), unsigned(dut.opt0_selected_used_cols_flat_o),
            unsigned(dut.opt0_selected_donor_flat_o), unsigned(dut.opt0_selected_row_assignment_flat_o),
            unsigned(dut.opt0_selected_borrow_count_o), unsigned(dut.opt0_selected_used_rows_total_o)};
}

Output readOpt1(const Dut &dut) {
    return {unsigned(dut.opt1_repairable_o), unsigned(dut.opt1_selected_valid_o),
            unsigned(dut.opt1_selected_attempt_flat_o), unsigned(dut.opt1_selected_pattern_flat_o),
            unsigned(dut.opt1_selected_used_rows_flat_o), unsigned(dut.opt1_selected_used_cols_flat_o),
            unsigned(dut.opt1_selected_donor_flat_o), unsigned(dut.opt1_selected_row_assignment_flat_o),
            unsigned(dut.opt1_selected_borrow_count_o), unsigned(dut.opt1_selected_used_rows_total_o)};
}

bool equal(const Output &left, const Output &right) {
    return left.repairable == right.repairable && left.valid == right.valid &&
        left.attempt == right.attempt && left.pattern == right.pattern &&
        left.rows == right.rows && left.columns == right.columns &&
        left.donor == right.donor && left.assignment == right.assignment &&
        left.borrows == right.borrows && left.usedRows == right.usedRows;
}

bool run(Dut &dut, const CandidateMap &map) {
    dut.rst_ni = 0;
    dut.start_i = 0;
    drive(dut, map);
    tick(dut);
    dut.rst_ni = 1;
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
    bool opt0Done = false;
    bool opt1Done = false;
    Output opt0{};
    Output opt1{};
    for (int cycle = 0; cycle < 200000; ++cycle) {
        drive(dut, map);
        tick(dut);
        if (dut.opt0_done_o) {
            opt0 = readOpt0(dut);
            opt0Done = true;
        }
        if (dut.opt1_done_o) {
            opt1 = readOpt1(dut);
            opt1Done = true;
        }
        if (opt0Done && opt1Done)
            return equal(opt0, opt1);
    }
    throw std::runtime_error("OPT0/OPT1 equivalence search timed out");
}

CandidateMap dToA() {
    CandidateMap map{};
    add(map, 0, 0, 0, 1, 0);
    add(map, 1, 0, 0, 1, 0);
    add(map, 2, 0, 0, 2, 0);
    add(map, 3, 1, 0, 3, 0);
    return map;
}

CandidateMap noForward() {
    CandidateMap map{};
    add(map, 0, 0, 0, 1, 0);
    add(map, 1, 1, 0, 3, 0);
    add(map, 2, 1, 0, 3, 0);
    add(map, 3, 0, 0, 1, 0);
    return map;
}

CandidateMap equalDemandTies() {
    CandidateMap map{};
    add(map, 0, 0, 0, 0, 0);
    add(map, 0, 0, 1, 0, 0);
    add(map, 1, 0, 0, 0, 0);
    add(map, 1, 1, 0, 0, 0);
    add(map, 2, 0, 0, 0, 0);
    add(map, 3, 0, 0, 0, 0);
    return map;
}
} // namespace

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    if (argc < 2) {
        std::cerr << "shared C++ GLOBAL oracle corpus path is required\n";
        return 1;
    }
    Dut dut;
    std::size_t directedMismatches = 0;
    std::size_t corpusMismatches = 0;
    try {
        directedMismatches += !run(dut, dToA());
        directedMismatches += !run(dut, noForward());
        directedMismatches += !run(dut, equalDemandTies());

        std::ifstream corpus(argv[1]);
        unsigned cases = 0;
        unsigned seed = 0;
        if (!(corpus >> cases >> seed) || cases != 1000)
            throw std::runtime_error("invalid shared GLOBAL oracle corpus header");
        for (unsigned vector = 0; vector < cases; ++vector) {
            unsigned ignored = 0;
            CandidateMap map{};
            corpus >> ignored >> ignored >> ignored;
            for (int subarray = 0; subarray < 4; ++subarray)
                for (int field = 0; field < 4; ++field)
                    corpus >> ignored;
            for (int donor = 0; donor < 4; ++donor)
                corpus >> ignored;
            for (int owner = 0; owner < 8; ++owner)
                corpus >> ignored;
            for (int candidate = 0; candidate < kCandidates; ++candidate) {
                unsigned valid = 0;
                unsigned rows = 0;
                unsigned columns = 0;
                corpus >> valid >> rows >> columns;
                map[candidate] = {{valid, (rows << 2) | columns}};
            }
            if (!corpus)
                throw std::runtime_error("truncated shared GLOBAL oracle corpus");
            corpusMismatches += !run(dut, map);
        }
        std::cout << "OPT0_OPT1_DIRECTED_CASES=3\n"
                  << "OPT0_OPT1_DIRECTED_MISMATCHES=" << directedMismatches << '\n'
                  << "OPT0_OPT1_CPP_ORACLE_CASES=" << cases << '\n'
                  << "OPT0_OPT1_CPP_ORACLE_SEED=" << seed << '\n'
                  << "OPT0_OPT1_CPP_ORACLE_MISMATCHES=" << corpusMismatches << '\n';
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return directedMismatches || corpusMismatches;
}
