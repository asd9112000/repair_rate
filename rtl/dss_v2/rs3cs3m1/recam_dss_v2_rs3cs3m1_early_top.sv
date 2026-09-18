`default_nettype none

module recam_dss_v2_rs3cs3m1_early_top (
    input logic clk_i, rst_ni, start_i,
    input logic [6:0] pivot_valid_i,
    input logic [62:0] pivot_rows_flat_i,
    input logic [34:0] pivot_cols_flat_i,
    input logic [6:0] row_gt1_i, row_gt2_i, row_gt3_i, row_gt4_i,
    input logic [6:0] col_gt1_i, col_gt2_i, col_gt3_i, col_gt4_i,
    input logic [16:0] hybrid_valid_i,
    input logic [50:0] hybrid_pointer_flat_i,
    input logic [16:0] hybrid_descriptor_i,
    input logic [152:0] hybrid_differing_flat_i,
    input logic conventional_overflow_i,
    output logic busy_o, done_o, group_repairable_o,
    output logic [3:0] sa_commit_valid_o,
    output logic [11:0] ledger_released_borrower_o,
    output logic [11:0] selected_config_flat_o,
    output logic [23:0] selected_pattern_flat_o,
    output logic [7:0] selected_donor_flat_o,
    output logic [3:0] borrow_flat_o, release_flat_o,
    output logic [1:0] failure_position_o
);
    logic [1:0] current_sa, current_slot;
    logic [2:0] current_config, rows, cols;
    logic [1:0] class_unused, action_unused;
    logic table_valid, transpose, release_unused, borrow_unused;
    logic [34:0] bitmap;
    logic [5:0] pattern;
    logic solution, repairable;
    recam_dss_v2_rs3cs3m1_early_core core (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i), .candidate_solution_valid_i(solution),
        .candidate_repairable_i(repairable), .candidate_pattern_id_i(pattern), .current_sa_o(current_sa),
        .current_slot_o(current_slot), .current_config_id_o(current_config), .busy_o(busy_o), .done_o(done_o),
        .group_repairable_o(group_repairable_o), .sa_commit_valid_o(sa_commit_valid_o),
        .ledger_released_borrower_o(ledger_released_borrower_o), .selected_config_flat_o(selected_config_flat_o),
        .selected_pattern_flat_o(selected_pattern_flat_o), .selected_donor_flat_o(selected_donor_flat_o),
        .borrow_flat_o(borrow_flat_o), .release_flat_o(release_flat_o), .failure_position_o(failure_position_o));
    dss_v2_rs3cs3m1_config_table cfg_table (
        .sa_i(current_sa), .slot_i(current_slot), .valid_o(table_valid), .config_id_o(),
        .row_count_o(rows), .col_count_o(cols), .canonical_class_o(class_unused), .transpose_o(transpose),
        .action_o(action_unused), .release_required_o(release_unused), .borrow_required_o(borrow_unused));
    dss_v2_rs3cs3m1_shared_config_analyzer analyzer (
        .row_count_i(rows), .col_count_i(cols), .transpose_i(transpose), .pivot_valid_i(pivot_valid_i),
        .pivot_rows_flat_i(pivot_rows_flat_i), .pivot_cols_flat_i(pivot_cols_flat_i),
        .row_gt1_i(row_gt1_i), .row_gt2_i(row_gt2_i), .row_gt3_i(row_gt3_i), .row_gt4_i(row_gt4_i),
        .col_gt1_i(col_gt1_i), .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i), .col_gt4_i(col_gt4_i),
        .hybrid_valid_i(hybrid_valid_i), .hybrid_pointer_flat_i(hybrid_pointer_flat_i),
        .hybrid_descriptor_i(hybrid_descriptor_i), .hybrid_differing_flat_i(hybrid_differing_flat_i),
        .conventional_overflow_i(conventional_overflow_i), .candidate_valid_o(bitmap), .pattern_id_o(pattern),
        .solution_valid_o(solution), .repairable_o(repairable), .dictionary_overflow_o());
endmodule
`default_nettype wire
