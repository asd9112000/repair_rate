`default_nettype none

// Correctness-first A->B->C->D GLOBAL search.  The core retains only the
// C++-canonical representative for each (usedRows, usedColumns) demand, then
// exhaustively visits the remaining tuple product with a private physical-row
// ledger at every depth.  No persistent resource state is modified here.
module recam_dss_line1x4_rs2_cs2_m1_normalized_global_core (
    input wire clk_i,
    input wire rst_ni,
    input wire start_i,
    input wire [179:0] candidate_valid_i,
    input wire [539:0] candidate_used_rows_flat_i,
    input wire [359:0] candidate_used_cols_flat_i,
    output wire busy_o,
    output wire search_done_o,
    output wire group_repairable_o,
    output wire [3:0] selected_valid_o,
    output wire [7:0] selected_attempt_flat_o,
    output wire [15:0] selected_pattern_flat_o,
    output wire [11:0] selected_used_rows_flat_o,
    output wire [7:0] selected_used_cols_flat_o,
    output wire [15:0] selected_donor_flat_o,
    output wire [23:0] selected_row_assignment_flat_o,
    output wire [1:0] selected_borrow_count_o,
    output wire [4:0] selected_used_rows_total_o,
    output wire [31:0] candidate_evaluations_o,
    output wire [31:0] dfs_cycles_o
);
    localparam [1:0] STATE_IDLE = 2'd0;
    localparam [1:0] STATE_SEARCH = 2'd1;
    localparam [2:0] ROW_UNASSIGNED = 3'd4;

    reg [1:0] state_q;
    reg [1:0] depth_q;
    reg [23:0] cursor_q;
    reg [95:0] ledger_stack_q;
    reg [7:0] borrow_stack_q;
    reg [19:0] used_rows_stack_q;
    reg [3:0] current_valid_q;
    reg [7:0] current_attempt_q;
    reg [15:0] current_pattern_q;
    reg [11:0] current_used_rows_q;
    reg [7:0] current_used_cols_q;
    reg [15:0] current_donor_q;
    reg best_valid_q;
    reg [3:0] best_selected_valid_q;
    reg [7:0] best_attempt_q;
    reg [15:0] best_pattern_q;
    reg [11:0] best_used_rows_q;
    reg [7:0] best_used_cols_q;
    reg [15:0] best_donor_q;
    reg [23:0] best_allocation_q;
    reg [1:0] best_borrow_count_q;
    reg [4:0] best_used_rows_total_q;
    reg search_done_q;
    reg repairable_q;
    reg [31:0] candidate_evaluations_q;
    reg [31:0] dfs_cycles_q;
    reg [179:0] candidate_valid_q;
    reg [539:0] candidate_rows_q;
    reg [359:0] candidate_cols_q;

    integer candidate_index;
    integer previous_index;
    integer line;
    integer donor_slot;
    reg [5:0] cursor;
    /* verilator lint_off UNUSED */
    reg [5:0] pattern_wide;
    /* verilator lint_on UNUSED */
    reg [1:0] candidate_attempt;
    reg [3:0] candidate_pattern;
    reg [2:0] candidate_rows;
    reg [1:0] candidate_cols;
    reg previous_valid;
    reg [1:0] previous_attempt;
    reg [3:0] previous_pattern;
    reg [2:0] previous_rows;
    reg [1:0] previous_cols;
    reg canonical_candidate;
    reg [23:0] allocation_before;
    reg [23:0] allocation_after;
    reg [2:0] rows_needed;
    reg [1:0] borrow_before;
    reg [1:0] borrow_after;
    reg [2:0] borrow_sum;
    reg [4:0] used_rows_before;
    reg [4:0] used_rows_after;
    reg [3:0] donor_after;
    reg candidate_legal;
    reg [3:0] leaf_valid;
    reg [7:0] leaf_attempt;
    reg [15:0] leaf_pattern;
    reg [11:0] leaf_used_rows;
    reg [7:0] leaf_used_cols;
    reg [15:0] leaf_donor;
    reg leaf_is_better;
    integer next_depth_index;

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

    function automatic pattern_tuple_less;
        input [15:0] left;
        input [15:0] right;
        begin
            pattern_tuple_less = 1'b0;
            if (left[3:0] != right[3:0])
                pattern_tuple_less = left[3:0] < right[3:0];
            else if (left[7:4] != right[7:4])
                pattern_tuple_less = left[7:4] < right[7:4];
            else if (left[11:8] != right[11:8])
                pattern_tuple_less = left[11:8] < right[11:8];
            else if (left[15:12] != right[15:12])
                pattern_tuple_less = left[15:12] < right[15:12];
        end
    endfunction

    function automatic attempt_tuple_less;
        input [7:0] left;
        input [7:0] right;
        begin
            attempt_tuple_less = 1'b0;
            if (left[1:0] != right[1:0])
                attempt_tuple_less = left[1:0] < right[1:0];
            else if (left[3:2] != right[3:2])
                attempt_tuple_less = left[3:2] < right[3:2];
            else if (left[5:4] != right[5:4])
                attempt_tuple_less = left[5:4] < right[5:4];
            else if (left[7:6] != right[7:6])
                attempt_tuple_less = left[7:6] < right[7:6];
        end
    endfunction

    always @* begin
        cursor = cursor_q[depth_q*6 +: 6];
        if (cursor < 6'd15) begin
            candidate_attempt = 2'd0;
            pattern_wide = cursor;
        end else if (cursor < 6'd30) begin
            candidate_attempt = 2'd1;
            pattern_wide = cursor - 6'd15;
        end else begin
            candidate_attempt = 2'd2;
            pattern_wide = cursor - 6'd30;
        end
        candidate_pattern = pattern_wide[3:0];
        candidate_index = depth_q * 32'd45 + {26'd0, cursor};
        case (depth_q)
            2'd0: next_depth_index = 1;
            2'd1: next_depth_index = 2;
            2'd2: next_depth_index = 3;
            default: next_depth_index = 4;
        endcase
        candidate_rows = 3'd0;
        candidate_cols = 2'd0;
        if (cursor < 6'd45) begin
            candidate_rows = candidate_rows_q[candidate_index*3 +: 3];
            candidate_cols = candidate_cols_q[candidate_index*2 +: 2];
        end

        canonical_candidate = 1'b1;
        for (previous_index = 0; previous_index < 45; previous_index = previous_index + 1) begin
            previous_valid = candidate_valid_q[depth_q*45 + previous_index];
            if (previous_index < 15) begin
                previous_attempt = 2'd0;
                previous_pattern = previous_index[3:0];
            end else if (previous_index < 30) begin
                previous_attempt = 2'd1;
                previous_pattern = previous_index[3:0] - 4'd15;
            end else begin
                previous_attempt = 2'd2;
                previous_pattern = previous_index[3:0] - 4'd14;
            end
            previous_rows = candidate_rows_q[(depth_q*45 + previous_index)*3 +: 3];
            previous_cols = candidate_cols_q[(depth_q*45 + previous_index)*2 +: 2];
            if (previous_valid && previous_rows == candidate_rows && previous_cols == candidate_cols &&
                (previous_pattern < candidate_pattern ||
                 (previous_pattern == candidate_pattern && previous_attempt < candidate_attempt)))
                canonical_candidate = 1'b0;
        end

        allocation_before = ledger_stack_q[depth_q*24 +: 24];
        allocation_after = allocation_before;
        rows_needed = candidate_rows;
        donor_after = 4'b1111;
        donor_slot = 0;
        candidate_legal = cursor < 6'd45 && candidate_valid_q[candidate_index] &&
                          canonical_candidate && candidate_cols <= 2;
        for (line = 0; line < 8; line = line + 1)
            if (line[2:1] == depth_q && assignment_at(allocation_after, line[2:0]) == ROW_UNASSIGNED &&
                rows_needed != 0) begin
                allocation_after[line*3 +: 3] = {1'b0, depth_q};
                rows_needed = rows_needed - 1'b1;
            end
        for (line = 0; line < 8; line = line + 1)
            if (rows_needed != 0 && line[0] && assignment_at(allocation_after, line[2:0]) == ROW_UNASSIGNED &&
                is_adjacent_owner(line[2:1], depth_q)) begin
                allocation_after[line*3 +: 3] = {1'b0, depth_q};
                if (donor_slot == 0)
                    donor_after[1:0] = line[2:1];
                else if (donor_slot == 1)
                    donor_after[3:2] = line[2:1];
                donor_slot = donor_slot + 1;
                rows_needed = rows_needed - 1'b1;
            end
        borrow_before = borrow_stack_q[depth_q*2 +: 2];
        borrow_sum = {1'b0, borrow_before} + donor_slot[2:0];
        borrow_after = borrow_sum[1:0];
        used_rows_before = used_rows_stack_q[depth_q*5 +: 5];
        used_rows_after = used_rows_before + {2'd0, candidate_rows};
        if (rows_needed != 0 || donor_slot > 2 || borrow_sum > 3)
            candidate_legal = 1'b0;

        leaf_valid = current_valid_q;
        leaf_attempt = current_attempt_q;
        leaf_pattern = current_pattern_q;
        leaf_used_rows = current_used_rows_q;
        leaf_used_cols = current_used_cols_q;
        leaf_donor = current_donor_q;
        leaf_valid[depth_q] = 1'b1;
        leaf_attempt[depth_q*2 +: 2] = candidate_attempt;
        leaf_pattern[depth_q*4 +: 4] = candidate_pattern + 1'b1;
        leaf_used_rows[depth_q*3 +: 3] = candidate_rows;
        leaf_used_cols[depth_q*2 +: 2] = candidate_cols;
        leaf_donor[depth_q*4 +: 4] = donor_after;
        leaf_is_better = !best_valid_q || borrow_after < best_borrow_count_q ||
                         (borrow_after == best_borrow_count_q && used_rows_after < best_used_rows_total_q) ||
                         (borrow_after == best_borrow_count_q && used_rows_after == best_used_rows_total_q &&
                          pattern_tuple_less(leaf_pattern, best_pattern_q)) ||
                         (borrow_after == best_borrow_count_q && used_rows_after == best_used_rows_total_q &&
                          leaf_pattern == best_pattern_q && attempt_tuple_less(leaf_attempt, best_attempt_q));
    end

    always @(posedge clk_i) begin
        search_done_q <= 1'b0;
        if (!rst_ni) begin
            state_q <= STATE_IDLE;
            depth_q <= 2'd0;
            cursor_q <= 24'd0;
            ledger_stack_q <= 96'd0;
            borrow_stack_q <= 8'd0;
            used_rows_stack_q <= 20'd0;
            current_valid_q <= 4'd0;
            current_attempt_q <= 8'd0;
            current_pattern_q <= 16'd0;
            current_used_rows_q <= 12'd0;
            current_used_cols_q <= 8'd0;
            current_donor_q <= 16'hffff;
            best_valid_q <= 1'b0;
            best_selected_valid_q <= 4'd0;
            best_attempt_q <= 8'd0;
            best_pattern_q <= 16'd0;
            best_used_rows_q <= 12'd0;
            best_used_cols_q <= 8'd0;
            best_donor_q <= 16'hffff;
            best_allocation_q <= {8{ROW_UNASSIGNED}};
            best_borrow_count_q <= 2'd0;
            best_used_rows_total_q <= 5'd0;
            repairable_q <= 1'b0;
            candidate_evaluations_q <= 32'd0;
            dfs_cycles_q <= 32'd0;
            candidate_valid_q <= 180'd0;
            candidate_rows_q <= 540'd0;
            candidate_cols_q <= 360'd0;
        end else if (state_q == STATE_IDLE) begin
            if (start_i) begin
                state_q <= STATE_SEARCH;
                depth_q <= 2'd0;
                cursor_q <= 24'd0;
                ledger_stack_q <= {4{ {8{ROW_UNASSIGNED}} }};
                borrow_stack_q <= 8'd0;
                used_rows_stack_q <= 20'd0;
                current_valid_q <= 4'd0;
                current_attempt_q <= 8'd0;
                current_pattern_q <= 16'd0;
                current_used_rows_q <= 12'd0;
                current_used_cols_q <= 8'd0;
                current_donor_q <= 16'hffff;
                best_valid_q <= 1'b0;
                best_selected_valid_q <= 4'd0;
                best_attempt_q <= 8'd0;
                best_pattern_q <= 16'd0;
                best_used_rows_q <= 12'd0;
                best_used_cols_q <= 8'd0;
                best_donor_q <= 16'hffff;
                best_allocation_q <= {8{ROW_UNASSIGNED}};
                best_borrow_count_q <= 2'd0;
                best_used_rows_total_q <= 5'd0;
                repairable_q <= 1'b0;
                candidate_evaluations_q <= 32'd0;
                dfs_cycles_q <= 32'd0;
                candidate_valid_q <= candidate_valid_i;
                candidate_rows_q <= candidate_used_rows_flat_i;
                candidate_cols_q <= candidate_used_cols_flat_i;
            end
        end else begin
            candidate_evaluations_q <= candidate_evaluations_q + 1'b1;
            dfs_cycles_q <= dfs_cycles_q + 1'b1;
            if (candidate_legal) begin
                if (depth_q == 2'd3) begin
                    if (leaf_is_better) begin
                        best_valid_q <= 1'b1;
                        best_selected_valid_q <= leaf_valid;
                        best_attempt_q <= leaf_attempt;
                        best_pattern_q <= leaf_pattern;
                        best_used_rows_q <= leaf_used_rows;
                        best_used_cols_q <= leaf_used_cols;
                        best_donor_q <= leaf_donor;
                        best_allocation_q <= allocation_after;
                        best_borrow_count_q <= borrow_after;
                        best_used_rows_total_q <= used_rows_after;
                    end
                    if (cursor < 6'd44)
                        cursor_q[18 +: 6] <= cursor + 1'b1;
                    else begin
                        depth_q <= 2'd2;
                        cursor_q[12 +: 6] <= cursor_q[12 +: 6] + 1'b1;
                    end
                end else begin
                    current_valid_q[depth_q] <= 1'b1;
                    current_attempt_q[depth_q*2 +: 2] <= candidate_attempt;
                    current_pattern_q[depth_q*4 +: 4] <= candidate_pattern + 1'b1;
                    current_used_rows_q[depth_q*3 +: 3] <= candidate_rows;
                    current_used_cols_q[depth_q*2 +: 2] <= candidate_cols;
                    current_donor_q[depth_q*4 +: 4] <= donor_after;
                    ledger_stack_q[next_depth_index*24 +: 24] <= allocation_after;
                    borrow_stack_q[next_depth_index*2 +: 2] <= borrow_after;
                    used_rows_stack_q[next_depth_index*5 +: 5] <= used_rows_after;
                    depth_q <= depth_q + 1'b1;
                    cursor_q[next_depth_index*6 +: 6] <= 6'd0;
                end
            end else if (cursor < 6'd44) begin
                cursor_q[depth_q*6 +: 6] <= cursor + 1'b1;
            end else if (depth_q != 2'd0) begin
                depth_q <= depth_q - 1'b1;
                cursor_q[(depth_q-1'b1)*6 +: 6] <= cursor_q[(depth_q-1'b1)*6 +: 6] + 1'b1;
                current_valid_q[depth_q] <= 1'b0;
            end else begin
                state_q <= STATE_IDLE;
                search_done_q <= 1'b1;
                repairable_q <= best_valid_q;
            end
        end
    end

    assign busy_o = state_q == STATE_SEARCH;
    assign search_done_o = search_done_q;
    assign group_repairable_o = repairable_q;
    assign selected_valid_o = best_selected_valid_q;
    assign selected_attempt_flat_o = best_attempt_q;
    assign selected_pattern_flat_o = best_pattern_q;
    assign selected_used_rows_flat_o = best_used_rows_q;
    assign selected_used_cols_flat_o = best_used_cols_q;
    assign selected_donor_flat_o = best_donor_q;
    assign selected_row_assignment_flat_o = best_allocation_q;
    assign selected_borrow_count_o = best_borrow_count_q;
    assign selected_used_rows_total_o = best_used_rows_total_q;
    assign candidate_evaluations_o = candidate_evaluations_q;
    assign dfs_cycles_o = dfs_cycles_q;
endmodule

`default_nettype wire
