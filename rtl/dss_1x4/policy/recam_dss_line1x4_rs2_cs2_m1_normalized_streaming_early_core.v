`default_nettype none

// Streaming A->B->C->D policy and committed physical eight-row ledger.
// A row is selected only while unassigned; borrowing scans physical owner
// order, giving B A-before-C and C B-before-D priority without forwarding.
module recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_core (
    input wire clk_i, input wire rst_ni, input wire start_i,
    input wire [179:0] candidate_valid_i,
    input wire [539:0] candidate_used_rows_flat_i,
    input wire [359:0] candidate_used_cols_flat_i,
    output wire busy_o, output wire done_o, output wire group_repairable_o,
    output wire [3:0] selected_valid_o,
    output wire [7:0] selected_attempt_flat_o,
    output wire [15:0] selected_pattern_flat_o,
    output wire [11:0] selected_used_rows_flat_o,
    output wire [7:0] selected_used_cols_flat_o,
    output wire [15:0] selected_donor_flat_o,
    output wire [23:0] row_assignment_flat_o,
    output wire [1:0] failure_position_o,
    output wire [7:0] candidate_evaluations_o
);
    localparam [1:0] STATE_IDLE = 2'd0;
    localparam [1:0] STATE_SEARCH = 2'd1;
    localparam [2:0] ROW_UNASSIGNED = 3'd4;
    reg [1:0] state_q, sa_q, attempt_q;
    reg [3:0] pattern_q;
    reg done_q, repairable_q;
    reg [3:0] selected_valid_q;
    reg [7:0] selected_attempt_q;
    reg [15:0] selected_pattern_q;
    reg [11:0] selected_used_rows_q;
    reg [7:0] selected_used_cols_q;
    reg [15:0] selected_donor_q;
    reg [23:0] row_assignment_q;
    reg [1:0] failure_position_q;
    reg [7:0] candidate_evaluations_q;
    reg [23:0] allocation_next;
    reg [3:0] donor_next;
    reg [2:0] needed_rows;
    reg legal_candidate;
    integer candidate_index;
    reg [2:0] candidate_rows;
    reg [1:0] candidate_cols;
    reg candidate_valid;
    integer line, donor_slot;

    function automatic [2:0] row_assignment_at;
        input [23:0] allocation;
        input [2:0] line_index;
        begin row_assignment_at = allocation[line_index*3 +: 3]; end
    endfunction
    function automatic [1:0] owner_of;
        input [1:0] owner_index;
        begin owner_of = owner_index; end
    endfunction
    function automatic is_adjacent_owner;
        input [1:0] owner;
        input [1:0] borrower;
        begin
            is_adjacent_owner = (owner == 2'd0 && borrower == 2'd1) ||
                                (owner == 2'd1 && (borrower == 2'd0 || borrower == 2'd2)) ||
                                (owner == 2'd2 && (borrower == 2'd1 || borrower == 2'd3)) ||
                                (owner == 2'd3 && borrower == 2'd2);
        end
    endfunction
    function automatic [1:0] max_attempt_for_sa;
        input [1:0] sa;
        begin max_attempt_for_sa = (sa == 2'd0 || sa == 2'd3) ? 2'd1 : 2'd2; end
    endfunction

    always @* begin
        candidate_index = {30'd0, sa_q} * 32'd45 + {30'd0, attempt_q} * 32'd15 + {28'd0, pattern_q};
        candidate_valid = candidate_valid_i[candidate_index];
        candidate_rows = candidate_used_rows_flat_i[candidate_index*3 +: 3];
        candidate_cols = candidate_used_cols_flat_i[candidate_index*2 +: 2];
        allocation_next = row_assignment_q;
        donor_next = 4'b1111;
        needed_rows = candidate_rows;
        legal_candidate = candidate_valid && candidate_cols <= 2;
        // Own rows are allocated private-first because physical IDs are owner
        // ordered and the shareable row is each owner's second line.
        for (line = 0; line < 8; line = line + 1)
            if (owner_of(line[2:1]) == sa_q && row_assignment_at(allocation_next, line[2:0]) == ROW_UNASSIGNED && needed_rows != 0) begin
                allocation_next[line*3 +: 3] = {1'b0, sa_q};
                needed_rows = needed_rows - 1'b1;
            end
        // Only an unassigned shareable row (odd owner-line index) may lend.
        // Static owner identity means a row borrowed earlier is never lendable.
        donor_slot = 0;
        for (line = 0; line < 8; line = line + 1)
            if (needed_rows != 0 && (line % 2 == 1) &&
                row_assignment_at(allocation_next, line[2:0]) == ROW_UNASSIGNED &&
                is_adjacent_owner(owner_of(line[2:1]), sa_q)) begin
                allocation_next[line*3 +: 3] = {1'b0, sa_q};
                if (donor_slot == 0) donor_next[1:0] = owner_of(line[2:1]);
                else if (donor_slot == 1) donor_next[3:2] = owner_of(line[2:1]);
                donor_slot = donor_slot + 1;
                needed_rows = needed_rows - 1'b1;
            end
        if (needed_rows != 0) legal_candidate = 1'b0;
    end

    always @(posedge clk_i) begin
        done_q <= 1'b0;
        if (!rst_ni) begin
            state_q <= STATE_IDLE; sa_q <= 2'd0; attempt_q <= 2'd0; pattern_q <= 4'd0;
            repairable_q <= 1'b0; selected_valid_q <= 4'd0; selected_attempt_q <= 8'd0;
            selected_pattern_q <= 16'd0; selected_used_rows_q <= 12'd0; selected_used_cols_q <= 8'd0;
            selected_donor_q <= 16'hffff; row_assignment_q <= {8{ROW_UNASSIGNED}};
            failure_position_q <= 2'd3; candidate_evaluations_q <= 8'd0;
        end else if (state_q == STATE_IDLE) begin
            if (start_i) begin
                state_q <= STATE_SEARCH; sa_q <= 2'd0; attempt_q <= 2'd0; pattern_q <= 4'd0;
                repairable_q <= 1'b0; selected_valid_q <= 4'd0; selected_attempt_q <= 8'd0;
                selected_pattern_q <= 16'd0; selected_used_rows_q <= 12'd0; selected_used_cols_q <= 8'd0;
                selected_donor_q <= 16'hffff; row_assignment_q <= {8{ROW_UNASSIGNED}};
                failure_position_q <= 2'd3; candidate_evaluations_q <= 8'd0;
            end
        end else begin
            candidate_evaluations_q <= candidate_evaluations_q + 1'b1;
            if (legal_candidate) begin
                row_assignment_q <= allocation_next;
                selected_valid_q[sa_q] <= 1'b1;
                selected_attempt_q[sa_q*2 +: 2] <= attempt_q;
                selected_pattern_q[sa_q*4 +: 4] <= pattern_q + 1'b1;
                selected_used_rows_q[sa_q*3 +: 3] <= candidate_rows;
                selected_used_cols_q[sa_q*2 +: 2] <= candidate_cols;
                selected_donor_q[sa_q*4 +: 4] <= donor_next;
                if (sa_q == 2'd3) begin
                    state_q <= STATE_IDLE; done_q <= 1'b1; repairable_q <= 1'b1;
                end else begin
                    sa_q <= sa_q + 1'b1; attempt_q <= 2'd0; pattern_q <= 4'd0;
                end
            end else if (pattern_q == 4'd14) begin
                pattern_q <= 4'd0;
                if (attempt_q == max_attempt_for_sa(sa_q)) begin
                    state_q <= STATE_IDLE; done_q <= 1'b1; repairable_q <= 1'b0;
                    failure_position_q <= sa_q;
                end else attempt_q <= attempt_q + 1'b1;
            end else pattern_q <= pattern_q + 1'b1;
        end
    end
    assign busy_o = state_q == STATE_SEARCH;
    assign done_o = done_q;
    assign group_repairable_o = repairable_q;
    assign selected_valid_o = selected_valid_q;
    assign selected_attempt_flat_o = selected_attempt_q;
    assign selected_pattern_flat_o = selected_pattern_q;
    assign selected_used_rows_flat_o = selected_used_rows_q;
    assign selected_used_cols_flat_o = selected_used_cols_q;
    assign selected_donor_flat_o = selected_donor_q;
    assign row_assignment_flat_o = row_assignment_q;
    assign failure_position_o = failure_position_q;
    assign candidate_evaluations_o = candidate_evaluations_q;
endmodule

`default_nettype wire
