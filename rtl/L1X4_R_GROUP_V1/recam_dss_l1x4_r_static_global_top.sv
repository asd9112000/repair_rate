`default_nettype none

// Integrated L1X4_R boundary: one shared analyzer feeds the registered 16-slot
// store, followed by fixed-priority static GLOBAL selection.
module recam_dss_l1x4_r_static_global_top #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 7
) (
    input wire clk_i,
    input wire rst_ni,
    input wire start_i,
    input wire [4:0] pivot_valid_i,
    input wire [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input wire [5*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input wire [4:0] row_gt1_i,
    input wire [4:0] row_gt2_i,
    input wire [4:0] row_gt3_i,
    input wire [4:0] col_gt1_i,
    input wire [4:0] col_gt2_i,
    input wire [4:0] col_gt3_i,
    input wire [HYBRID_ENTRIES-1:0] hybrid_valid_i,
    input wire [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input wire [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,
    input wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,
    input wire conventional_overflow_i,
    output wire busy_o,
    output wire done_o,
    output wire group_repairable_o,
    output wire [3:0] sa_commit_valid_o,
    output wire [11:0] ledger_released_borrower_o,
    output wire [11:0] selected_config_flat_o,
    output wire [15:0] selected_pattern_flat_o,
    output wire [7:0] selected_donor_flat_o,
    output wire [3:0] borrow_flat_o,
    output wire [3:0] release_flat_o,
    output wire [1:0] failure_position_o
);
    wire [2:0] current_config;
    wire [3:0] pattern_id;
    wire solution_valid;
    wire repairable;

    /* verilator lint_off PINCONNECTEMPTY */
    recam_dss_l1x4_r_static_global_core core (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(start_i),
        .candidate_valid_i(solution_valid && repairable),
        .candidate_pattern_id_i(pattern_id),
        .collection_active_o(),
        .allocation_active_o(),
        .current_sa_o(),
        .current_slot_o(),
        .current_config_id_o(current_config),
        .candidate_store_image_o(),
        .busy_o(busy_o),
        .done_o(done_o),
        .group_repairable_o(group_repairable_o),
        .sa_commit_valid_o(sa_commit_valid_o),
        .ledger_released_borrower_o(ledger_released_borrower_o),
        .selected_config_flat_o(selected_config_flat_o),
        .selected_pattern_flat_o(selected_pattern_flat_o),
        .selected_donor_flat_o(selected_donor_flat_o),
        .borrow_flat_o(borrow_flat_o),
        .release_flat_o(release_flat_o),
        .failure_position_o(failure_position_o)
    );

    recam_shared_config_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W),
        .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) analyzer (
        .config_id_i(current_config),
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
        .candidate_valid_o(),
        .pattern_id_o(pattern_id),
        .solution_valid_o(solution_valid),
        .repairable_o(repairable),
        .dictionary_overflow_o()
    );
    /* verilator lint_on PINCONNECTEMPTY */
endmodule
`default_nettype wire
