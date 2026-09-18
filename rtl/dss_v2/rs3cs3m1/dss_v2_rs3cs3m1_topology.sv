`default_nettype none

// Stateless target m=1 legality/feasibility view. The four-resource graph and
// donor order are identical to the frozen V2 directional topology.
module dss_v2_rs3cs3m1_topology (
    input  logic [1:0] sa_i,
    input  logic       descriptor_valid_i,
    input  logic       release_required_i,
    input  logic       borrow_required_i,
    input  logic [3:0] resource_released_i,
    input  logic [3:0] resource_borrowed_i,
    output logic       physical_feasible_o,
    output logic       donor_valid_o,
    output logic [1:0] donor_o,
    output logic       release_valid_o,
    output logic [1:0] release_resource_o
);
    logic [1:0] primary_donor, secondary_donor;
    always_comb begin
        release_resource_o = 2'd0;
        primary_donor = 2'd2;
        secondary_donor = 2'd3;
        unique case (sa_i)
            2'd0: begin release_resource_o = 2'd0; primary_donor = 2'd2; secondary_donor = 2'd3; end // A
            2'd3: begin release_resource_o = 2'd1; primary_donor = 2'd3; secondary_donor = 2'd2; end // D
            2'd1: begin release_resource_o = 2'd2; primary_donor = 2'd0; secondary_donor = 2'd1; end // B
            2'd2: begin release_resource_o = 2'd3; primary_donor = 2'd1; secondary_donor = 2'd0; end // C
        endcase
        donor_valid_o = 1'b0;
        donor_o = primary_donor;
        if (resource_released_i[primary_donor] && !resource_borrowed_i[primary_donor]) begin
            donor_valid_o = 1'b1;
            donor_o = primary_donor;
        end else if (resource_released_i[secondary_donor] && !resource_borrowed_i[secondary_donor]) begin
            donor_valid_o = 1'b1;
            donor_o = secondary_donor;
        end
        release_valid_o = descriptor_valid_i && release_required_i;
        physical_feasible_o = descriptor_valid_i && (!borrow_required_i || donor_valid_o);
    end
endmodule
`default_nettype wire
