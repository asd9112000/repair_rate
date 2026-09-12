`default_nettype none

// Phase 3D integration boundary: Phase 3C always completes its four scans
// before this combinational EARLY selector observes the stored map.
module recam_phase3d_integrated_top #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 7
) (
    input wire clk_i, rst_ni, start_i, sa_role_i,
    input wire [4:0] pivot_valid_i,
    input wire [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input wire [5*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input wire [4:0] row_gt1_i, row_gt2_i, row_gt3_i,
    input wire [4:0] col_gt1_i, col_gt2_i, col_gt3_i,
    input wire [HYBRID_ENTRIES-1:0] hybrid_valid_i,
    input wire [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input wire [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,
    input wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,
    input wire conventional_overflow_i,
    output wire busy_o, done_o, early_solution_valid_o,
    output wire [1:0] selected_map_index_o,
    output wire [2:0] selected_config_id_o,
    output wire [3:0] selected_pattern_id_o,
    output wire [9:0] selected_candidate_valid_o,
    output wire selected_repairable_o,
    // Preserve the accepted Phase 3C integrated boundary for a comparable
    // analyzer timing/area observation; EARLY does not consume these ports.
    output wire [2:0] current_config_id_o,
    output wire [9:0] current_candidate_valid_o,
    output wire [3:0] current_pattern_id_o,
    output wire current_solution_valid_o,
    output wire current_repairable_o
);
    wire [79:0] config_pattern_map;

    recam_role_aware_config_scheduler #(
        .ROW_ADDR_W(ROW_ADDR_W), .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W), .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) role_aware_scheduler (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i), .sa_role_i(sa_role_i),
        .pivot_valid_i(pivot_valid_i), .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(pivot_cols_flat_i), .row_gt1_i(row_gt1_i),
        .row_gt2_i(row_gt2_i), .row_gt3_i(row_gt3_i), .col_gt1_i(col_gt1_i),
        .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i), .hybrid_valid_i(hybrid_valid_i),
        .hybrid_pointer_flat_i(hybrid_pointer_flat_i), .hybrid_descriptor_i(hybrid_descriptor_i),
        .hybrid_differing_flat_i(hybrid_differing_flat_i),
        .conventional_overflow_i(conventional_overflow_i), .busy_o(busy_o), .done_o(done_o),
        .current_config_id_o(current_config_id_o), .config_pattern_map_o(config_pattern_map),
        .current_candidate_valid_o(current_candidate_valid_o),
        .current_pattern_id_o(current_pattern_id_o),
        .current_solution_valid_o(current_solution_valid_o),
        .current_repairable_o(current_repairable_o)
    );

    recam_early_solution_selector early_selector (
        .config_pattern_map_i(config_pattern_map), .early_solution_valid_o(early_solution_valid_o),
        .selected_map_index_o(selected_map_index_o), .selected_config_id_o(selected_config_id_o),
        .selected_pattern_id_o(selected_pattern_id_o),
        .selected_candidate_valid_o(selected_candidate_valid_o),
        .selected_repairable_o(selected_repairable_o)
    );
endmodule
