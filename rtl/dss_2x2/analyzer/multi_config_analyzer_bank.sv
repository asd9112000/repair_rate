`default_nettype none
module multi_config_analyzer_bank #(
    parameter integer MAX_K=5,NUM_CFG=7,HYBRID_ENTRY_NUM=14,
    parameter integer ROW_W=10,COL_W=10,PATTERN_ID_W=4
) (
    input wire [MAX_K*NUM_CFG-1:0] cfg_pivot_valid_i,
    input wire [MAX_K*ROW_W-1:0] pivot_rows_flat_i,
    input wire [MAX_K*COL_W-1:0] pivot_cols_flat_i,
    input wire [NUM_CFG*MAX_K-1:0] row_must_by_cfg_i,
    input wire [NUM_CFG*MAX_K-1:0] col_must_by_cfg_i,
    input wire [HYBRID_ENTRY_NUM-1:0] hybrid_valid_i,
    input wire [HYBRID_ENTRY_NUM*ROW_W-1:0] hybrid_rows_flat_i,
    input wire [HYBRID_ENTRY_NUM*COL_W-1:0] hybrid_cols_flat_i,
    input wire [HYBRID_ENTRY_NUM*$clog2(MAX_K)-1:0] hybrid_ptrs_flat_i,
    input wire [HYBRID_ENTRY_NUM-1:0] hybrid_descriptors_i,
    input wire [HYBRID_ENTRY_NUM*NUM_CFG-1:0] hybrid_cfg_valid_flat_i,
    input wire shared_storage_overflow_i,
    output wire [NUM_CFG*PATTERN_ID_W-1:0] config_patterns_o,
    output wire [NUM_CFG-1:0] config_valid_o
);
    function automatic [2:0] cfg_rows(input integer id);
        case(id) 0,1,5:cfg_rows=2;2,3:cfg_rows=3;4,6:cfg_rows=1;default:cfg_rows=0;endcase
    endfunction
    function automatic [2:0] cfg_cols(input integer id);
        case(id) 0,2,4:cfg_cols=2;1,3:cfg_cols=1;5,6:cfg_cols=3;default:cfg_cols=0;endcase
    endfunction
    genvar cfg,entry;
    generate for(cfg=0;cfg<NUM_CFG;cfg=cfg+1) begin:g_cfg
        wire [MAX_K-1:0] pivots;
        wire [HYBRID_ENTRY_NUM-1:0] hybrids;
        wire [2:0] hybrid_ptr [0:HYBRID_ENTRY_NUM-1];
        wire [9:0] unused_candidates; wire [MAX_K*MAX_K-1:0] unused_matrix;
        for(entry=0;entry<MAX_K;entry=entry+1)
            assign pivots[entry]=cfg_pivot_valid_i[entry*NUM_CFG+cfg];
        for(entry=0;entry<HYBRID_ENTRY_NUM;entry=entry+1) begin:g_hybrid_view
            assign hybrid_ptr[entry] = hybrid_ptrs_flat_i[entry*$clog2(MAX_K) +: $clog2(MAX_K)];
            // A row-related record is retired when its pivot row becomes
            // RowMust; a column-related record is retired on ColumnMust.
            // The payload remains physically shared and only this logical
            // ConfigID view is filtered.
            assign hybrids[entry] = hybrid_valid_i[entry] &&
                hybrid_cfg_valid_flat_i[entry*NUM_CFG+cfg] &&
                (hybrid_ptr[entry] < MAX_K) &&
                !(hybrid_descriptors_i[entry]
                    ? col_must_by_cfg_i[cfg*MAX_K+hybrid_ptr[entry]]
                    : row_must_by_cfg_i[cfg*MAX_K+hybrid_ptr[entry]]);
        end
        config_analyzer #(.MAX_K(MAX_K),.HYBRID_ENTRY_NUM(HYBRID_ENTRY_NUM),
            .ROW_W(ROW_W),.COL_W(COL_W),.PATTERN_ID_W(PATTERN_ID_W),.MAX_PATTERNS(10)) analyzer(
            .active_rows_i(cfg_rows(cfg)),.active_cols_i(cfg_cols(cfg)),.pivot_valid_i(pivots),
            .pivot_rows_flat_i(pivot_rows_flat_i),.pivot_cols_flat_i(pivot_cols_flat_i),
            .row_must_i(row_must_by_cfg_i[cfg*MAX_K +: MAX_K]),
            .col_must_i(col_must_by_cfg_i[cfg*MAX_K +: MAX_K]),.hybrid_valid_i(hybrids),
            .hybrid_rows_flat_i(hybrid_rows_flat_i),.hybrid_cols_flat_i(hybrid_cols_flat_i),
            .shared_storage_overflow_i(shared_storage_overflow_i),.repairable_o(config_valid_o[cfg]),
            .candidate_valid_o(unused_candidates),.lowest_pattern_id_o(config_patterns_o[cfg*PATTERN_ID_W +: PATTERN_ID_W]),
            .matrix_flat_o(unused_matrix));
    end endgenerate
endmodule
`default_nettype wire
