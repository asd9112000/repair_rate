`default_nettype none

// HYP02 GLOBAL core: collect one fixed record for every {SA, slot}, then
// atomically publish the lowest-numbered valid fixed-four-edge path.
module recam_dss_hyp02_static_global_core (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic start_i,
    input  logic candidate_valid_i,
    input  logic [3:0] candidate_pattern_id_i,
    output logic collection_active_o,
    output logic allocation_active_o,
    output logic [1:0] current_sa_o,
    output logic [1:0] current_slot_o,
    output logic [2:0] current_config_id_o,
    output logic [79:0] candidate_store_image_o,
    output logic busy_o,
    output logic done_o,
    output logic group_repairable_o,
    output logic [3:0] sa_commit_valid_o,
    output logic [11:0] ledger_released_borrower_o,
    output logic [11:0] selected_config_flat_o,
    output logic [15:0] selected_pattern_flat_o,
    output logic [7:0] selected_donor_flat_o,
    output logic [3:0] borrow_flat_o,
    output logic [3:0] release_flat_o,
    output logic [1:0] failure_position_o
);
    import dss_v2_types_pkg::*;

    typedef enum logic [1:0] {IDLE, COLLECT, DECIDE} state_t;

    state_t state_q;
    logic [1:0] collect_sa_q;
    logic [1:0] collect_slot_q;
    logic [79:0] store_image;
    logic selector_valid;
    logic [1:0] selected_a_slot;
    logic [1:0] selected_b_slot;
    logic [1:0] selected_c_slot;
    logic [1:0] selected_d_slot;
    logic [2:0] selected_a_config;
    logic [2:0] selected_b_config;
    logic [2:0] selected_c_config;
    logic [2:0] selected_d_config;
    logic [3:0] selected_a_pattern;
    logic [3:0] selected_b_pattern;
    logic [3:0] selected_c_pattern;
    logic [3:0] selected_d_pattern;
    logic [7:0] selected_donor_comb;
    logic [3:0] selected_borrow_comb;
    logic [3:0] selected_release_comb;
    logic [11:0] legacy_ledger_comb;
    logic unused_store_read_valid;
    dss_v2_pattern_id_t unused_store_read_pattern;
    dss_v2_config_descriptor_t unused_collect_config_descriptor;

    assign collection_active_o = state_q == COLLECT;
    assign allocation_active_o = state_q == DECIDE;
    assign current_sa_o = collect_sa_q;
    assign current_slot_o = collect_slot_q;
    assign candidate_store_image_o = store_image;

    dss_v2_group_candidate_store candidate_store (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .write_enable_i(state_q == COLLECT),
        .write_sa_i(collect_sa_q),
        .write_slot_i(collect_slot_q),
        .write_candidate_valid_i(candidate_valid_i),
        .write_pattern_id_i(candidate_pattern_id_i),
        .read_sa_i(2'd0),
        .read_slot_i(2'd0),
        .read_candidate_valid_o(unused_store_read_valid),
        .read_pattern_id_o(unused_store_read_pattern),
        .candidate_store_image_o(store_image)
    );

    dss_v2_group_slot_decode collect_slot_decode (
        .sa_id_i(collect_sa_q),
        .canonical_slot_i(collect_slot_q),
        .legacy_config_id_o(current_config_id_o),
        .config_descriptor_o(unused_collect_config_descriptor)
    );

    recam_dss_hyp02_static_selector static_selector (
        .candidate_store_image_i(store_image),
        .selected_valid_o(selector_valid),
        .selected_a_slot_o(selected_a_slot),
        .selected_b_slot_o(selected_b_slot),
        .selected_c_slot_o(selected_c_slot),
        .selected_d_slot_o(selected_d_slot),
        .selected_a_config_id_o(selected_a_config),
        .selected_b_config_id_o(selected_b_config),
        .selected_c_config_id_o(selected_c_config),
        .selected_d_config_id_o(selected_d_config),
        .selected_a_pattern_id_o(selected_a_pattern),
        .selected_b_pattern_id_o(selected_b_pattern),
        .selected_c_pattern_id_o(selected_c_pattern),
        .selected_d_pattern_id_o(selected_d_pattern)
    );

    always_comb begin
        selected_release_comb = {
            selected_c_slot[0], selected_b_slot[0],
            selected_d_slot[0], selected_a_slot[0]};
        selected_borrow_comb = {
            selected_d_slot[1], selected_c_slot[1],
            selected_b_slot[1], selected_a_slot[1]};
        // A<-B_COL, B<-D_ROW, C<-A_ROW, D<-C_COL.  These resource IDs are
        // fixed diagnostic metadata, not the output of a donor search.
        selected_donor_comb = {2'd3, 2'd0, 2'd1, 2'd2};
        legacy_ledger_comb = '0;
        legacy_ledger_comb[3:0] = selected_release_comb;
        legacy_ledger_comb[5:4] = selected_c_slot[1] ? 2'b10 : 2'b00;
        legacy_ledger_comb[7:6] = selected_b_slot[1] ? 2'b01 : 2'b00;
        legacy_ledger_comb[9:8] = selected_a_slot[1] ? 2'b01 : 2'b00;
        legacy_ledger_comb[11:10] = selected_d_slot[1] ? 2'b10 : 2'b00;
    end

    always_ff @(posedge clk_i) begin
        done_o <= 1'b0;
        if (!rst_ni) begin
            state_q <= IDLE;
            collect_sa_q <= 2'd0;
            collect_slot_q <= 2'd0;
            busy_o <= 1'b0;
            group_repairable_o <= 1'b0;
            sa_commit_valid_o <= 4'd0;
            ledger_released_borrower_o <= 12'd0;
            selected_config_flat_o <= 12'd0;
            selected_pattern_flat_o <= 16'd0;
            selected_donor_flat_o <= 8'd0;
            borrow_flat_o <= 4'd0;
            release_flat_o <= 4'd0;
            failure_position_o <= 2'd0;
        end else begin
            case (state_q)
                IDLE: begin
                    if (start_i) begin
                        state_q <= COLLECT;
                        collect_sa_q <= 2'd0;
                        collect_slot_q <= 2'd0;
                        busy_o <= 1'b1;
                        group_repairable_o <= 1'b0;
                        sa_commit_valid_o <= 4'd0;
                        ledger_released_borrower_o <= 12'd0;
                        selected_config_flat_o <= 12'd0;
                        selected_pattern_flat_o <= 16'd0;
                        selected_donor_flat_o <= 8'd0;
                        borrow_flat_o <= 4'd0;
                        release_flat_o <= 4'd0;
                        failure_position_o <= 2'd0;
                    end
                end
                COLLECT: begin
                    if (collect_slot_q == 2'd3) begin
                        collect_slot_q <= 2'd0;
                        if (collect_sa_q == 2'd3) begin
                            collect_sa_q <= 2'd0;
                            state_q <= DECIDE;
                        end else begin
                            collect_sa_q <= collect_sa_q + 2'd1;
                        end
                    end else begin
                        collect_slot_q <= collect_slot_q + 2'd1;
                    end
                end
                DECIDE: begin
                    state_q <= IDLE;
                    busy_o <= 1'b0;
                    done_o <= 1'b1;
                    group_repairable_o <= selector_valid;
                    if (selector_valid) begin
                        sa_commit_valid_o <= 4'hf;
                        ledger_released_borrower_o <= legacy_ledger_comb;
                        selected_config_flat_o <= {
                            selected_d_config, selected_c_config,
                            selected_b_config, selected_a_config};
                        selected_pattern_flat_o <= {
                            selected_d_pattern, selected_c_pattern,
                            selected_b_pattern, selected_a_pattern};
                        selected_donor_flat_o <= selected_donor_comb;
                        borrow_flat_o <= selected_borrow_comb;
                        release_flat_o <= selected_release_comb;
                    end else begin
                        sa_commit_valid_o <= 4'd0;
                        ledger_released_borrower_o <= 12'd0;
                        selected_config_flat_o <= 12'd0;
                        selected_pattern_flat_o <= 16'd0;
                        selected_donor_flat_o <= 8'd0;
                        borrow_flat_o <= 4'd0;
                        release_flat_o <= 4'd0;
                        failure_position_o <= 2'd0;
                    end
                end
                default: begin
                    state_q <= IDLE;
                    busy_o <= 1'b0;
                end
            endcase
        end
    end
endmodule
`default_nettype wire
