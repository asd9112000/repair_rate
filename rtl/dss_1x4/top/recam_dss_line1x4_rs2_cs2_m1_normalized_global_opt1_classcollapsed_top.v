`default_nettype none

// Separate SYN-D OPT1 synthesis boundary.  The referenced top fixes the
// class-collapse parameter during elaboration; it does not expose a run-time
// optimization-selection input.
module recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_top #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 10
) (
    input wire clk_i,
    input wire rst_ni,
    input wire start_i,
    input wire [4*6-1:0] pivot_valid_flat_i,
    input wire [4*6*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input wire [4*6*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input wire [4*6-1:0] row_gt1_flat_i,
    input wire [4*6-1:0] row_gt2_flat_i,
    input wire [4*6-1:0] row_gt3_flat_i,
    input wire [4*6-1:0] row_gt4_flat_i,
    input wire [4*6-1:0] col_gt1_flat_i,
    input wire [4*6-1:0] col_gt2_flat_i,
    input wire [4*6-1:0] col_gt3_flat_i,
    input wire [4*6-1:0] col_gt4_flat_i,
    input wire [4*HYBRID_ENTRIES-1:0] hybrid_valid_flat_i,
    input wire [4*HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input wire [4*HYBRID_ENTRIES-1:0] hybrid_descriptor_flat_i,
    input wire [4*HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,
    input wire [3:0] conventional_overflow_i,
    output wire busy_o,
    output wire done_o,
    output wire group_repairable_o,
    output wire [3:0] selected_valid_o,
    output wire [7:0] selected_attempt_flat_o,
    output wire [15:0] selected_pattern_flat_o,
    output wire [11:0] selected_used_rows_flat_o,
    output wire [7:0] selected_used_cols_flat_o,
    output wire [15:0] selected_donor_flat_o,
    output wire [23:0] row_assignment_flat_o,
    output wire [1:0] selected_borrow_count_o,
    output wire [4:0] selected_used_rows_total_o,
    output wire [31:0] candidate_evaluations_o,
    output wire [31:0] dfs_cycles_o,
    output wire [179:0] candidate_valid_debug_o,
    output wire atomic_commit_error_o
);
    recam_dss_line1x4_rs2_cs2_m1_normalized_global_top #(
        .ROW_ADDR_W(ROW_ADDR_W),
        .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES),
        .ENABLE_OPT1_CLASS_COLLAPSE(1)
    ) impl (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(start_i),
        .pivot_valid_flat_i(pivot_valid_flat_i),
        .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(pivot_cols_flat_i),
        .row_gt1_flat_i(row_gt1_flat_i),
        .row_gt2_flat_i(row_gt2_flat_i),
        .row_gt3_flat_i(row_gt3_flat_i),
        .row_gt4_flat_i(row_gt4_flat_i),
        .col_gt1_flat_i(col_gt1_flat_i),
        .col_gt2_flat_i(col_gt2_flat_i),
        .col_gt3_flat_i(col_gt3_flat_i),
        .col_gt4_flat_i(col_gt4_flat_i),
        .hybrid_valid_flat_i(hybrid_valid_flat_i),
        .hybrid_pointer_flat_i(hybrid_pointer_flat_i),
        .hybrid_descriptor_flat_i(hybrid_descriptor_flat_i),
        .hybrid_differing_flat_i(hybrid_differing_flat_i),
        .conventional_overflow_i(conventional_overflow_i),
        .busy_o(busy_o),
        .done_o(done_o),
        .group_repairable_o(group_repairable_o),
        .selected_valid_o(selected_valid_o),
        .selected_attempt_flat_o(selected_attempt_flat_o),
        .selected_pattern_flat_o(selected_pattern_flat_o),
        .selected_used_rows_flat_o(selected_used_rows_flat_o),
        .selected_used_cols_flat_o(selected_used_cols_flat_o),
        .selected_donor_flat_o(selected_donor_flat_o),
        .row_assignment_flat_o(row_assignment_flat_o),
        .selected_borrow_count_o(selected_borrow_count_o),
        .selected_used_rows_total_o(selected_used_rows_total_o),
        .candidate_evaluations_o(candidate_evaluations_o),
        .dfs_cycles_o(dfs_cycles_o),
        .candidate_valid_debug_o(candidate_valid_debug_o),
        .atomic_commit_error_o(atomic_commit_error_o)
    );
endmodule

`default_nettype wire
