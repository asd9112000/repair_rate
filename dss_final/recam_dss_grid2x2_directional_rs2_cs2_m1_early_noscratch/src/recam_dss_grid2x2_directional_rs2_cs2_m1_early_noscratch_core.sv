`default_nettype none

// Final Model-B2 directional EARLY controller.  It evaluates one shared
// analyzer result each cycle, commits the first legal R/L/RB/B slot for the
// current SA, and never rolls a committed prefix back.
module recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core (
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
    output logic [3:0] borrow_flat_o,
    output logic [3:0] common_spare_available_o,
    output logic [1:0] failure_position_o
);
    logic [1:0] sa_q;
    logic [1:0] priority_rank_q;
    logic [1:0] selected_slot;
    logic [3:0] claim_mask;
    logic slot_borrows;
    logic resource_legal;
    logic borrow_legal;
    logic commit_now;

    assign current_sa_o = sa_q;
    assign current_slot_o = selected_slot;

    // Rank is the frozen P0 priority R,L,RB,B, while slot numbering remains
    // the archive's L,R,B,RB encoding.
    always_comb begin
        case (priority_rank_q)
            2'd0: selected_slot = 2'd1;
            2'd1: selected_slot = 2'd0;
            2'd2: selected_slot = 2'd3;
            default: selected_slot = 2'd2;
        endcase

        current_config_id_o = 3'd0;
        claim_mask = 4'b0000;
        case (sa_q)
            2'd0: begin
                case (selected_slot)
                    2'd0: begin current_config_id_o = 3'd0; claim_mask = 4'b0001; end
                    2'd1: begin current_config_id_o = 3'd4; claim_mask = 4'b0000; end
                    2'd2: begin current_config_id_o = 3'd5; claim_mask = 4'b0101; end
                    default: begin current_config_id_o = 3'd6; claim_mask = 4'b0100; end
                endcase
            end
            2'd1: begin
                case (selected_slot)
                    2'd0: begin current_config_id_o = 3'd0; claim_mask = 4'b0100; end
                    2'd1: begin current_config_id_o = 3'd1; claim_mask = 4'b0000; end
                    2'd2: begin current_config_id_o = 3'd2; claim_mask = 4'b0110; end
                    default: begin current_config_id_o = 3'd3; claim_mask = 4'b0010; end
                endcase
            end
            2'd2: begin
                case (selected_slot)
                    2'd0: begin current_config_id_o = 3'd0; claim_mask = 4'b1000; end
                    2'd1: begin current_config_id_o = 3'd1; claim_mask = 4'b0000; end
                    2'd2: begin current_config_id_o = 3'd2; claim_mask = 4'b1001; end
                    default: begin current_config_id_o = 3'd3; claim_mask = 4'b0001; end
                endcase
            end
            default: begin
                case (selected_slot)
                    2'd0: begin current_config_id_o = 3'd0; claim_mask = 4'b0010; end
                    2'd1: begin current_config_id_o = 3'd4; claim_mask = 4'b0000; end
                    2'd2: begin current_config_id_o = 3'd5; claim_mask = 4'b1010; end
                    default: begin current_config_id_o = 3'd6; claim_mask = 4'b1000; end
                endcase
            end
        endcase
    end

    assign slot_borrows = selected_slot[1];
    assign resource_legal =
        (common_spare_available_o & claim_mask) == claim_mask;
    // borrow_flat_o is committed repair-result metadata.  It is not another
    // resource ledger or sharing-policy register; its already-required result
    // lifetime lets m=1 be checked without adding persistent policy bits.
    assign borrow_legal = !slot_borrows || !(|borrow_flat_o);
    assign commit_now = busy_o && candidate_valid_i && resource_legal &&
                        borrow_legal;

    always_ff @(posedge clk_i) begin
        done_o <= 1'b0;
        if (!rst_ni) begin
            sa_q <= 2'd0;
            priority_rank_q <= 2'd0;
            busy_o <= 1'b0;
            group_repairable_o <= 1'b0;
            sa_commit_valid_o <= 4'b0000;
            selected_config_flat_o <= 12'b0;
            selected_pattern_flat_o <= 16'b0;
            borrow_flat_o <= 4'b0000;
            common_spare_available_o <= 4'b1111;
            failure_position_o <= 2'd0;
        end else if (start_i && !busy_o) begin
            sa_q <= 2'd0;
            priority_rank_q <= 2'd0;
            busy_o <= 1'b1;
            group_repairable_o <= 1'b0;
            sa_commit_valid_o <= 4'b0000;
            selected_config_flat_o <= 12'b0;
            selected_pattern_flat_o <= 16'b0;
            borrow_flat_o <= 4'b0000;
            common_spare_available_o <= 4'b1111;
            failure_position_o <= 2'd0;
        end else if (busy_o) begin
            if (commit_now) begin
                case (sa_q)
                    2'd0: begin
                        sa_commit_valid_o[0] <= 1'b1;
                        selected_config_flat_o[2:0] <= current_config_id_o;
                        selected_pattern_flat_o[3:0] <= candidate_pattern_id_i;
                        borrow_flat_o[0] <= slot_borrows;
                    end
                    2'd1: begin
                        sa_commit_valid_o[1] <= 1'b1;
                        selected_config_flat_o[5:3] <= current_config_id_o;
                        selected_pattern_flat_o[7:4] <= candidate_pattern_id_i;
                        borrow_flat_o[1] <= slot_borrows;
                    end
                    2'd2: begin
                        sa_commit_valid_o[2] <= 1'b1;
                        selected_config_flat_o[8:6] <= current_config_id_o;
                        selected_pattern_flat_o[11:8] <= candidate_pattern_id_i;
                        borrow_flat_o[2] <= slot_borrows;
                    end
                    default: begin
                        sa_commit_valid_o[3] <= 1'b1;
                        selected_config_flat_o[11:9] <= current_config_id_o;
                        selected_pattern_flat_o[15:12] <= candidate_pattern_id_i;
                        borrow_flat_o[3] <= slot_borrows;
                    end
                endcase
                common_spare_available_o <=
                    common_spare_available_o & ~claim_mask;
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
