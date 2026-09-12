`default_nettype none

module cam_reuse_temp_buffer #(
    parameter integer ENTRY_NUM = 5,
    parameter integer ROW_W = 10,
    parameter integer COL_W = 10,
    parameter integer NUM_CFG = 7,
    parameter integer COUNT_W = (ENTRY_NUM <= 1) ? 1 : $clog2(ENTRY_NUM + 1)
) (
    input wire clk_i, input wire rst_ni, input wire clear_i,
    input wire write_valid_i, input wire [ROW_W-1:0] row_i,
    input wire [COL_W-1:0] col_i, input wire [NUM_CFG-1:0] cfg_valid_i,
    output wire full_o, output reg overflow_o, output reg [COUNT_W-1:0] occupancy_o,
    output wire [ENTRY_NUM-1:0] valid_o,
    output wire [ENTRY_NUM*ROW_W-1:0] rows_flat_o,
    output wire [ENTRY_NUM*COL_W-1:0] cols_flat_o,
    output wire [ENTRY_NUM*NUM_CFG-1:0] cfg_valid_flat_o
);
    reg [ENTRY_NUM-1:0] valid_mem;
    reg [ROW_W-1:0] row_mem [0:ENTRY_NUM-1];
    reg [COL_W-1:0] col_mem [0:ENTRY_NUM-1];
    reg [NUM_CFG-1:0] cfg_mem [0:ENTRY_NUM-1];
    /* verilator lint_off WIDTH */
    assign full_o = occupancy_o == ENTRY_NUM;
    /* verilator lint_on WIDTH */
    assign valid_o = valid_mem;
    genvar entry;
    generate for (entry = 0; entry < ENTRY_NUM; entry = entry + 1) begin : g_flat
        assign rows_flat_o[entry*ROW_W +: ROW_W] = row_mem[entry];
        assign cols_flat_o[entry*COL_W +: COL_W] = col_mem[entry];
        assign cfg_valid_flat_o[entry*NUM_CFG +: NUM_CFG] = cfg_mem[entry];
    end endgenerate
    always @(posedge clk_i) begin
        if (!rst_ni || clear_i) begin
            occupancy_o <= {COUNT_W{1'b0}}; valid_mem <= {ENTRY_NUM{1'b0}}; overflow_o <= 1'b0;
        end else if (write_valid_i) begin
            if (!full_o) begin
                valid_mem[occupancy_o] <= 1'b1; row_mem[occupancy_o] <= row_i;
                col_mem[occupancy_o] <= col_i; cfg_mem[occupancy_o] <= cfg_valid_i;
                occupancy_o <= occupancy_o + 1'b1;
            end else overflow_o <= 1'b1;
        end
    end
endmodule

`default_nettype wire
