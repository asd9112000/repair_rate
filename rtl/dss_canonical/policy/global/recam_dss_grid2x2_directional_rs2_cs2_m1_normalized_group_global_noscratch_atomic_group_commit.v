`default_nettype none

// Stages a complete canonical GLOBAL tuple in private shadow state. Persistent
// directional resource state changes only on the publish edge after all four
// tuple entries satisfy the same release-obligation rule as the DFS core.
module recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_atomic_group_commit (
    input  wire         clk_i,
    input  wire         rst_ni,
    input  wire         start_i,
    input  wire [3:0]   selected_valid_i,
    input  wire [7:0]   selected_action_flat_i,
    input  wire [3:0]   selected_release_i,
    input  wire [3:0]   selected_borrow_i,
    input  wire [7:0]   selected_donor_flat_i,
    output wire         busy_o,
    output wire         commit_accepted_o,
    output wire         commit_error_o,
    output wire [3:0]   resource_released_o,
    output wire [3:0]   resource_borrowed_o,
    output wire [7:0]   borrower_id_flat_o
);
    localparam [1:0] STATE_IDLE = 2'd0;
    localparam [1:0] STATE_STAGE = 2'd1;
    localparam [1:0] STATE_PUBLISH = 2'd2;

    reg [1:0] state_q;
    reg [1:0] stage_q;
    reg [3:0] persistent_released_q;
    reg [3:0] persistent_borrowed_q;
    reg [7:0] persistent_borrower_id_q;
    reg [3:0] shadow_released_q;
    reg [3:0] shadow_borrowed_q;
    reg [7:0] shadow_borrower_id_q;
    reg [3:0] shadow_requirement_q;
    reg [1:0] shadow_borrow_count_q;
    reg commit_accepted_q;
    reg commit_error_q;

    reg [1:0] selected_action;
    reg selected_release;
    reg selected_borrow;
    reg [1:0] selected_donor;
    reg explicit_release;
    reg [1:0] release_resource;
    reg [1:0] donor_resource;
    reg [1:0] donor_owner;
    reg stage_legal;
    reg [3:0] released_after;
    reg [3:0] borrowed_after;
    reg [7:0] borrower_id_after;
    reg [3:0] requirement_after;
    reg [1:0] borrow_count_after;

    always @* begin
        selected_action = selected_action_flat_i[stage_q*2 +: 2];
        selected_release = selected_release_i[stage_q];
        selected_borrow = selected_borrow_i[stage_q];
        selected_donor = selected_donor_flat_i[stage_q*2 +: 2];
        explicit_release = (selected_action == 2'd1) || (selected_action == 2'd3);
        release_resource = 2'd0;
        donor_resource = 2'd0;
        donor_owner = 2'd0;
        case (stage_q)
            2'd0: begin release_resource = 2'd0; donor_resource = 2'd2; donor_owner = 2'd1; end
            2'd1: begin release_resource = 2'd2; donor_resource = 2'd1; donor_owner = 2'd3; end
            2'd2: begin release_resource = 2'd3; donor_resource = 2'd0; donor_owner = 2'd0; end
            default: begin release_resource = 2'd1; donor_resource = 2'd3; donor_owner = 2'd2; end
        endcase

        released_after = shadow_released_q;
        borrowed_after = shadow_borrowed_q;
        borrower_id_after = shadow_borrower_id_q;
        requirement_after = shadow_requirement_q;
        borrow_count_after = shadow_borrow_count_q;
        stage_legal = selected_valid_i[stage_q] && (selected_donor == donor_resource || !selected_borrow);

        if (shadow_requirement_q[release_resource] && !(explicit_release && selected_release))
            stage_legal = 1'b0;
        if (selected_release) begin
            if (shadow_released_q[release_resource])
                stage_legal = 1'b0;
            released_after[release_resource] = 1'b1;
            if (explicit_release)
                requirement_after[release_resource] = 1'b0;
        end
        if (selected_borrow) begin
            if (shadow_borrow_count_q == 2'd3 || shadow_borrowed_q[donor_resource])
                stage_legal = 1'b0;
            else if (!shadow_released_q[donor_resource] && donor_owner <= stage_q)
                stage_legal = 1'b0;
            else begin
                borrowed_after[donor_resource] = 1'b1;
                borrower_id_after[donor_resource*2 +: 2] = stage_q;
                borrow_count_after = shadow_borrow_count_q + 1'b1;
                if (!shadow_released_q[donor_resource])
                    requirement_after[donor_resource] = 1'b1;
            end
        end
        if (stage_q == 2'd3 && requirement_after != 4'd0)
            stage_legal = 1'b0;
    end

    always @(posedge clk_i) begin
        commit_accepted_q <= 1'b0;
        commit_error_q <= 1'b0;
        if (!rst_ni) begin
            state_q <= STATE_IDLE;
            stage_q <= 2'd0;
            persistent_released_q <= 4'd0;
            persistent_borrowed_q <= 4'd0;
            persistent_borrower_id_q <= 8'd0;
            shadow_released_q <= 4'd0;
            shadow_borrowed_q <= 4'd0;
            shadow_borrower_id_q <= 8'd0;
            shadow_requirement_q <= 4'd0;
            shadow_borrow_count_q <= 2'd0;
        end else if (state_q == STATE_IDLE) begin
            if (start_i) begin
                if (persistent_released_q != 4'd0 || persistent_borrowed_q != 4'd0) begin
                    commit_error_q <= 1'b1;
                end else begin
                    state_q <= STATE_STAGE;
                    stage_q <= 2'd0;
                    shadow_released_q <= persistent_released_q;
                    shadow_borrowed_q <= persistent_borrowed_q;
                    shadow_borrower_id_q <= persistent_borrower_id_q;
                    shadow_requirement_q <= 4'd0;
                    shadow_borrow_count_q <= 2'd0;
                end
            end
        end else if (state_q == STATE_STAGE) begin
            if (!stage_legal) begin
                state_q <= STATE_IDLE;
                commit_error_q <= 1'b1;
            end else begin
                shadow_released_q <= released_after;
                shadow_borrowed_q <= borrowed_after;
                shadow_borrower_id_q <= borrower_id_after;
                shadow_requirement_q <= requirement_after;
                shadow_borrow_count_q <= borrow_count_after;
                if (stage_q == 2'd3)
                    state_q <= STATE_PUBLISH;
                else
                    stage_q <= stage_q + 1'b1;
            end
        end else begin
            persistent_released_q <= shadow_released_q;
            persistent_borrowed_q <= shadow_borrowed_q;
            persistent_borrower_id_q <= shadow_borrower_id_q;
            state_q <= STATE_IDLE;
            commit_accepted_q <= 1'b1;
        end
    end

    assign busy_o = state_q != STATE_IDLE;
    assign commit_accepted_o = commit_accepted_q;
    assign commit_error_o = commit_error_q;
    assign resource_released_o = persistent_released_q;
    assign resource_borrowed_o = persistent_borrowed_q;
    assign borrower_id_flat_o = persistent_borrower_id_q;
endmodule

`default_nettype wire
