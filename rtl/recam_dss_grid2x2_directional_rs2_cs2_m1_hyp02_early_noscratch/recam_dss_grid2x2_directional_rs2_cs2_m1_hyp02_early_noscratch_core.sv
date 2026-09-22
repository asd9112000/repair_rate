`default_nettype none

// HYP02-static streaming EARLY controller.  Prefix legality is defined only
// by the frozen 81-path slot universe; it does not reconstruct physical demand.
module recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_core (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic start_i,
    input  logic candidate_valid_i,
    input  logic [3:0] candidate_pattern_id_i,
    output logic [1:0] current_sa_o,
    output logic [1:0] current_slot_o,
    output logic [2:0] current_config_id_o,
    output logic busy_o,
    output logic done_o,
    output logic group_repairable_o,
    output logic [3:0] sa_commit_valid_o,
    output logic [11:0] selected_config_flat_o,
    output logic [15:0] selected_pattern_flat_o,
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
    // They are the exact past-SA facts referenced by the HYP02 implications:
    // C.borrow -> A.release, B.borrow -> D.release,
    // A.borrow -> B.release, D.borrow -> C.release.
    logic a_released_q;
    logic a_borrows_q;
    logic b_borrows_q;
    logic c_released_q;

    assign current_sa_o = sa_q;
    assign current_slot_o = selected_slot;

    always_comb begin
        case (priority_rank_q)
            2'd0: selected_slot = 2'd1;
            2'd1: selected_slot = 2'd0;
            2'd2: selected_slot = 2'd3;
            default: selected_slot = 2'd2;
        endcase

        current_config_id_o = 3'd0;
        case (sa_q)
            2'd1, 2'd2: begin
                case (selected_slot)
                    2'd0: current_config_id_o = 3'd0;
                    2'd1: current_config_id_o = 3'd1;
                    2'd2: current_config_id_o = 3'd2;
                    default: current_config_id_o = 3'd3;
                endcase
            end
            default: begin
                case (selected_slot)
                    2'd0: current_config_id_o = 3'd0;
                    2'd1: current_config_id_o = 3'd4;
                    2'd2: current_config_id_o = 3'd5;
                    default: current_config_id_o = 3'd6;
                endcase
            end
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
            sa_commit_valid_o <= 4'b0000;
            selected_config_flat_o <= 12'b0;
            selected_pattern_flat_o <= 16'b0;
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
            sa_commit_valid_o <= 4'b0000;
            selected_config_flat_o <= 12'b0;
            selected_pattern_flat_o <= 16'b0;
            failure_position_o <= 2'd0;
        end else if (busy_o) begin
            if (commit_now) begin
                case (sa_q)
                    2'd0: begin
                        sa_commit_valid_o[0] <= 1'b1;
                        selected_config_flat_o[2:0] <= current_config_id_o;
                        selected_pattern_flat_o[3:0] <= candidate_pattern_id_i;
                        a_released_q <= slot_releases;
                        a_borrows_q <= slot_borrows;
                    end
                    2'd1: begin
                        sa_commit_valid_o[1] <= 1'b1;
                        selected_config_flat_o[5:3] <= current_config_id_o;
                        selected_pattern_flat_o[7:4] <= candidate_pattern_id_i;
                        b_borrows_q <= slot_borrows;
                    end
                    2'd2: begin
                        sa_commit_valid_o[2] <= 1'b1;
                        selected_config_flat_o[8:6] <= current_config_id_o;
                        selected_pattern_flat_o[11:8] <= candidate_pattern_id_i;
                        c_released_q <= slot_releases;
                    end
                    default: begin
                        sa_commit_valid_o[3] <= 1'b1;
                        selected_config_flat_o[11:9] <= current_config_id_o;
                        selected_pattern_flat_o[15:12] <= candidate_pattern_id_i;
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
