`default_nettype none

// Reused from: V2 config tables, topology/feasibility, ledger, diagnostics.
// Adapted from: historical recam_dss_v2_early_core streaming control.
// New logic: uniform semantic rank R,L,RB,B and canonical action trace.
// Contract: NORMALIZED_STREAMING_EARLY; A->B->C->D; no rollback/backtracking.
// The analyzer remains external.  This block consumes one completed candidate
// at the requested SA/action and commits the first ledger-legal candidate.
module recam_dss_canonical_streaming_early_core #(
    parameter integer RESOURCE_POINT = 2,
    parameter integer PATTERN_ID_W = 6
) (
    input  wire                         clk_i,
    input  wire                         rst_ni,
    input  wire                         start_i,
    input  wire                         candidate_solution_valid_i,
    input  wire                         candidate_repairable_i,
    input  wire [PATTERN_ID_W-1:0]      candidate_pattern_id_i,
    output wire [1:0]                   current_sa_o,
    output wire [1:0]                   current_action_o,
    output wire [2:0]                   current_config_id_o,
    output wire                         busy_o,
    output wire                         done_o,
    output wire                         group_repairable_o,
    output wire [3:0]                   sa_commit_valid_o,
    output wire [7:0]                   selected_action_flat_o,
    output wire [11:0]                  selected_config_flat_o,
    output wire [4*PATTERN_ID_W-1:0]    selected_pattern_flat_o,
    output wire [7:0]                   selected_donor_flat_o,
    output wire [3:0]                   borrow_flat_o,
    output wire [3:0]                   release_flat_o,
    output wire [1:0]                   failure_position_o,
    output wire [3:0]                   resource_released_o,
    output wire [3:0]                   resource_borrowed_o,
    output wire [7:0]                   borrower_id_flat_o,
    output wire [11:0]                  ledger_released_borrower_o
);

    localparam STATE_IDLE = 1'b0;
    localparam STATE_SEARCH = 1'b1;

    reg state_q;
    reg state_d;
    reg [1:0] sa_q;
    reg [1:0] rank_q;
    reg done_q;
    reg group_repairable_q;
    reg [3:0] sa_commit_valid_q;
    reg [7:0] selected_action_flat_q;
    reg [11:0] selected_config_flat_q;
    reg [4*PATTERN_ID_W-1:0] selected_pattern_flat_q;
    reg [7:0] selected_donor_flat_q;
    reg [3:0] borrow_flat_q;
    reg [3:0] release_flat_q;
    reg [1:0] failure_position_q;

    reg [1:0] current_action;
    wire [2:0] decoded_config;
    wire descriptor_valid;
    wire release_required;
    wire borrow_required;
    wire release_resource_valid;
    wire [1:0] release_resource;
    wire selected_donor_valid;
    wire [1:0] selected_donor;
    wire physical_feasible;
    wire resource_state_valid;
    wire [3:0] released;
    wire [3:0] borrowed;
    wire [3:0] borrower_valid;
    wire [7:0] borrower_ids;
    wire candidate_accept;
    /* verilator lint_off UNUSED */
    wire commit_accepted_unused;
    wire commit_error_unused;
    wire diagnostic_state_valid_unused;
    /* verilator lint_on UNUSED */

    // Semantic action encoding is the canonical slot identity:
    // 0 LOCAL, 1 RELEASE_ONLY, 2 BORROW_ONLY, 3 RELEASE_AND_BORROW.
    always @* begin
        case (rank_q)
            2'd0: current_action = 2'd1;
            2'd1: current_action = 2'd0;
            2'd2: current_action = 2'd3;
            default: current_action = 2'd2;
        endcase
    end

    generate
        if (RESOURCE_POINT == 2) begin : generate_rs2_decode
            wire unused_descriptor_legal;
            wire [5:0] decoded_descriptor;

            dss_v2_group_slot_decode config_decode (
                .sa_id_i(sa_q),
                .canonical_slot_i(current_action),
                .legacy_config_id_o(decoded_config),
                .config_descriptor_o(decoded_descriptor)
            );

            dss_v2_resource_feasibility feasibility (
                .sa_id_i(sa_q),
                .sa_valid_i(1'b1),
                .config_descriptor_i(decoded_descriptor),
                .resource_released_i(released),
                .resource_borrowed_i(borrowed),
                .resource_state_valid_o(resource_state_valid),
                .descriptor_legal_o(unused_descriptor_legal),
                .physical_feasible_o(physical_feasible),
                .borrow_required_o(borrow_required),
                .selected_donor_valid_o(selected_donor_valid),
                .selected_donor_resource_o(selected_donor),
                .release_required_o(release_required),
                .release_resource_valid_o(release_resource_valid),
                .release_resource_o(release_resource)
            );

            assign descriptor_valid = unused_descriptor_legal;
        end else begin : generate_rs3_decode
            wire [2:0] unused_row_count;
            wire [2:0] unused_column_count;
            wire [1:0] unused_canonical_class;
            wire unused_transpose;
            wire [1:0] unused_action;

            dss_v2_rs3cs3m1_config_table config_decode (
                .sa_i(sa_q),
                .slot_i(current_action),
                .valid_o(descriptor_valid),
                .config_id_o(decoded_config),
                .row_count_o(unused_row_count),
                .col_count_o(unused_column_count),
                .canonical_class_o(unused_canonical_class),
                .transpose_o(unused_transpose),
                .action_o(unused_action),
                .release_required_o(release_required),
                .borrow_required_o(borrow_required)
            );

            dss_v2_rs3cs3m1_topology feasibility (
                .sa_i(sa_q),
                .descriptor_valid_i(descriptor_valid),
                .release_required_i(release_required),
                .borrow_required_i(borrow_required),
                .resource_released_i(released),
                .resource_borrowed_i(borrowed),
                .physical_feasible_o(physical_feasible),
                .donor_valid_o(selected_donor_valid),
                .donor_o(selected_donor),
                .release_valid_o(release_resource_valid),
                .release_resource_o(release_resource)
            );

            assign resource_state_valid = 1'b1;
        end
    endgenerate

    assign candidate_accept = (state_q == STATE_SEARCH) &&
                              candidate_solution_valid_i &&
                              candidate_repairable_i &&
                              descriptor_valid &&
                              physical_feasible &&
                              resource_state_valid;

    dss_v2_resource_ledger resource_ledger (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .commit_i(candidate_accept),
        .transaction_valid_i(candidate_accept),
        .requester_sa_i(sa_q),
        .release_required_i(release_required),
        .release_resource_valid_i(release_resource_valid),
        .release_resource_id_i(release_resource),
        .borrow_required_i(borrow_required),
        .selected_donor_valid_i(selected_donor_valid),
        .selected_donor_resource_id_i(selected_donor),
        .resource_released_o(released),
        .resource_borrowed_o(borrowed),
        .borrower_valid_o(borrower_valid),
        .borrower_id_flat_o(borrower_ids),
        .commit_accepted_o(commit_accepted_unused),
        .commit_error_o(commit_error_unused)
    );

    dss_v2_legacy_ledger_diagnostic_adapter ledger_diagnostic (
        .resource_released_i(released),
        .borrower_valid_i(borrower_valid),
        .borrower_id_flat_i(borrower_ids),
        .legacy_ledger_o(ledger_released_borrower_o),
        .canonical_state_valid_o(diagnostic_state_valid_unused)
    );

    // State register.
    always @(posedge clk_i) begin
        if (!rst_ni)
            state_q <= STATE_IDLE;
        else
            state_q <= state_d;
    end

    // State transition logic.  A terminal acceptance or exhausted SA returns
    // directly to IDLE; earlier accepted SAs are never revisited.
    always @* begin
        state_d = state_q;
        case (state_q)
            STATE_IDLE: begin
                if (start_i)
                    state_d = STATE_SEARCH;
            end
            STATE_SEARCH: begin
                if ((candidate_accept && sa_q == 2'd3) ||
                    (!candidate_accept && rank_q == 2'd3))
                    state_d = STATE_IDLE;
            end
            default: state_d = STATE_IDLE;
        endcase
    end

    // Search/output state.  Only the current SA is explored.  A legal result
    // updates the shared ledger on this same edge and advances permanently.
    always @(posedge clk_i) begin
        done_q <= 1'b0;
        if (!rst_ni) begin
            sa_q <= 2'd0;
            rank_q <= 2'd0;
            group_repairable_q <= 1'b0;
            sa_commit_valid_q <= 4'b0000;
            selected_action_flat_q <= 8'b00000000;
            selected_config_flat_q <= 12'b000000000000;
            selected_pattern_flat_q <= {4*PATTERN_ID_W{1'b0}};
            selected_donor_flat_q <= 8'b00000000;
            borrow_flat_q <= 4'b0000;
            release_flat_q <= 4'b0000;
            failure_position_q <= 2'd3;
        end else if (start_i && state_q == STATE_IDLE) begin
            sa_q <= 2'd0;
            rank_q <= 2'd0;
            group_repairable_q <= 1'b0;
            sa_commit_valid_q <= 4'b0000;
            selected_action_flat_q <= 8'b00000000;
            selected_config_flat_q <= 12'b000000000000;
            selected_pattern_flat_q <= {4*PATTERN_ID_W{1'b0}};
            selected_donor_flat_q <= 8'b00000000;
            borrow_flat_q <= 4'b0000;
            release_flat_q <= 4'b0000;
            failure_position_q <= 2'd3;
        end else if (state_q == STATE_SEARCH) begin
            if (candidate_accept) begin
                sa_commit_valid_q[sa_q] <= 1'b1;
                selected_action_flat_q[sa_q*2 +: 2] <= current_action;
                selected_config_flat_q[sa_q*3 +: 3] <= decoded_config;
                selected_pattern_flat_q[sa_q*PATTERN_ID_W +: PATTERN_ID_W] <=
                    candidate_pattern_id_i;
                if (borrow_required)
                    selected_donor_flat_q[sa_q*2 +: 2] <= selected_donor;
                else
                    selected_donor_flat_q[sa_q*2 +: 2] <= 2'd0;
                borrow_flat_q[sa_q] <= borrow_required;
                release_flat_q[sa_q] <= release_required;
                if (sa_q == 2'd3) begin
                    done_q <= 1'b1;
                    group_repairable_q <= 1'b1;
                end else begin
                    sa_q <= sa_q + 1'b1;
                    rank_q <= 2'd0;
                end
            end else if (rank_q == 2'd3) begin
                done_q <= 1'b1;
                group_repairable_q <= 1'b0;
                failure_position_q <= sa_q;
            end else begin
                rank_q <= rank_q + 1'b1;
            end
        end
    end

    assign current_sa_o = sa_q;
    assign current_action_o = current_action;
    assign current_config_id_o = decoded_config;
    assign busy_o = (state_q == STATE_SEARCH);
    assign done_o = done_q;
    assign group_repairable_o = group_repairable_q;
    assign sa_commit_valid_o = sa_commit_valid_q;
    assign selected_action_flat_o = selected_action_flat_q;
    assign selected_config_flat_o = selected_config_flat_q;
    assign selected_pattern_flat_o = selected_pattern_flat_q;
    assign selected_donor_flat_o = selected_donor_flat_q;
    assign borrow_flat_o = borrow_flat_q;
    assign release_flat_o = release_flat_q;
    assign failure_position_o = failure_position_q;
    assign resource_released_o = released;
    assign resource_borrowed_o = borrowed;
    assign borrower_id_flat_o = borrower_ids;

    initial begin
        if (RESOURCE_POINT != 2 && RESOURCE_POINT != 3)
            $display("ERROR: RESOURCE_POINT must be 2 or 3");
        if (PATTERN_ID_W < 4)
            $display("ERROR: PATTERN_ID_W must preserve canonical PatternID");
    end

endmodule

`default_nettype wire
