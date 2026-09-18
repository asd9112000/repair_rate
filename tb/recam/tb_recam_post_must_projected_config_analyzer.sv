`default_nettype none

// Verification-only comparison top for the isolated post-Must analyzer path.
module tb_recam_post_must_projected_config_analyzer (
    input  wire [2:0] config_id_i,
    input  wire [4:0] pivot_valid_i,
    input  wire [44:0] pivot_rows_flat_i,
    input  wire [24:0] pivot_cols_flat_i,
    input  wire [4:0] row_gt1_i,
    input  wire [4:0] row_gt2_i,
    input  wire [4:0] row_gt3_i,
    input  wire [4:0] col_gt1_i,
    input  wire [4:0] col_gt2_i,
    input  wire [4:0] col_gt3_i,
    input  wire [13:0] physical_hybrid_valid_i,
    input  wire [41:0] physical_hybrid_pointer_flat_i,
    input  wire [13:0] physical_hybrid_descriptor_i,
    input  wire [125:0] physical_hybrid_differing_flat_i,
    input  wire [97:0] physical_hybrid_cfg_valid_flat_i,
    input  wire [34:0] row_must_by_cfg_i,
    input  wire [34:0] col_must_by_cfg_i,
    input  wire conventional_overflow_i,
    output wire [9:0] new_candidate_valid_o,
    output wire [3:0] new_pattern_id_o,
    output wire new_solution_valid_o,
    output wire new_repairable_o,
    output wire new_dictionary_overflow_o,
    output wire [8:0] projected_hybrid_valid_o,
    output wire [26:0] projected_hybrid_pointer_flat_o,
    output wire [8:0] projected_hybrid_descriptor_o,
    output wire [80:0] projected_hybrid_differing_flat_o,
    output wire [3:0] projected_count_o,
    output wire projection_overflow_o,
    output wire [9:0] old_candidate_valid_o,
    output wire [3:0] old_pattern_id_o,
    output wire old_solution_valid_o,
    output wire old_repairable_o,
    output wire old_dictionary_overflow_o
);
    recam_post_must_projected_config_analyzer new_path (
        .config_id_i(config_id_i),
        .pivot_valid_i(pivot_valid_i),
        .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(pivot_cols_flat_i),
        .row_gt1_i(row_gt1_i), .row_gt2_i(row_gt2_i), .row_gt3_i(row_gt3_i),
        .col_gt1_i(col_gt1_i), .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i),
        .physical_hybrid_valid_i(physical_hybrid_valid_i),
        .physical_hybrid_pointer_flat_i(physical_hybrid_pointer_flat_i),
        .physical_hybrid_descriptor_i(physical_hybrid_descriptor_i),
        .physical_hybrid_differing_flat_i(physical_hybrid_differing_flat_i),
        .physical_hybrid_cfg_valid_flat_i(physical_hybrid_cfg_valid_flat_i),
        .row_must_by_cfg_i(row_must_by_cfg_i), .col_must_by_cfg_i(col_must_by_cfg_i),
        .conventional_overflow_i(conventional_overflow_i),
        .candidate_valid_o(new_candidate_valid_o), .pattern_id_o(new_pattern_id_o),
        .solution_valid_o(new_solution_valid_o), .repairable_o(new_repairable_o),
        .dictionary_overflow_o(new_dictionary_overflow_o),
        .projected_hybrid_valid_o(projected_hybrid_valid_o),
        .projected_hybrid_pointer_flat_o(projected_hybrid_pointer_flat_o),
        .projected_hybrid_descriptor_o(projected_hybrid_descriptor_o),
        .projected_hybrid_differing_flat_o(projected_hybrid_differing_flat_o),
        .projected_count_o(projected_count_o), .projection_overflow_o(projection_overflow_o)
    );

    recam_shared_config_analyzer #(.HYBRID_ENTRIES(7)) old_path (
        .config_id_i(config_id_i), .pivot_valid_i(pivot_valid_i),
        .pivot_rows_flat_i(pivot_rows_flat_i), .pivot_cols_flat_i(pivot_cols_flat_i),
        .row_gt1_i(row_gt1_i), .row_gt2_i(row_gt2_i), .row_gt3_i(row_gt3_i),
        .col_gt1_i(col_gt1_i), .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i),
        .hybrid_valid_i(projected_hybrid_valid_o[6:0]),
        .hybrid_pointer_flat_i(projected_hybrid_pointer_flat_o[20:0]),
        .hybrid_descriptor_i(projected_hybrid_descriptor_o[6:0]),
        .hybrid_differing_flat_i(projected_hybrid_differing_flat_o[62:0]),
        .conventional_overflow_i(conventional_overflow_i),
        .candidate_valid_o(old_candidate_valid_o), .pattern_id_o(old_pattern_id_o),
        .solution_valid_o(old_solution_valid_o), .repairable_o(old_repairable_o),
        .dictionary_overflow_o(old_dictionary_overflow_o)
    );
endmodule

`default_nettype wire
