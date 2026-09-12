`default_nettype none
module dss_v2_candidate_resource_path_test_top (
    input logic clk_i, rst_ni, commit_i, sa_valid_i,
    input dss_v2_types_pkg::dss_v2_sa_id_t sa_id_i,
    input logic [1:0] selected_candidate_index_i,
    input logic [79:0] legacy_config_pattern_map_i,
    output logic selected_candidate_valid_o, physical_feasible_o,
    output logic resource_state_valid_o, descriptor_legal_o,
    output logic [3:0] resource_released_o, resource_borrowed_o, borrower_valid_o,
    output logic [7:0] borrower_id_flat_o,
    output logic commit_accepted_o, commit_error_o,
    output dss_v2_types_pkg::dss_v2_candidate_reconstruction_metadata_t reconstruction_metadata_o
);
    import dss_v2_types_pkg::*;
    localparam int SUMMARY_W = $bits(dss_v2_candidate_summary_t);
    dss_v2_candidate_reconstruction_metadata_t metadata_zero;
    logic [4*SUMMARY_W-1:0] summaries;
    logic [SUMMARY_W-1:0] selected_summary_bits;
    dss_v2_config_descriptor_t selected_descriptor;
    logic borrow, donor_valid, release_action, release_valid;
    dss_v2_resource_id_t donor, release_resource;
    always_comb begin
        metadata_zero = '0;
        selected_summary_bits = summaries[selected_candidate_index_i*SUMMARY_W +: SUMMARY_W];
        selected_descriptor = selected_summary_bits[19 +: $bits(dss_v2_config_descriptor_t)];
    end
    dss_legacy_recam_candidate_adapter adapter (.sa_id_i(sa_id_i), .sa_valid_i(sa_valid_i),
        .legacy_config_pattern_map_i(legacy_config_pattern_map_i), .legacy_reconstruction_metadata_i(metadata_zero),
        .candidate_summary_flat_o(summaries), .reconstruction_metadata_o(reconstruction_metadata_o));
    assign selected_candidate_valid_o = selected_summary_bits[SUMMARY_W-1];
    dss_v2_resource_feasibility feasibility (.sa_id_i(sa_id_i), .sa_valid_i(sa_valid_i),
        .config_descriptor_i(selected_descriptor), .resource_released_i(resource_released_o),
        .resource_borrowed_i(resource_borrowed_o), .resource_state_valid_o(resource_state_valid_o), .descriptor_legal_o(descriptor_legal_o),
        .physical_feasible_o(physical_feasible_o), .borrow_required_o(borrow), .selected_donor_valid_o(donor_valid),
        .selected_donor_resource_o(donor), .release_required_o(release_action), .release_resource_valid_o(release_valid), .release_resource_o(release_resource));
    dss_v2_resource_ledger ledger (.clk_i(clk_i), .rst_ni(rst_ni), .commit_i(commit_i),
        .transaction_valid_i(selected_candidate_valid_o && physical_feasible_o), .requester_sa_i(sa_id_i),
        .release_required_i(release_action), .release_resource_valid_i(release_valid), .release_resource_id_i(release_resource),
        .borrow_required_i(borrow), .selected_donor_valid_i(donor_valid), .selected_donor_resource_id_i(donor),
        .resource_released_o(resource_released_o), .resource_borrowed_o(resource_borrowed_o), .borrower_valid_o(borrower_valid_o), .borrower_id_flat_o(borrower_id_flat_o),
        .commit_accepted_o(commit_accepted_o), .commit_error_o(commit_error_o));
endmodule
`default_nettype wire
