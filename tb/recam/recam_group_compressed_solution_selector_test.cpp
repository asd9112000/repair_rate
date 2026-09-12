#include "Vrecam_phase3e_comparison_top.h"
#include <cstdint>
#include <iostream>
#include <stdexcept>
static void req(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
static void clear(Vrecam_phase3e_comparison_top &dut) { for (auto &word : dut.config_pattern_map_i) word = 0; }
static void put(Vrecam_phase3e_comparison_top &dut, unsigned index, bool valid, bool solution, bool repairable, unsigned config, unsigned pattern, unsigned candidate) {
    unsigned bit = index * 20, word = bit / 32, offset = bit % 32;
    std::uint64_t value = (std::uint64_t(candidate) << 10) | (std::uint64_t(pattern) << 6) |
                          (std::uint64_t(repairable) << 5) | (std::uint64_t(solution) << 4) |
                          (std::uint64_t(config) << 1) | valid;
    dut.config_pattern_map_i[word] |= value << offset;
    if (offset > 12) dut.config_pattern_map_i[word + 1] |= value >> (32 - offset);
}
static void expect_group(Vrecam_phase3e_comparison_top &dut, unsigned index, unsigned config, unsigned pattern, unsigned candidate, unsigned score) {
    dut.eval();
    req(dut.group_solution_valid_o && dut.early_solution_valid_o == dut.group_solution_valid_o, "solution-valid invariant");
    req(dut.group_selected_map_index_o == index && dut.group_selected_config_id_o == config, "group selection");
    req(dut.group_selected_pattern_id_o == pattern && dut.group_selected_candidate_valid_o == candidate, "PatternID/bitmap alignment");
    req(dut.group_selected_repairable_o && dut.group_selected_score_o == score, "repairable/score alignment");
}
static void expect_difference(Vrecam_phase3e_comparison_top &dut, unsigned early_index, unsigned group_index) {
    dut.eval(); req(dut.early_solution_valid_o == dut.group_solution_valid_o, "solution-valid equivalence");
    req(dut.early_selected_map_index_o == early_index && dut.group_selected_map_index_o == group_index, "EARLY/GROUP policy difference");
}
int main() { try {
    Vrecam_phase3e_comparison_top dut;
    dut.sa_role_i = 0;
    clear(dut); put(dut,0,1,1,1,0,1,1); put(dut,1,1,1,1,4,2,2); expect_group(dut,1,4,2,2,0); expect_difference(dut,0,1); std::cout << "A1 PASS\n";
    clear(dut); put(dut,0,1,1,1,0,3,3); expect_group(dut,0,0,3,3,1); std::cout << "A2 PASS\n";
    clear(dut); put(dut,2,1,1,1,5,4,4); put(dut,3,1,1,1,6,5,5); expect_group(dut,3,6,5,5,2); expect_difference(dut,2,3); std::cout << "A3 PASS\n";
    clear(dut); put(dut,2,1,1,1,5,6,6); expect_group(dut,2,5,6,6,3); std::cout << "A4 PASS\n";
    clear(dut); put(dut,0,1,1,1,0,1,1); put(dut,1,1,1,1,4,2,2); put(dut,2,1,1,1,5,3,3); put(dut,3,1,1,1,6,4,4); expect_group(dut,1,4,2,2,0); std::cout << "A5 PASS\n";
    dut.sa_role_i = 1;
    clear(dut); put(dut,0,1,1,1,0,1,1); put(dut,2,1,1,1,1,2,0x155); expect_group(dut,2,1,2,0x155,0); expect_difference(dut,0,2); std::cout << "B1 PASS\n";
    clear(dut); put(dut,0,1,1,1,0,3,3); expect_group(dut,0,0,3,3,1); std::cout << "B2 PASS\n";
    clear(dut); put(dut,1,1,1,1,2,4,4); put(dut,3,1,1,1,3,5,5); expect_group(dut,3,3,5,5,2); expect_difference(dut,1,3); std::cout << "B3 PASS\n";
    clear(dut); put(dut,1,1,1,1,2,6,6); expect_group(dut,1,2,6,6,3); std::cout << "B4 PASS\n";
    clear(dut); put(dut,0,1,1,1,0,1,1); put(dut,1,1,1,1,2,2,2); put(dut,2,1,1,1,1,3,3); put(dut,3,1,1,1,3,4,4); expect_group(dut,2,1,3,3,0); std::cout << "B5 PASS\n";
    clear(dut); put(dut,0,0,1,1,0,1,1); put(dut,1,1,1,0,2,2,2); put(dut,2,1,0,1,1,3,3); dut.eval(); req(!dut.group_solution_valid_o && !dut.early_solution_valid_o && dut.group_selected_score_o == 0 && dut.group_selected_candidate_valid_o == 0, "invalid/default zero"); std::cout << "INVALID AND NO-SOLUTION PASS\n";
    std::cout << "PHASE3E_GROUP_FUNCTIONAL_REGRESSION PASS\n";
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; } }
