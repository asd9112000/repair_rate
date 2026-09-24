`default_nettype none

// Converts a locally repairable RS2 candidate descriptor into the frozen
// directional sharing effects. The descriptor, rather than the action name,
// determines whether this candidate actually releases or borrows a line.
module recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_candidate_transition_adapter (
    input  wire [1:0] sa_id_i,
    input  wire       candidate_valid_i,
    input  wire [2:0] row_count_i,
    input  wire [2:0] col_count_i,
    output reg        actual_release_o,
    output reg        actual_borrow_o,
    output reg [1:0]  release_resource_o,
    output reg [1:0]  donor_resource_o
);
    always @* begin
        actual_release_o = 1'b0;
        actual_borrow_o = 1'b0;
        release_resource_o = 2'd0;
        donor_resource_o = 2'd0;

        case (sa_id_i)
            2'd0: begin
                release_resource_o = 2'd0;
                donor_resource_o = 2'd2;
                actual_release_o = candidate_valid_i && (row_count_i < 3'd2);
                actual_borrow_o = candidate_valid_i && (col_count_i > 3'd2);
            end
            2'd1: begin
                release_resource_o = 2'd2;
                donor_resource_o = 2'd1;
                actual_release_o = candidate_valid_i && (col_count_i < 3'd2);
                actual_borrow_o = candidate_valid_i && (row_count_i > 3'd2);
            end
            2'd2: begin
                release_resource_o = 2'd3;
                donor_resource_o = 2'd0;
                actual_release_o = candidate_valid_i && (col_count_i < 3'd2);
                actual_borrow_o = candidate_valid_i && (row_count_i > 3'd2);
            end
            default: begin
                release_resource_o = 2'd1;
                donor_resource_o = 2'd3;
                actual_release_o = candidate_valid_i && (row_count_i < 3'd2);
                actual_borrow_o = candidate_valid_i && (col_count_i > 3'd2);
            end
        endcase
    end
endmodule

`default_nettype wire
