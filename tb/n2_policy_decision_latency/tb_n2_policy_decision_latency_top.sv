`default_nettype none

// Test-only paired boundary. Both controllers consume one immutable,
// caller-owned finalized analyzer summary for every measured vector.
module tb_n2_policy_decision_latency_top (
    input logic clk_i,
    input logic rst_ni,
    input logic start_i,
    input logic [4:0] pivot_valid_i,
    input logic [44:0] pivot_rows_flat_i,
    input logic [64:0] pivot_cols_flat_i,
    input logic [4:0] row_gt1_i,
    input logic [4:0] row_gt2_i,
    input logic [4:0] row_gt3_i,
    input logic [4:0] col_gt1_i,
    input logic [4:0] col_gt2_i,
    input logic [4:0] col_gt3_i,
    input logic [6:0] hybrid_valid_i,
    input logic [20:0] hybrid_pointer_flat_i,
    input logic [6:0] hybrid_descriptor_i,
    input logic [90:0] hybrid_differing_flat_i,
    input logic conventional_overflow_i,
    output logic early_busy_o,
    output logic early_done_o,
    output logic early_repairable_o,
    output logic early_commit_valid_o,
    output logic [1:0] early_solution_sa_o,
    output logic [2:0] early_solution_config_o,
    output logic [3:0] early_solution_pattern_o,
    output logic group_busy_o,
    output logic group_done_o,
    output logic group_repairable_o,
    output logic [3:0] group_commit_valid_o,
    output logic [11:0] group_selected_config_flat_o,
    output logic [15:0] group_selected_pattern_flat_o
);
    logic [1:0] early_failure_position_unused;
    logic [64:0] early_line_address_unused;
    logic [4:0] early_line_is_row_unused;
    logic [4:0] early_line_valid_unused;
    logic [11:0] group_ledger_unused;
    logic [7:0] group_donor_unused;
    logic [3:0] group_borrow_unused;
    logic [3:0] group_release_unused;
    logic [1:0] group_failure_position_unused;
    logic [259:0] group_address_unused;
    logic [19:0] group_is_row_unused;
    logic [19:0] group_line_valid_unused;

    recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top early (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i),
        .pivot_valid_i(pivot_valid_i), .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(pivot_cols_flat_i), .row_gt1_i(row_gt1_i),
        .row_gt2_i(row_gt2_i), .row_gt3_i(row_gt3_i), .col_gt1_i(col_gt1_i),
        .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i), .hybrid_valid_i(hybrid_valid_i),
        .hybrid_pointer_flat_i(hybrid_pointer_flat_i),
        .hybrid_descriptor_i(hybrid_descriptor_i),
        .hybrid_differing_flat_i(hybrid_differing_flat_i),
        .conventional_overflow_i(conventional_overflow_i), .busy_o(early_busy_o),
        .done_o(early_done_o), .group_repairable_o(early_repairable_o),
        .failure_position_o(early_failure_position_unused),
        .solution_commit_valid_o(early_commit_valid_o),
        .solution_sa_o(early_solution_sa_o),
        .solution_config_o(early_solution_config_o),
        .solution_pattern_o(early_solution_pattern_o),
        .solution_line_address_flat_o(early_line_address_unused),
        .solution_line_is_row_o(early_line_is_row_unused),
        .solution_line_valid_o(early_line_valid_unused)
    );

    recam_dss_hyp02_static_global_top group (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i),
        .pivot_valid_i(pivot_valid_i), .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(pivot_cols_flat_i), .row_gt1_i(row_gt1_i),
        .row_gt2_i(row_gt2_i), .row_gt3_i(row_gt3_i), .col_gt1_i(col_gt1_i),
        .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i), .hybrid_valid_i(hybrid_valid_i),
        .hybrid_pointer_flat_i(hybrid_pointer_flat_i),
        .hybrid_descriptor_i(hybrid_descriptor_i),
        .hybrid_differing_flat_i(hybrid_differing_flat_i),
        .conventional_overflow_i(conventional_overflow_i), .busy_o(group_busy_o),
        .done_o(group_done_o), .group_repairable_o(group_repairable_o),
        .sa_commit_valid_o(group_commit_valid_o),
        .ledger_released_borrower_o(group_ledger_unused),
        .selected_config_flat_o(group_selected_config_flat_o),
        .selected_pattern_flat_o(group_selected_pattern_flat_o),
        .selected_donor_flat_o(group_donor_unused), .borrow_flat_o(group_borrow_unused),
        .release_flat_o(group_release_unused),
        .failure_position_o(group_failure_position_unused),
        .final_repair_address_flat_o(group_address_unused),
        .final_repair_is_row_flat_o(group_is_row_unused),
        .final_repair_line_valid_flat_o(group_line_valid_unused)
    );
endmodule

`default_nettype wire
