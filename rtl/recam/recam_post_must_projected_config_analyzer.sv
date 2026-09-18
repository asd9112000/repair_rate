`default_nettype none

// Isolated Option-A analyzer boundary.  It converts a historical 14-entry
// physical Hybrid view into the exact post-Must view before the canonical
// shared analyzer consumes nine compact slots.
module recam_post_must_projected_config_analyzer #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer MAX_K = 5,
    parameter integer NUM_CONFIGS = 7,
    parameter integer PHYSICAL_HYBRID_ENTRIES = 14,
    parameter integer POST_MUST_VIEW_ENTRIES = 9
) (
    input  wire [2:0] config_id_i,
    input  wire [4:0] pivot_valid_i,
    input  wire [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input  wire [5*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input  wire [4:0] row_gt1_i,
    input  wire [4:0] row_gt2_i,
    input  wire [4:0] row_gt3_i,
    input  wire [4:0] col_gt1_i,
    input  wire [4:0] col_gt2_i,
    input  wire [4:0] col_gt3_i,
    input  wire [PHYSICAL_HYBRID_ENTRIES-1:0] physical_hybrid_valid_i,
    input  wire [PHYSICAL_HYBRID_ENTRIES*3-1:0] physical_hybrid_pointer_flat_i,
    input  wire [PHYSICAL_HYBRID_ENTRIES-1:0] physical_hybrid_descriptor_i,
    input  wire [PHYSICAL_HYBRID_ENTRIES*DIFF_ADDR_W-1:0] physical_hybrid_differing_flat_i,
    input  wire [PHYSICAL_HYBRID_ENTRIES*NUM_CONFIGS-1:0] physical_hybrid_cfg_valid_flat_i,
    input  wire [NUM_CONFIGS*MAX_K-1:0] row_must_by_cfg_i,
    input  wire [NUM_CONFIGS*MAX_K-1:0] col_must_by_cfg_i,
    input  wire conventional_overflow_i,
    output wire [9:0] candidate_valid_o,
    output wire [3:0] pattern_id_o,
    output wire       solution_valid_o,
    output wire       repairable_o,
    output wire       dictionary_overflow_o,
    output wire [POST_MUST_VIEW_ENTRIES-1:0] projected_hybrid_valid_o,
    output wire [POST_MUST_VIEW_ENTRIES*3-1:0] projected_hybrid_pointer_flat_o,
    output wire [POST_MUST_VIEW_ENTRIES-1:0] projected_hybrid_descriptor_o,
    output wire [POST_MUST_VIEW_ENTRIES*DIFF_ADDR_W-1:0] projected_hybrid_differing_flat_o,
    output wire [3:0] projected_count_o,
    output wire       projection_overflow_o
);
    recam_post_must_hybrid_projector #(
        .ROW_ADDR_W(ROW_ADDR_W),
        .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W),
        .MAX_K(MAX_K),
        .NUM_CONFIGS(NUM_CONFIGS),
        .PHYSICAL_HYBRID_ENTRIES(PHYSICAL_HYBRID_ENTRIES),
        .POST_MUST_VIEW_ENTRIES(POST_MUST_VIEW_ENTRIES)
    ) projector (
        .config_id_i(config_id_i),
        .physical_hybrid_valid_i(physical_hybrid_valid_i),
        .physical_hybrid_pointer_flat_i(physical_hybrid_pointer_flat_i),
        .physical_hybrid_descriptor_i(physical_hybrid_descriptor_i),
        .physical_hybrid_differing_flat_i(physical_hybrid_differing_flat_i),
        .physical_hybrid_cfg_valid_flat_i(physical_hybrid_cfg_valid_flat_i),
        .row_must_by_cfg_i(row_must_by_cfg_i),
        .col_must_by_cfg_i(col_must_by_cfg_i),
        .projected_hybrid_valid_o(projected_hybrid_valid_o),
        .projected_hybrid_pointer_flat_o(projected_hybrid_pointer_flat_o),
        .projected_hybrid_descriptor_o(projected_hybrid_descriptor_o),
        .projected_hybrid_differing_flat_o(projected_hybrid_differing_flat_o),
        .projected_count_o(projected_count_o),
        .projection_overflow_o(projection_overflow_o)
    );

    recam_shared_config_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W),
        .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W),
        .HYBRID_ENTRIES(POST_MUST_VIEW_ENTRIES)
    ) analyzer (
        .config_id_i(config_id_i),
        .pivot_valid_i(pivot_valid_i),
        .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(pivot_cols_flat_i),
        .row_gt1_i(row_gt1_i),
        .row_gt2_i(row_gt2_i),
        .row_gt3_i(row_gt3_i),
        .col_gt1_i(col_gt1_i),
        .col_gt2_i(col_gt2_i),
        .col_gt3_i(col_gt3_i),
        .hybrid_valid_i(projected_hybrid_valid_o),
        .hybrid_pointer_flat_i(projected_hybrid_pointer_flat_o),
        .hybrid_descriptor_i(projected_hybrid_descriptor_o),
        .hybrid_differing_flat_i(projected_hybrid_differing_flat_o),
        .conventional_overflow_i(conventional_overflow_i),
        .candidate_valid_o(candidate_valid_o),
        .pattern_id_o(pattern_id_o),
        .solution_valid_o(solution_valid_o),
        .repairable_o(repairable_o),
        .dictionary_overflow_o(dictionary_overflow_o)
    );
endmodule

`default_nettype wire
