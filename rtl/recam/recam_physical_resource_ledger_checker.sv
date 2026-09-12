`default_nettype none

// Read-only Phase 3F snapshot: resource 0=A_ROW, 1=D_ROW, 2=B_COL, 3=C_COL.
// Each borrower field is resource-local: 00 NONE, 01/10 valid neighbor, 11 invalid.
module recam_physical_resource_ledger_checker (
    input wire [3:0] resource_released_i,
    input wire [7:0] resource_borrower_flat_i,
    output reg ledger_state_valid_o,
    output reg [3:0] resource_available_o
);
    integer index;
    reg [1:0] borrower;
    always @* begin
        ledger_state_valid_o = 1'b1;
        resource_available_o = 4'd0;
        for (index = 0; index < 4; index = index + 1) begin
            borrower = resource_borrower_flat_i[index*2 +: 2];
            if (borrower == 2'b11 || (!resource_released_i[index] && borrower != 2'b00))
                ledger_state_valid_o = 1'b0;
            resource_available_o[index] = resource_released_i[index] && borrower == 2'b00;
        end
    end
endmodule
