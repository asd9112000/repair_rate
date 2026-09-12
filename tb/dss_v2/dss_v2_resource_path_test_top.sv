`default_nettype none

// Test-only Phase 4B -> 4C composition; no traversal or policy is present.
module dss_v2_resource_path_test_top (
    input logic clk_i,
    input logic rst_ni,
    input logic commit_i,
    input dss_v2_types_pkg::dss_v2_sa_id_t sa_id_i,
    input logic sa_valid_i,
    input dss_v2_types_pkg::dss_v2_config_descriptor_t config_descriptor_i,
    output logic resource_state_valid_o,
    output logic descriptor_legal_o,
    output logic physical_feasible_o,
    output logic borrow_required_o,
    output logic selected_donor_valid_o,
    output dss_v2_types_pkg::dss_v2_resource_id_t selected_donor_resource_o,
    output logic release_required_o,
    output logic release_resource_valid_o,
    output dss_v2_types_pkg::dss_v2_resource_id_t release_resource_o,
    output logic [dss_v2_params_pkg::RESOURCE_NUM-1:0] resource_released_o,
    output logic [dss_v2_params_pkg::RESOURCE_NUM-1:0] resource_borrowed_o,
    output logic [dss_v2_params_pkg::RESOURCE_NUM-1:0] borrower_valid_o,
    output logic [dss_v2_params_pkg::RESOURCE_NUM*dss_v2_params_pkg::SA_ID_W-1:0] borrower_id_flat_o,
    output logic commit_accepted_o,
    output logic commit_error_o
);
    import dss_v2_params_pkg::*;
    import dss_v2_types_pkg::*;

    dss_v2_resource_feasibility feasibility (
        .sa_id_i(sa_id_i), .sa_valid_i(sa_valid_i), .config_descriptor_i(config_descriptor_i),
        .resource_released_i(resource_released_o), .resource_borrowed_i(resource_borrowed_o),
        .resource_state_valid_o(resource_state_valid_o), .descriptor_legal_o(descriptor_legal_o),
        .physical_feasible_o(physical_feasible_o), .borrow_required_o(borrow_required_o),
        .selected_donor_valid_o(selected_donor_valid_o),
        .selected_donor_resource_o(selected_donor_resource_o),
        .release_required_o(release_required_o), .release_resource_valid_o(release_resource_valid_o),
        .release_resource_o(release_resource_o)
    );
    dss_v2_resource_ledger ledger (
        .clk_i(clk_i), .rst_ni(rst_ni), .commit_i(commit_i),
        .transaction_valid_i(physical_feasible_o), .requester_sa_i(sa_id_i),
        .release_required_i(release_required_o), .release_resource_valid_i(release_resource_valid_o),
        .release_resource_id_i(release_resource_o), .borrow_required_i(borrow_required_o),
        .selected_donor_valid_i(selected_donor_valid_o),
        .selected_donor_resource_id_i(selected_donor_resource_o),
        .resource_released_o(resource_released_o), .resource_borrowed_o(resource_borrowed_o),
        .borrower_valid_o(borrower_valid_o), .borrower_id_flat_o(borrower_id_flat_o),
        .commit_accepted_o(commit_accepted_o), .commit_error_o(commit_error_o)
    );
endmodule
`default_nettype wire
