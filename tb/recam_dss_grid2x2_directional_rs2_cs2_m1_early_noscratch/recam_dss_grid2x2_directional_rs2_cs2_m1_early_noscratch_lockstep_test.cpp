#include "Vrecam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core.h"

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
struct Vector
{
    bool success = false;
    int failure = -1;
    std::array<int, 4> slots{};
    std::array<int, 4> configs{};
    std::array<int, 4> patterns{};
    std::array<std::array<bool, 4>, 4> valid{};
    std::array<std::array<int, 4>, 4> candidatePattern{};
};

void require(bool condition, const std::string &message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

std::array<int, 4> decodeQuad(const std::string &text)
{
    std::array<int, 4> values{};
    std::stringstream parser(text);
    char separator = 0;
    for (int &value : values) {
        parser >> separator >> value;
        require(separator == ':', "invalid four-field CSV encoding");
    }
    return values;
}

void tick(Vrecam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

void reset(Vrecam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core &dut)
{
    dut.clk_i = 0;
    dut.rst_ni = 0;
    dut.start_i = 0;
    dut.candidate_valid_i = 0;
    dut.candidate_pattern_id_i = 0;
    tick(dut);
    dut.rst_ni = 1;
}

void check(const Vector &vector, int id)
{
    Vrecam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core dut;
    reset(dut);
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
    for (int cycle = 0; cycle != 20; ++cycle) {
        const unsigned sa = dut.current_sa_o;
        const unsigned slot = dut.current_slot_o;
        require(sa < 4 && slot < 4, "RTL candidate coordinate out of range");
        dut.candidate_valid_i = vector.valid[sa][slot];
        dut.candidate_pattern_id_i = vector.candidatePattern[sa][slot];
        tick(dut);
        if (dut.done_o)
            break;
    }
    require(dut.done_o, "RTL did not terminate for vector " + std::to_string(id));
    require(static_cast<bool>(dut.group_repairable_o) == vector.success,
            "repairability mismatch at vector " + std::to_string(id));
    const int expectedFailure = vector.success ? 0 : vector.failure;
    require(static_cast<int>(dut.failure_position_o) == expectedFailure,
            "failure position mismatch at vector " + std::to_string(id));
    for (int sa = 0; sa != 4; ++sa) {
        const bool expectedCommit = vector.slots[sa] >= 0;
        require(static_cast<bool>((dut.sa_commit_valid_o >> sa) & 1) == expectedCommit,
                "commit bitmap mismatch at vector " + std::to_string(id));
        if (!expectedCommit)
            continue;
        const int config = (dut.selected_config_flat_o >> (sa * 3)) & 0x7;
        const int pattern = (dut.selected_pattern_flat_o >> (sa * 4)) & 0xf;
        require(config == vector.configs[sa],
                "ConfigID mismatch at vector " + std::to_string(id));
        require(pattern == vector.patterns[sa],
                "PatternID mismatch at vector " + std::to_string(id));
    }
}
} // namespace

int main()
{
    std::ifstream input("tmp/date2026/final_early_c3/model_b2_lockstep.csv");
    require(input.good(), "missing generated Model-B2 C3 corpus");
    std::string line;
    std::getline(input, line);
    int vectors = 0;
    while (std::getline(input, line)) {
        std::stringstream parser(line);
        std::string field;
        std::getline(parser, field, ',');
        const int id = std::stoi(field);
        Vector vector;
        std::getline(parser, field, ','); vector.success = std::stoi(field) != 0;
        std::getline(parser, field, ','); vector.failure = std::stoi(field);
        std::getline(parser, field, ','); vector.slots = decodeQuad(field);
        std::getline(parser, field, ','); vector.configs = decodeQuad(field);
        std::getline(parser, field, ','); vector.patterns = decodeQuad(field);
        for (int sa = 0; sa != 4; ++sa)
            for (int slot = 0; slot != 4; ++slot) {
                std::getline(parser, field, ','); vector.valid[sa][slot] = std::stoi(field) != 0;
                std::getline(parser, field, ','); vector.candidatePattern[sa][slot] = std::stoi(field);
            }
        check(vector, id);
        ++vectors;
    }
    require(vectors == 1000, "C3 lockstep did not consume 1000 vectors");
    std::cout << "FINAL_EARLY_C3_RTL_MODEL_B2_LOCKSTEP=PASS VECTORS="
              << vectors << " MISMATCHES=0\n";
    return 0;
}
