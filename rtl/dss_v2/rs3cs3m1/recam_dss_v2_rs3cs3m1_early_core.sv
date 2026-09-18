`default_nettype none

module recam_dss_v2_rs3cs3m1_early_core (
    input logic clk_i, rst_ni, start_i,
    input logic candidate_solution_valid_i, candidate_repairable_i,
    input logic [5:0] candidate_pattern_id_i,
    output logic [1:0] current_sa_o, current_slot_o,
    output logic [2:0] current_config_id_o,
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
    logic [1:0] sa_q, rank_q, slot;
    logic table_valid, release_required, borrow_required;
    logic [2:0] table_config, row_count_unused, col_count_unused;
    logic [1:0] class_unused, action_unused;
    logic transpose_unused;
    logic [3:0] released, borrowed, borrower_valid;
    logic [7:0] borrower_ids;
    logic feasible, donor_valid, release_valid;
    logic [1:0] donor, release_resource;
    logic accept;

    always_comb begin
        if (sa_q == 2'd1 || sa_q == 2'd2) begin
            unique case (rank_q)
                2'd0: slot = 2'd0;
                2'd1: slot = 2'd2;
                2'd2: slot = 2'd1;
                default: slot = 2'd3;
            endcase
        end else slot = rank_q;
    end
    assign current_sa_o = sa_q;
    assign current_slot_o = slot;
    dss_v2_rs3cs3m1_config_table cfg_table (
        .sa_i(sa_q), .slot_i(slot), .valid_o(table_valid), .config_id_o(table_config),
        .row_count_o(row_count_unused), .col_count_o(col_count_unused),
        .canonical_class_o(class_unused), .transpose_o(transpose_unused), .action_o(action_unused),
        .release_required_o(release_required), .borrow_required_o(borrow_required));
    assign current_config_id_o = table_config;
    dss_v2_rs3cs3m1_topology topology (
        .sa_i(sa_q), .descriptor_valid_i(table_valid), .release_required_i(release_required),
        .borrow_required_i(borrow_required), .resource_released_i(released),
        .resource_borrowed_i(borrowed), .physical_feasible_o(feasible),
        .donor_valid_o(donor_valid), .donor_o(donor), .release_valid_o(release_valid),
        .release_resource_o(release_resource));
    assign accept = candidate_solution_valid_i && candidate_repairable_i && feasible;

    dss_v2_resource_ledger ledger (
        .clk_i(clk_i), .rst_ni(rst_ni), .commit_i(busy_o && accept),
        .transaction_valid_i(accept), .requester_sa_i(sa_q),
        .release_required_i(release_required), .release_resource_valid_i(release_valid),
        .release_resource_id_i(release_resource), .borrow_required_i(borrow_required),
        .selected_donor_valid_i(donor_valid), .selected_donor_resource_id_i(donor),
        .resource_released_o(released), .resource_borrowed_o(borrowed),
        .borrower_valid_o(borrower_valid), .borrower_id_flat_o(borrower_ids),
        .commit_accepted_o(), .commit_error_o());
    dss_v2_legacy_ledger_diagnostic_adapter diagnostic (
        .resource_released_i(released), .borrower_valid_i(borrower_valid),
        .borrower_id_flat_i(borrower_ids), .legacy_ledger_o(ledger_released_borrower_o),
        .canonical_state_valid_o());

    always_ff @(posedge clk_i) begin
        done_o <= 1'b0;
        if (!rst_ni) begin
            busy_o <= 1'b0; group_repairable_o <= 1'b0; sa_q <= '0; rank_q <= '0;
            sa_commit_valid_o <= '0; selected_config_flat_o <= '0; selected_pattern_flat_o <= '0;
            selected_donor_flat_o <= '0; borrow_flat_o <= '0; release_flat_o <= '0; failure_position_o <= 2'd3;
        end else if (start_i && !busy_o) begin
            busy_o <= 1'b1; group_repairable_o <= 1'b0; sa_q <= '0; rank_q <= '0;
            sa_commit_valid_o <= '0; selected_config_flat_o <= '0; selected_pattern_flat_o <= '0;
            selected_donor_flat_o <= '0; borrow_flat_o <= '0; release_flat_o <= '0;
        end else if (busy_o) begin
            if (accept) begin
                sa_commit_valid_o[sa_q] <= 1'b1;
                selected_config_flat_o[sa_q*3 +: 3] <= table_config;
                selected_pattern_flat_o[sa_q*6 +: 6] <= candidate_pattern_id_i;
                selected_donor_flat_o[sa_q*2 +: 2] <= donor;
                borrow_flat_o[sa_q] <= borrow_required;
                release_flat_o[sa_q] <= release_required;
                if (sa_q == 2'd3) begin busy_o <= 1'b0; done_o <= 1'b1; group_repairable_o <= 1'b1; end
                else begin sa_q <= sa_q + 1'b1; rank_q <= '0; end
            end else if (rank_q == 2'd3) begin
                busy_o <= 1'b0; done_o <= 1'b1; group_repairable_o <= 1'b0; failure_position_o <= sa_q;
            end else rank_q <= rank_q + 1'b1;
        end
    end
endmodule
`default_nettype wire
