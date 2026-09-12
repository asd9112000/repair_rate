`default_nettype none

// Temporary per-subarray retention of the single shared/tagged Hybrid view.
// This is not a ConfigID-replicated CAM: each stored record carries its
// ConfigID-valid mask and is used only to replay the selected view at final
// decode time.
module hybrid_delta_bank #(
    parameter integer NUM_SA = 4,
    parameter integer NUM_CFG = 7,
    parameter integer MAX_K = 5,
    parameter integer ENTRY_NUM = 14,
    parameter integer ROW_W = 10,
    parameter integer COL_W = 10,
    parameter integer PTR_W = $clog2(MAX_K)
) (
    input wire clk_i,
    input wire rst_ni,
    input wire clear_i,
    input wire capture_i,
    input wire [$clog2(NUM_SA)-1:0] subarray_i,
    input wire [ENTRY_NUM-1:0] hybrid_valid_i,
    input wire [ENTRY_NUM*ROW_W-1:0] hybrid_rows_flat_i,
    input wire [ENTRY_NUM*COL_W-1:0] hybrid_cols_flat_i,
    input wire [ENTRY_NUM*PTR_W-1:0] hybrid_ptrs_flat_i,
    input wire [ENTRY_NUM-1:0] hybrid_descriptors_i,
    input wire [ENTRY_NUM*NUM_CFG-1:0] hybrid_cfg_valid_flat_i,
    input wire [NUM_CFG*MAX_K-1:0] row_must_by_cfg_i,
    input wire [NUM_CFG*MAX_K-1:0] col_must_by_cfg_i,
    output reg [NUM_SA*ENTRY_NUM-1:0] hybrid_valid_o,
    output reg [NUM_SA*ENTRY_NUM*ROW_W-1:0] hybrid_rows_flat_o,
    output reg [NUM_SA*ENTRY_NUM*COL_W-1:0] hybrid_cols_flat_o,
    output reg [NUM_SA*ENTRY_NUM*PTR_W-1:0] hybrid_ptrs_flat_o,
    output reg [NUM_SA*ENTRY_NUM-1:0] hybrid_descriptors_o,
    output reg [NUM_SA*ENTRY_NUM*NUM_CFG-1:0] hybrid_cfg_valid_flat_o,
    output reg [NUM_SA*NUM_CFG*MAX_K-1:0] row_must_by_cfg_o,
    output reg [NUM_SA*NUM_CFG*MAX_K-1:0] col_must_by_cfg_o
);
    always @(posedge clk_i) begin
        if (!rst_ni || clear_i) begin
            hybrid_valid_o <= 0;
            hybrid_rows_flat_o <= 0;
            hybrid_cols_flat_o <= 0;
            hybrid_ptrs_flat_o <= 0;
            hybrid_descriptors_o <= 0;
            hybrid_cfg_valid_flat_o <= 0;
            row_must_by_cfg_o <= 0;
            col_must_by_cfg_o <= 0;
        end else if (capture_i) begin
            hybrid_valid_o[subarray_i*ENTRY_NUM +: ENTRY_NUM] <= hybrid_valid_i;
            hybrid_rows_flat_o[subarray_i*ENTRY_NUM*ROW_W +: ENTRY_NUM*ROW_W] <= hybrid_rows_flat_i;
            hybrid_cols_flat_o[subarray_i*ENTRY_NUM*COL_W +: ENTRY_NUM*COL_W] <= hybrid_cols_flat_i;
            hybrid_ptrs_flat_o[subarray_i*ENTRY_NUM*PTR_W +: ENTRY_NUM*PTR_W] <= hybrid_ptrs_flat_i;
            hybrid_descriptors_o[subarray_i*ENTRY_NUM +: ENTRY_NUM] <= hybrid_descriptors_i;
            hybrid_cfg_valid_flat_o[subarray_i*ENTRY_NUM*NUM_CFG +: ENTRY_NUM*NUM_CFG] <= hybrid_cfg_valid_flat_i;
            row_must_by_cfg_o[subarray_i*NUM_CFG*MAX_K +: NUM_CFG*MAX_K] <= row_must_by_cfg_i;
            col_must_by_cfg_o[subarray_i*NUM_CFG*MAX_K +: NUM_CFG*MAX_K] <= col_must_by_cfg_i;
        end
    end
endmodule

`default_nettype wire
