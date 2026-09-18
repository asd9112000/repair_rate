`default_nettype none

module recam_dss_v2_rs3cs3m1_group_core (
    input logic clk_i, rst_ni, start_i, candidate_valid_i,
    input logic [5:0] candidate_pattern_id_i,
    output logic collection_active_o, allocation_active_o,
    output logic [1:0] current_sa_o, current_slot_o,
    output logic [2:0] current_config_id_o,
    output logic [111:0] candidate_store_image_o,
    output logic busy_o, done_o, group_repairable_o,
    output logic [3:0] sa_commit_valid_o,
    output logic [11:0] ledger_released_borrower_o,
    output logic [11:0] selected_config_flat_o,
    output logic [23:0] selected_pattern_flat_o,
    output logic [7:0] selected_donor_flat_o,
    output logic [3:0] borrow_flat_o, release_flat_o,
    output logic [1:0] failure_position_o
);
    import dss_v2_types_pkg::*;
    typedef enum logic [1:0] {IDLE, COLLECT, ALLOCATE} state_t;
    state_t state_q;
    logic [1:0] sa_q, index_q, priority_slot;
    logic stored_valid, table_valid, release_required, borrow_required;
    logic [5:0] stored_pattern;
    logic [2:0] table_config, unused_rows, unused_cols;
    logic [1:0] unused_class, unused_action;
    logic unused_transpose;
    logic [3:0] released, borrowed, borrower_valid;
    logic [7:0] borrower_ids;
    logic feasible, donor_valid, release_valid, accept;
    logic [1:0] donor, release_resource;

    always_comb begin
        // GROUP semantic order: RELEASE_ONLY, LOCAL, RELEASE_AND_BORROW, BORROW_ONLY.
        unique case (index_q)
            2'd0: priority_slot = 2'd1;
            2'd1: priority_slot = 2'd0;
            2'd2: priority_slot = 2'd3;
            default: priority_slot = 2'd2;
        endcase
        current_slot_o = (state_q == COLLECT) ? index_q : priority_slot;
    end
    assign current_sa_o = sa_q;
    dss_v2_rs3cs3m1_config_table cfg_table (
        .sa_i(sa_q), .slot_i(current_slot_o), .valid_o(table_valid), .config_id_o(table_config),
        .row_count_o(unused_rows), .col_count_o(unused_cols), .canonical_class_o(unused_class),
        .transpose_o(unused_transpose), .action_o(unused_action),
        .release_required_o(release_required), .borrow_required_o(borrow_required));
    assign current_config_id_o = table_config;
    dss_v2_rs3cs3m1_group_candidate_store store (
        .clk_i(clk_i), .rst_ni(rst_ni), .write_enable_i(state_q == COLLECT),
        .write_sa_i(sa_q), .write_slot_i(index_q), .write_valid_i(candidate_valid_i),
        .write_pattern_id_i(candidate_pattern_id_i), .read_sa_i(sa_q), .read_slot_i(priority_slot),
        .read_valid_o(stored_valid), .read_pattern_id_o(stored_pattern), .image_o(candidate_store_image_o));
    dss_v2_rs3cs3m1_topology topology (
        .sa_i(sa_q), .descriptor_valid_i(table_valid), .release_required_i(release_required),
        .borrow_required_i(borrow_required), .resource_released_i(released), .resource_borrowed_i(borrowed),
        .physical_feasible_o(feasible), .donor_valid_o(donor_valid), .donor_o(donor),
        .release_valid_o(release_valid), .release_resource_o(release_resource));
    assign accept = (state_q == ALLOCATE) && stored_valid && feasible;
    assign collection_active_o = (state_q == COLLECT);
    assign allocation_active_o = (state_q == ALLOCATE);

    dss_v2_resource_ledger ledger (
        .clk_i(clk_i), .rst_ni(rst_ni), .commit_i(accept), .transaction_valid_i(accept),
        .requester_sa_i(sa_q), .release_required_i(release_required), .release_resource_valid_i(release_valid),
        .release_resource_id_i(release_resource), .borrow_required_i(borrow_required),
        .selected_donor_valid_i(donor_valid), .selected_donor_resource_id_i(donor),
        .resource_released_o(released), .resource_borrowed_o(borrowed), .borrower_valid_o(borrower_valid),
        .borrower_id_flat_o(borrower_ids), .commit_accepted_o(), .commit_error_o());
    dss_v2_legacy_ledger_diagnostic_adapter diagnostic (
        .resource_released_i(released), .borrower_valid_i(borrower_valid), .borrower_id_flat_i(borrower_ids),
        .legacy_ledger_o(ledger_released_borrower_o), .canonical_state_valid_o());

    always_ff @(posedge clk_i) begin
        done_o <= 1'b0;
        if (!rst_ni) begin
            state_q <= IDLE; busy_o <= 1'b0; group_repairable_o <= 1'b0; sa_q <= '0; index_q <= '0;
            sa_commit_valid_o <= '0; selected_config_flat_o <= '0; selected_pattern_flat_o <= '0;
            selected_donor_flat_o <= '0; borrow_flat_o <= '0; release_flat_o <= '0; failure_position_o <= 2'd3;
        end else if (start_i && state_q == IDLE) begin
            state_q <= COLLECT; busy_o <= 1'b1; group_repairable_o <= 1'b0; sa_q <= '0; index_q <= '0;
            sa_commit_valid_o <= '0; selected_config_flat_o <= '0; selected_pattern_flat_o <= '0;
            selected_donor_flat_o <= '0; borrow_flat_o <= '0; release_flat_o <= '0;
        end else if (state_q == COLLECT) begin
            if (index_q == 2'd3) begin
                index_q <= '0;
                if (sa_q == 2'd3) begin state_q <= ALLOCATE; sa_q <= '0; end
                else sa_q <= sa_q + 1'b1;
            end else index_q <= index_q + 1'b1;
        end else if (state_q == ALLOCATE) begin
            if (accept) begin
                sa_commit_valid_o[sa_q] <= 1'b1;
                selected_config_flat_o[sa_q*3 +: 3] <= table_config;
                selected_pattern_flat_o[sa_q*6 +: 6] <= stored_pattern;
                selected_donor_flat_o[sa_q*2 +: 2] <= donor;
                borrow_flat_o[sa_q] <= borrow_required; release_flat_o[sa_q] <= release_required;
                if (sa_q == 2'd3) begin state_q <= IDLE; busy_o <= 1'b0; done_o <= 1'b1; group_repairable_o <= 1'b1; end
                else begin sa_q <= sa_q + 1'b1; index_q <= '0; end
            end else if (index_q == 2'd3) begin
                state_q <= IDLE; busy_o <= 1'b0; done_o <= 1'b1; group_repairable_o <= 1'b0; failure_position_o <= sa_q;
            end else index_q <= index_q + 1'b1;
        end
    end
endmodule
`default_nettype wire
