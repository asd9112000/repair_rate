`default_nettype none
module config_resource_decode(
    input wire [2:0] config_id_i,
    output reg [2:0] rows_o,
    output reg [2:0] cols_o,
    output reg valid_o
);
    always @* begin
        valid_o = 1'b1;
        case (config_id_i)
            3'd0: begin rows_o=2; cols_o=2; end
            3'd1: begin rows_o=2; cols_o=1; end
            3'd2: begin rows_o=3; cols_o=2; end
            3'd3: begin rows_o=3; cols_o=1; end
            3'd4: begin rows_o=1; cols_o=2; end
            3'd5: begin rows_o=2; cols_o=3; end
            3'd6: begin rows_o=1; cols_o=3; end
            default: begin rows_o=0; cols_o=0; valid_o=1'b0; end
        endcase
    end
endmodule
`default_nettype wire
