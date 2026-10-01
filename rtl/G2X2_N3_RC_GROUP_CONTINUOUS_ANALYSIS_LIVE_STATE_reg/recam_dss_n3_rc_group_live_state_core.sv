`default_nettype none

// CA-LIVE GROUP controller.  It retains only the canonical candidate map.
module recam_dss_n3_rc_group_live_state_core (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic state_update_i,
    input  logic [1:0] state_sa_i,
    input  logic test_done_valid_i,
    input  logic [1:0] test_done_sa_i,
    input  logic candidate_valid_i,
    input  logic [5:0] candidate_pattern_id_i,
    output logic scan_active_o,
    output logic [1:0] active_sa_o,
    output logic [1:0] scan_slot_o,
    output logic [2:0] scan_config_id_o,
    output logic [3:0] sa_result_frozen_o,
    output logic solution_ready_o,
    output logic [111:0] candidate_store_image_o,
    output logic group_repairable_o,
    output logic [3:0] sa_commit_valid_o,
    output logic [11:0] ledger_released_borrower_o,
    output logic [11:0] selected_config_flat_o,
    output logic [23:0] selected_pattern_flat_o,
    output logic [7:0] selected_donor_flat_o,
    output logic [3:0] borrow_flat_o,
    output logic [3:0] release_flat_o
);
    typedef enum logic [2:0] {
        ST_WAIT_UPDATE,
        ST_SCAN,
        ST_WAIT_DONE,
        ST_DECIDE,
        ST_READY
    } state_t;

    
    state_t state_q;
    logic [1:0] active_sa_q;
    logic [1:0] scan_slot_q;
    logic test_done_seen_q;
    logic [3:0] frozen_q;
    logic active_state_update;
    logic active_test_done;
    logic scan_final_slot;
    logic [111:0] store_image;
    logic selector_valid;
    logic [1:0] selected_a_slot;
    logic [1:0] selected_b_slot;
    logic [1:0] selected_c_slot;
    logic [1:0] selected_d_slot;
    logic [2:0] selected_a_config;
    logic [2:0] selected_b_config;
    logic [2:0] selected_c_config;
    logic [2:0] selected_d_config;
    logic [5:0] selected_a_pattern;
    logic [5:0] selected_b_pattern;
    logic [5:0] selected_c_pattern;
    logic [5:0] selected_d_pattern;
    logic [7:0] selected_donor_comb;
    logic [3:0] selected_borrow_comb;
    logic [3:0] selected_release_comb;
    logic [11:0] legacy_ledger_comb;
    logic unused_store_read_valid;
    logic [5:0] unused_store_read_pattern;
    

    always_comb begin
        active_state_update = state_update_i && (state_sa_i == active_sa_q);
        active_test_done = test_done_valid_i && (test_done_sa_i == active_sa_q);
        scan_final_slot = (scan_slot_q == 2'd3);
        scan_active_o = (state_q == ST_SCAN);
        active_sa_o = active_sa_q;
        scan_slot_o = scan_slot_q;
        sa_result_frozen_o = frozen_q;
        candidate_store_image_o = store_image;
    end

    dss_n3_rc_group_candidate_store candidate_store (
        .clk_i(clk_i), .rst_ni(rst_ni),
        .write_enable_i((state_q == ST_SCAN) && !active_state_update),
        .write_sa_i(active_sa_q), .write_slot_i(scan_slot_q),
        .write_valid_i(candidate_valid_i),
        .write_pattern_id_i(candidate_pattern_id_i),
        .read_sa_i(2'd0), .read_slot_i(2'd0),
        .read_valid_o(unused_store_read_valid),
        .read_pattern_id_o(unused_store_read_pattern),
        .image_o(store_image)
    );

    dss_n3_rc_group_slot_decode slot_decode (
        .sa_i(active_sa_q), .slot_i(scan_slot_q), .valid_o(),
        .config_id_o(scan_config_id_o), .row_count_o(), .col_count_o(),
        .canonical_class_o(), .transpose_o(), .action_o(),
        .release_required_o(), .borrow_required_o()
    );

    recam_n3_rc_group_selector static_selector (
        .candidate_store_image_i(store_image), .selected_valid_o(selector_valid),
        .selected_a_slot_o(selected_a_slot), .selected_b_slot_o(selected_b_slot),
        .selected_c_slot_o(selected_c_slot), .selected_d_slot_o(selected_d_slot),
        .selected_a_config_id_o(selected_a_config), .selected_b_config_id_o(selected_b_config),
        .selected_c_config_id_o(selected_c_config), .selected_d_config_id_o(selected_d_config),
        .selected_a_pattern_id_o(selected_a_pattern), .selected_b_pattern_id_o(selected_b_pattern),
        .selected_c_pattern_id_o(selected_c_pattern), .selected_d_pattern_id_o(selected_d_pattern)
    );

    always_comb begin
        selected_release_comb = {
            selected_c_slot[0], selected_b_slot[0], selected_d_slot[0], selected_a_slot[0]};
        selected_borrow_comb = {
            selected_d_slot[1], selected_c_slot[1], selected_b_slot[1], selected_a_slot[1]};
        selected_donor_comb = {2'd3, 2'd0, 2'd1, 2'd2};
        legacy_ledger_comb = '0;
        legacy_ledger_comb[3:0] = selected_release_comb;
        legacy_ledger_comb[5:4] = selected_c_slot[1] ? 2'b10 : 2'b00;
        legacy_ledger_comb[7:6] = selected_b_slot[1] ? 2'b01 : 2'b00;
        legacy_ledger_comb[9:8] = selected_a_slot[1] ? 2'b01 : 2'b00;
        legacy_ledger_comb[11:10] = selected_d_slot[1] ? 2'b10 : 2'b00;
    end

    always_ff @(posedge clk_i) begin
        if (!rst_ni) begin
            state_q <= ST_WAIT_UPDATE;
            active_sa_q <= 2'd0;
            scan_slot_q <= 2'd0;
            test_done_seen_q <= 1'b0;
            frozen_q <= 4'd0;
            solution_ready_o <= 1'b0;
            group_repairable_o <= 1'b0;
            sa_commit_valid_o <= 4'd0;
            ledger_released_borrower_o <= 12'd0;
            selected_config_flat_o <= 12'd0;
            selected_pattern_flat_o <= 24'd0;
            selected_donor_flat_o <= 8'd0;
            borrow_flat_o <= 4'd0;
            release_flat_o <= 4'd0;
        end else if (state_q != ST_READY) begin
            if (active_state_update) begin
                // Update wins over the old scan write, final-slot freeze, and selector.
                state_q <= ST_SCAN;
                scan_slot_q <= 2'd0;
                frozen_q[active_sa_q] <= 1'b0;
                solution_ready_o <= 1'b0;
                sa_commit_valid_o <= 4'd0;
                if (active_test_done)
                    test_done_seen_q <= 1'b1;
            end else begin
                if (active_test_done)
                    test_done_seen_q <= 1'b1;
                case (state_q)
                    ST_SCAN: begin
                        if (scan_final_slot) begin
                            scan_slot_q <= 2'd0;
                            if (test_done_seen_q || active_test_done) begin
                                frozen_q[active_sa_q] <= 1'b1;
                                test_done_seen_q <= 1'b0;
                                if (active_sa_q == 2'd3) begin
                                    state_q <= ST_DECIDE;
                                end else begin
                                    active_sa_q <= active_sa_q + 2'd1;
                                    state_q <= ST_WAIT_UPDATE;
                                end
                            end else begin
                                state_q <= ST_WAIT_DONE;
                            end
                        end else begin
                            scan_slot_q <= scan_slot_q + 2'd1;
                        end
                    end
                    ST_WAIT_DONE: begin
                        if (active_test_done) begin
                            frozen_q[active_sa_q] <= 1'b1;
                            test_done_seen_q <= 1'b0;
                            if (active_sa_q == 2'd3) begin
                                state_q <= ST_DECIDE;
                            end else begin
                                active_sa_q <= active_sa_q + 2'd1;
                                state_q <= ST_WAIT_UPDATE;
                            end
                        end
                    end
                    ST_DECIDE: begin
                        state_q <= ST_READY;
                        solution_ready_o <= 1'b1;
                        group_repairable_o <= selector_valid;
                        if (selector_valid) begin
                            sa_commit_valid_o <= 4'hf;
                            ledger_released_borrower_o <= legacy_ledger_comb;
                            selected_config_flat_o <= {selected_d_config, selected_c_config,
                                                       selected_b_config, selected_a_config};
                            selected_pattern_flat_o <= {selected_d_pattern, selected_c_pattern,
                                                        selected_b_pattern, selected_a_pattern};
                            selected_donor_flat_o <= selected_donor_comb;
                            borrow_flat_o <= selected_borrow_comb;
                            release_flat_o <= selected_release_comb;
                        end else begin
                            sa_commit_valid_o <= 4'd0;
                            ledger_released_borrower_o <= 12'd0;
                            selected_config_flat_o <= 12'd0;
                            selected_pattern_flat_o <= 24'd0;
                            selected_donor_flat_o <= 8'd0;
                            borrow_flat_o <= 4'd0;
                            release_flat_o <= 4'd0;
                        end
                    end
                    default: begin end
                endcase
            end
        end
    end
endmodule

`default_nettype wire
