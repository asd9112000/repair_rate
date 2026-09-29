`default_nettype none

// G2X2_R EARLY CA-LIVE controller: live state remains upstream of this policy.
module recam_dss_g2x2_r_static_early_live_state_core (
    input logic clk_i, input logic rst_ni,
    input logic state_update_i, input logic [1:0] state_sa_i,
    input logic test_done_valid_i, input logic [1:0] test_done_sa_i,
    input logic candidate_valid_i,
    output logic scan_active_o, output logic [1:0] active_sa_o,
    output logic [1:0] scan_slot_o, output logic [2:0] scan_config_id_o,
    output logic solution_commit_o, output logic done_o,
    output logic group_repairable_o, output logic [1:0] failure_position_o
);
    typedef enum logic [1:0] { ST_WAIT_UPDATE, ST_SCAN, ST_HOLD, ST_DONE } state_t;
    state_t state_q;
    logic [1:0] active_sa_q, priority_rank_q, selected_slot;
    logic test_done_seen_q, a_released_q, a_borrows_q, b_borrows_q, c_released_q;
    logic slot_releases, slot_borrows, prefix_compatible;
    logic active_state_update, active_test_done, candidate_accepted, commit_now;

    always_comb begin
        case (priority_rank_q)
            2'd0: selected_slot = 2'd1;
            2'd1: selected_slot = 2'd0;
            2'd2: selected_slot = 2'd3;
            default: selected_slot = 2'd2;
        endcase
        case (selected_slot)
            2'd1: scan_config_id_o = 3'd4;
            2'd2: scan_config_id_o = 3'd2;
            default: scan_config_id_o = 3'd0;
        endcase
        slot_releases = (selected_slot == 2'd1) || (selected_slot == 2'd3);
        slot_borrows = (selected_slot == 2'd2) || (selected_slot == 2'd3);
        case (active_sa_q)
            2'd0: prefix_compatible = 1'b1;
            2'd1: prefix_compatible = !a_borrows_q || slot_releases;
            2'd2: prefix_compatible = !slot_borrows || a_released_q;
            default: prefix_compatible = (!slot_borrows || c_released_q) &&
                                         (!b_borrows_q || slot_releases);
        endcase
        active_state_update = state_update_i && (state_sa_i == active_sa_q);
        active_test_done = test_done_valid_i && (test_done_sa_i == active_sa_q);
        candidate_accepted = (state_q == ST_SCAN) && candidate_valid_i &&
                             prefix_compatible && !active_state_update;
        commit_now = !active_state_update &&
            ((candidate_accepted && (test_done_seen_q || active_test_done)) ||
             ((state_q == ST_HOLD) && active_test_done));
        scan_active_o = state_q == ST_SCAN;
        active_sa_o = active_sa_q;
        scan_slot_o = selected_slot;
        solution_commit_o = commit_now;
    end

    always_ff @(posedge clk_i) begin
        done_o <= 1'b0;
        if (!rst_ni) begin
            state_q <= ST_WAIT_UPDATE; active_sa_q <= 2'd0; priority_rank_q <= 2'd0;
            test_done_seen_q <= 1'b0; a_released_q <= 1'b0; a_borrows_q <= 1'b0;
            b_borrows_q <= 1'b0; c_released_q <= 1'b0; group_repairable_o <= 1'b0;
            failure_position_o <= 2'd0;
        end else if (state_q != ST_DONE) begin
            if (active_state_update) begin
                state_q <= ST_SCAN; priority_rank_q <= 2'd0;
                if (active_test_done) test_done_seen_q <= 1'b1;
            end else if (commit_now) begin
                case (active_sa_q)
                    2'd0: begin a_released_q <= slot_releases; a_borrows_q <= slot_borrows; end
                    2'd1: b_borrows_q <= slot_borrows;
                    2'd2: c_released_q <= slot_releases;
                    default: begin end
                endcase
                test_done_seen_q <= 1'b0; priority_rank_q <= 2'd0;
                if (active_sa_q == 2'd3) begin
                    state_q <= ST_DONE; done_o <= 1'b1; group_repairable_o <= 1'b1;
                end else begin
                    active_sa_q <= active_sa_q + 2'd1; state_q <= ST_WAIT_UPDATE;
                end
            end else begin
                if (active_test_done) test_done_seen_q <= 1'b1;
                if (state_q == ST_SCAN) begin
                    if (candidate_accepted) state_q <= ST_HOLD;
                    else if (priority_rank_q == 2'd3) begin
                        state_q <= ST_DONE; done_o <= 1'b1; group_repairable_o <= 1'b0;
                        failure_position_o <= active_sa_q;
                    end else priority_rank_q <= priority_rank_q + 2'd1;
                end
            end
        end
    end
endmodule

`default_nettype wire
