`default_nettype none

// G2X2_R EARLY CA-LIVE policy top with one caller-owned analyzer projection.
module recam_dss_g2x2_r_static_early_live_state_top #(
    parameter integer ROW_ADDR_W = 9, parameter integer PHYS_COL_ADDR_W = 13,
    parameter integer WORD_COL_ADDR_W = 5, parameter integer HYBRID_LINE_ADDR_W = 13,
    parameter integer HYBRID_ENTRIES = 7
) (
    input logic clk_i, input logic rst_ni, input logic state_update_i,
    input logic [1:0] state_sa_i, input logic test_done_valid_i, input logic [1:0] test_done_sa_i,
    input logic [4:0] pivot_valid_i, input logic [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input logic [5*PHYS_COL_ADDR_W-1:0] pivot_cols_flat_i,
    input logic [4:0] row_gt1_i, input logic [4:0] row_gt2_i, input logic [4:0] row_gt3_i,
    input logic [4:0] col_gt1_i, input logic [4:0] col_gt2_i, input logic [4:0] col_gt3_i,
    input logic [HYBRID_ENTRIES-1:0] hybrid_valid_i,
    input logic [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input logic [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,
    input logic [HYBRID_ENTRIES*HYBRID_LINE_ADDR_W-1:0] hybrid_differing_flat_i,
    input logic conventional_overflow_i,
    output logic scan_active_o, output logic [1:0] active_sa_o, output logic [1:0] scan_slot_o,
    output logic [2:0] scan_config_o, output logic solution_commit_valid_o,
    output logic [1:0] solution_sa_o, output logic [2:0] solution_config_o,
    output logic [3:0] solution_pattern_o, output logic [64:0] solution_line_address_flat_o,
    output logic [4:0] solution_line_is_row_o, output logic [4:0] solution_line_valid_o,
    output logic done_o, output logic group_repairable_o, output logic [1:0] failure_position_o
);
    logic [3:0] candidate_pattern;
    logic solution_valid, repairable;

    recam_dss_g2x2_r_static_early_live_state_core core (
        .clk_i(clk_i), .rst_ni(rst_ni), .state_update_i(state_update_i), .state_sa_i(state_sa_i),
        .test_done_valid_i(test_done_valid_i), .test_done_sa_i(test_done_sa_i),
        .candidate_valid_i(solution_valid && repairable), .scan_active_o(scan_active_o),
        .active_sa_o(active_sa_o), .scan_slot_o(scan_slot_o), .scan_config_id_o(scan_config_o),
        .solution_commit_o(solution_commit_valid_o), .done_o(done_o),
        .group_repairable_o(group_repairable_o), .failure_position_o(failure_position_o)
    );
    recam_shared_config_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W), .PHYS_COL_ADDR_W(PHYS_COL_ADDR_W),
        .WORD_COL_ADDR_W(WORD_COL_ADDR_W), .HYBRID_LINE_ADDR_W(HYBRID_LINE_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) analyzer (
        .config_id_i(scan_config_o), .pivot_valid_i(pivot_valid_i), .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(pivot_cols_flat_i), .row_gt1_i(row_gt1_i), .row_gt2_i(row_gt2_i),
        .row_gt3_i(row_gt3_i), .col_gt1_i(col_gt1_i), .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i),
        .hybrid_valid_i(hybrid_valid_i), .hybrid_pointer_flat_i(hybrid_pointer_flat_i),
        .hybrid_descriptor_i(hybrid_descriptor_i), .hybrid_differing_flat_i(hybrid_differing_flat_i),
        .conventional_overflow_i(conventional_overflow_i), .candidate_valid_o(), .pattern_id_o(candidate_pattern),
        .solution_valid_o(solution_valid), .repairable_o(repairable), .dictionary_overflow_o()
    );
    assign solution_sa_o = active_sa_o;
    assign solution_config_o = scan_config_o;
    assign solution_pattern_o = candidate_pattern;
    dss_early_selected_address_mux #(.ROW_ADDR_W(ROW_ADDR_W), .PHYS_COL_ADDR_W(PHYS_COL_ADDR_W)) selected_address_mux (
        .selected_config_i(scan_config_o), .selected_pattern_id_i(candidate_pattern),
        .pivot_rows_flat_i(pivot_rows_flat_i), .pivot_cols_flat_i(pivot_cols_flat_i),
        .solution_line_address_flat_o(solution_line_address_flat_o),
        .solution_line_is_row_o(solution_line_is_row_o), .solution_line_valid_o(solution_line_valid_o)
    );
endmodule

`default_nettype wire
