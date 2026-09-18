`default_nettype none

// Verification-only bridge: it retains the final injected summaries for the
// untouched EARLY_BASELINE oracle while driving the S1G-A overlap core.
module recam_dss_v2_early_overlap_equivalence_test_top (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic overlap_start_i,
    input  logic baseline_start_i,
    input  logic summary_update_valid_i,
    input  logic [1:0] summary_update_sa_i,
    input  logic [203:0] summary_payload_i,
    input  logic test_done_valid_i,
    input  logic [1:0] test_done_sa_i,

    output logic overlap_busy_o,
    output logic overlap_done_o,
    output logic overlap_group_repairable_o,
    output logic [3:0] overlap_sa_commit_valid_o,
    output logic [11:0] overlap_ledger_o,
    output logic [11:0] overlap_selected_config_flat_o,
    output logic [15:0] overlap_selected_pattern_flat_o,
    output logic [7:0] overlap_selected_donor_flat_o,
    output logic [3:0] overlap_borrow_flat_o,
    output logic [3:0] overlap_release_flat_o,
    output logic [1:0] overlap_failure_position_o,
    output logic [3:0] overlap_analysis_valid_o,
    output logic [15:0] overlap_fault_generation_o,
    output logic [15:0] overlap_analysis_generation_o,
    output logic [3:0] overlap_summary_update_event_o,
    output logic [3:0] overlap_latest_candidate_ready_o,
    output logic [3:0] overlap_test_done_event_o,
    output logic overlap_final_ledger_decision_ready_o,

    output logic baseline_busy_o,
    output logic baseline_done_o,
    output logic baseline_group_repairable_o,
    output logic [3:0] baseline_sa_commit_valid_o,
    output logic [11:0] baseline_ledger_o,
    output logic [11:0] baseline_selected_config_flat_o,
    output logic [15:0] baseline_selected_pattern_flat_o,
    output logic [7:0] baseline_selected_donor_flat_o,
    output logic [3:0] baseline_borrow_flat_o,
    output logic [3:0] baseline_release_flat_o,
    output logic [1:0] baseline_failure_position_o
);
    localparam int unsigned ROW_ADDR_W = 9;
    localparam int unsigned COL_ADDR_W = 5;
    localparam int unsigned DIFF_ADDR_W = 9;
    localparam int unsigned HYBRID_ENTRIES = 7;
    localparam int unsigned PIVOT_VALID_LSB = 0;
    localparam int unsigned PIVOT_ROWS_LSB = 5;
    localparam int unsigned PIVOT_COLS_LSB = 50;
    localparam int unsigned ROW_GT_LSB = 75;
    localparam int unsigned COL_GT_LSB = 90;
    localparam int unsigned HYBRID_VALID_LSB = 105;
    localparam int unsigned HYBRID_POINTER_LSB = 112;
    localparam int unsigned HYBRID_DESCRIPTOR_LSB = 133;
    localparam int unsigned HYBRID_DIFFERING_LSB = 140;
    localparam int unsigned OVERFLOW_LSB = 203;

    logic [203:0] baseline_summary_q [0:3];
    logic [1:0] baseline_current_sa;
    logic [2:0] baseline_current_config;
    logic [203:0] baseline_summary;
    logic [9:0] baseline_bitmap;
    logic [3:0] baseline_pattern;
    logic baseline_solution;
    logic baseline_repairable;
    logic [1:0] unused_owner;
    logic [3:0] unused_canonical_released;
    logic [3:0] unused_canonical_borrowed;
    logic [7:0] unused_canonical_borrower;

    always_ff @(posedge clk_i) begin
        if (!rst_ni || overlap_start_i) begin
            baseline_summary_q[0] <= '0;
            baseline_summary_q[1] <= '0;
            baseline_summary_q[2] <= '0;
            baseline_summary_q[3] <= '0;
        end else if (summary_update_valid_i) begin
            baseline_summary_q[summary_update_sa_i] <= summary_payload_i;
        end
    end

    assign baseline_summary = baseline_summary_q[baseline_current_sa];

    recam_shared_config_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W),
        .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) baseline_analyzer (
        .config_id_i(baseline_current_config),
        .pivot_valid_i(baseline_summary[PIVOT_VALID_LSB +: 5]),
        .pivot_rows_flat_i(baseline_summary[PIVOT_ROWS_LSB +: 45]),
        .pivot_cols_flat_i(baseline_summary[PIVOT_COLS_LSB +: 25]),
        .row_gt1_i(baseline_summary[ROW_GT_LSB +: 5]),
        .row_gt2_i(baseline_summary[ROW_GT_LSB + 5 +: 5]),
        .row_gt3_i(baseline_summary[ROW_GT_LSB + 10 +: 5]),
        .col_gt1_i(baseline_summary[COL_GT_LSB +: 5]),
        .col_gt2_i(baseline_summary[COL_GT_LSB + 5 +: 5]),
        .col_gt3_i(baseline_summary[COL_GT_LSB + 10 +: 5]),
        .hybrid_valid_i(baseline_summary[HYBRID_VALID_LSB +: 7]),
        .hybrid_pointer_flat_i(
            baseline_summary[HYBRID_POINTER_LSB +: 21]),
        .hybrid_descriptor_i(
            baseline_summary[HYBRID_DESCRIPTOR_LSB +: 7]),
        .hybrid_differing_flat_i(
            baseline_summary[HYBRID_DIFFERING_LSB +: 63]),
        .conventional_overflow_i(baseline_summary[OVERFLOW_LSB]),
        .candidate_valid_o(baseline_bitmap),
        .pattern_id_o(baseline_pattern),
        .solution_valid_o(baseline_solution),
        .repairable_o(baseline_repairable),
        .dictionary_overflow_o()
    );

    recam_dss_v2_early_core baseline (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(baseline_start_i),
        .candidate_solution_valid_i(baseline_solution),
        .candidate_repairable_i(baseline_repairable),
        .candidate_pattern_id_i(baseline_pattern),
        .current_config_id_o(baseline_current_config),
        .current_sa_o(baseline_current_sa),
        .busy_o(baseline_busy_o),
        .done_o(baseline_done_o),
        .group_repairable_o(baseline_group_repairable_o),
        .sa_commit_valid_o(baseline_sa_commit_valid_o),
        .ledger_released_borrower_o(baseline_ledger_o),
        .selected_config_flat_o(baseline_selected_config_flat_o),
        .selected_pattern_flat_o(baseline_selected_pattern_flat_o),
        .selected_donor_flat_o(baseline_selected_donor_flat_o),
        .borrow_flat_o(baseline_borrow_flat_o),
        .release_flat_o(baseline_release_flat_o),
        .failure_position_o(baseline_failure_position_o),
        .canonical_released_o(),
        .canonical_borrowed_o(),
        .canonical_borrower_id_flat_o()
    );

    recam_dss_v2_early_overlap_core overlap (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(overlap_start_i),
        .summary_update_valid_i(summary_update_valid_i),
        .summary_update_sa_i(summary_update_sa_i),
        .summary_payload_i(summary_payload_i),
        .test_done_valid_i(test_done_valid_i),
        .test_done_sa_i(test_done_sa_i),
        .busy_o(overlap_busy_o),
        .done_o(overlap_done_o),
        .group_repairable_o(overlap_group_repairable_o),
        .sa_commit_valid_o(overlap_sa_commit_valid_o),
        .ledger_released_borrower_o(overlap_ledger_o),
        .selected_config_flat_o(overlap_selected_config_flat_o),
        .selected_pattern_flat_o(overlap_selected_pattern_flat_o),
        .selected_donor_flat_o(overlap_selected_donor_flat_o),
        .borrow_flat_o(overlap_borrow_flat_o),
        .release_flat_o(overlap_release_flat_o),
        .failure_position_o(overlap_failure_position_o),
        .summary_update_event_o(overlap_summary_update_event_o),
        .latest_candidate_ready_o(overlap_latest_candidate_ready_o),
        .test_done_event_o(overlap_test_done_event_o),
        .final_ledger_decision_ready_o(overlap_final_ledger_decision_ready_o),
        .analyzer_owner_sa_o(unused_owner),
        .analysis_valid_o(overlap_analysis_valid_o),
        .fault_generation_flat_o(overlap_fault_generation_o),
        .analysis_generation_flat_o(overlap_analysis_generation_o),
        .canonical_released_o(unused_canonical_released),
        .canonical_borrowed_o(unused_canonical_borrowed),
        .canonical_borrower_id_flat_o(unused_canonical_borrower)
    );
endmodule

`default_nettype wire
