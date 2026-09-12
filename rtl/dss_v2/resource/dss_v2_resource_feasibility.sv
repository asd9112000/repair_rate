`default_nettype none

// Read-only physical feasibility primitive.  resource_borrowed_i is a
// resource-local borrower-present view; no ownership state is changed here.
module dss_v2_resource_feasibility (
    input  dss_v2_types_pkg::dss_v2_sa_id_t sa_id_i,
    input  logic sa_valid_i,
    input  dss_v2_types_pkg::dss_v2_config_descriptor_t config_descriptor_i,
    input  logic [dss_v2_params_pkg::RESOURCE_NUM-1:0] resource_released_i,
    input  logic [dss_v2_params_pkg::RESOURCE_NUM-1:0] resource_borrowed_i,
    output logic resource_state_valid_o,
    output logic descriptor_legal_o,
    output logic physical_feasible_o,
    output logic borrow_required_o,
    output logic selected_donor_valid_o,
    output dss_v2_types_pkg::dss_v2_resource_id_t selected_donor_resource_o,
    output logic release_required_o,
    output logic release_resource_valid_o,
    output dss_v2_types_pkg::dss_v2_resource_id_t release_resource_o
);
    import dss_v2_params_pkg::*;
    import dss_v2_types_pkg::*;

    dss_v2_resource_id_t donor_primary;
    dss_v2_resource_id_t donor_secondary;
    logic [RESOURCE_NUM-1:0] resource_available;
    integer resource_index;

    dss_topology_2x2_directional topology (
        .sa_id_i(sa_id_i), .sa_valid_i(sa_valid_i),
        .config_descriptor_i(config_descriptor_i),
        .descriptor_legal_o(descriptor_legal_o),
        .borrow_required_o(borrow_required_o),
        .release_required_o(release_required_o),
        .release_resource_valid_o(release_resource_valid_o),
        .release_resource_o(release_resource_o),
        .donor_primary_o(donor_primary),
        .donor_secondary_o(donor_secondary)
    );

    always_comb begin
        resource_state_valid_o = 1'b1;
        resource_available = '0;
        for (resource_index = 0; resource_index < RESOURCE_NUM; resource_index = resource_index + 1) begin
            if (!resource_released_i[resource_index] && resource_borrowed_i[resource_index])
                resource_state_valid_o = 1'b0;
            resource_available[resource_index] = resource_released_i[resource_index] &&
                                                !resource_borrowed_i[resource_index];
        end

        selected_donor_valid_o = 1'b0;
        selected_donor_resource_o = donor_primary;
        if (borrow_required_o) begin
            if (resource_available[donor_primary]) begin
                selected_donor_valid_o = 1'b1;
                selected_donor_resource_o = donor_primary;
            end else if (resource_available[donor_secondary]) begin
                selected_donor_valid_o = 1'b1;
                selected_donor_resource_o = donor_secondary;
            end
        end
        physical_feasible_o = resource_state_valid_o && descriptor_legal_o &&
                              (!borrow_required_o || selected_donor_valid_o);
    end

    initial begin
        if (RESOURCE_NUM != 4)
            $fatal(1, "Phase 4B resource state is defined for four DATE directional resources");
    end
endmodule
`default_nettype wire
