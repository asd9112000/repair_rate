`default_nettype none

// One shared analyzer serially enumerates four SAs and four stored actions.
// Map storage order is L, R, B, RB; DFS priority remains the core's R,L,RB,B.
module recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_candidate_map_producer #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 7
) (
    input  wire                                      clk_i,
    input  wire                                      rst_ni,
    input  wire                                      start_i,
    input  wire [4*5-1:0]                            pivot_valid_flat_i,
    input  wire [4*5*ROW_ADDR_W-1:0]                 pivot_rows_flat_i,
    input  wire [4*5*COL_ADDR_W-1:0]                 pivot_cols_flat_i,
    input  wire [4*5-1:0]                            row_gt1_flat_i,
    input  wire [4*5-1:0]                            row_gt2_flat_i,
    input  wire [4*5-1:0]                            row_gt3_flat_i,
    input  wire [4*5-1:0]                            col_gt1_flat_i,
    input  wire [4*5-1:0]                            col_gt2_flat_i,
    input  wire [4*5-1:0]                            col_gt3_flat_i,
    input  wire [4*HYBRID_ENTRIES-1:0]               hybrid_valid_flat_i,
    input  wire [4*HYBRID_ENTRIES*3-1:0]             hybrid_pointer_flat_i,
    input  wire [4*HYBRID_ENTRIES-1:0]               hybrid_descriptor_flat_i,
    input  wire [4*HYBRID_ENTRIES*DIFF_ADDR_W-1:0]   hybrid_differing_flat_i,
    input  wire [3:0]                                conventional_overflow_i,
    output wire                                      busy_o,
    output wire                                      done_o,
    output wire [159:0]                              candidate_valid_o,
    output wire [159:0]                              candidate_release_o,
    output wire [159:0]                              candidate_borrow_o,
    output wire [1:0]                                active_sa_o,
    output wire [1:0]                                active_action_o,
    output wire [2:0]                                active_config_id_o
);
    localparam [1:0] STATE_IDLE = 2'd0;
    localparam [1:0] STATE_BUILD = 2'd1;

    reg [1:0] state_q;
    reg [1:0] sa_q;
    reg [1:0] action_q;
    reg done_q;
    reg [159:0] candidate_valid_q;
    reg [159:0] candidate_release_q;
    reg [159:0] candidate_borrow_q;

    wire [4:0] active_pivot_valid;
    wire [5*ROW_ADDR_W-1:0] active_pivot_rows;
    wire [5*COL_ADDR_W-1:0] active_pivot_cols;
    wire [4:0] active_row_gt1;
    wire [4:0] active_row_gt2;
    wire [4:0] active_row_gt3;
    wire [4:0] active_col_gt1;
    wire [4:0] active_col_gt2;
    wire [4:0] active_col_gt3;
    wire [HYBRID_ENTRIES-1:0] active_hybrid_valid;
    wire [HYBRID_ENTRIES*3-1:0] active_hybrid_pointer;
    wire [HYBRID_ENTRIES-1:0] active_hybrid_descriptor;
    wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] active_hybrid_differing;
    wire active_overflow;
    wire [2:0] active_config_id;
    wire [5:0] active_descriptor;
    wire [9:0] analyzer_candidate_valid;
    wire [3:0] unused_pattern_id;
    wire unused_solution_valid;
    wire unused_repairable;
    wire unused_dictionary_overflow;
    wire actual_release;
    wire actual_borrow;
    wire [1:0] unused_release_resource;
    wire [1:0] unused_donor_resource;

    function [7:0] candidate_base;
        input [1:0] subarray;
        input [1:0] action;
        begin
            candidate_base = {6'd0, subarray} * 8'd40 + {6'd0, action} * 8'd10;
        end
    endfunction

    assign active_pivot_valid = pivot_valid_flat_i[sa_q*5 +: 5];
    assign active_pivot_rows = pivot_rows_flat_i[sa_q*(5*ROW_ADDR_W) +: 5*ROW_ADDR_W];
    assign active_pivot_cols = pivot_cols_flat_i[sa_q*(5*COL_ADDR_W) +: 5*COL_ADDR_W];
    assign active_row_gt1 = row_gt1_flat_i[sa_q*5 +: 5];
    assign active_row_gt2 = row_gt2_flat_i[sa_q*5 +: 5];
    assign active_row_gt3 = row_gt3_flat_i[sa_q*5 +: 5];
    assign active_col_gt1 = col_gt1_flat_i[sa_q*5 +: 5];
    assign active_col_gt2 = col_gt2_flat_i[sa_q*5 +: 5];
    assign active_col_gt3 = col_gt3_flat_i[sa_q*5 +: 5];
    assign active_hybrid_valid = hybrid_valid_flat_i[sa_q*HYBRID_ENTRIES +: HYBRID_ENTRIES];
    assign active_hybrid_pointer = hybrid_pointer_flat_i[sa_q*(HYBRID_ENTRIES*3) +: HYBRID_ENTRIES*3];
    assign active_hybrid_descriptor = hybrid_descriptor_flat_i[sa_q*HYBRID_ENTRIES +: HYBRID_ENTRIES];
    assign active_hybrid_differing = hybrid_differing_flat_i[sa_q*(HYBRID_ENTRIES*DIFF_ADDR_W) +: HYBRID_ENTRIES*DIFF_ADDR_W];
    assign active_overflow = conventional_overflow_i[sa_q];

    dss_v2_group_slot_decode slot_decode (
        .sa_id_i(sa_q),
        .canonical_slot_i(action_q),
        .legacy_config_id_o(active_config_id),
        .config_descriptor_o(active_descriptor)
    );

    recam_shared_config_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W),
        .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) shared_analyzer (
        .config_id_i(active_config_id),
        .pivot_valid_i(active_pivot_valid),
        .pivot_rows_flat_i(active_pivot_rows),
        .pivot_cols_flat_i(active_pivot_cols),
        .row_gt1_i(active_row_gt1),
        .row_gt2_i(active_row_gt2),
        .row_gt3_i(active_row_gt3),
        .col_gt1_i(active_col_gt1),
        .col_gt2_i(active_col_gt2),
        .col_gt3_i(active_col_gt3),
        .hybrid_valid_i(active_hybrid_valid),
        .hybrid_pointer_flat_i(active_hybrid_pointer),
        .hybrid_descriptor_i(active_hybrid_descriptor),
        .hybrid_differing_flat_i(active_hybrid_differing),
        .conventional_overflow_i(active_overflow),
        .candidate_valid_o(analyzer_candidate_valid),
        .pattern_id_o(unused_pattern_id),
        .solution_valid_o(unused_solution_valid),
        .repairable_o(unused_repairable),
        .dictionary_overflow_o(unused_dictionary_overflow)
    );

    recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_candidate_transition_adapter transition_adapter (
        .sa_id_i(sa_q),
        .candidate_valid_i(|analyzer_candidate_valid),
        .row_count_i(active_descriptor[5:3]),
        .col_count_i(active_descriptor[2:0]),
        .actual_release_o(actual_release),
        .actual_borrow_o(actual_borrow),
        .release_resource_o(unused_release_resource),
        .donor_resource_o(unused_donor_resource)
    );

    always @(posedge clk_i) begin
        done_q <= 1'b0;
        if (!rst_ni) begin
            state_q <= STATE_IDLE;
            sa_q <= 2'd0;
            action_q <= 2'd0;
            candidate_valid_q <= 160'd0;
            candidate_release_q <= 160'd0;
            candidate_borrow_q <= 160'd0;
        end else if (state_q == STATE_IDLE) begin
            if (start_i) begin
                state_q <= STATE_BUILD;
                sa_q <= 2'd0;
                action_q <= 2'd0;
                candidate_valid_q <= 160'd0;
                candidate_release_q <= 160'd0;
                candidate_borrow_q <= 160'd0;
            end
        end else begin
            candidate_valid_q[candidate_base(sa_q, action_q) +: 10] <= analyzer_candidate_valid;
            candidate_release_q[candidate_base(sa_q, action_q) +: 10] <= {10{actual_release}} & analyzer_candidate_valid;
            candidate_borrow_q[candidate_base(sa_q, action_q) +: 10] <= {10{actual_borrow}} & analyzer_candidate_valid;
            if (action_q == 2'd3) begin
                action_q <= 2'd0;
                if (sa_q == 2'd3) begin
                    state_q <= STATE_IDLE;
                    done_q <= 1'b1;
                end else begin
                    sa_q <= sa_q + 1'b1;
                end
            end else begin
                action_q <= action_q + 1'b1;
            end
        end
    end

    assign busy_o = state_q == STATE_BUILD;
    assign done_o = done_q;
    assign candidate_valid_o = candidate_valid_q;
    assign candidate_release_o = candidate_release_q;
    assign candidate_borrow_o = candidate_borrow_q;
    assign active_sa_o = sa_q;
    assign active_action_o = action_q;
    assign active_config_id_o = active_config_id;
endmodule

`default_nettype wire
