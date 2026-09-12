#include "Vdss_legacy_recam_candidate_adapter.h"
#include <cstdlib>
#include <iostream>

namespace {
constexpr unsigned SUMMARY_W = 26;
void require(bool v, const char* s) { if (!v) { std::cerr << "FAIL: " << s << '\n'; std::exit(1); } }
void put(std::uint32_t* words, unsigned bit, unsigned width, std::uint32_t value) {
    words[bit / 32] |= value << (bit % 32);
    if ((bit % 32) + width > 32) words[bit / 32 + 1] |= value >> (32 - (bit % 32));
}
unsigned get(const std::uint32_t* words, unsigned bit, unsigned width) {
    std::uint64_t v = words[bit / 32] >> (bit % 32);
    if ((bit % 32) + width > 32) v |= std::uint64_t(words[bit / 32 + 1]) << (32 - (bit % 32));
    return v & ((1u << width) - 1);
}
void entry(std::uint32_t* words, unsigned slot, unsigned cfg, unsigned pattern, unsigned bitmap,
           bool solution = true, bool repairable = true) {
    const unsigned base = slot * 20;
    put(words, base, 1, 1); put(words, base + 1, 3, cfg); put(words, base + 4, 1, solution);
    put(words, base + 5, 1, repairable); put(words, base + 6, 4, pattern); put(words, base + 10, 10, bitmap);
}
void check_summary(const Vdss_legacy_recam_candidate_adapter& dut, unsigned slot, bool valid,
                   unsigned rows, unsigned cols, unsigned pattern, unsigned bitmap, unsigned index, unsigned cfg) {
    const unsigned base = slot * SUMMARY_W;
    require(get(dut.candidate_summary_flat_o, base + 25, 1) == valid, "candidate validity");
    require(get(dut.candidate_summary_flat_o, base + 22, 3) == rows, "descriptor rows");
    require(get(dut.candidate_summary_flat_o, base + 19, 3) == cols, "descriptor cols");
    require(get(dut.candidate_summary_flat_o, base + 15, 4) == pattern, "pattern id");
    require(get(dut.candidate_summary_flat_o, base + 5, 10) == bitmap, "pattern bitmap");
    require(get(dut.candidate_summary_flat_o, base + 3, 2) == index, "canonical index");
    require(get(dut.candidate_summary_flat_o, base, 3) == cfg, "legacy debug config");
}
}
int main() {
    Vdss_legacy_recam_candidate_adapter dut;
    for (auto& word : dut.legacy_config_pattern_map_i) word = 0;
    for (auto& word : dut.legacy_reconstruction_metadata_i) word = 0;
    dut.sa_valid_i = 1;
    // Actual A/D scheduler order is already canonical: 0,4,5,6.
    entry(dut.legacy_config_pattern_map_i, 0, 0, 1, 0x003);
    entry(dut.legacy_config_pattern_map_i, 1, 4, 2, 0x007);
    entry(dut.legacy_config_pattern_map_i, 2, 5, 3, 0x00f);
    entry(dut.legacy_config_pattern_map_i, 3, 6, 4, 0x03f);
    dut.sa_id_i = 0; dut.eval();
    check_summary(dut, 0, true, 2, 2, 1, 0x003, 0, 0);
    check_summary(dut, 1, true, 1, 2, 2, 0x007, 1, 4);
    check_summary(dut, 2, true, 2, 3, 3, 0x00f, 2, 5);
    check_summary(dut, 3, true, 1, 3, 4, 0x03f, 3, 6);

    // Actual B/C scheduler order is 0,2,1,3; V2 normalizes it to 0,1,2,3.
    for (auto& word : dut.legacy_config_pattern_map_i) word = 0;
    entry(dut.legacy_config_pattern_map_i, 0, 0, 1, 0x003);
    entry(dut.legacy_config_pattern_map_i, 1, 2, 2, 0x3ff);
    entry(dut.legacy_config_pattern_map_i, 2, 1, 3, 0x007);
    entry(dut.legacy_config_pattern_map_i, 3, 3, 4, 0x00f);
    dut.sa_id_i = 1; dut.eval();
    check_summary(dut, 0, true, 2, 2, 1, 0x003, 0, 0);
    check_summary(dut, 1, true, 2, 1, 3, 0x007, 1, 1);
    check_summary(dut, 2, true, 3, 2, 2, 0x3ff, 2, 2);
    check_summary(dut, 3, true, 3, 1, 4, 0x00f, 3, 3);
    // A locally invalid candidate and an invalid ConfigID are both explicit;
    // neither is encoded by a magic PatternID.
    for (auto& word : dut.legacy_config_pattern_map_i) word = 0;
    entry(dut.legacy_config_pattern_map_i, 0, 0, 9, 0x001, false, true);
    dut.sa_id_i = 0; dut.eval();
    check_summary(dut, 0, false, 2, 2, 9, 0x001, 0, 0);
    for (auto& word : dut.legacy_config_pattern_map_i) word = 0;
    entry(dut.legacy_config_pattern_map_i, 0, 7, 9, 0x001);
    dut.sa_id_i = 0; dut.eval();
    check_summary(dut, 0, false, 0, 0, 9, 0x001, 0, 7);
    std::cout << "DSS_V2_PHASE4D_CANDIDATE_ADAPTER PASS\n";
}
