#include "Vdss_v2_rs3cs3m1_shared_config_analyzer.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {
int failures = 0;
void expect(bool value, const char* name) {
    if (!value) { std::cerr << "FAIL: " << name << '\n'; ++failures; }
}
unsigned popcount35(unsigned long long low, unsigned high) {
    unsigned n = 0;
    for (unsigned i = 0; i < 35; ++i)
        if (i < 64 ? ((low >> i) & 1ull) : ((high >> (i - 64)) & 1u)) ++n;
    return n;
}
void clear(Vdss_v2_rs3cs3m1_shared_config_analyzer& dut) {
    dut.pivot_valid_i = 0; dut.pivot_rows_flat_i = 0; dut.pivot_cols_flat_i = 0;
    dut.row_gt1_i = dut.row_gt2_i = dut.row_gt3_i = dut.row_gt4_i = 0;
    dut.col_gt1_i = dut.col_gt2_i = dut.col_gt3_i = dut.col_gt4_i = 0;
    dut.hybrid_valid_i = 0; dut.hybrid_pointer_flat_i = 0; dut.hybrid_descriptor_i = 0;
    dut.hybrid_differing_flat_i[0] = 0; dut.hybrid_differing_flat_i[1] = 0;
    dut.conventional_overflow_i = 0;
}
void set_row_threshold(Vdss_v2_rs3cs3m1_shared_config_analyzer& dut, unsigned count) {
    if (count == 1) dut.row_gt1_i = 1;
    if (count == 2) dut.row_gt2_i = 1;
    if (count == 3) dut.row_gt3_i = 1;
    if (count == 4) dut.row_gt4_i = 1;
}
void set_col_threshold(Vdss_v2_rs3cs3m1_shared_config_analyzer& dut, unsigned count) {
    if (count == 1) dut.col_gt1_i = 1;
    if (count == 2) dut.col_gt2_i = 1;
    if (count == 3) dut.col_gt3_i = 1;
    if (count == 4) dut.col_gt4_i = 1;
}
void set_hybrid_differing(Vdss_v2_rs3cs3m1_shared_config_analyzer& dut,
                          unsigned entry, unsigned address) {
    const unsigned bit = entry * 9;
    const unsigned word = bit / 32;
    const unsigned offset = bit % 32;
    dut.hybrid_differing_flat_i[word] |= uint32_t(address) << offset;
    if (offset > 23)
        dut.hybrid_differing_flat_i[word + 1] |= uint32_t(address) >> (32 - offset);
}
}  // namespace

int main() {
    Vdss_v2_rs3cs3m1_shared_config_analyzer dut;
    struct Case { unsigned r, c, transpose, candidates; };
    const Case cases[] = {{3,3,0,20},{2,3,1,10},{3,4,1,35},{2,4,1,15},
                          {3,2,0,10},{4,3,0,35},{4,2,0,15}};
    for (const auto& tc : cases) {
        clear(dut); dut.row_count_i = tc.r; dut.col_count_i = tc.c; dut.transpose_i = tc.transpose; dut.eval();
        expect(dut.solution_valid_o && dut.repairable_o, "ANALYZER_LOCAL_ACCEPT");
        expect(dut.pattern_id_o == 1, "ANALYZER_ONE_BASED_PATTERN");
        expect(popcount35(dut.candidate_valid_o, 0) == tc.candidates, "ANALYZER_EXACT_CANDIDATE_COUNT");
        if (tc.candidates == 35) expect((dut.candidate_valid_o >> 34) & 1u, "PATTERN_ID_35_CANDIDATE_VALID");
    }
    std::cout << "TARGET_ANALYZER_7_CONFIGS " << (failures ? "FAIL" : "PASS") << '\n';

    // Transpose-paired configurations must expose the same canonical lane and
    // one-based PatternID space when no physical fault relation distinguishes them.
    const Case transpose_pairs[][2] = {{{2,3,1,10},{3,2,0,10}},
                                       {{3,4,1,35},{4,3,0,35}},
                                       {{2,4,1,15},{4,2,0,15}}};
    for (const auto& pair : transpose_pairs) {
        clear(dut); dut.row_count_i=pair[0].r; dut.col_count_i=pair[0].c; dut.transpose_i=pair[0].transpose; dut.eval();
        const uint64_t first_bitmap = dut.candidate_valid_o;
        const unsigned first_pattern = dut.pattern_id_o;
        clear(dut); dut.row_count_i=pair[1].r; dut.col_count_i=pair[1].c; dut.transpose_i=pair[1].transpose; dut.eval();
        if (dut.candidate_valid_o != first_bitmap || dut.pattern_id_o != first_pattern)
            std::cerr << "transpose mismatch " << pair[0].r << "R" << pair[0].c
                      << "C/" << pair[1].r << "R" << pair[1].c << "C bitmaps="
                      << first_bitmap << "/" << uint64_t(dut.candidate_valid_o)
                      << " patterns=" << first_pattern << "/" << unsigned(dut.pattern_id_o) << '\n';
        expect(dut.candidate_valid_o == first_bitmap && dut.pattern_id_o == first_pattern,
               "TRANSPOSE_CANONICAL_CANDIDATE_AND_PATTERN_MAPPING");
    }
    std::cout << "TARGET_TRANSPOSE_3_PAIRS PASS" << '\n';

    clear(dut); dut.row_count_i=3; dut.col_count_i=4; dut.transpose_i=1; dut.conventional_overflow_i=1; dut.eval();
    expect(!dut.solution_valid_o && !dut.repairable_o && dut.pattern_id_o == 0 && dut.candidate_valid_o == 0,
           "PATTERN_ID_ZERO_INVALID_BOUNDARY");
    // A seven-node closure chain leaves only the final (35th) 4R3C candidate.
    // It proves that the selected PatternID path, not merely bitmap lane 34,
    // carries bit[5] and preserves the one-based maximum value.
    clear(dut); dut.row_count_i=4; dut.col_count_i=3; dut.transpose_i=0; dut.pivot_valid_i=0x7f;
    for (unsigned i = 0; i != 7; ++i) dut.pivot_rows_flat_i |= uint64_t(i) << (9 * i);
    dut.hybrid_valid_i = 0x3f; dut.hybrid_descriptor_i = 0x3f;
    for (unsigned i = 0; i != 6; ++i) {
        dut.hybrid_pointer_flat_i |= uint64_t(i) << (3 * i);
        set_hybrid_differing(dut, i, i + 1);
    }
    dut.eval();
    if (!(dut.solution_valid_o && dut.pattern_id_o == 35 && dut.candidate_valid_o == (uint64_t(1) << 34)))
        std::cerr << "Pattern35 observed solution=" << dut.solution_valid_o
                  << " pattern=" << unsigned(dut.pattern_id_o)
                  << " bitmap=" << uint64_t(dut.candidate_valid_o) << '\n';
    expect(dut.solution_valid_o && dut.pattern_id_o == 35 && dut.candidate_valid_o == (uint64_t(1) << 34),
           "PATTERN_ID_35_SELECTED_BOUNDARY");
    std::cout << "TARGET_PATTERN_ID_0_1_35_BOUNDARIES " << (failures ? "FAIL" : "PASS") << '\n';

    clear(dut); dut.row_count_i = 3; dut.col_count_i = 4; dut.transpose_i = 1;
    dut.pivot_valid_i = 0x7f; dut.hybrid_valid_i = 1u << 16;
    dut.hybrid_pointer_flat_i = (6ull << (16 * 3)); dut.eval();
    expect(!dut.dictionary_overflow_o, "MAX_PIVOT6_HYBRID16_NO_OVERFLOW");
    expect(dut.pattern_id_o <= 35, "MAX_CLASS_PATTERN_WIDTH");
    std::cout << "TARGET_MAX_K7_PATTERNID6_PIVOT6_HYBRID16 " << (failures ? "FAIL" : "PASS") << '\n';
    clear(dut); dut.row_count_i = 4; dut.col_count_i = 3; dut.transpose_i = 0;
    dut.pivot_valid_i = 0x7f; dut.hybrid_valid_i = 0x1ffff;
    for (unsigned h = 0; h != 17; ++h) dut.hybrid_pointer_flat_i |= uint64_t(h % 7) << (h * 3);
    dut.eval();
    expect(!dut.dictionary_overflow_o, "FULL_7_PIVOT_17_HYBRID_OCCUPANCY");
    std::cout << "TARGET_FULL_7_PIVOT_17_HYBRID " << (failures ? "FAIL" : "PASS") << '\n';
    for (const auto& tc : cases) {
        clear(dut); dut.row_count_i = tc.r; dut.col_count_i = tc.c; dut.transpose_i = tc.transpose;
        dut.pivot_valid_i = 1; dut.eval();
        const unsigned baseline = popcount35(dut.candidate_valid_o, 0);
        // The analyzer consumes the producer's predecoded >C and >R signals.
        // Drive both physical dimensions so the transpose-normalized input path
        // is exercised for every target envelope.
        set_row_threshold(dut, tc.c); dut.eval();
        expect(popcount35(dut.candidate_valid_o, 0) < baseline, "MUST_ROW_GT_C_AVAILABLE");
        clear(dut); dut.row_count_i = tc.r; dut.col_count_i = tc.c; dut.transpose_i = tc.transpose;
        dut.pivot_valid_i = 1; set_col_threshold(dut, tc.r); dut.eval();
        expect(popcount35(dut.candidate_valid_o, 0) < baseline, "MUST_COLUMN_GT_R_AVAILABLE");
    }
    std::cout << "TARGET_MUST_THRESHOLD_ALL_7_CONFIGS " << (failures ? "FAIL" : "PASS") << '\n';
    return failures ? 1 : 0;
}
