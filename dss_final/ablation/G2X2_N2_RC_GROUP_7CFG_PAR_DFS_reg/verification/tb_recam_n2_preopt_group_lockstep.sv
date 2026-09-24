`default_nettype none

// Drives the frozen canonical top and the supporting ablation from one SA
// snapshot. The ablation receives that snapshot for all four scheduled SAs,
// matching the canonical top's existing single-snapshot interface.
module tb_recam_n2_preopt_group_lockstep (
    input wire clk_i, input wire rst_ni, input wire start_i,
    input wire [4:0] pivot_valid_i, input wire [44:0] pivot_rows_flat_i,
    input wire [64:0] pivot_cols_flat_i, input wire [4:0] row_gt1_i,
    input wire [4:0] row_gt2_i, input wire [4:0] row_gt3_i,
    input wire [4:0] col_gt1_i, input wire [4:0] col_gt2_i, input wire [4:0] col_gt3_i,
    input wire [6:0] hybrid_valid_i, input wire [20:0] hybrid_pointer_flat_i,
    input wire [6:0] hybrid_descriptor_i, input wire [90:0] hybrid_differing_flat_i,
    input wire conventional_overflow_i,
    output wire canonical_done_o, output wire canonical_repairable_o,
    output wire [3:0] canonical_valid_o, output wire [11:0] canonical_config_o,
    output wire [15:0] canonical_pattern_o, output wire [7:0] canonical_action_o, output wire [7:0] canonical_donor_o,
    output wire [3:0] canonical_borrow_o, output wire [3:0] canonical_release_o,
    output wire [259:0] canonical_address_o, output wire [19:0] canonical_is_row_o,
    output wire [19:0] canonical_line_valid_o,
    output wire ablation_done_o, output wire ablation_repairable_o,
    output wire [3:0] ablation_valid_o, output wire [11:0] ablation_config_o,
    output wire [15:0] ablation_pattern_o, output wire [7:0] ablation_donor_o,
    output wire [3:0] ablation_borrow_o, output wire [3:0] ablation_release_o,
    output wire [259:0] ablation_address_o, output wire [19:0] ablation_is_row_o,
    output wire [19:0] ablation_line_valid_o, output wire [56:0] historical_dfs_selection_debug_o, output wire ablation_busy_o, output wire [7:0] ablation_action_o, output wire ablation_commit_error_o
);
    /* verilator lint_off PINCONNECTEMPTY */
    wire [11:0] unused_canonical_ledger;
    recam_dss_hyp02_static_global_top canonical (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i), .pivot_valid_i(pivot_valid_i),
        .pivot_rows_flat_i(pivot_rows_flat_i), .pivot_cols_flat_i(pivot_cols_flat_i),
        .row_gt1_i(row_gt1_i), .row_gt2_i(row_gt2_i), .row_gt3_i(row_gt3_i),
        .col_gt1_i(col_gt1_i), .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i),
        .hybrid_valid_i(hybrid_valid_i), .hybrid_pointer_flat_i(hybrid_pointer_flat_i),
        .hybrid_descriptor_i(hybrid_descriptor_i), .hybrid_differing_flat_i(hybrid_differing_flat_i),
        .conventional_overflow_i(conventional_overflow_i), .busy_o(), .done_o(canonical_done_o),
        .group_repairable_o(canonical_repairable_o), .sa_commit_valid_o(canonical_valid_o),
        .ledger_released_borrower_o(unused_canonical_ledger), .selected_config_flat_o(canonical_config_o),
        .selected_pattern_flat_o(canonical_pattern_o), .selected_donor_flat_o(canonical_donor_o),
        .borrow_flat_o(canonical_borrow_o), .release_flat_o(canonical_release_o), .failure_position_o(),
        .final_repair_address_flat_o(canonical_address_o), .final_repair_is_row_flat_o(canonical_is_row_o),
        .final_repair_line_valid_flat_o(canonical_line_valid_o));
    assign canonical_action_o[1:0] = canonical_config_o[2:0] == 3'd0 ? 2'd0 : canonical_config_o[2:0] == 3'd4 ? 2'd1 : canonical_config_o[2:0] == 3'd5 ? 2'd2 : 2'd3;
    assign canonical_action_o[3:2] = canonical_config_o[4:3];
    assign canonical_action_o[5:4] = canonical_config_o[7:6];
    assign canonical_action_o[7:6] = canonical_config_o[11:9] == 3'd0 ? 2'd0 : canonical_config_o[11:9] == 3'd4 ? 2'd1 : canonical_config_o[11:9] == 3'd5 ? 2'd2 : 2'd3;
    recam_n2_preopt_group_top ablation (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i),
        .pivot_valid_flat_i({4{pivot_valid_i}}), .pivot_rows_flat_i({4{pivot_rows_flat_i}}),
        .pivot_cols_flat_i({4{pivot_cols_flat_i}}), .row_gt1_flat_i({4{row_gt1_i}}),
        .row_gt2_flat_i({4{row_gt2_i}}), .row_gt3_flat_i({4{row_gt3_i}}),
        .col_gt1_flat_i({4{col_gt1_i}}), .col_gt2_flat_i({4{col_gt2_i}}), .col_gt3_flat_i({4{col_gt3_i}}),
        .hybrid_valid_flat_i({4{hybrid_valid_i}}), .hybrid_pointer_flat_i({4{hybrid_pointer_flat_i}}),
        .hybrid_descriptor_flat_i({4{hybrid_descriptor_i}}), .hybrid_differing_flat_i({4{hybrid_differing_flat_i}}),
        .conventional_overflow_i({4{conventional_overflow_i}}), .busy_o(ablation_busy_o), .done_o(ablation_done_o),
        .group_repairable_o(ablation_repairable_o), .commit_error_o(ablation_commit_error_o), .selected_valid_o(ablation_valid_o),
        .selected_action_flat_o(ablation_action_o), .selected_config_flat_o(ablation_config_o), .selected_pattern_flat_o(ablation_pattern_o),
        .selected_donor_flat_o(ablation_donor_o), .selected_release_o(ablation_release_o), .selected_borrow_o(ablation_borrow_o),
        .resource_released_o(), .resource_borrowed_o(), .borrower_id_flat_o(), .candidate_valid_debug_o(),
        .candidate_release_debug_o(), .candidate_borrow_debug_o(), .historical_dfs_selection_debug_o(historical_dfs_selection_debug_o), .final_repair_address_flat_o(ablation_address_o),
        .final_repair_is_row_flat_o(ablation_is_row_o), .final_repair_line_valid_flat_o(ablation_line_valid_o));
endmodule
`default_nettype wire
