`default_nettype none

// Hardware-first bounded DFS for the frozen four-SA directional GLOBAL policy.
// Candidate index = SA*40 + action*10 + zero-based PatternID, where action is
// 0:L, 1:R, 2:B, 3:RB.  Search order is R,L,RB,B then PatternID ascending.
module recam_dss_canonical_global_noscratch_core (
    input  wire         clk_i,
    input  wire         rst_ni,
    input  wire         start_i,
    input  wire [159:0] candidate_valid_i,
    input  wire [159:0] candidate_release_i,
    input  wire [159:0] candidate_borrow_i,
    output wire         busy_o,
    output wire         done_o,
    output wire         group_repairable_o,
    output wire [3:0]   selected_valid_o,
    output wire [7:0]   selected_action_flat_o,
    output wire [11:0]  selected_config_flat_o,
    output wire [15:0]  selected_pattern_flat_o,
    output wire [7:0]   selected_donor_flat_o,
    output wire [3:0]   selected_release_o,
    output wire [3:0]   selected_borrow_o,
    output wire [3:0]   final_released_mask_o,
    output wire [3:0]   final_used_mask_o
);
    localparam [1:0] STATE_IDLE = 2'd0;
    localparam [1:0] STATE_SEARCH = 2'd1;

    reg [1:0] state_q, state_d;
    reg [1:0] depth_q, depth_d;
    reg [23:0] cursor_q, cursor_d;       // four 6-bit cursors, 0..39
    reg [15:0] released_stack_q, released_stack_d;
    reg [15:0] used_stack_q, used_stack_d;
    reg [15:0] req_stack_q, req_stack_d;
    reg [7:0] borrow_count_stack_q, borrow_count_stack_d;
    reg [159:0] candidate_valid_q, candidate_valid_d;
    reg [159:0] candidate_release_q, candidate_release_d;
    reg [159:0] candidate_borrow_q, candidate_borrow_d;
    reg done_q, done_d;
    reg repairable_q, repairable_d;
    reg [3:0] selected_valid_q, selected_valid_d;
    reg [7:0] selected_action_q, selected_action_d;
    reg [11:0] selected_config_q, selected_config_d;
    reg [15:0] selected_pattern_q, selected_pattern_d;
    reg [7:0] selected_donor_q, selected_donor_d;
    reg [3:0] selected_release_q, selected_release_d;
    reg [3:0] selected_borrow_q, selected_borrow_d;
    reg [3:0] final_released_q, final_released_d;
    reg [3:0] final_used_q, final_used_d;

    reg [5:0] cursor;
    reg [3:0] pattern;
    /* verilator lint_off UNUSED */
    reg [5:0] pattern_wide;
    /* verilator lint_on UNUSED */
    reg [1:0] action;
    reg [2:0] config_id;
    reg [7:0] candidate_index;
    reg [7:0] sa_base;
    reg [7:0] action_base;
    reg candidate_valid;
    reg actual_release;
    reg actual_borrow;
    reg explicit_release;
    reg [1:0] release_resource;
    reg [1:0] donor_resource;
    reg [1:0] donor_owner;
    reg [3:0] released_before, used_before, req_before;
    reg [1:0] borrow_count_before;
    reg [3:0] released_after, used_after, req_after;
    reg [1:0] borrow_count_after;
    reg candidate_legal;

    // State register.
    always @(posedge clk_i) begin
        if (!rst_ni)
            state_q <= STATE_IDLE;
        else
            state_q <= state_d;
    end

    // Datapath/output registers.
    always @(posedge clk_i) begin
        if (!rst_ni) begin
            depth_q <= 2'd0;
            cursor_q <= 24'd0;
            released_stack_q <= 16'd0;
            used_stack_q <= 16'd0;
            req_stack_q <= 16'd0;
            borrow_count_stack_q <= 8'd0;
            candidate_valid_q <= 160'd0;
            candidate_release_q <= 160'd0;
            candidate_borrow_q <= 160'd0;
            done_q <= 1'b0;
            repairable_q <= 1'b0;
            selected_valid_q <= 4'd0;
            selected_action_q <= 8'd0;
            selected_config_q <= 12'd0;
            selected_pattern_q <= 16'd0;
            selected_donor_q <= 8'd0;
            selected_release_q <= 4'd0;
            selected_borrow_q <= 4'd0;
            final_released_q <= 4'd0;
            final_used_q <= 4'd0;
        end else begin
            depth_q <= depth_d;
            cursor_q <= cursor_d;
            released_stack_q <= released_stack_d;
            used_stack_q <= used_stack_d;
            req_stack_q <= req_stack_d;
            borrow_count_stack_q <= borrow_count_stack_d;
            candidate_valid_q <= candidate_valid_d;
            candidate_release_q <= candidate_release_d;
            candidate_borrow_q <= candidate_borrow_d;
            done_q <= done_d;
            repairable_q <= repairable_d;
            selected_valid_q <= selected_valid_d;
            selected_action_q <= selected_action_d;
            selected_config_q <= selected_config_d;
            selected_pattern_q <= selected_pattern_d;
            selected_donor_q <= selected_donor_d;
            selected_release_q <= selected_release_d;
            selected_borrow_q <= selected_borrow_d;
            final_released_q <= final_released_d;
            final_used_q <= final_used_d;
        end
    end

    // Next-state and bounded DFS datapath.
    always @* begin
        state_d = state_q;
        depth_d = depth_q;
        cursor_d = cursor_q;
        released_stack_d = released_stack_q;
        used_stack_d = used_stack_q;
        req_stack_d = req_stack_q;
        borrow_count_stack_d = borrow_count_stack_q;
        candidate_valid_d = candidate_valid_q;
        candidate_release_d = candidate_release_q;
        candidate_borrow_d = candidate_borrow_q;
        done_d = 1'b0;
        repairable_d = repairable_q;
        selected_valid_d = selected_valid_q;
        selected_action_d = selected_action_q;
        selected_config_d = selected_config_q;
        selected_pattern_d = selected_pattern_q;
        selected_donor_d = selected_donor_q;
        selected_release_d = selected_release_q;
        selected_borrow_d = selected_borrow_q;
        final_released_d = final_released_q;
        final_used_d = final_used_q;

        cursor = cursor_q[depth_q*6 +: 6];
        if (cursor < 6'd10) begin
            action = 2'd1; pattern_wide = cursor;
        end else if (cursor < 6'd20) begin
            action = 2'd0; pattern_wide = cursor - 6'd10;
        end else if (cursor < 6'd30) begin
            action = 2'd3; pattern_wide = cursor - 6'd20;
        end else begin
            action = 2'd2; pattern_wide = cursor - 6'd30;
        end
        pattern = pattern_wide[3:0];
        case (depth_q)
            2'd0: sa_base = 8'd0;
            2'd1: sa_base = 8'd40;
            2'd2: sa_base = 8'd80;
            default: sa_base = 8'd120;
        endcase
        case (action)
            2'd0: action_base = 8'd0;
            2'd1: action_base = 8'd10;
            2'd2: action_base = 8'd20;
            default: action_base = 8'd30;
        endcase
        if (depth_q == 2'd0 || depth_q == 2'd3) begin
            case (action)
                2'd0: config_id = 3'd0;
                2'd1: config_id = 3'd4;
                2'd2: config_id = 3'd5;
                default: config_id = 3'd6;
            endcase
        end else begin
            config_id = {1'b0, action};
        end
        candidate_index = sa_base + action_base + {4'd0, pattern};
        candidate_valid = (cursor < 6'd40) && candidate_valid_q[candidate_index];
        actual_release = candidate_release_q[candidate_index];
        actual_borrow = candidate_borrow_q[candidate_index];
        explicit_release = action == 2'd1 || action == 2'd3;
        case (depth_q)
            2'd0: begin release_resource = 2'd0; donor_resource = 2'd2; donor_owner = 2'd1; end
            2'd1: begin release_resource = 2'd2; donor_resource = 2'd1; donor_owner = 2'd3; end
            2'd2: begin release_resource = 2'd3; donor_resource = 2'd0; donor_owner = 2'd0; end
            default: begin release_resource = 2'd1; donor_resource = 2'd3; donor_owner = 2'd2; end
        endcase
        released_before = released_stack_q[depth_q*4 +: 4];
        used_before = used_stack_q[depth_q*4 +: 4];
        req_before = req_stack_q[depth_q*4 +: 4];
        borrow_count_before = borrow_count_stack_q[depth_q*2 +: 2];
        released_after = released_before;
        used_after = used_before;
        req_after = req_before;
        borrow_count_after = borrow_count_before;
        candidate_legal = candidate_valid;

        if (req_before[release_resource] &&
            !(explicit_release && actual_release))
            candidate_legal = 1'b0;
        if (actual_release) begin
            released_after[release_resource] = 1'b1;
            if (explicit_release)
                req_after[release_resource] = 1'b0;
        end
        if (actual_borrow) begin
            if (borrow_count_before == 2'd3 || used_before[donor_resource])
                candidate_legal = 1'b0;
            else if (!released_before[donor_resource] && donor_owner <= depth_q)
                candidate_legal = 1'b0;
            else begin
                used_after[donor_resource] = 1'b1;
                borrow_count_after = borrow_count_before + 1'b1;
                if (!released_before[donor_resource])
                    req_after[donor_resource] = 1'b1;
            end
        end

        if (state_q == STATE_IDLE) begin
            if (start_i) begin
                state_d = STATE_SEARCH;
                depth_d = 2'd0;
                cursor_d = 24'd0;
                released_stack_d = 16'd0;
                used_stack_d = 16'd0;
                req_stack_d = 16'd0;
                borrow_count_stack_d = 8'd0;
                candidate_valid_d = candidate_valid_i;
                candidate_release_d = candidate_release_i;
                candidate_borrow_d = candidate_borrow_i;
                repairable_d = 1'b0;
                selected_valid_d = 4'd0;
                selected_action_d = 8'd0;
                selected_config_d = 12'd0;
                selected_pattern_d = 16'd0;
                selected_donor_d = 8'd0;
                selected_release_d = 4'd0;
                selected_borrow_d = 4'd0;
                final_released_d = 4'd0;
                final_used_d = 4'd0;
            end
        end else begin
            if (candidate_legal) begin
                selected_valid_d[depth_q] = 1'b1;
                selected_action_d[depth_q*2 +: 2] = action;
                selected_config_d[depth_q*3 +: 3] = config_id;
                selected_pattern_d[depth_q*4 +: 4] = pattern + 1'b1;
                selected_donor_d[depth_q*2 +: 2] = donor_resource;
                selected_release_d[depth_q] = actual_release;
                selected_borrow_d[depth_q] = actual_borrow;
                if (depth_q == 2'd3) begin
                    if (req_after == 4'd0) begin
                        state_d = STATE_IDLE;
                        done_d = 1'b1;
                        repairable_d = 1'b1;
                        final_released_d = released_after;
                        final_used_d = used_after;
                    end else if (cursor >= 6'd39) begin
                        depth_d = 2'd2;
                        cursor_d[12 +: 6] = cursor_q[12 +: 6] + 1'b1;
                    end else begin
                        cursor_d[18 +: 6] = cursor + 1'b1;
                    end
                end else begin
                    depth_d = depth_q + 1'b1;
                    cursor_d[(depth_q+1'b1)*6 +: 6] = 6'd0;
                    released_stack_d[(depth_q+1'b1)*4 +: 4] = released_after;
                    used_stack_d[(depth_q+1'b1)*4 +: 4] = used_after;
                    req_stack_d[(depth_q+1'b1)*4 +: 4] = req_after;
                    borrow_count_stack_d[(depth_q+1'b1)*2 +: 2] = borrow_count_after;
                end
            end else if (cursor < 6'd39) begin
                cursor_d[depth_q*6 +: 6] = cursor + 1'b1;
            end else if (depth_q != 2'd0) begin
                depth_d = depth_q - 1'b1;
                cursor_d[(depth_q-1'b1)*6 +: 6] =
                    cursor_q[(depth_q-1'b1)*6 +: 6] + 1'b1;
                selected_valid_d[depth_q] = 1'b0;
            end else begin
                state_d = STATE_IDLE;
                done_d = 1'b1;
                repairable_d = 1'b0;
                selected_valid_d = 4'd0;
            end
        end
    end

    assign busy_o = state_q == STATE_SEARCH;
    assign done_o = done_q;
    assign group_repairable_o = repairable_q;
    assign selected_valid_o = selected_valid_q;
    assign selected_action_flat_o = selected_action_q;
    assign selected_config_flat_o = selected_config_q;
    assign selected_pattern_flat_o = selected_pattern_q;
    assign selected_donor_flat_o = selected_donor_q;
    assign selected_release_o = selected_release_q;
    assign selected_borrow_o = selected_borrow_q;
    assign final_released_mask_o = final_released_q;
    assign final_used_mask_o = final_used_q;
endmodule

`default_nettype wire
