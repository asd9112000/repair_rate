`default_nettype none

// G2X2_R GROUP CA-LIVE top keeps its canonical map and pivot reconstruction.
module recam_dss_g2x2_r_static_global_live_state_top #(
    parameter integer ROW_ADDR_W = 9, parameter integer PHYS_COL_ADDR_W = 13,
    parameter integer WORD_COL_ADDR_W = 5, parameter integer HYBRID_LINE_ADDR_W = 13,
    parameter integer HYBRID_ENTRIES = 7
) (
    input logic clk_i, input logic rst_ni, input logic state_update_i, input logic [1:0] state_sa_i,
    input logic test_done_valid_i, input logic [1:0] test_done_sa_i,
    input logic [4:0] pivot_valid_i, input logic [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input logic [5*PHYS_COL_ADDR_W-1:0] pivot_cols_flat_i,
    input logic [4:0] row_gt1_i, input logic [4:0] row_gt2_i, input logic [4:0] row_gt3_i,
    input logic [4:0] col_gt1_i, input logic [4:0] col_gt2_i, input logic [4:0] col_gt3_i,
    input logic [HYBRID_ENTRIES-1:0] hybrid_valid_i, input logic [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input logic [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,
    input logic [HYBRID_ENTRIES*HYBRID_LINE_ADDR_W-1:0] hybrid_differing_flat_i,
    input logic conventional_overflow_i,
    output logic scan_active_o, output logic [1:0] active_sa_o, output logic [1:0] scan_slot_o,
    output logic [2:0] scan_config_o, output logic [3:0] sa_result_frozen_o, output logic solution_ready_o,
    output logic [59:0] candidate_store_image_o, output logic group_repairable_o,
    output logic [3:0] sa_commit_valid_o, output logic [11:0] ledger_released_borrower_o,
    output logic [11:0] selected_config_flat_o, output logic [15:0] selected_pattern_flat_o,
    output logic [7:0] selected_donor_flat_o, output logic [3:0] borrow_flat_o, output logic [3:0] release_flat_o,
    output logic [1:0] failure_position_o, output logic [259:0] final_repair_address_flat_o,
    output logic [19:0] final_repair_is_row_flat_o, output logic [19:0] final_repair_line_valid_flat_o
);
    logic [3:0] candidate_pattern;
    logic solution_valid, repairable, active_state_update;
    assign active_state_update = state_update_i && (state_sa_i == active_sa_o);
    recam_dss_g2x2_r_static_global_live_state_core core (
        .clk_i(clk_i), .rst_ni(rst_ni), .state_update_i(state_update_i), .state_sa_i(state_sa_i),
        .test_done_valid_i(test_done_valid_i), .test_done_sa_i(test_done_sa_i),
        .candidate_valid_i(solution_valid && repairable), .candidate_pattern_id_i(candidate_pattern),
        .scan_active_o(scan_active_o), .active_sa_o(active_sa_o), .scan_slot_o(scan_slot_o),
        .scan_config_id_o(scan_config_o), .sa_result_frozen_o(sa_result_frozen_o),
        .solution_ready_o(solution_ready_o), .candidate_store_image_o(candidate_store_image_o),
        .group_repairable_o(group_repairable_o), .sa_commit_valid_o(sa_commit_valid_o),
        .ledger_released_borrower_o(ledger_released_borrower_o), .selected_config_flat_o(selected_config_flat_o),
        .selected_pattern_flat_o(selected_pattern_flat_o), .selected_donor_flat_o(selected_donor_flat_o),
        .borrow_flat_o(borrow_flat_o), .release_flat_o(release_flat_o), .failure_position_o(failure_position_o)
    );
    recam_shared_config_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W), .PHYS_COL_ADDR_W(PHYS_COL_ADDR_W), .WORD_COL_ADDR_W(WORD_COL_ADDR_W),
        .HYBRID_LINE_ADDR_W(HYBRID_LINE_ADDR_W), .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) analyzer (
        .config_id_i(scan_config_o), .pivot_valid_i(pivot_valid_i), .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(pivot_cols_flat_i), .row_gt1_i(row_gt1_i), .row_gt2_i(row_gt2_i), .row_gt3_i(row_gt3_i),
        .col_gt1_i(col_gt1_i), .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i), .hybrid_valid_i(hybrid_valid_i),
        .hybrid_pointer_flat_i(hybrid_pointer_flat_i), .hybrid_descriptor_i(hybrid_descriptor_i),
        .hybrid_differing_flat_i(hybrid_differing_flat_i), .conventional_overflow_i(conventional_overflow_i),
        .candidate_valid_o(), .pattern_id_o(candidate_pattern), .solution_valid_o(solution_valid),
        .repairable_o(repairable), .dictionary_overflow_o()
    );
    dss_group_pivot_address_regs #(.ROW_ADDR_W(ROW_ADDR_W), .PHYS_COL_ADDR_W(PHYS_COL_ADDR_W)) pivot_address_regs (
        .clk_i(clk_i), .rst_ni(rst_ni), .capture_enable_i(scan_active_o && (scan_slot_o == 2'd2) && !active_state_update),
        .capture_sa_i(active_sa_o), .pivot_rows_flat_i(pivot_rows_flat_i), .pivot_cols_flat_i(pivot_cols_flat_i),
        .group_commit_valid_i(sa_commit_valid_o), .selected_config_flat_i(selected_config_flat_o),
        .selected_pattern_flat_i(selected_pattern_flat_o), .final_repair_address_flat_o(final_repair_address_flat_o),
        .final_repair_is_row_flat_o(final_repair_is_row_flat_o), .final_repair_line_valid_flat_o(final_repair_line_valid_flat_o)
    );
endmodule

`default_nettype wire
