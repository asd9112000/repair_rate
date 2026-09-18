`default_nettype none

// Shared integration shell for separately elaborated OPT0 and OPT1 tops.
// USE_OPT1 is an elaboration-time parameter; it is not a runtime datapath mux.
module recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_integrated_shell #(
    parameter integer USE_OPT1 = 0,
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
    output wire                                      group_repairable_o,
    output wire                                      commit_error_o,
    output wire [3:0]                                selected_valid_o,
    output wire [7:0]                                selected_action_flat_o,
    output wire [11:0]                               selected_config_flat_o,
    output wire [15:0]                               selected_pattern_flat_o,
    output wire [7:0]                                selected_donor_flat_o,
    output wire [3:0]                                selected_release_o,
    output wire [3:0]                                selected_borrow_o,
    output wire [3:0]                                resource_released_o,
    output wire [3:0]                                resource_borrowed_o,
    output wire [7:0]                                borrower_id_flat_o,
    output wire [159:0]                              candidate_valid_debug_o,
    output wire [159:0]                              candidate_release_debug_o,
    output wire [159:0]                              candidate_borrow_debug_o
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
    reg commit_error_q;
    reg [3:0] selected_valid_q;
    reg [7:0] selected_action_q;
    reg [11:0] selected_config_q;
    reg [15:0] selected_pattern_q;
    reg [7:0] selected_donor_q;
    reg [3:0] selected_release_q;
    reg [3:0] selected_borrow_q;
    reg [4*5-1:0] pivot_valid_snapshot_q;
    reg [4*5*ROW_ADDR_W-1:0] pivot_rows_snapshot_q;
    reg [4*5*COL_ADDR_W-1:0] pivot_cols_snapshot_q;
    reg [4*5-1:0] row_gt1_snapshot_q;
    reg [4*5-1:0] row_gt2_snapshot_q;
    reg [4*5-1:0] row_gt3_snapshot_q;
    reg [4*5-1:0] col_gt1_snapshot_q;
    reg [4*5-1:0] col_gt2_snapshot_q;
    reg [4*5-1:0] col_gt3_snapshot_q;
    reg [4*HYBRID_ENTRIES-1:0] hybrid_valid_snapshot_q;
    reg [4*HYBRID_ENTRIES*3-1:0] hybrid_pointer_snapshot_q;
    reg [4*HYBRID_ENTRIES-1:0] hybrid_descriptor_snapshot_q;
    reg [4*HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_snapshot_q;
    reg [3:0] conventional_overflow_snapshot_q;

    wire producer_busy;
    wire producer_done;
    wire [159:0] candidate_valid;
    wire [159:0] candidate_release;
    wire [159:0] candidate_borrow;
    wire search_busy;
    wire search_done;
    wire search_repairable;
    wire [3:0] search_selected_valid;
    wire [7:0] search_selected_action;
    wire [11:0] search_selected_config;
    wire [15:0] search_selected_pattern;
    wire [7:0] search_selected_donor;
    wire [3:0] search_selected_release;
    wire [3:0] search_selected_borrow;
    wire [3:0] unused_search_released;
    wire [3:0] unused_search_used;
    wire commit_busy;
    wire commit_accepted;
    wire commit_error;
    /* verilator lint_off UNUSED */
    wire [1:0] producer_active_sa;
    wire [1:0] producer_active_action;
    wire [2:0] producer_active_config;
    /* verilator lint_on UNUSED */

    recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_candidate_map_producer #(
        .ROW_ADDR_W(ROW_ADDR_W), .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W), .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) candidate_map_producer (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(producer_start_q),
        .pivot_valid_flat_i(pivot_valid_snapshot_q),
        .pivot_rows_flat_i(pivot_rows_snapshot_q), .pivot_cols_flat_i(pivot_cols_snapshot_q),
        .row_gt1_flat_i(row_gt1_snapshot_q), .row_gt2_flat_i(row_gt2_snapshot_q),
        .row_gt3_flat_i(row_gt3_snapshot_q), .col_gt1_flat_i(col_gt1_snapshot_q),
        .col_gt2_flat_i(col_gt2_snapshot_q), .col_gt3_flat_i(col_gt3_snapshot_q),
        .hybrid_valid_flat_i(hybrid_valid_snapshot_q),
        .hybrid_pointer_flat_i(hybrid_pointer_snapshot_q),
        .hybrid_descriptor_flat_i(hybrid_descriptor_snapshot_q),
        .hybrid_differing_flat_i(hybrid_differing_snapshot_q),
        .conventional_overflow_i(conventional_overflow_snapshot_q),
        .busy_o(producer_busy), .done_o(producer_done),
        .candidate_valid_o(candidate_valid), .candidate_release_o(candidate_release),
        .candidate_borrow_o(candidate_borrow), .active_sa_o(producer_active_sa),
        .active_action_o(producer_active_action), .active_config_id_o(producer_active_config)
    );

    generate
        if (USE_OPT1 == 0) begin : generate_opt0_search
            recam_dss_canonical_global_noscratch_core search_core (
                .clk_i(clk_i), .rst_ni(rst_ni), .start_i(search_start_q),
                .candidate_valid_i(candidate_valid), .candidate_release_i(candidate_release),
                .candidate_borrow_i(candidate_borrow), .busy_o(search_busy),
                .done_o(search_done), .group_repairable_o(search_repairable),
                .selected_valid_o(search_selected_valid), .selected_action_flat_o(search_selected_action),
                .selected_config_flat_o(search_selected_config), .selected_pattern_flat_o(search_selected_pattern),
                .selected_donor_flat_o(search_selected_donor), .selected_release_o(search_selected_release),
                .selected_borrow_o(search_selected_borrow), .final_released_mask_o(unused_search_released),
                .final_used_mask_o(unused_search_used)
            );
        end else begin : generate_opt1_search
            recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_core search_core (
                .clk_i(clk_i), .rst_ni(rst_ni), .start_i(search_start_q),
                .candidate_valid_i(candidate_valid), .candidate_release_i(candidate_release),
                .candidate_borrow_i(candidate_borrow), .busy_o(search_busy),
                .done_o(search_done), .group_repairable_o(search_repairable),
                .selected_valid_o(search_selected_valid), .selected_action_flat_o(search_selected_action),
                .selected_config_flat_o(search_selected_config), .selected_pattern_flat_o(search_selected_pattern),
                .selected_donor_flat_o(search_selected_donor), .selected_release_o(search_selected_release),
                .selected_borrow_o(search_selected_borrow), .final_released_mask_o(unused_search_released),
                .final_used_mask_o(unused_search_used)
            );
        end
    endgenerate

    recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_atomic_group_commit atomic_group_commit (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(commit_start_q),
        .selected_valid_i(search_selected_valid), .selected_action_flat_i(search_selected_action),
        .selected_release_i(search_selected_release), .selected_borrow_i(search_selected_borrow),
        .selected_donor_flat_i(search_selected_donor), .busy_o(commit_busy),
        .commit_accepted_o(commit_accepted), .commit_error_o(commit_error),
        .resource_released_o(resource_released_o), .resource_borrowed_o(resource_borrowed_o),
        .borrower_id_flat_o(borrower_id_flat_o)
    );

    always @(posedge clk_i) begin
        producer_start_q <= 1'b0;
        search_start_q <= 1'b0;
        commit_start_q <= 1'b0;
        done_q <= 1'b0;
        if (!rst_ni) begin
            state_q <= STATE_IDLE;
            repairable_q <= 1'b0;
            commit_error_q <= 1'b0;
            selected_valid_q <= 4'd0;
            selected_action_q <= 8'd0;
            selected_config_q <= 12'd0;
            selected_pattern_q <= 16'd0;
            selected_donor_q <= 8'd0;
            selected_release_q <= 4'd0;
            selected_borrow_q <= 4'd0;
            pivot_valid_snapshot_q <= '0;
            pivot_rows_snapshot_q <= '0;
            pivot_cols_snapshot_q <= '0;
            row_gt1_snapshot_q <= '0;
            row_gt2_snapshot_q <= '0;
            row_gt3_snapshot_q <= '0;
            col_gt1_snapshot_q <= '0;
            col_gt2_snapshot_q <= '0;
            col_gt3_snapshot_q <= '0;
            hybrid_valid_snapshot_q <= '0;
            hybrid_pointer_snapshot_q <= '0;
            hybrid_descriptor_snapshot_q <= '0;
            hybrid_differing_snapshot_q <= '0;
            conventional_overflow_snapshot_q <= '0;
        end else begin
            case (state_q)
                STATE_IDLE: if (start_i && resource_released_o == 4'd0 && resource_borrowed_o == 4'd0) begin
                    pivot_valid_snapshot_q <= pivot_valid_flat_i;
                    pivot_rows_snapshot_q <= pivot_rows_flat_i;
                    pivot_cols_snapshot_q <= pivot_cols_flat_i;
                    row_gt1_snapshot_q <= row_gt1_flat_i;
                    row_gt2_snapshot_q <= row_gt2_flat_i;
                    row_gt3_snapshot_q <= row_gt3_flat_i;
                    col_gt1_snapshot_q <= col_gt1_flat_i;
                    col_gt2_snapshot_q <= col_gt2_flat_i;
                    col_gt3_snapshot_q <= col_gt3_flat_i;
                    hybrid_valid_snapshot_q <= hybrid_valid_flat_i;
                    hybrid_pointer_snapshot_q <= hybrid_pointer_flat_i;
                    hybrid_descriptor_snapshot_q <= hybrid_descriptor_flat_i;
                    hybrid_differing_snapshot_q <= hybrid_differing_flat_i;
                    conventional_overflow_snapshot_q <= conventional_overflow_i;
                    repairable_q <= 1'b0;
                    commit_error_q <= 1'b0;
                    selected_valid_q <= 4'd0;
                    selected_action_q <= 8'd0;
                    selected_config_q <= 12'd0;
                    selected_pattern_q <= 16'd0;
                    selected_donor_q <= 8'd0;
                    selected_release_q <= 4'd0;
                    selected_borrow_q <= 4'd0;
                    state_q <= STATE_PRODUCER_START;
                end
                STATE_PRODUCER_START: begin producer_start_q <= 1'b1; state_q <= STATE_PRODUCER_WAIT; end
                STATE_PRODUCER_WAIT: if (producer_done) state_q <= STATE_SEARCH_START;
                STATE_SEARCH_START: begin search_start_q <= 1'b1; state_q <= STATE_SEARCH_WAIT; end
                STATE_SEARCH_WAIT: if (search_done) begin
                    if (search_repairable)
                        state_q <= STATE_COMMIT_START;
                    else begin
                        done_q <= 1'b1;
                        repairable_q <= 1'b0;
                        state_q <= STATE_IDLE;
                    end
                end
                STATE_COMMIT_START: begin commit_start_q <= 1'b1; state_q <= STATE_COMMIT_WAIT; end
                default: if (commit_accepted || commit_error) begin
                    if (commit_accepted) begin
                        selected_valid_q <= search_selected_valid;
                        selected_action_q <= search_selected_action;
                        selected_config_q <= search_selected_config;
                        selected_pattern_q <= search_selected_pattern;
                        selected_donor_q <= search_selected_donor;
                        selected_release_q <= search_selected_release;
                        selected_borrow_q <= search_selected_borrow;
                        repairable_q <= 1'b1;
                    end else begin
                        repairable_q <= 1'b0;
                        commit_error_q <= 1'b1;
                    end
                    done_q <= 1'b1;
                    state_q <= STATE_IDLE;
                end
            endcase
        end
    end

    assign busy_o = (state_q != STATE_IDLE) || producer_busy || search_busy || commit_busy;
    assign done_o = done_q;
    assign group_repairable_o = repairable_q;
    assign commit_error_o = commit_error_q;
    assign selected_valid_o = selected_valid_q;
    assign selected_action_flat_o = selected_action_q;
    assign selected_config_flat_o = selected_config_q;
    assign selected_pattern_flat_o = selected_pattern_q;
    assign selected_donor_flat_o = selected_donor_q;
    assign selected_release_o = selected_release_q;
    assign selected_borrow_o = selected_borrow_q;
    assign candidate_valid_debug_o = candidate_valid;
    assign candidate_release_debug_o = candidate_release;
    assign candidate_borrow_debug_o = candidate_borrow;
endmodule

`default_nettype wire
