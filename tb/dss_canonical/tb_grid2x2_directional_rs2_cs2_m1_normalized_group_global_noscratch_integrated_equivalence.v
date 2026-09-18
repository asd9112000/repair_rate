`default_nettype none

module tb_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_integrated_equivalence (
    input wire clk_i, input wire rst_ni, input wire start_i,
    input wire [19:0] pivot_valid_flat_i, input wire [179:0] pivot_rows_flat_i, input wire [99:0] pivot_cols_flat_i,
    input wire [19:0] row_gt1_flat_i, input wire [19:0] row_gt2_flat_i, input wire [19:0] row_gt3_flat_i,
    input wire [19:0] col_gt1_flat_i, input wire [19:0] col_gt2_flat_i, input wire [19:0] col_gt3_flat_i,
    input wire [27:0] hybrid_valid_flat_i, input wire [83:0] hybrid_pointer_flat_i, input wire [27:0] hybrid_descriptor_flat_i,
    input wire [251:0] hybrid_differing_flat_i, input wire [3:0] conventional_overflow_i,
    output wire opt0_busy_o, output wire opt0_done_o, output wire opt0_repairable_o, output wire opt0_commit_error_o,
    output wire [3:0] opt0_selected_valid_o, output wire [7:0] opt0_action_o, output wire [11:0] opt0_config_o,
    output wire [15:0] opt0_pattern_o, output wire [7:0] opt0_donor_o, output wire [3:0] opt0_release_o,
    output wire [3:0] opt0_borrow_o, output wire [3:0] opt0_released_o, output wire [3:0] opt0_borrowed_o,
    output wire [7:0] opt0_borrower_id_o, output wire [159:0] opt0_candidate_valid_o,
    output wire [159:0] opt0_candidate_release_o, output wire [159:0] opt0_candidate_borrow_o,
    output wire opt1_busy_o, output wire opt1_done_o, output wire opt1_repairable_o, output wire opt1_commit_error_o,
    output wire [3:0] opt1_selected_valid_o, output wire [7:0] opt1_action_o, output wire [11:0] opt1_config_o,
    output wire [15:0] opt1_pattern_o, output wire [7:0] opt1_donor_o, output wire [3:0] opt1_release_o,
    output wire [3:0] opt1_borrow_o, output wire [3:0] opt1_released_o, output wire [3:0] opt1_borrowed_o,
    output wire [7:0] opt1_borrower_id_o, output wire [159:0] opt1_candidate_valid_o,
    output wire [159:0] opt1_candidate_release_o, output wire [159:0] opt1_candidate_borrow_o
);
    recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_integrated_top opt0_top (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i), .pivot_valid_flat_i(pivot_valid_flat_i),
        .pivot_rows_flat_i(pivot_rows_flat_i), .pivot_cols_flat_i(pivot_cols_flat_i), .row_gt1_flat_i(row_gt1_flat_i),
        .row_gt2_flat_i(row_gt2_flat_i), .row_gt3_flat_i(row_gt3_flat_i), .col_gt1_flat_i(col_gt1_flat_i),
        .col_gt2_flat_i(col_gt2_flat_i), .col_gt3_flat_i(col_gt3_flat_i), .hybrid_valid_flat_i(hybrid_valid_flat_i),
        .hybrid_pointer_flat_i(hybrid_pointer_flat_i), .hybrid_descriptor_flat_i(hybrid_descriptor_flat_i),
        .hybrid_differing_flat_i(hybrid_differing_flat_i), .conventional_overflow_i(conventional_overflow_i),
        .busy_o(opt0_busy_o), .done_o(opt0_done_o), .group_repairable_o(opt0_repairable_o), .commit_error_o(opt0_commit_error_o),
        .selected_valid_o(opt0_selected_valid_o), .selected_action_flat_o(opt0_action_o), .selected_config_flat_o(opt0_config_o),
        .selected_pattern_flat_o(opt0_pattern_o), .selected_donor_flat_o(opt0_donor_o), .selected_release_o(opt0_release_o),
        .selected_borrow_o(opt0_borrow_o), .resource_released_o(opt0_released_o), .resource_borrowed_o(opt0_borrowed_o),
        .borrower_id_flat_o(opt0_borrower_id_o), .candidate_valid_debug_o(opt0_candidate_valid_o),
        .candidate_release_debug_o(opt0_candidate_release_o), .candidate_borrow_debug_o(opt0_candidate_borrow_o)
    );

    recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_integrated_top opt1_top (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i), .pivot_valid_flat_i(pivot_valid_flat_i),
        .pivot_rows_flat_i(pivot_rows_flat_i), .pivot_cols_flat_i(pivot_cols_flat_i), .row_gt1_flat_i(row_gt1_flat_i),
        .row_gt2_flat_i(row_gt2_flat_i), .row_gt3_flat_i(row_gt3_flat_i), .col_gt1_flat_i(col_gt1_flat_i),
        .col_gt2_flat_i(col_gt2_flat_i), .col_gt3_flat_i(col_gt3_flat_i), .hybrid_valid_flat_i(hybrid_valid_flat_i),
        .hybrid_pointer_flat_i(hybrid_pointer_flat_i), .hybrid_descriptor_flat_i(hybrid_descriptor_flat_i),
        .hybrid_differing_flat_i(hybrid_differing_flat_i), .conventional_overflow_i(conventional_overflow_i),
        .busy_o(opt1_busy_o), .done_o(opt1_done_o), .group_repairable_o(opt1_repairable_o), .commit_error_o(opt1_commit_error_o),
        .selected_valid_o(opt1_selected_valid_o), .selected_action_flat_o(opt1_action_o), .selected_config_flat_o(opt1_config_o),
        .selected_pattern_flat_o(opt1_pattern_o), .selected_donor_flat_o(opt1_donor_o), .selected_release_o(opt1_release_o),
        .selected_borrow_o(opt1_borrow_o), .resource_released_o(opt1_released_o), .resource_borrowed_o(opt1_borrowed_o),
        .borrower_id_flat_o(opt1_borrower_id_o), .candidate_valid_debug_o(opt1_candidate_valid_o),
        .candidate_release_debug_o(opt1_candidate_release_o), .candidate_borrow_debug_o(opt1_candidate_borrow_o)
    );
endmodule

`default_nettype wire
