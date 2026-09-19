`default_nettype none

// Separate OPT1 policy elaboration.  The child parameter is compile-time only;
// this wrapper adds no run-time optimization selection signal or mux.
module recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_core (
    input wire clk_i,
    input wire rst_ni,
    input wire start_i,
    input wire [179:0] candidate_valid_i,
    input wire [539:0] candidate_used_rows_flat_i,
    input wire [359:0] candidate_used_cols_flat_i,
    output wire busy_o,
    output wire search_done_o,
    output wire group_repairable_o,
    output wire [3:0] selected_valid_o,
    output wire [7:0] selected_attempt_flat_o,
    output wire [15:0] selected_pattern_flat_o,
    output wire [11:0] selected_used_rows_flat_o,
    output wire [7:0] selected_used_cols_flat_o,
    output wire [15:0] selected_donor_flat_o,
    output wire [23:0] selected_row_assignment_flat_o,
    output wire [1:0] selected_borrow_count_o,
    output wire [4:0] selected_used_rows_total_o,
    output wire [31:0] candidate_evaluations_o,
    output wire [31:0] dfs_cycles_o
);
    recam_dss_line1x4_rs2_cs2_m1_normalized_global_core #(
        .ENABLE_OPT1_CLASS_COLLAPSE(1)
    ) impl (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(start_i),
        .candidate_valid_i(candidate_valid_i),
        .candidate_used_rows_flat_i(candidate_used_rows_flat_i),
        .candidate_used_cols_flat_i(candidate_used_cols_flat_i),
        .busy_o(busy_o),
        .search_done_o(search_done_o),
        .group_repairable_o(group_repairable_o),
        .selected_valid_o(selected_valid_o),
        .selected_attempt_flat_o(selected_attempt_flat_o),
        .selected_pattern_flat_o(selected_pattern_flat_o),
        .selected_used_rows_flat_o(selected_used_rows_flat_o),
        .selected_used_cols_flat_o(selected_used_cols_flat_o),
        .selected_donor_flat_o(selected_donor_flat_o),
        .selected_row_assignment_flat_o(selected_row_assignment_flat_o),
        .selected_borrow_count_o(selected_borrow_count_o),
        .selected_used_rows_total_o(selected_used_rows_total_o),
        .candidate_evaluations_o(candidate_evaluations_o),
        .dfs_cycles_o(dfs_cycles_o)
    );
endmodule

`default_nettype wire
