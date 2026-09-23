`default_nettype none

// G2X2_R-static streaming EARLY controller.  Prefix legality is defined only
// by the frozen 81-path slot universe; it does not reconstruct physical demand.
module recam_dss_g2x2_r_static_early_core (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic start_i,
    input  logic candidate_valid_i,
    output logic [1:0] current_sa_o,
    output logic [1:0] current_slot_o,
    output logic [2:0] current_config_id_o,
    output logic selected_commit_o,
    output logic busy_o,
    output logic done_o,
    output logic group_repairable_o,
    output logic [1:0] failure_position_o
);
    logic [1:0] sa_q;
    logic [1:0] priority_rank_q;
    logic [1:0] selected_slot;
    logic slot_releases;
    logic slot_borrows;
    logic prefix_compatible;
    logic commit_now;

    // These are prefix facts, not physical-resource availability tokens.
    // They are the exact past-SA facts referenced by the G2X2_R implications:
    // C.borrow -> A.release, B.borrow -> D.release,
    // A.borrow -> B.release, D.borrow -> C.release.
    logic a_released_q;
    logic a_borrows_q;
    logic b_borrows_q;
    logic c_released_q;

    assign current_sa_o = sa_q;
    assign current_slot_o = selected_slot;
    assign selected_commit_o = commit_now;

    always_comb begin
        case (priority_rank_q)
            2'd0: selected_slot = 2'd1;
            2'd1: selected_slot = 2'd0;
            2'd2: selected_slot = 2'd3;
            default: selected_slot = 2'd2;
        endcase

        current_config_id_o = 3'd0;
        case (selected_slot)
            2'd1: current_config_id_o = 3'd4;
            2'd2: current_config_id_o = 3'd2;
            default: current_config_id_o = 3'd0;
        endcase
    end

    assign slot_releases = (selected_slot == 2'd1) ||
                           (selected_slot == 2'd3);
    assign slot_borrows = (selected_slot == 2'd2) ||
                          (selected_slot == 2'd3);

    always_comb begin
        prefix_compatible = 1'b0;
        case (sa_q)
            2'd0: prefix_compatible = 1'b1;
            2'd1: prefix_compatible = !a_borrows_q || slot_releases;
            2'd2: prefix_compatible = !slot_borrows || a_released_q;
            default: prefix_compatible =
                (!slot_borrows || c_released_q) &&
                (!b_borrows_q || slot_releases);
        endcase
    end

    assign commit_now = busy_o && candidate_valid_i && prefix_compatible;

    always_ff @(posedge clk_i) begin
        done_o <= 1'b0;
        if (!rst_ni) begin
            sa_q <= 2'd0;
            priority_rank_q <= 2'd0;
            a_released_q <= 1'b0;
            a_borrows_q <= 1'b0;
            b_borrows_q <= 1'b0;
            c_released_q <= 1'b0;
            busy_o <= 1'b0;
            group_repairable_o <= 1'b0;
            failure_position_o <= 2'd0;
        end else if (start_i && !busy_o) begin
            sa_q <= 2'd0;
            priority_rank_q <= 2'd0;
            a_released_q <= 1'b0;
            a_borrows_q <= 1'b0;
            b_borrows_q <= 1'b0;
            c_released_q <= 1'b0;
            busy_o <= 1'b1;
            group_repairable_o <= 1'b0;
            failure_position_o <= 2'd0;
        end else if (busy_o) begin
            if (commit_now) begin
                case (sa_q)
                    2'd0: begin
                        a_released_q <= slot_releases;
                        a_borrows_q <= slot_borrows;
                    end
                    2'd1: begin
                        b_borrows_q <= slot_borrows;
                    end
                    2'd2: begin
                        c_released_q <= slot_releases;
                    end
                    default: begin
                    end
                endcase
                if (sa_q == 2'd3) begin
                    busy_o <= 1'b0;
                    done_o <= 1'b1;
                    group_repairable_o <= 1'b1;
                end else begin
                    sa_q <= sa_q + 2'd1;
                    priority_rank_q <= 2'd0;
                end
            end else if (priority_rank_q == 2'd3) begin
                busy_o <= 1'b0;
                done_o <= 1'b1;
                group_repairable_o <= 1'b0;
                failure_position_o <= sa_q;
            end else begin
                priority_rank_q <= priority_rank_q + 2'd1;
            end
        end
    end
endmodule

`default_nettype wire
