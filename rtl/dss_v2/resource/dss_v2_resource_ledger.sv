`default_nettype none

// Phase 4C canonical persistent ledger.  It deliberately consumes completed
// transaction intent and does not evaluate descriptor, topology, or policy.
module dss_v2_resource_ledger (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic commit_i,
    input  logic transaction_valid_i,
    input  dss_v2_types_pkg::dss_v2_sa_id_t requester_sa_i,
    input  logic release_required_i,
    input  logic release_resource_valid_i,
    input  dss_v2_types_pkg::dss_v2_resource_id_t release_resource_id_i,
    input  logic borrow_required_i,
    input  logic selected_donor_valid_i,
    input  dss_v2_types_pkg::dss_v2_resource_id_t selected_donor_resource_id_i,
    output logic [dss_v2_params_pkg::RESOURCE_NUM-1:0] resource_released_o,
    output logic [dss_v2_params_pkg::RESOURCE_NUM-1:0] resource_borrowed_o,
    output logic [dss_v2_params_pkg::RESOURCE_NUM-1:0] borrower_valid_o,
    output logic [dss_v2_params_pkg::RESOURCE_NUM*dss_v2_params_pkg::SA_ID_W-1:0] borrower_id_flat_o,
    output logic commit_accepted_o,
    output logic commit_error_o
);
    import dss_v2_params_pkg::*;
    import dss_v2_types_pkg::*;

    logic [RESOURCE_NUM-1:0] released_q;
    logic [RESOURCE_NUM-1:0] borrower_valid_q;
    logic [RESOURCE_NUM*SA_ID_W-1:0] borrower_id_q;
    logic transaction_well_formed;

    always_comb begin
        transaction_well_formed = transaction_valid_i;
        if (release_required_i) begin
            if (!release_resource_valid_i || released_q[release_resource_id_i])
                transaction_well_formed = 1'b0;
        end
        if (borrow_required_i) begin
            if (!selected_donor_valid_i || !released_q[selected_donor_resource_id_i] ||
                borrower_valid_q[selected_donor_resource_id_i])
                transaction_well_formed = 1'b0;
        end
        if (release_required_i && borrow_required_i &&
            release_resource_id_i == selected_donor_resource_id_i)
            transaction_well_formed = 1'b0;
    end

    always_ff @(posedge clk_i) begin
        commit_accepted_o <= 1'b0;
        commit_error_o <= 1'b0;
        if (!rst_ni) begin
            released_q <= '0;
            borrower_valid_q <= '0;
            borrower_id_q <= '0;
        end else if (commit_i) begin
            if (transaction_well_formed) begin
                // Nonblocking updates make release+borrow one observable edge.
                if (release_required_i)
                    released_q[release_resource_id_i] <= 1'b1;
                if (borrow_required_i) begin
                    borrower_valid_q[selected_donor_resource_id_i] <= 1'b1;
                    borrower_id_q[selected_donor_resource_id_i*SA_ID_W +: SA_ID_W] <= requester_sa_i;
                end
                commit_accepted_o <= 1'b1;
            end else begin
                commit_error_o <= 1'b1;
            end
        end
    end

    assign resource_released_o = released_q;
    assign resource_borrowed_o = borrower_valid_q;
    assign borrower_valid_o = borrower_valid_q;
    assign borrower_id_flat_o = borrower_id_q;

    initial begin
        if (RESOURCE_NUM == 0 || SA_ID_W == 0 || RESOURCE_ID_W == 0)
            $fatal(1, "dss_v2_resource_ledger has an invalid V2 parameter width");
    end
endmodule
`default_nettype wire
