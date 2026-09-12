`default_nettype none
module config_result_bank #(
    parameter integer NUM_SA = 4,
    parameter integer NUM_CFG = 7,
    parameter integer PATTERN_ID_W = 4
) (
    input wire clk_i, input wire rst_ni, input wire clear_i,
    input wire capture_i,
    input wire [$clog2(NUM_SA)-1:0] subarray_i,
    input wire [NUM_CFG*PATTERN_ID_W-1:0] config_patterns_i,
    output reg [NUM_SA*NUM_CFG*PATTERN_ID_W-1:0] config_pattern_map_o,
    output reg [NUM_SA-1:0] subarray_valid_o
);
    always @(posedge clk_i) begin
        if (!rst_ni || clear_i) begin
            config_pattern_map_o <= {(NUM_SA*NUM_CFG*PATTERN_ID_W){1'b0}};
            subarray_valid_o <= {NUM_SA{1'b0}};
        end else if (capture_i) begin
            config_pattern_map_o[subarray_i*NUM_CFG*PATTERN_ID_W +: NUM_CFG*PATTERN_ID_W] <= config_patterns_i;
            subarray_valid_o[subarray_i] <= 1'b1;
        end
    end
endmodule
`default_nettype wire
