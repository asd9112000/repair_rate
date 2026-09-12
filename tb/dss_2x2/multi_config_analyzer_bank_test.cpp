#include "Vmulti_config_analyzer_bank.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>
static void require(bool v, const char *m) { if (!v) { std::cerr << m << '\n'; std::exit(1); } }

static void clear_inputs(Vmulti_config_analyzer_bank &d) {
    d.cfg_pivot_valid_i = 0;
    d.pivot_rows_flat_i = 0;
    d.pivot_cols_flat_i = 0;
    d.row_must_by_cfg_i = 0;
    d.col_must_by_cfg_i = 0;
    d.hybrid_valid_i = 0;
    d.hybrid_descriptors_i = 0;
    d.hybrid_ptrs_flat_i = 0;
    for (auto &word : d.hybrid_rows_flat_i) word=0;
    for (auto &word : d.hybrid_cols_flat_i) word=0;
    for (auto &word : d.hybrid_cfg_valid_flat_i) word=0;
    d.shared_storage_overflow_i=0;
}

static void activate_pivots(Vmulti_config_analyzer_bank &d, unsigned cfg,
                            unsigned count) {
    for (unsigned entry=0; entry<count; ++entry) {
        d.cfg_pivot_valid_i |= uint64_t{1} << (entry*7+cfg);
        d.pivot_rows_flat_i |= uint64_t{entry+1} << (entry*10);
        d.pivot_cols_flat_i |= uint64_t{entry+11} << (entry*10);
    }
}

static unsigned pattern(const Vmulti_config_analyzer_bank &d, unsigned cfg) {
    return (d.config_patterns_o >> (cfg*4)) & 0xfU;
}

static void require_transpose_pair(Vmulti_config_analyzer_bank &d,
                                   unsigned canonical_cfg,
                                   unsigned physical_cfg,
                                   unsigned k,
                                   uint64_t canonical_row_must,
                                   uint64_t canonical_col_must,
                                   unsigned canonical_pattern,
                                   unsigned physical_pattern) {
    clear_inputs(d);
    activate_pivots(d, canonical_cfg, k);
    d.row_must_by_cfg_i = canonical_row_must << (canonical_cfg*5);
    d.col_must_by_cfg_i = canonical_col_must << (canonical_cfg*5);
    d.eval();
    require((d.config_valid_o >> canonical_cfg) & 1U,
            "canonical member of transpose pair is invalid");
    require(pattern(d, canonical_cfg)==canonical_pattern,
            "canonical member retained the wrong lowest PatternID");

    clear_inputs(d);
    activate_pivots(d, physical_cfg, k);
    // Complete logical transpose: RowMust and ColumnMust exchange roles.
    d.row_must_by_cfg_i = canonical_col_must << (physical_cfg*5);
    d.col_must_by_cfg_i = canonical_row_must << (physical_cfg*5);
    d.eval();
    require((d.config_valid_o >> physical_cfg) & 1U,
            "transposed physical ConfigID is invalid");
    require(pattern(d, physical_cfg)==physical_pattern,
            "transposed physical ConfigID retained the wrong PatternID");
}

int main() {
    Vmulti_config_analyzer_bank d;
    // One physical pivot, active in every ConfigID view.
    d.cfg_pivot_valid_i = 0x7f;
    d.pivot_rows_flat_i = 7; d.pivot_cols_flat_i = 11;
    d.row_must_by_cfg_i = 0; d.col_must_by_cfg_i = 0;
    d.hybrid_valid_i = 0; d.hybrid_descriptors_i = 0;
    d.hybrid_ptrs_flat_i = 0;
    for (auto &word : d.hybrid_rows_flat_i) word=0;
    for (auto &word : d.hybrid_cols_flat_i) word=0;
    for (auto &word : d.hybrid_cfg_valid_flat_i) word=0;
    d.shared_storage_overflow_i=0; d.eval();
    require(d.config_valid_o==0x7f, "seven physical ConfigIDs were not exposed as valid");
    for (unsigned cfg=0; cfg<7; ++cfg)
        require(((d.config_patterns_o>>(cfg*4))&0xfU)!=0, "valid ConfigID has zero PatternID");
    d.shared_storage_overflow_i=1; d.eval();
    require(d.config_valid_o==0 && d.config_patterns_o==0, "shared overflow did not invalidate all views");

    // The complement orientation reverses candidate order.  These directed
    // Must masks force the first canonical pattern and therefore the last
    // physical pattern after a complete R/C transpose.
    require_transpose_pair(d, 1, 4, 3, 0x3, 0x4, 1, 3);  // 2R1C <-> 1R2C
    require_transpose_pair(d, 2, 5, 5, 0x7, 0x18, 1, 10); // 3R2C <-> 2R3C
    require_transpose_pair(d, 3, 6, 4, 0x7, 0x8, 1, 4);  // 3R1C <-> 1R3C

    std::cout << "multi_config_analyzer_bank_test PASS\n";
}
