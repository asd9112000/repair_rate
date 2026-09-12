`default_nettype none

// Complete Phase 3A analyzer: no clock, reset, or retained state.
module recam_2r2c_analyzer #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer HYBRID_ENTRY_NUM = 7
) (
    input  wire [3:0] pivot_valid_i,
    input  wire [4*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input  wire [4*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input  wire [3:0] row_must_i,
    input  wire [3:0] col_must_i,
    input  wire [HYBRID_ENTRY_NUM-1:0] hybrid_valid_i,
    input  wire [HYBRID_ENTRY_NUM*ROW_ADDR_W-1:0] hybrid_rows_flat_i,
    input  wire [HYBRID_ENTRY_NUM*COL_ADDR_W-1:0] hybrid_cols_flat_i,
    input  wire cam_overflow_i,
    output wire [15:0] matrix_flat_o,
    output wire [5:0] candidate_valid_o,
    output wire repairable_o,
    output wire [3:0] pattern_id_o
);
    recam_2r2c_matrix_builder #(
        .ROW_ADDR_W(ROW_ADDR_W), .COL_ADDR_W(COL_ADDR_W),
        .HYBRID_ENTRY_NUM(HYBRID_ENTRY_NUM)
    ) matrix_builder (
        .pivot_valid_i(pivot_valid_i), .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(pivot_cols_flat_i), .row_must_i(row_must_i),
        .col_must_i(col_must_i), .hybrid_valid_i(hybrid_valid_i),
        .hybrid_rows_flat_i(hybrid_rows_flat_i), .hybrid_cols_flat_i(hybrid_cols_flat_i),
        .matrix_flat_o(matrix_flat_o)
    );
    recam_2r2c_pattern_analyzer pattern_analyzer (
        .matrix_flat_i(matrix_flat_o), .cam_overflow_i(cam_overflow_i),
        .candidate_valid_o(candidate_valid_o)
    );
    recam_2r2c_pattern_encoder pattern_encoder (
        .candidate_valid_i(candidate_valid_o), .repairable_o(repairable_o),
        .pattern_id_o(pattern_id_o)
    );
endmodule

`default_nettype wire
