`default_nettype none
/* verilator lint_off PINCONNECTEMPTY */
/* verilator lint_off UNUSED */

// Test-only timing observer.  Both DUT instances are the unmodified
// production V2 tops.  The two overflow inputs model the already-collected
// analyzer state for the current SA; BIST_end remains a testbench timestamp.
module recam_dss_v2_phase4i_rtl_timing_top (
    input wire clk_i, rst_ni, early_start_i, group_start_i,
    input wire early_conventional_overflow_i,
    input wire group_conventional_overflow_i,
    output wire early_busy_o, early_done_o, early_group_repairable_o,
    output wire [3:0] early_sa_commit_valid_o,
    output wire [11:0] early_ledger_o, early_selected_config_o,
    output wire [15:0] early_selected_pattern_o,
    output wire [1:0] early_failure_position_o,
    output wire [1:0] early_current_sa_o,
    output wire [2:0] early_current_config_o,
    output wire group_busy_o, group_done_o, group_group_repairable_o,
    output wire [3:0] group_sa_commit_valid_o,
    output wire [11:0] group_ledger_o, group_selected_config_o,
    output wire [15:0] group_selected_pattern_o,
    output wire [1:0] group_failure_position_o,
    output wire [1:0] group_current_sa_o, group_current_slot_o,
    output wire [2:0] group_current_config_o
);
    recam_dss_v2_early_top early (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(early_start_i),
        .pivot_valid_i(5'd0), .pivot_rows_flat_i(45'd0),
        .pivot_cols_flat_i(25'd0), .row_gt1_i(5'd0), .row_gt2_i(5'd0),
        .row_gt3_i(5'd0), .col_gt1_i(5'd0), .col_gt2_i(5'd0),
        .col_gt3_i(5'd0), .hybrid_valid_i(7'd0),
        .hybrid_pointer_flat_i(21'd0), .hybrid_descriptor_i(7'd0),
        .hybrid_differing_flat_i(63'd0),
        .conventional_overflow_i(early_conventional_overflow_i),
        .busy_o(early_busy_o), .done_o(early_done_o),
        .group_repairable_o(early_group_repairable_o),
        .sa_commit_valid_o(early_sa_commit_valid_o),
        .ledger_released_borrower_o(early_ledger_o),
        .selected_config_flat_o(early_selected_config_o),
        .selected_pattern_flat_o(early_selected_pattern_o),
        .selected_donor_flat_o(), .borrow_flat_o(), .release_flat_o(),
        .failure_position_o(early_failure_position_o)
    );

    recam_dss_v2_group_top group (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(group_start_i),
        .pivot_valid_i(5'd0), .pivot_rows_flat_i(45'd0),
        .pivot_cols_flat_i(25'd0), .row_gt1_i(5'd0), .row_gt2_i(5'd0),
        .row_gt3_i(5'd0), .col_gt1_i(5'd0), .col_gt2_i(5'd0),
        .col_gt3_i(5'd0), .hybrid_valid_i(7'd0),
        .hybrid_pointer_flat_i(21'd0), .hybrid_descriptor_i(7'd0),
        .hybrid_differing_flat_i(63'd0),
        .conventional_overflow_i(group_conventional_overflow_i),
        .busy_o(group_busy_o), .done_o(group_done_o),
        .group_repairable_o(group_group_repairable_o),
        .sa_commit_valid_o(group_sa_commit_valid_o),
        .ledger_released_borrower_o(group_ledger_o),
        .selected_config_flat_o(group_selected_config_o),
        .selected_pattern_flat_o(group_selected_pattern_o),
        .selected_donor_flat_o(), .borrow_flat_o(), .release_flat_o(),
        .failure_position_o(group_failure_position_o)
    );

    // These are observation-only hierarchical references to the production
    // cores; they add no DUT datapath or control behavior.
    assign early_current_sa_o = early.c.current_sa_o;
    assign early_current_config_o = early.c.current_config_id_o;
    assign group_current_sa_o = group.core.current_sa_o;
    assign group_current_slot_o = group.core.current_slot_o;
    assign group_current_config_o = group.core.current_config_id_o;
endmodule

`default_nettype wire
/* verilator lint_on UNUSED */
/* verilator lint_on PINCONNECTEMPTY */
