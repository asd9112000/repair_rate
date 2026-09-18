`default_nettype none

// Serializes the ten C++ capacity attempts: A/D use attempts 0..1 and B/C
// use 0..2.  Each capture stores 15 PatternID slots and their local demands.
module recam_dss_line1x4_rs2_cs2_m1_normalized_candidate_producer #(
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
    output wire busy_o, output wire done_o,
    output wire [179:0] candidate_valid_o,
    output wire [4*3*15*3-1:0] candidate_used_rows_flat_o,
    output wire [4*3*15*2-1:0] candidate_used_cols_flat_o,
    output wire [1:0] active_sa_o, output wire [1:0] active_attempt_o
);
    localparam [1:0] STATE_IDLE = 2'd0;
    localparam [1:0] STATE_BUILD = 2'd1;
    reg [1:0] state_q, sa_q, attempt_q;
    reg done_q;
    reg [179:0] candidate_valid_q;
    reg [539:0] candidate_used_rows_q;
    reg [359:0] candidate_used_cols_q;
    wire [5:0] active_pivot_valid = pivot_valid_flat_i[sa_q*6 +: 6];
    wire [6*ROW_ADDR_W-1:0] active_pivot_rows = pivot_rows_flat_i[sa_q*(6*ROW_ADDR_W) +: 6*ROW_ADDR_W];
    wire [6*COL_ADDR_W-1:0] active_pivot_cols = pivot_cols_flat_i[sa_q*(6*COL_ADDR_W) +: 6*COL_ADDR_W];
    wire [5:0] active_row_gt1 = row_gt1_flat_i[sa_q*6 +: 6];
    wire [5:0] active_row_gt2 = row_gt2_flat_i[sa_q*6 +: 6];
    wire [5:0] active_row_gt3 = row_gt3_flat_i[sa_q*6 +: 6];
    wire [5:0] active_row_gt4 = row_gt4_flat_i[sa_q*6 +: 6];
    wire [5:0] active_col_gt1 = col_gt1_flat_i[sa_q*6 +: 6];
    wire [5:0] active_col_gt2 = col_gt2_flat_i[sa_q*6 +: 6];
    wire [5:0] active_col_gt3 = col_gt3_flat_i[sa_q*6 +: 6];
    wire [5:0] active_col_gt4 = col_gt4_flat_i[sa_q*6 +: 6];
    wire [HYBRID_ENTRIES-1:0] active_hybrid_valid = hybrid_valid_flat_i[sa_q*HYBRID_ENTRIES +: HYBRID_ENTRIES];
    wire [HYBRID_ENTRIES*3-1:0] active_hybrid_pointer = hybrid_pointer_flat_i[sa_q*(HYBRID_ENTRIES*3) +: HYBRID_ENTRIES*3];
    wire [HYBRID_ENTRIES-1:0] active_hybrid_descriptor = hybrid_descriptor_flat_i[sa_q*HYBRID_ENTRIES +: HYBRID_ENTRIES];
    wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] active_hybrid_differing = hybrid_differing_flat_i[sa_q*(HYBRID_ENTRIES*DIFF_ADDR_W) +: HYBRID_ENTRIES*DIFF_ADDR_W];
    wire [14:0] analyzer_valid;
    wire [44:0] analyzer_rows;
    wire [29:0] analyzer_cols;
    wire unused_dictionary_overflow;
    wire active_overflow = conventional_overflow_i[sa_q];
    wire is_last_attempt = (sa_q == 2'd0 || sa_q == 2'd3) ? attempt_q == 2'd1 : attempt_q == 2'd2;

    recam_dss_line1x4_rs2_cs2_m1_normalized_shared_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W), .COL_ADDR_W(COL_ADDR_W), .DIFF_ADDR_W(DIFF_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) shared_analyzer (
        .attempt_i(attempt_q), .pivot_valid_i(active_pivot_valid),
        .pivot_rows_flat_i(active_pivot_rows), .pivot_cols_flat_i(active_pivot_cols),
        .row_gt1_i(active_row_gt1), .row_gt2_i(active_row_gt2), .row_gt3_i(active_row_gt3), .row_gt4_i(active_row_gt4),
        .col_gt1_i(active_col_gt1), .col_gt2_i(active_col_gt2), .col_gt3_i(active_col_gt3), .col_gt4_i(active_col_gt4),
        .hybrid_valid_i(active_hybrid_valid), .hybrid_pointer_flat_i(active_hybrid_pointer),
        .hybrid_descriptor_i(active_hybrid_descriptor), .hybrid_differing_flat_i(active_hybrid_differing),
        .conventional_overflow_i(active_overflow), .candidate_valid_o(analyzer_valid),
        .candidate_used_rows_flat_o(analyzer_rows), .candidate_used_cols_flat_o(analyzer_cols),
        .dictionary_overflow_o(unused_dictionary_overflow)
    );

    always @(posedge clk_i) begin
        done_q <= 1'b0;
        if (!rst_ni) begin
            state_q <= STATE_IDLE; sa_q <= 2'd0; attempt_q <= 2'd0;
            candidate_valid_q <= 180'd0; candidate_used_rows_q <= 540'd0; candidate_used_cols_q <= 360'd0;
        end else if (state_q == STATE_IDLE) begin
            if (start_i) begin
                state_q <= STATE_BUILD; sa_q <= 2'd0; attempt_q <= 2'd0;
                candidate_valid_q <= 180'd0; candidate_used_rows_q <= 540'd0; candidate_used_cols_q <= 360'd0;
            end
        end else begin
            candidate_valid_q[(sa_q*45 + attempt_q*15) +: 15] <= analyzer_valid;
            candidate_used_rows_q[(sa_q*135 + attempt_q*45) +: 45] <= analyzer_rows;
            candidate_used_cols_q[(sa_q*90 + attempt_q*30) +: 30] <= analyzer_cols;
            if (is_last_attempt) begin
                attempt_q <= 2'd0;
                if (sa_q == 2'd3) begin state_q <= STATE_IDLE; done_q <= 1'b1; end
                else sa_q <= sa_q + 1'b1;
            end else attempt_q <= attempt_q + 1'b1;
        end
    end
    assign busy_o = state_q == STATE_BUILD;
    assign done_o = done_q;
    assign candidate_valid_o = candidate_valid_q;
    assign candidate_used_rows_flat_o = candidate_used_rows_q;
    assign candidate_used_cols_flat_o = candidate_used_cols_q;
    assign active_sa_o = sa_q;
    assign active_attempt_o = attempt_q;
endmodule

`default_nettype wire
