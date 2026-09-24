`default_nettype none

// Adapter for proving that the width migration preserves all low-column
// decisions while exposing the historical five-bit alias as a negative control.
module recam_n2_2r2c_comparison_top (
    input  wire [3:0] pivot_valid_i,
    input  wire [35:0] pivot_rows_flat_i,
    input  wire [51:0] pivot_cols_flat_i,
    input  wire [3:0] row_must_i,
    input  wire [3:0] col_must_i,
    input  wire [6:0] hybrid_valid_i,
    input  wire [62:0] hybrid_rows_flat_i,
    input  wire [90:0] hybrid_cols_flat_i,
    input  wire cam_overflow_i,
    output wire [15:0] corrected_matrix_flat_o,
    output wire [5:0] corrected_candidate_valid_o,
    output wire corrected_repairable_o,
    output wire [3:0] corrected_pattern_id_o,
    output wire [15:0] historical_matrix_flat_o,
    output wire [5:0] historical_candidate_valid_o,
    output wire historical_repairable_o,
    output wire [3:0] historical_pattern_id_o
);
    wire [19:0] historical_pivot_cols_flat;
    wire [34:0] historical_hybrid_cols_flat;
    genvar entry;

    generate
        for (entry = 0; entry < 4; entry = entry + 1) begin : gen_pivot_low_word_column
            assign historical_pivot_cols_flat[entry*5 +: 5] = pivot_cols_flat_i[entry*13 +: 5];
        end
        for (entry = 0; entry < 7; entry = entry + 1) begin : gen_hybrid_low_word_column
            assign historical_hybrid_cols_flat[entry*5 +: 5] = hybrid_cols_flat_i[entry*13 +: 5];
        end
    endgenerate

    recam_n2_2r2c_analyzer corrected (
        .pivot_valid_i(pivot_valid_i), .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(pivot_cols_flat_i), .row_must_i(row_must_i),
        .col_must_i(col_must_i), .hybrid_valid_i(hybrid_valid_i),
        .hybrid_rows_flat_i(hybrid_rows_flat_i), .hybrid_cols_flat_i(hybrid_cols_flat_i),
        .cam_overflow_i(cam_overflow_i), .matrix_flat_o(corrected_matrix_flat_o),
        .candidate_valid_o(corrected_candidate_valid_o), .repairable_o(corrected_repairable_o),
        .pattern_id_o(corrected_pattern_id_o)
    );

    recam_2r2c_analyzer historical (
        .pivot_valid_i(pivot_valid_i), .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(historical_pivot_cols_flat), .row_must_i(row_must_i),
        .col_must_i(col_must_i), .hybrid_valid_i(hybrid_valid_i),
        .hybrid_rows_flat_i(hybrid_rows_flat_i), .hybrid_cols_flat_i(historical_hybrid_cols_flat),
        .cam_overflow_i(cam_overflow_i), .matrix_flat_o(historical_matrix_flat_o),
        .candidate_valid_o(historical_candidate_valid_o), .repairable_o(historical_repairable_o),
        .pattern_id_o(historical_pattern_id_o)
    );
endmodule

`default_nettype wire
