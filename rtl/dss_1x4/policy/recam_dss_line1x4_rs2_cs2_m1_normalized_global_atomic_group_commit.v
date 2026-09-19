`default_nettype none

// Replays a selected 1x4 tuple into private shadow ownership and publishes the
// eight-row persistent ledger only after A, B, C, and D all validate.
module recam_dss_line1x4_rs2_cs2_m1_normalized_global_atomic_group_commit (
    input wire clk_i,
    input wire rst_ni,
    input wire start_i,
    input wire [3:0] selected_valid_i,
    input wire [11:0] selected_used_rows_flat_i,
    input wire [7:0] selected_used_cols_flat_i,
    input wire [15:0] selected_donor_flat_i,
    input wire [23:0] selected_row_assignment_flat_i,
    output wire busy_o,
    output wire commit_accepted_o,
    output wire commit_error_o,
    output wire [23:0] persistent_row_assignment_flat_o
);
    localparam [1:0] STATE_IDLE = 2'd0;
    localparam [1:0] STATE_STAGE = 2'd1;
    localparam [1:0] STATE_PUBLISH = 2'd2;
    localparam [2:0] ROW_UNASSIGNED = 3'd4;

    reg [1:0] state_q;
    reg [1:0] stage_q;
    reg [23:0] persistent_allocation_q;
    reg [23:0] shadow_allocation_q;
    reg [1:0] shadow_borrow_count_q;
    reg commit_accepted_q;
    reg commit_error_q;

    integer line;
    integer donor_slot;
    reg [2:0] selected_rows;
    reg [1:0] selected_cols;
    reg [3:0] selected_donor;
    reg [23:0] allocation_after;
    reg [2:0] rows_needed;
    reg [3:0] donor_after;
    reg [1:0] borrow_after;
    reg [2:0] borrow_sum;
    reg stage_legal;

    function automatic [2:0] assignment_at;
        input [23:0] allocation;
        input [2:0] line_index;
        begin
            assignment_at = allocation[line_index*3 +: 3];
        end
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

    always @* begin
        selected_rows = selected_used_rows_flat_i[stage_q*3 +: 3];
        selected_cols = selected_used_cols_flat_i[stage_q*2 +: 2];
        selected_donor = selected_donor_flat_i[stage_q*4 +: 4];
        allocation_after = shadow_allocation_q;
        rows_needed = selected_rows;
        donor_after = 4'b1111;
        donor_slot = 0;
        stage_legal = selected_valid_i[stage_q] && selected_cols <= 2;
        for (line = 0; line < 8; line = line + 1)
            if (line[2:1] == stage_q && assignment_at(allocation_after, line[2:0]) == ROW_UNASSIGNED &&
                rows_needed != 0) begin
                allocation_after[line*3 +: 3] = {1'b0, stage_q};
                rows_needed = rows_needed - 1'b1;
            end
        for (line = 0; line < 8; line = line + 1)
            if (rows_needed != 0 && line[0] && assignment_at(allocation_after, line[2:0]) == ROW_UNASSIGNED &&
                is_adjacent_owner(line[2:1], stage_q)) begin
                allocation_after[line*3 +: 3] = {1'b0, stage_q};
                if (donor_slot == 0)
                    donor_after[1:0] = line[2:1];
                else if (donor_slot == 1)
                    donor_after[3:2] = line[2:1];
                donor_slot = donor_slot + 1;
                rows_needed = rows_needed - 1'b1;
            end
        borrow_sum = {1'b0, shadow_borrow_count_q} + donor_slot[2:0];
        borrow_after = borrow_sum[1:0];
        if (rows_needed != 0 || donor_slot > 2 || borrow_sum > 3 || selected_donor != donor_after)
            stage_legal = 1'b0;
        if (stage_q == 2'd3 && allocation_after != selected_row_assignment_flat_i)
            stage_legal = 1'b0;
    end

    always @(posedge clk_i) begin
        commit_accepted_q <= 1'b0;
        commit_error_q <= 1'b0;
        if (!rst_ni) begin
            state_q <= STATE_IDLE;
            stage_q <= 2'd0;
            persistent_allocation_q <= {8{ROW_UNASSIGNED}};
            shadow_allocation_q <= {8{ROW_UNASSIGNED}};
            shadow_borrow_count_q <= 2'd0;
        end else if (state_q == STATE_IDLE) begin
            if (start_i) begin
                if (persistent_allocation_q != {8{ROW_UNASSIGNED}} || selected_valid_i != 4'hf) begin
                    commit_error_q <= 1'b1;
                end else begin
                    state_q <= STATE_STAGE;
                    stage_q <= 2'd0;
                    shadow_allocation_q <= persistent_allocation_q;
                    shadow_borrow_count_q <= 2'd0;
                end
            end
        end else if (state_q == STATE_STAGE) begin
            if (!stage_legal) begin
                state_q <= STATE_IDLE;
                commit_error_q <= 1'b1;
            end else begin
                shadow_allocation_q <= allocation_after;
                shadow_borrow_count_q <= borrow_after;
                if (stage_q == 2'd3)
                    state_q <= STATE_PUBLISH;
                else
                    stage_q <= stage_q + 1'b1;
            end
        end else begin
            persistent_allocation_q <= shadow_allocation_q;
            state_q <= STATE_IDLE;
            commit_accepted_q <= 1'b1;
        end
    end

    assign busy_o = state_q != STATE_IDLE;
    assign commit_accepted_o = commit_accepted_q;
    assign commit_error_o = commit_error_q;
    assign persistent_row_assignment_flat_o = persistent_allocation_q;
endmodule

`default_nettype wire
