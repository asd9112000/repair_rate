`default_nettype none

// Seven fixed ConfigID analyzer lanes for the active GROUP subarray.  The
// producer selects a role-legal lane only after every lane has evaluated.
module recam_n2_preopt_parallel_config_bank #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer PHYS_COL_ADDR_W = 13,
    parameter integer WORD_COL_ADDR_W = 5,
    parameter integer HYBRID_LINE_ADDR_W = 13,
    parameter integer HYBRID_ENTRIES = 7
) (
    input  wire [4:0] pivot_valid_i,
    input  wire [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input  wire [5*PHYS_COL_ADDR_W-1:0] pivot_cols_flat_i,
    input  wire [4:0] row_gt1_i, input wire [4:0] row_gt2_i,
    input  wire [4:0] row_gt3_i, input wire [4:0] col_gt1_i,
    input  wire [4:0] col_gt2_i, input wire [4:0] col_gt3_i,
    input  wire [HYBRID_ENTRIES-1:0] hybrid_valid_i,
    input  wire [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input  wire [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,
    input  wire [HYBRID_ENTRIES*HYBRID_LINE_ADDR_W-1:0] hybrid_differing_flat_i,
    input  wire conventional_overflow_i,
    output wire [7*10-1:0] candidate_valid_flat_o,
    output wire [7*4-1:0] first_pattern_id_flat_o,
    output wire [6:0] solution_valid_o
);
    genvar lane;
    generate
        for (lane = 0; lane < 7; lane = lane + 1) begin : generate_config_lane
            wire unused_repairable;
            wire unused_dictionary_overflow;
            recam_shared_config_analyzer #(
                .ROW_ADDR_W(ROW_ADDR_W),
                .PHYS_COL_ADDR_W(PHYS_COL_ADDR_W),
                .WORD_COL_ADDR_W(WORD_COL_ADDR_W),
                .HYBRID_LINE_ADDR_W(HYBRID_LINE_ADDR_W),
                .HYBRID_ENTRIES(HYBRID_ENTRIES)
            ) analyzer (
                .config_id_i(lane[2:0]), .pivot_valid_i(pivot_valid_i),
                .pivot_rows_flat_i(pivot_rows_flat_i), .pivot_cols_flat_i(pivot_cols_flat_i),
                .row_gt1_i(row_gt1_i), .row_gt2_i(row_gt2_i), .row_gt3_i(row_gt3_i),
                .col_gt1_i(col_gt1_i), .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i),
                .hybrid_valid_i(hybrid_valid_i), .hybrid_pointer_flat_i(hybrid_pointer_flat_i),
                .hybrid_descriptor_i(hybrid_descriptor_i), .hybrid_differing_flat_i(hybrid_differing_flat_i),
                .conventional_overflow_i(conventional_overflow_i),
                .candidate_valid_o(candidate_valid_flat_o[lane*10 +: 10]),
                .pattern_id_o(first_pattern_id_flat_o[lane*4 +: 4]),
                .solution_valid_o(solution_valid_o[lane]), .repairable_o(unused_repairable),
                .dictionary_overflow_o(unused_dictionary_overflow)
            );
        end
    endgenerate
endmodule

`default_nettype wire
