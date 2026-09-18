`default_nettype none

// SYN-C integrated boundary: four retained 1x4 collector snapshots, one
// shared analyzer producer, streaming single-hop policy, and persistent rows.
module recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_top #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 10
) (
    input wire clk_i, input wire rst_ni, input wire start_i,
    input wire [4*6-1:0] pivot_valid_flat_i,
    input wire [4*6*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input wire [4*6*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input wire [4*6-1:0] row_gt1_flat_i, input wire [4*6-1:0] row_gt2_flat_i,
    input wire [4*6-1:0] row_gt3_flat_i, input wire [4*6-1:0] row_gt4_flat_i,
    input wire [4*6-1:0] col_gt1_flat_i, input wire [4*6-1:0] col_gt2_flat_i,
    input wire [4*6-1:0] col_gt3_flat_i, input wire [4*6-1:0] col_gt4_flat_i,
    input wire [4*HYBRID_ENTRIES-1:0] hybrid_valid_flat_i,
    input wire [4*HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input wire [4*HYBRID_ENTRIES-1:0] hybrid_descriptor_flat_i,
    input wire [4*HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,
    input wire [3:0] conventional_overflow_i,
    output wire busy_o, output wire done_o, output wire group_repairable_o,
    output wire [3:0] selected_valid_o, output wire [7:0] selected_attempt_flat_o,
    output wire [15:0] selected_pattern_flat_o, output wire [11:0] selected_used_rows_flat_o,
    output wire [7:0] selected_used_cols_flat_o, output wire [15:0] selected_donor_flat_o,
    output wire [23:0] row_assignment_flat_o, output wire [1:0] failure_position_o,
    output wire [7:0] candidate_evaluations_o, output wire [179:0] candidate_valid_debug_o
);
    localparam [1:0] STATE_IDLE = 2'd0, STATE_PRODUCER_START = 2'd1,
                     STATE_PRODUCER_WAIT = 2'd2, STATE_POLICY_START = 2'd3;
    reg [1:0] state_q;
    reg producer_start_q, policy_start_q;
    reg [4*6-1:0] pivot_valid_snapshot_q;
    reg [4*6*ROW_ADDR_W-1:0] pivot_rows_snapshot_q;
    reg [4*6*COL_ADDR_W-1:0] pivot_cols_snapshot_q;
    reg [4*6-1:0] row_gt1_snapshot_q, row_gt2_snapshot_q, row_gt3_snapshot_q, row_gt4_snapshot_q;
    reg [4*6-1:0] col_gt1_snapshot_q, col_gt2_snapshot_q, col_gt3_snapshot_q, col_gt4_snapshot_q;
    reg [4*HYBRID_ENTRIES-1:0] hybrid_valid_snapshot_q, hybrid_descriptor_snapshot_q;
    reg [4*HYBRID_ENTRIES*3-1:0] hybrid_pointer_snapshot_q;
    reg [4*HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_snapshot_q;
    reg [3:0] overflow_snapshot_q;
    wire producer_busy, producer_done, policy_busy, policy_done;
    wire [179:0] candidate_valid;
    wire [539:0] candidate_rows;
    wire [359:0] candidate_cols;
    /* verilator lint_off UNUSED */
    wire [1:0] producer_active_sa, producer_active_attempt;
    /* verilator lint_on UNUSED */

    recam_dss_line1x4_rs2_cs2_m1_normalized_candidate_producer #(
        .ROW_ADDR_W(ROW_ADDR_W), .COL_ADDR_W(COL_ADDR_W), .DIFF_ADDR_W(DIFF_ADDR_W), .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) producer (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(producer_start_q),
        .pivot_valid_flat_i(pivot_valid_snapshot_q), .pivot_rows_flat_i(pivot_rows_snapshot_q), .pivot_cols_flat_i(pivot_cols_snapshot_q),
        .row_gt1_flat_i(row_gt1_snapshot_q), .row_gt2_flat_i(row_gt2_snapshot_q), .row_gt3_flat_i(row_gt3_snapshot_q), .row_gt4_flat_i(row_gt4_snapshot_q),
        .col_gt1_flat_i(col_gt1_snapshot_q), .col_gt2_flat_i(col_gt2_snapshot_q), .col_gt3_flat_i(col_gt3_snapshot_q), .col_gt4_flat_i(col_gt4_snapshot_q),
        .hybrid_valid_flat_i(hybrid_valid_snapshot_q), .hybrid_pointer_flat_i(hybrid_pointer_snapshot_q),
        .hybrid_descriptor_flat_i(hybrid_descriptor_snapshot_q), .hybrid_differing_flat_i(hybrid_differing_snapshot_q),
        .conventional_overflow_i(overflow_snapshot_q), .busy_o(producer_busy), .done_o(producer_done),
        .candidate_valid_o(candidate_valid), .candidate_used_rows_flat_o(candidate_rows), .candidate_used_cols_flat_o(candidate_cols),
        .active_sa_o(producer_active_sa), .active_attempt_o(producer_active_attempt)
    );
    recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_core policy (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(policy_start_q), .candidate_valid_i(candidate_valid),
        .candidate_used_rows_flat_i(candidate_rows), .candidate_used_cols_flat_i(candidate_cols),
        .busy_o(policy_busy), .done_o(policy_done), .group_repairable_o(group_repairable_o),
        .selected_valid_o(selected_valid_o), .selected_attempt_flat_o(selected_attempt_flat_o),
        .selected_pattern_flat_o(selected_pattern_flat_o), .selected_used_rows_flat_o(selected_used_rows_flat_o),
        .selected_used_cols_flat_o(selected_used_cols_flat_o), .selected_donor_flat_o(selected_donor_flat_o),
        .row_assignment_flat_o(row_assignment_flat_o), .failure_position_o(failure_position_o),
        .candidate_evaluations_o(candidate_evaluations_o)
    );
    always @(posedge clk_i) begin
        producer_start_q <= 1'b0; policy_start_q <= 1'b0;
        if (!rst_ni) begin
            state_q <= STATE_IDLE;
            pivot_valid_snapshot_q <= '0; pivot_rows_snapshot_q <= '0; pivot_cols_snapshot_q <= '0;
            row_gt1_snapshot_q <= '0; row_gt2_snapshot_q <= '0; row_gt3_snapshot_q <= '0; row_gt4_snapshot_q <= '0;
            col_gt1_snapshot_q <= '0; col_gt2_snapshot_q <= '0; col_gt3_snapshot_q <= '0; col_gt4_snapshot_q <= '0;
            hybrid_valid_snapshot_q <= '0; hybrid_pointer_snapshot_q <= '0; hybrid_descriptor_snapshot_q <= '0;
            hybrid_differing_snapshot_q <= '0; overflow_snapshot_q <= '0;
        end else case (state_q)
            STATE_IDLE: if (start_i) begin
                pivot_valid_snapshot_q <= pivot_valid_flat_i; pivot_rows_snapshot_q <= pivot_rows_flat_i; pivot_cols_snapshot_q <= pivot_cols_flat_i;
                row_gt1_snapshot_q <= row_gt1_flat_i; row_gt2_snapshot_q <= row_gt2_flat_i; row_gt3_snapshot_q <= row_gt3_flat_i; row_gt4_snapshot_q <= row_gt4_flat_i;
                col_gt1_snapshot_q <= col_gt1_flat_i; col_gt2_snapshot_q <= col_gt2_flat_i; col_gt3_snapshot_q <= col_gt3_flat_i; col_gt4_snapshot_q <= col_gt4_flat_i;
                hybrid_valid_snapshot_q <= hybrid_valid_flat_i; hybrid_pointer_snapshot_q <= hybrid_pointer_flat_i;
                hybrid_descriptor_snapshot_q <= hybrid_descriptor_flat_i; hybrid_differing_snapshot_q <= hybrid_differing_flat_i;
                overflow_snapshot_q <= conventional_overflow_i; state_q <= STATE_PRODUCER_START;
            end
            STATE_PRODUCER_START: begin producer_start_q <= 1'b1; state_q <= STATE_PRODUCER_WAIT; end
            STATE_PRODUCER_WAIT: if (producer_done) begin policy_start_q <= 1'b1; state_q <= STATE_POLICY_START; end
            STATE_POLICY_START: if (policy_done) state_q <= STATE_IDLE;
        endcase
    end
    assign busy_o = state_q != STATE_IDLE || producer_busy || policy_busy;
    assign done_o = policy_done;
    assign candidate_valid_debug_o = candidate_valid;
endmodule

`default_nettype wire
