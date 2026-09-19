`default_nettype none

// Parallel OPT0/OPT1 policy harness.  It compares static elaborations only;
// candidate evaluation counters intentionally differ and are not outputs here.
module tb_recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_equivalence (
    input wire clk_i,
    input wire rst_ni,
    input wire start_i,
    input wire [179:0] candidate_valid_i,
    input wire [539:0] candidate_used_rows_flat_i,
    input wire [359:0] candidate_used_cols_flat_i,
    output wire opt0_done_o,
    output wire opt0_repairable_o,
    output wire [3:0] opt0_selected_valid_o,
    output wire [7:0] opt0_selected_attempt_flat_o,
    output wire [15:0] opt0_selected_pattern_flat_o,
    output wire [11:0] opt0_selected_used_rows_flat_o,
    output wire [7:0] opt0_selected_used_cols_flat_o,
    output wire [15:0] opt0_selected_donor_flat_o,
    output wire [23:0] opt0_selected_row_assignment_flat_o,
    output wire [1:0] opt0_selected_borrow_count_o,
    output wire [4:0] opt0_selected_used_rows_total_o,
    output wire opt1_done_o,
    output wire opt1_repairable_o,
    output wire [3:0] opt1_selected_valid_o,
    output wire [7:0] opt1_selected_attempt_flat_o,
    output wire [15:0] opt1_selected_pattern_flat_o,
    output wire [11:0] opt1_selected_used_rows_flat_o,
    output wire [7:0] opt1_selected_used_cols_flat_o,
    output wire [15:0] opt1_selected_donor_flat_o,
    output wire [23:0] opt1_selected_row_assignment_flat_o,
    output wire [1:0] opt1_selected_borrow_count_o,
    output wire [4:0] opt1_selected_used_rows_total_o
);
    /* verilator lint_off UNUSED */
    wire unused_opt0_busy;
    wire unused_opt1_busy;
    wire [31:0] unused_opt0_evaluations;
    wire [31:0] unused_opt0_cycles;
    wire [31:0] unused_opt1_evaluations;
    wire [31:0] unused_opt1_cycles;
    /* verilator lint_on UNUSED */

    recam_dss_line1x4_rs2_cs2_m1_normalized_global_core opt0 (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i),
        .candidate_valid_i(candidate_valid_i),
        .candidate_used_rows_flat_i(candidate_used_rows_flat_i),
        .candidate_used_cols_flat_i(candidate_used_cols_flat_i),
        .busy_o(unused_opt0_busy), .search_done_o(opt0_done_o),
        .group_repairable_o(opt0_repairable_o),
        .selected_valid_o(opt0_selected_valid_o),
        .selected_attempt_flat_o(opt0_selected_attempt_flat_o),
        .selected_pattern_flat_o(opt0_selected_pattern_flat_o),
        .selected_used_rows_flat_o(opt0_selected_used_rows_flat_o),
        .selected_used_cols_flat_o(opt0_selected_used_cols_flat_o),
        .selected_donor_flat_o(opt0_selected_donor_flat_o),
        .selected_row_assignment_flat_o(opt0_selected_row_assignment_flat_o),
        .selected_borrow_count_o(opt0_selected_borrow_count_o),
        .selected_used_rows_total_o(opt0_selected_used_rows_total_o),
        .candidate_evaluations_o(unused_opt0_evaluations), .dfs_cycles_o(unused_opt0_cycles)
    );

    recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_core opt1 (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i),
        .candidate_valid_i(candidate_valid_i),
        .candidate_used_rows_flat_i(candidate_used_rows_flat_i),
        .candidate_used_cols_flat_i(candidate_used_cols_flat_i),
        .busy_o(unused_opt1_busy), .search_done_o(opt1_done_o),
        .group_repairable_o(opt1_repairable_o),
        .selected_valid_o(opt1_selected_valid_o),
        .selected_attempt_flat_o(opt1_selected_attempt_flat_o),
        .selected_pattern_flat_o(opt1_selected_pattern_flat_o),
        .selected_used_rows_flat_o(opt1_selected_used_rows_flat_o),
        .selected_used_cols_flat_o(opt1_selected_used_cols_flat_o),
        .selected_donor_flat_o(opt1_selected_donor_flat_o),
        .selected_row_assignment_flat_o(opt1_selected_row_assignment_flat_o),
        .selected_borrow_count_o(opt1_selected_borrow_count_o),
        .selected_used_rows_total_o(opt1_selected_used_rows_total_o),
        .candidate_evaluations_o(unused_opt1_evaluations), .dfs_cycles_o(unused_opt1_cycles)
    );
endmodule

`default_nettype wire
