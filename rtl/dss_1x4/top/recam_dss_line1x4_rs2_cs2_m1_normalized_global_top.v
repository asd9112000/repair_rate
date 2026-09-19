`default_nettype none

// SYN-D integrated boundary: four 1x4 snapshots, reused ten-request producer,
// private GLOBAL search, shadow-staged atomic group commit, and persistent rows.
module recam_dss_line1x4_rs2_cs2_m1_normalized_global_top #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 10,
    parameter integer ENABLE_OPT1_CLASS_COLLAPSE = 0
) (
    input wire clk_i,
    input wire rst_ni,
    input wire start_i,
    input wire [4*6-1:0] pivot_valid_flat_i,
    input wire [4*6*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input wire [4*6*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input wire [4*6-1:0] row_gt1_flat_i,
    input wire [4*6-1:0] row_gt2_flat_i,
    input wire [4*6-1:0] row_gt3_flat_i,
    input wire [4*6-1:0] row_gt4_flat_i,
    input wire [4*6-1:0] col_gt1_flat_i,
    input wire [4*6-1:0] col_gt2_flat_i,
    input wire [4*6-1:0] col_gt3_flat_i,
    input wire [4*6-1:0] col_gt4_flat_i,
    input wire [4*HYBRID_ENTRIES-1:0] hybrid_valid_flat_i,
    input wire [4*HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input wire [4*HYBRID_ENTRIES-1:0] hybrid_descriptor_flat_i,
    input wire [4*HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,
    input wire [3:0] conventional_overflow_i,
    output wire busy_o,
    output wire done_o,
    output wire group_repairable_o,
    output wire [3:0] selected_valid_o,
    output wire [7:0] selected_attempt_flat_o,
    output wire [15:0] selected_pattern_flat_o,
    output wire [11:0] selected_used_rows_flat_o,
    output wire [7:0] selected_used_cols_flat_o,
    output wire [15:0] selected_donor_flat_o,
    output wire [23:0] row_assignment_flat_o,
    output wire [1:0] selected_borrow_count_o,
    output wire [4:0] selected_used_rows_total_o,
    output wire [31:0] candidate_evaluations_o,
    output wire [31:0] dfs_cycles_o,
    output wire [179:0] candidate_valid_debug_o,
    output wire atomic_commit_error_o
);
    localparam [2:0] STATE_IDLE = 3'd0;
    localparam [2:0] STATE_PRODUCER_START = 3'd1;
    localparam [2:0] STATE_PRODUCER_WAIT = 3'd2;
    localparam [2:0] STATE_SEARCH_START = 3'd3;
    localparam [2:0] STATE_SEARCH_WAIT = 3'd4;
    localparam [2:0] STATE_COMMIT_START = 3'd5;
    localparam [2:0] STATE_COMMIT_WAIT = 3'd6;

    reg [2:0] state_q;
    reg producer_start_q;
    reg search_start_q;
    reg commit_start_q;
    reg done_q;
    reg repairable_q;
    reg [3:0] result_valid_q;
    reg [7:0] result_attempt_q;
    reg [15:0] result_pattern_q;
    reg [11:0] result_used_rows_q;
    reg [7:0] result_used_cols_q;
    reg [15:0] result_donor_q;
    reg [1:0] result_borrow_count_q;
    reg [4:0] result_used_rows_total_q;
    reg [4*6-1:0] pivot_valid_snapshot_q;
    reg [4*6*ROW_ADDR_W-1:0] pivot_rows_snapshot_q;
    reg [4*6*COL_ADDR_W-1:0] pivot_cols_snapshot_q;
    reg [4*6-1:0] row_gt1_snapshot_q;
    reg [4*6-1:0] row_gt2_snapshot_q;
    reg [4*6-1:0] row_gt3_snapshot_q;
    reg [4*6-1:0] row_gt4_snapshot_q;
    reg [4*6-1:0] col_gt1_snapshot_q;
    reg [4*6-1:0] col_gt2_snapshot_q;
    reg [4*6-1:0] col_gt3_snapshot_q;
    reg [4*6-1:0] col_gt4_snapshot_q;
    reg [4*HYBRID_ENTRIES-1:0] hybrid_valid_snapshot_q;
    reg [4*HYBRID_ENTRIES*3-1:0] hybrid_pointer_snapshot_q;
    reg [4*HYBRID_ENTRIES-1:0] hybrid_descriptor_snapshot_q;
    reg [4*HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_snapshot_q;
    reg [3:0] overflow_snapshot_q;

    wire producer_busy;
    wire producer_done;
    wire search_busy;
    wire search_done;
    wire search_repairable;
    wire commit_busy;
    wire commit_accepted;
    wire commit_error;
    wire [179:0] candidate_valid;
    wire [539:0] candidate_rows;
    wire [359:0] candidate_cols;
    wire [3:0] search_valid;
    wire [7:0] search_attempt;
    wire [15:0] search_pattern;
    wire [11:0] search_used_rows;
    wire [7:0] search_used_cols;
    wire [15:0] search_donor;
    wire [23:0] search_row_assignment;
    wire [1:0] search_borrow_count;
    wire [4:0] search_used_rows_total;
    wire [23:0] persistent_row_assignment;
    /* verilator lint_off UNUSED */
    wire [1:0] producer_active_sa;
    wire [1:0] producer_active_attempt;
    /* verilator lint_on UNUSED */

    recam_dss_line1x4_rs2_cs2_m1_normalized_candidate_producer #(
        .ROW_ADDR_W(ROW_ADDR_W),
        .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) producer (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(producer_start_q),
        .pivot_valid_flat_i(pivot_valid_snapshot_q),
        .pivot_rows_flat_i(pivot_rows_snapshot_q),
        .pivot_cols_flat_i(pivot_cols_snapshot_q),
        .row_gt1_flat_i(row_gt1_snapshot_q),
        .row_gt2_flat_i(row_gt2_snapshot_q),
        .row_gt3_flat_i(row_gt3_snapshot_q),
        .row_gt4_flat_i(row_gt4_snapshot_q),
        .col_gt1_flat_i(col_gt1_snapshot_q),
        .col_gt2_flat_i(col_gt2_snapshot_q),
        .col_gt3_flat_i(col_gt3_snapshot_q),
        .col_gt4_flat_i(col_gt4_snapshot_q),
        .hybrid_valid_flat_i(hybrid_valid_snapshot_q),
        .hybrid_pointer_flat_i(hybrid_pointer_snapshot_q),
        .hybrid_descriptor_flat_i(hybrid_descriptor_snapshot_q),
        .hybrid_differing_flat_i(hybrid_differing_snapshot_q),
        .conventional_overflow_i(overflow_snapshot_q),
        .busy_o(producer_busy),
        .done_o(producer_done),
        .candidate_valid_o(candidate_valid),
        .candidate_used_rows_flat_o(candidate_rows),
        .candidate_used_cols_flat_o(candidate_cols),
        .active_sa_o(producer_active_sa),
        .active_attempt_o(producer_active_attempt)
    );

    recam_dss_line1x4_rs2_cs2_m1_normalized_global_core #(
        .ENABLE_OPT1_CLASS_COLLAPSE(ENABLE_OPT1_CLASS_COLLAPSE)
    ) search (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(search_start_q),
        .candidate_valid_i(candidate_valid),
        .candidate_used_rows_flat_i(candidate_rows),
        .candidate_used_cols_flat_i(candidate_cols),
        .busy_o(search_busy),
        .search_done_o(search_done),
        .group_repairable_o(search_repairable),
        .selected_valid_o(search_valid),
        .selected_attempt_flat_o(search_attempt),
        .selected_pattern_flat_o(search_pattern),
        .selected_used_rows_flat_o(search_used_rows),
        .selected_used_cols_flat_o(search_used_cols),
        .selected_donor_flat_o(search_donor),
        .selected_row_assignment_flat_o(search_row_assignment),
        .selected_borrow_count_o(search_borrow_count),
        .selected_used_rows_total_o(search_used_rows_total),
        .candidate_evaluations_o(candidate_evaluations_o),
        .dfs_cycles_o(dfs_cycles_o)
    );

    recam_dss_line1x4_rs2_cs2_m1_normalized_global_atomic_group_commit commit (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(commit_start_q),
        .selected_valid_i(search_valid),
        .selected_used_rows_flat_i(search_used_rows),
        .selected_used_cols_flat_i(search_used_cols),
        .selected_donor_flat_i(search_donor),
        .selected_row_assignment_flat_i(search_row_assignment),
        .busy_o(commit_busy),
        .commit_accepted_o(commit_accepted),
        .commit_error_o(commit_error),
        .persistent_row_assignment_flat_o(persistent_row_assignment)
    );

    always @(posedge clk_i) begin
        producer_start_q <= 1'b0;
        search_start_q <= 1'b0;
        commit_start_q <= 1'b0;
        done_q <= 1'b0;
        if (!rst_ni) begin
            state_q <= STATE_IDLE;
            repairable_q <= 1'b0;
            result_valid_q <= 4'd0;
            result_attempt_q <= 8'd0;
            result_pattern_q <= 16'd0;
            result_used_rows_q <= 12'd0;
            result_used_cols_q <= 8'd0;
            result_donor_q <= 16'hffff;
            result_borrow_count_q <= 2'd0;
            result_used_rows_total_q <= 5'd0;
            pivot_valid_snapshot_q <= '0;
            pivot_rows_snapshot_q <= '0;
            pivot_cols_snapshot_q <= '0;
            row_gt1_snapshot_q <= '0;
            row_gt2_snapshot_q <= '0;
            row_gt3_snapshot_q <= '0;
            row_gt4_snapshot_q <= '0;
            col_gt1_snapshot_q <= '0;
            col_gt2_snapshot_q <= '0;
            col_gt3_snapshot_q <= '0;
            col_gt4_snapshot_q <= '0;
            hybrid_valid_snapshot_q <= '0;
            hybrid_pointer_snapshot_q <= '0;
            hybrid_descriptor_snapshot_q <= '0;
            hybrid_differing_snapshot_q <= '0;
            overflow_snapshot_q <= '0;
        end else begin
            case (state_q)
                STATE_IDLE: begin
                    if (start_i) begin
                        pivot_valid_snapshot_q <= pivot_valid_flat_i;
                        pivot_rows_snapshot_q <= pivot_rows_flat_i;
                        pivot_cols_snapshot_q <= pivot_cols_flat_i;
                        row_gt1_snapshot_q <= row_gt1_flat_i;
                        row_gt2_snapshot_q <= row_gt2_flat_i;
                        row_gt3_snapshot_q <= row_gt3_flat_i;
                        row_gt4_snapshot_q <= row_gt4_flat_i;
                        col_gt1_snapshot_q <= col_gt1_flat_i;
                        col_gt2_snapshot_q <= col_gt2_flat_i;
                        col_gt3_snapshot_q <= col_gt3_flat_i;
                        col_gt4_snapshot_q <= col_gt4_flat_i;
                        hybrid_valid_snapshot_q <= hybrid_valid_flat_i;
                        hybrid_pointer_snapshot_q <= hybrid_pointer_flat_i;
                        hybrid_descriptor_snapshot_q <= hybrid_descriptor_flat_i;
                        hybrid_differing_snapshot_q <= hybrid_differing_flat_i;
                        overflow_snapshot_q <= conventional_overflow_i;
                        repairable_q <= 1'b0;
                        result_valid_q <= 4'd0;
                        result_attempt_q <= 8'd0;
                        result_pattern_q <= 16'd0;
                        result_used_rows_q <= 12'd0;
                        result_used_cols_q <= 8'd0;
                        result_donor_q <= 16'hffff;
                        result_borrow_count_q <= 2'd0;
                        result_used_rows_total_q <= 5'd0;
                        state_q <= STATE_PRODUCER_START;
                    end
                end
                STATE_PRODUCER_START: begin
                    producer_start_q <= 1'b1;
                    state_q <= STATE_PRODUCER_WAIT;
                end
                STATE_PRODUCER_WAIT: begin
                    if (producer_done) begin
                        search_start_q <= 1'b1;
                        state_q <= STATE_SEARCH_START;
                    end
                end
                STATE_SEARCH_START: begin
                    state_q <= STATE_SEARCH_WAIT;
                end
                STATE_SEARCH_WAIT: begin
                    if (search_done) begin
                        if (search_repairable) begin
                            commit_start_q <= 1'b1;
                            state_q <= STATE_COMMIT_START;
                        end else begin
                            done_q <= 1'b1;
                            repairable_q <= 1'b0;
                            state_q <= STATE_IDLE;
                        end
                    end
                end
                STATE_COMMIT_START: begin
                    state_q <= STATE_COMMIT_WAIT;
                end
                default: begin
                    if (commit_accepted) begin
                        result_valid_q <= search_valid;
                        result_attempt_q <= search_attempt;
                        result_pattern_q <= search_pattern;
                        result_used_rows_q <= search_used_rows;
                        result_used_cols_q <= search_used_cols;
                        result_donor_q <= search_donor;
                        result_borrow_count_q <= search_borrow_count;
                        result_used_rows_total_q <= search_used_rows_total;
                        repairable_q <= 1'b1;
                        done_q <= 1'b1;
                        state_q <= STATE_IDLE;
                    end else if (commit_error) begin
                        repairable_q <= 1'b0;
                        done_q <= 1'b1;
                        state_q <= STATE_IDLE;
                    end
                end
            endcase
        end
    end

    assign busy_o = state_q != STATE_IDLE || producer_busy || search_busy || commit_busy;
    assign done_o = done_q;
    assign group_repairable_o = repairable_q;
    assign selected_valid_o = result_valid_q;
    assign selected_attempt_flat_o = result_attempt_q;
    assign selected_pattern_flat_o = result_pattern_q;
    assign selected_used_rows_flat_o = result_used_rows_q;
    assign selected_used_cols_flat_o = result_used_cols_q;
    assign selected_donor_flat_o = result_donor_q;
    assign row_assignment_flat_o = persistent_row_assignment;
    assign selected_borrow_count_o = result_borrow_count_q;
    assign selected_used_rows_total_o = result_used_rows_total_q;
    assign candidate_valid_debug_o = candidate_valid;
    assign atomic_commit_error_o = commit_error;
endmodule

`default_nettype wire
