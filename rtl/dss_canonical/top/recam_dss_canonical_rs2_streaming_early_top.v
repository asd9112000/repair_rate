`default_nettype none

// Reused from: unchanged recam_shared_config_analyzer RS2 implementation.
// Adapted from: historical recam_dss_v2_early_top analyzer binding.
// New logic: none beyond the canonical controller instance and width bridge.
// Contract: RS2 / NORMALIZED_STREAMING_EARLY / STREAMING.
module recam_dss_canonical_rs2_streaming_early_top #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 7
) (
    input  wire                              clk_i,
    input  wire                              rst_ni,
    input  wire                              start_i,
    input  wire [4:0]                        pivot_valid_i,
    input  wire [5*ROW_ADDR_W-1:0]           pivot_rows_flat_i,
    input  wire [5*COL_ADDR_W-1:0]           pivot_cols_flat_i,
    input  wire [4:0]                        row_gt1_i,
    input  wire [4:0]                        row_gt2_i,
    input  wire [4:0]                        row_gt3_i,
    input  wire [4:0]                        col_gt1_i,
    input  wire [4:0]                        col_gt2_i,
    input  wire [4:0]                        col_gt3_i,
    input  wire [HYBRID_ENTRIES-1:0]         hybrid_valid_i,
    input  wire [HYBRID_ENTRIES*3-1:0]       hybrid_pointer_flat_i,
    input  wire [HYBRID_ENTRIES-1:0]         hybrid_descriptor_i,
    input  wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,
    input  wire                              conventional_overflow_i,
    output wire                              busy_o,
    output wire                              done_o,
    output wire                              group_repairable_o,
    output wire [1:0]                        current_sa_o,
    output wire [1:0]                        current_action_o,
    output wire [2:0]                        current_config_id_o,
    output wire [3:0]                        sa_commit_valid_o,
    output wire [7:0]                        selected_action_flat_o,
    output wire [11:0]                       selected_config_flat_o,
    output wire [23:0]                       selected_pattern_flat_o,
    output wire [7:0]                        selected_donor_flat_o,
    output wire [3:0]                        borrow_flat_o,
    output wire [3:0]                        release_flat_o,
    output wire [1:0]                        failure_position_o,
    output wire [3:0]                        resource_released_o,
    output wire [3:0]                        resource_borrowed_o,
    output wire [7:0]                        borrower_id_flat_o,
    output wire [11:0]                       ledger_released_borrower_o
);

    /* verilator lint_off UNUSED */
    wire [9:0] candidate_valid_bitmap;
    wire [3:0] analyzer_pattern_id;
    wire analyzer_solution_valid;
    wire analyzer_repairable;
    wire dictionary_overflow;
    /* verilator lint_on UNUSED */

    recam_shared_config_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W),
        .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) analyzer (
        .config_id_i(current_config_id_o),
        .pivot_valid_i(pivot_valid_i),
        .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(pivot_cols_flat_i),
        .row_gt1_i(row_gt1_i),
        .row_gt2_i(row_gt2_i),
        .row_gt3_i(row_gt3_i),
        .col_gt1_i(col_gt1_i),
        .col_gt2_i(col_gt2_i),
        .col_gt3_i(col_gt3_i),
        .hybrid_valid_i(hybrid_valid_i),
        .hybrid_pointer_flat_i(hybrid_pointer_flat_i),
        .hybrid_descriptor_i(hybrid_descriptor_i),
        .hybrid_differing_flat_i(hybrid_differing_flat_i),
        .conventional_overflow_i(conventional_overflow_i),
        .candidate_valid_o(candidate_valid_bitmap),
        .pattern_id_o(analyzer_pattern_id),
        .solution_valid_o(analyzer_solution_valid),
        .repairable_o(analyzer_repairable),
        .dictionary_overflow_o(dictionary_overflow)
    );

    recam_dss_canonical_streaming_early_core #(
        .RESOURCE_POINT(2),
        .PATTERN_ID_W(6)
    ) policy_core (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(start_i),
        .candidate_solution_valid_i(analyzer_solution_valid),
        .candidate_repairable_i(analyzer_repairable),
        .candidate_pattern_id_i({2'b00, analyzer_pattern_id}),
        .current_sa_o(current_sa_o),
        .current_action_o(current_action_o),
        .current_config_id_o(current_config_id_o),
        .busy_o(busy_o),
        .done_o(done_o),
        .group_repairable_o(group_repairable_o),
        .sa_commit_valid_o(sa_commit_valid_o),
        .selected_action_flat_o(selected_action_flat_o),
        .selected_config_flat_o(selected_config_flat_o),
        .selected_pattern_flat_o(selected_pattern_flat_o),
        .selected_donor_flat_o(selected_donor_flat_o),
        .borrow_flat_o(borrow_flat_o),
        .release_flat_o(release_flat_o),
        .failure_position_o(failure_position_o),
        .resource_released_o(resource_released_o),
        .resource_borrowed_o(resource_borrowed_o),
        .borrower_id_flat_o(borrower_id_flat_o),
        .ledger_released_borrower_o(ledger_released_borrower_o)
    );

endmodule

`default_nettype wire
