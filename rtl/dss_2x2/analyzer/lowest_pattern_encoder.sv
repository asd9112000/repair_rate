`default_nettype none
module lowest_pattern_encoder #(
    parameter integer PATTERN_NUM = 10,
    parameter integer PATTERN_ID_W = 4
) (
    input wire [PATTERN_NUM-1:0] candidate_valid_i,
    output reg config_valid_o,
    output reg [PATTERN_ID_W-1:0] pattern_id_o
);
    integer index;
    always @* begin
        config_valid_o = 1'b0;
        pattern_id_o = {PATTERN_ID_W{1'b0}};
        for (index = 0; index < PATTERN_NUM; index = index + 1)
            if (!config_valid_o && candidate_valid_i[index]) begin
                config_valid_o = 1'b1;
                pattern_id_o = index[PATTERN_ID_W-1:0] + 1'b1;
            end
    end
endmodule
`default_nettype wire
