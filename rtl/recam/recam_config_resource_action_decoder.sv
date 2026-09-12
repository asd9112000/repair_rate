`default_nettype none

// Phase 3F action decoder. Resource IDs: 0=A_ROW, 1=D_ROW, 2=B_COL, 3=C_COL.
module recam_config_resource_action_decoder (
    input wire [1:0] sa_id_i,
    input wire [2:0] config_id_i,
    output reg config_legal_for_sa_o,
    output reg borrow_required_o,
    output reg release_required_o,
    output reg borrow_resource_type_o,  // 0=ROW, 1=COLUMN
    output reg release_resource_type_o, // 0=ROW, 1=COLUMN
    output reg [1:0] release_resource_o,
    output reg [1:0] donor_primary_o,
    output reg [1:0] donor_secondary_o
);
    always @* begin
        config_legal_for_sa_o = 1'b0;
        borrow_required_o = 1'b0;
        release_required_o = 1'b0;
        borrow_resource_type_o = 1'b0;
        release_resource_type_o = 1'b0;
        release_resource_o = 2'd0;
        donor_primary_o = 2'd0;
        donor_secondary_o = 2'd0;
        case (sa_id_i)
            2'd0: begin // A: B_COL then C_COL
                donor_primary_o = 2'd2; donor_secondary_o = 2'd3;
                case (config_id_i)
                    3'd0: config_legal_for_sa_o = 1'b1;
                    3'd4: begin config_legal_for_sa_o=1'b1; release_required_o=1'b1; release_resource_o=2'd0; end
                    3'd5: begin config_legal_for_sa_o=1'b1; borrow_required_o=1'b1; borrow_resource_type_o=1'b1; end
                    3'd6: begin config_legal_for_sa_o=1'b1; borrow_required_o=1'b1; borrow_resource_type_o=1'b1; release_required_o=1'b1; release_resource_o=2'd0; end
                    default: ;
                endcase
            end
            2'd3: begin // D: C_COL then B_COL
                donor_primary_o = 2'd3; donor_secondary_o = 2'd2;
                case (config_id_i)
                    3'd0: config_legal_for_sa_o = 1'b1;
                    3'd4: begin config_legal_for_sa_o=1'b1; release_required_o=1'b1; release_resource_o=2'd1; end
                    3'd5: begin config_legal_for_sa_o=1'b1; borrow_required_o=1'b1; borrow_resource_type_o=1'b1; end
                    3'd6: begin config_legal_for_sa_o=1'b1; borrow_required_o=1'b1; borrow_resource_type_o=1'b1; release_required_o=1'b1; release_resource_o=2'd1; end
                    default: ;
                endcase
            end
            2'd1: begin // B: A_ROW then D_ROW
                donor_primary_o = 2'd0; donor_secondary_o = 2'd1;
                case (config_id_i)
                    3'd0: config_legal_for_sa_o = 1'b1;
                    3'd1: begin config_legal_for_sa_o=1'b1; release_required_o=1'b1; release_resource_type_o=1'b1; release_resource_o=2'd2; end
                    3'd2: begin config_legal_for_sa_o=1'b1; borrow_required_o=1'b1; end
                    3'd3: begin config_legal_for_sa_o=1'b1; borrow_required_o=1'b1; release_required_o=1'b1; release_resource_type_o=1'b1; release_resource_o=2'd2; end
                    default: ;
                endcase
            end
            default: begin // C: D_ROW then A_ROW
                donor_primary_o = 2'd1; donor_secondary_o = 2'd0;
                case (config_id_i)
                    3'd0: config_legal_for_sa_o = 1'b1;
                    3'd1: begin config_legal_for_sa_o=1'b1; release_required_o=1'b1; release_resource_type_o=1'b1; release_resource_o=2'd3; end
                    3'd2: begin config_legal_for_sa_o=1'b1; borrow_required_o=1'b1; end
                    3'd3: begin config_legal_for_sa_o=1'b1; borrow_required_o=1'b1; release_required_o=1'b1; release_resource_type_o=1'b1; release_resource_o=2'd3; end
                    default: ;
                endcase
            end
        endcase
    end
endmodule
