`default_nettype none

module recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_integrated_top #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 7
) (
    input wire clk_i, input wire rst_ni, input wire start_i,
    input wire [4*5-1:0] pivot_valid_flat_i, input wire [4*5*ROW_ADDR_W-1:0] pivot_rows_flat_i, input wire [4*5*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input wire [4*5-1:0] row_gt1_flat_i, input wire [4*5-1:0] row_gt2_flat_i, input wire [4*5-1:0] row_gt3_flat_i,
    input wire [4*5-1:0] col_gt1_flat_i, input wire [4*5-1:0] col_gt2_flat_i, input wire [4*5-1:0] col_gt3_flat_i,
    input wire [4*HYBRID_ENTRIES-1:0] hybrid_valid_flat_i, input wire [4*HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input wire [4*HYBRID_ENTRIES-1:0] hybrid_descriptor_flat_i, input wire [4*HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,
    input wire [3:0] conventional_overflow_i, output wire busy_o, output wire done_o, output wire group_repairable_o, output wire commit_error_o,
    output wire [3:0] selected_valid_o, output wire [7:0] selected_action_flat_o, output wire [11:0] selected_config_flat_o,
    output wire [15:0] selected_pattern_flat_o, output wire [7:0] selected_donor_flat_o, output wire [3:0] selected_release_o,
    output wire [3:0] selected_borrow_o, output wire [3:0] resource_released_o, output wire [3:0] resource_borrowed_o,
    output wire [7:0] borrower_id_flat_o, output wire [159:0] candidate_valid_debug_o,
    output wire [159:0] candidate_release_debug_o, output wire [159:0] candidate_borrow_debug_o
);
    recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_integrated_shell #(
        .USE_OPT1(1), .ROW_ADDR_W(ROW_ADDR_W), .COL_ADDR_W(COL_ADDR_W), .DIFF_ADDR_W(DIFF_ADDR_W), .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) integrated_shell (.*);
endmodule

`default_nettype wire
