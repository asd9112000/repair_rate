`default_nettype none

// Provenance-backed port of the SYN-B 3x160 registered candidate maps.  SA
// scheduling and slot storage remain L,R,B,RB; Config analysis is seven-way
// parallel and the current canonical role map selects the stored slot source.
module recam_n2_preopt_candidate_map_producer #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer PHYS_COL_ADDR_W = 13,
    parameter integer WORD_COL_ADDR_W = 5,
    parameter integer HYBRID_LINE_ADDR_W = 13,
    parameter integer HYBRID_ENTRIES = 7
) (
    input wire clk_i, input wire rst_ni, input wire start_i,
    input wire [4*5-1:0] pivot_valid_flat_i,
    input wire [4*5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input wire [4*5*PHYS_COL_ADDR_W-1:0] pivot_cols_flat_i,
    input wire [4*5-1:0] row_gt1_flat_i, input wire [4*5-1:0] row_gt2_flat_i,
    input wire [4*5-1:0] row_gt3_flat_i, input wire [4*5-1:0] col_gt1_flat_i,
    input wire [4*5-1:0] col_gt2_flat_i, input wire [4*5-1:0] col_gt3_flat_i,
    input wire [4*HYBRID_ENTRIES-1:0] hybrid_valid_flat_i,
    input wire [4*HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input wire [4*HYBRID_ENTRIES-1:0] hybrid_descriptor_flat_i,
    input wire [4*HYBRID_ENTRIES*HYBRID_LINE_ADDR_W-1:0] hybrid_differing_flat_i,
    input wire [3:0] conventional_overflow_i,
    output wire busy_o, output wire done_o,
    output wire [159:0] candidate_valid_o, output wire [159:0] candidate_release_o,
    output wire [159:0] candidate_borrow_o, output wire [1:0] active_sa_o,
    output wire [1:0] active_action_o, output wire [2:0] active_config_id_o
);
    localparam [1:0] STATE_IDLE = 2'd0, STATE_BUILD = 2'd1;
    reg [1:0] state_q, sa_q, action_q;
    reg done_q;
    reg [159:0] candidate_valid_q, candidate_release_q, candidate_borrow_q;
    wire [4:0] active_pivot_valid = pivot_valid_flat_i[sa_q*5 +: 5];
    wire [5*ROW_ADDR_W-1:0] active_pivot_rows = pivot_rows_flat_i[sa_q*(5*ROW_ADDR_W) +: 5*ROW_ADDR_W];
    wire [5*PHYS_COL_ADDR_W-1:0] active_pivot_cols = pivot_cols_flat_i[sa_q*(5*PHYS_COL_ADDR_W) +: 5*PHYS_COL_ADDR_W];
    wire [4:0] active_row_gt1 = row_gt1_flat_i[sa_q*5 +: 5];
    wire [4:0] active_row_gt2 = row_gt2_flat_i[sa_q*5 +: 5];
    wire [4:0] active_row_gt3 = row_gt3_flat_i[sa_q*5 +: 5];
    wire [4:0] active_col_gt1 = col_gt1_flat_i[sa_q*5 +: 5];
    wire [4:0] active_col_gt2 = col_gt2_flat_i[sa_q*5 +: 5];
    wire [4:0] active_col_gt3 = col_gt3_flat_i[sa_q*5 +: 5];
    wire [HYBRID_ENTRIES-1:0] active_hybrid_valid = hybrid_valid_flat_i[sa_q*HYBRID_ENTRIES +: HYBRID_ENTRIES];
    wire [HYBRID_ENTRIES*3-1:0] active_hybrid_pointer = hybrid_pointer_flat_i[sa_q*(HYBRID_ENTRIES*3) +: HYBRID_ENTRIES*3];
    wire [HYBRID_ENTRIES-1:0] active_hybrid_descriptor = hybrid_descriptor_flat_i[sa_q*HYBRID_ENTRIES +: HYBRID_ENTRIES];
    wire [HYBRID_ENTRIES*HYBRID_LINE_ADDR_W-1:0] active_hybrid_differing = hybrid_differing_flat_i[sa_q*(HYBRID_ENTRIES*HYBRID_LINE_ADDR_W) +: HYBRID_ENTRIES*HYBRID_LINE_ADDR_W];
    wire active_overflow = conventional_overflow_i[sa_q];
    reg [2:0] active_config_id;
    reg [2:0] active_rows, active_cols;
    wire [69:0] lane_candidate_valid;
    wire [27:0] unused_lane_pattern;
    wire [6:0] unused_lane_solution;
    wire [9:0] selected_candidate_valid = lane_candidate_valid[active_config_id*10 +: 10];
    wire actual_release, actual_borrow;
    wire [1:0] unused_release_resource, unused_donor_resource;

    function [7:0] candidate_base;
        input [1:0] subarray; input [1:0] action;
        candidate_base = {6'd0, subarray} * 8'd40 + {6'd0, action} * 8'd10;
    endfunction
    always @* begin
        if (sa_q == 2'd1 || sa_q == 2'd2)
            active_config_id = {1'b0, action_q};
        else case (action_q)
            2'd0: active_config_id = 3'd0;
            2'd1: active_config_id = 3'd4;
            2'd2: active_config_id = 3'd5;
            default: active_config_id = 3'd6;
        endcase
        case (active_config_id)
            3'd0: begin active_rows = 3'd2; active_cols = 3'd2; end
            3'd1: begin active_rows = 3'd2; active_cols = 3'd1; end
            3'd2: begin active_rows = 3'd3; active_cols = 3'd2; end
            3'd3: begin active_rows = 3'd3; active_cols = 3'd1; end
            3'd4: begin active_rows = 3'd1; active_cols = 3'd2; end
            3'd5: begin active_rows = 3'd2; active_cols = 3'd3; end
            default: begin active_rows = 3'd1; active_cols = 3'd3; end
        endcase
    end
    recam_n2_preopt_parallel_config_bank #(
        .ROW_ADDR_W(ROW_ADDR_W), .PHYS_COL_ADDR_W(PHYS_COL_ADDR_W),
        .WORD_COL_ADDR_W(WORD_COL_ADDR_W), .HYBRID_LINE_ADDR_W(HYBRID_LINE_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) parallel_bank (
        .pivot_valid_i(active_pivot_valid), .pivot_rows_flat_i(active_pivot_rows),
        .pivot_cols_flat_i(active_pivot_cols), .row_gt1_i(active_row_gt1),
        .row_gt2_i(active_row_gt2), .row_gt3_i(active_row_gt3), .col_gt1_i(active_col_gt1),
        .col_gt2_i(active_col_gt2), .col_gt3_i(active_col_gt3), .hybrid_valid_i(active_hybrid_valid),
        .hybrid_pointer_flat_i(active_hybrid_pointer), .hybrid_descriptor_i(active_hybrid_descriptor),
        .hybrid_differing_flat_i(active_hybrid_differing), .conventional_overflow_i(active_overflow),
        .candidate_valid_flat_o(lane_candidate_valid), .first_pattern_id_flat_o(unused_lane_pattern),
        .solution_valid_o(unused_lane_solution)
    );
    recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_candidate_transition_adapter transition_adapter (
        .sa_id_i(sa_q), .candidate_valid_i(|selected_candidate_valid), .row_count_i(active_rows),
        .col_count_i(active_cols), .actual_release_o(actual_release), .actual_borrow_o(actual_borrow),
        .release_resource_o(unused_release_resource), .donor_resource_o(unused_donor_resource)
    );
    always @(posedge clk_i) begin
        done_q <= 1'b0;
        if (!rst_ni) begin
            state_q <= STATE_IDLE; sa_q <= 2'd0; action_q <= 2'd0;
            candidate_valid_q <= 160'd0; candidate_release_q <= 160'd0; candidate_borrow_q <= 160'd0;
        end else if (state_q == STATE_IDLE) begin
            if (start_i) begin
                state_q <= STATE_BUILD; sa_q <= 2'd0; action_q <= 2'd0;
                candidate_valid_q <= 160'd0; candidate_release_q <= 160'd0; candidate_borrow_q <= 160'd0;
            end
        end else begin
            candidate_valid_q[candidate_base(sa_q, action_q) +: 10] <= selected_candidate_valid;
            candidate_release_q[candidate_base(sa_q, action_q) +: 10] <= {10{actual_release}} & selected_candidate_valid;
            candidate_borrow_q[candidate_base(sa_q, action_q) +: 10] <= {10{actual_borrow}} & selected_candidate_valid;
            if (action_q == 2'd3) begin
                action_q <= 2'd0;
                if (sa_q == 2'd3) begin state_q <= STATE_IDLE; done_q <= 1'b1; end
                else sa_q <= sa_q + 1'b1;
            end else action_q <= action_q + 1'b1;
        end
    end
    assign busy_o = state_q == STATE_BUILD; assign done_o = done_q;
    assign candidate_valid_o = candidate_valid_q; assign candidate_release_o = candidate_release_q;
    assign candidate_borrow_o = candidate_borrow_q; assign active_sa_o = sa_q;
    assign active_action_o = action_q; assign active_config_id_o = active_config_id;
endmodule

`default_nettype wire
