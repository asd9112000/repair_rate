`default_nettype none

// Final Model-B2 EARLY top.  One 260-bit retained shared-collector summary
// is supplied for each SA; the controller selects exactly one bank at a time.
module recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 11
) (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic start_i,
    input  logic [1039:0] shared_summary_flat_i,
    output logic busy_o,
    output logic done_o,
    output logic group_repairable_o,
    output logic [3:0] sa_commit_valid_o,
    output logic [11:0] selected_config_flat_o,
    output logic [15:0] selected_pattern_flat_o,
    output logic [3:0] borrow_flat_o,
    output logic [3:0] common_spare_available_o,
    output logic [1:0] failure_position_o
);
    // Model-B2 retains eleven Hybrid entries.  Seven was the historical
    // standalone analyzer input capacity and is deliberately not restored.
    localparam integer SUMMARY_W = 260;
    localparam integer PIVOT_VALID_LSB = 0;
    localparam integer PIVOT_ROWS_LSB = 5;
    localparam integer PIVOT_COLS_LSB = 50;
    localparam integer ROW_GT_LSB = 75;
    localparam integer COL_GT_LSB = 90;
    localparam integer HYBRID_VALID_LSB = 105;
    localparam integer HYBRID_POINTER_LSB = 116;
    localparam integer HYBRID_DESCRIPTOR_LSB = 149;
    localparam integer HYBRID_DIFFERING_LSB = 160;
    localparam integer OVERFLOW_LSB = 259;

    logic [1:0] current_sa;
    /* verilator lint_off UNUSED */
    logic [1:0] current_slot;
    /* verilator lint_on UNUSED */
    logic [2:0] current_config_id;
    logic [SUMMARY_W-1:0] current_summary;
    logic [9:0] candidate_valid_bitmap;
    logic [3:0] candidate_pattern_id;
    logic solution_valid;
    logic repairable;
    logic dictionary_overflow;

    always_comb begin
        case (current_sa)
            2'd0: current_summary = shared_summary_flat_i[0 +: SUMMARY_W];
            2'd1: current_summary = shared_summary_flat_i[SUMMARY_W +: SUMMARY_W];
            2'd2: current_summary = shared_summary_flat_i[2 * SUMMARY_W +: SUMMARY_W];
            default: current_summary = shared_summary_flat_i[3 * SUMMARY_W +: SUMMARY_W];
        endcase
    end

    recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core core (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(start_i),
        .candidate_valid_i(solution_valid && repairable &&
                           (|candidate_valid_bitmap) && !dictionary_overflow),
        .candidate_pattern_id_i(candidate_pattern_id),
        .current_sa_o(current_sa),
        .current_slot_o(current_slot),
        .current_config_id_o(current_config_id),
        .busy_o(busy_o),
        .done_o(done_o),
        .group_repairable_o(group_repairable_o),
        .sa_commit_valid_o(sa_commit_valid_o),
        .selected_config_flat_o(selected_config_flat_o),
        .selected_pattern_flat_o(selected_pattern_flat_o),
        .borrow_flat_o(borrow_flat_o),
        .common_spare_available_o(common_spare_available_o),
        .failure_position_o(failure_position_o)
    );

    recam_shared_config_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W),
        .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) analyzer (
        .config_id_i(current_config_id),
        .pivot_valid_i(current_summary[PIVOT_VALID_LSB +: 5]),
        .pivot_rows_flat_i(current_summary[PIVOT_ROWS_LSB +: 5 * ROW_ADDR_W]),
        .pivot_cols_flat_i(current_summary[PIVOT_COLS_LSB +: 5 * COL_ADDR_W]),
        .row_gt1_i(current_summary[ROW_GT_LSB +: 5]),
        .row_gt2_i(current_summary[ROW_GT_LSB + 5 +: 5]),
        .row_gt3_i(current_summary[ROW_GT_LSB + 10 +: 5]),
        .col_gt1_i(current_summary[COL_GT_LSB +: 5]),
        .col_gt2_i(current_summary[COL_GT_LSB + 5 +: 5]),
        .col_gt3_i(current_summary[COL_GT_LSB + 10 +: 5]),
        .hybrid_valid_i(current_summary[HYBRID_VALID_LSB +: HYBRID_ENTRIES]),
        .hybrid_pointer_flat_i(
            current_summary[HYBRID_POINTER_LSB +: HYBRID_ENTRIES * 3]),
        .hybrid_descriptor_i(
            current_summary[HYBRID_DESCRIPTOR_LSB +: HYBRID_ENTRIES]),
        .hybrid_differing_flat_i(
            current_summary[HYBRID_DIFFERING_LSB +: HYBRID_ENTRIES * DIFF_ADDR_W]),
        .conventional_overflow_i(current_summary[OVERFLOW_LSB]),
        .candidate_valid_o(candidate_valid_bitmap),
        .pattern_id_o(candidate_pattern_id),
        .solution_valid_o(solution_valid),
        .repairable_o(repairable),
        .dictionary_overflow_o(dictionary_overflow)
    );
endmodule

`default_nettype wire
