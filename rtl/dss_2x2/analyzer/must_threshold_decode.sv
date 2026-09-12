`default_nettype none

module must_threshold_decode #(
    parameter integer COUNT_W = 5,
    parameter integer NUM_CFG = 7
) (
    input  wire [COUNT_W-1:0] row_count_i,
    input  wire [COUNT_W-1:0] col_count_i,
    output wire               row_gt1_o,
    output wire               row_gt2_o,
    output wire               row_gt3_o,
    output wire               col_gt1_o,
    output wire               col_gt2_o,
    output wire               col_gt3_o,
    output reg  [NUM_CFG-1:0] row_must_by_cfg_o,
    output reg  [NUM_CFG-1:0] col_must_by_cfg_o
);
    assign row_gt1_o = row_count_i > 1;
    assign row_gt2_o = row_count_i > 2;
    assign row_gt3_o = row_count_i > 3;
    assign col_gt1_o = col_count_i > 1;
    assign col_gt2_o = col_count_i > 2;
    assign col_gt3_o = col_count_i > 3;

    always @* begin
        // CFG={2R2C,2R1C,3R2C,3R1C,1R2C,2R3C,1R3C}.
        row_must_by_cfg_o[0] = row_gt2_o;
        row_must_by_cfg_o[1] = row_gt1_o;
        row_must_by_cfg_o[2] = row_gt2_o;
        row_must_by_cfg_o[3] = row_gt1_o;
        row_must_by_cfg_o[4] = row_gt2_o;
        row_must_by_cfg_o[5] = row_gt3_o;
        row_must_by_cfg_o[6] = row_gt3_o;
        col_must_by_cfg_o[0] = col_gt2_o;
        col_must_by_cfg_o[1] = col_gt2_o;
        col_must_by_cfg_o[2] = col_gt3_o;
        col_must_by_cfg_o[3] = col_gt3_o;
        col_must_by_cfg_o[4] = col_gt1_o;
        col_must_by_cfg_o[5] = col_gt2_o;
        col_must_by_cfg_o[6] = col_gt1_o;
    end
endmodule

`default_nettype wire
