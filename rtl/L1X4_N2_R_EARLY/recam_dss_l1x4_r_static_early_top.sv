`default_nettype none

// L1X4_R-matched EARLY top: one unexpanded 204-bit/7-Hybrid analyzer boundary.
module recam_dss_l1x4_r_static_early_top #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer PHYS_COL_ADDR_W = 13,
    parameter integer WORD_COL_ADDR_W = 5,
    parameter integer HYBRID_LINE_ADDR_W = 13,
    parameter integer HYBRID_ENTRIES = 7
) (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic start_i,
    input  logic [4:0] pivot_valid_i,
    input  logic [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input  logic [5*PHYS_COL_ADDR_W-1:0] pivot_cols_flat_i,
    input  logic [4:0] row_gt1_i,
    input  logic [4:0] row_gt2_i,
    input  logic [4:0] row_gt3_i,
    input  logic [4:0] col_gt1_i,
    input  logic [4:0] col_gt2_i,
    input  logic [4:0] col_gt3_i,
    input  logic [HYBRID_ENTRIES-1:0] hybrid_valid_i,
    input  logic [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input  logic [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,
    input  logic [HYBRID_ENTRIES*HYBRID_LINE_ADDR_W-1:0] hybrid_differing_flat_i,
    input  logic conventional_overflow_i,
    output logic busy_o,
    output logic done_o,
    output logic group_repairable_o,
    output logic [3:0] sa_commit_valid_o,
    output logic [11:0] selected_config_flat_o,
    output logic [15:0] selected_pattern_flat_o,
    output logic [1:0] failure_position_o,
    output logic [259:0] final_repair_address_flat_o,
    output logic [19:0] final_repair_is_row_flat_o,
    output logic [19:0] final_repair_line_valid_flat_o
);
    logic [1:0] current_sa;
    logic [2:0] current_config_id;
    logic [3:0] candidate_pattern_id;
    logic solution_valid;
    logic repairable;
    logic selected_commit;

    /* verilator lint_off PINCONNECTEMPTY */
    recam_dss_l1x4_r_static_early_core core (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(start_i),
        .candidate_valid_i(solution_valid && repairable),
        .candidate_pattern_id_i(candidate_pattern_id),
        .current_sa_o(current_sa),
        .current_slot_o(),
        .current_config_id_o(current_config_id),
        .selected_commit_o(selected_commit),
        .busy_o(busy_o),
        .done_o(done_o),
        .group_repairable_o(group_repairable_o),
        .sa_commit_valid_o(sa_commit_valid_o),
        .selected_config_flat_o(selected_config_flat_o),
        .selected_pattern_flat_o(selected_pattern_flat_o),
        .failure_position_o(failure_position_o)
    );

    recam_shared_config_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W),
        .PHYS_COL_ADDR_W(PHYS_COL_ADDR_W),
        .WORD_COL_ADDR_W(WORD_COL_ADDR_W),
        .HYBRID_LINE_ADDR_W(HYBRID_LINE_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) analyzer (
        .config_id_i(current_config_id),
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
        .pattern_id_o(candidate_pattern_id),
        .solution_valid_o(solution_valid),
        .repairable_o(repairable),
        .dictionary_overflow_o()
    );
    dss_early_selected_address_regs #(
        .ROW_ADDR_W(ROW_ADDR_W),
        .PHYS_COL_ADDR_W(PHYS_COL_ADDR_W)
    ) selected_address_regs (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .commit_enable_i(selected_commit),
        .commit_sa_i(current_sa),
        .selected_config_i(current_config_id),
        .selected_pattern_id_i(candidate_pattern_id),
        .pivot_rows_flat_i(pivot_rows_flat_i),
        .pivot_cols_flat_i(pivot_cols_flat_i),
        .final_repair_address_flat_o(final_repair_address_flat_o),
        .final_repair_is_row_flat_o(final_repair_is_row_flat_o),
        .final_repair_line_valid_flat_o(final_repair_line_valid_flat_o)
    );

    /* verilator lint_on PINCONNECTEMPTY */
endmodule

`default_nettype wire
