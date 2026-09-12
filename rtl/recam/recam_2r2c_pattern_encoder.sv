`default_nettype none

module recam_2r2c_pattern_encoder (
    input  wire [5:0] candidate_valid_i,
    output reg        repairable_o,
    output reg  [3:0] pattern_id_o
);
    integer candidate;
    always @* begin
        repairable_o = 1'b0;
        pattern_id_o = 4'd0;
        for (candidate = 0; candidate < 6; candidate = candidate + 1)
            if (!repairable_o && candidate_valid_i[candidate]) begin
                repairable_o = 1'b1;
                pattern_id_o = candidate[3:0] + 1'b1;
            end
    end
endmodule

`default_nettype wire
