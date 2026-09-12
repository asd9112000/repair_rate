`default_nettype none

module shared_pivot_cam #(
    parameter integer ROW_W = 10,
    parameter integer COL_W = 10,
    parameter integer MAX_K = 5,
    parameter integer NUM_CFG = 7,
    parameter integer COUNT_W = (MAX_K <= 1) ? 1 : $clog2(MAX_K + 1)
) (
    input  wire                         clk_i,
    input  wire                         rst_ni,
    input  wire                         clear_i,
    input  wire                         pivot_write_i,
    input  wire [ROW_W-1:0]             pivot_row_i,
    input  wire [COL_W-1:0]             pivot_col_i,
    output wire                         full_o,
    output reg                          overflow_o,
    output reg  [COUNT_W-1:0]           occupancy_o,
    output wire [MAX_K-1:0]             valid_o,
    output wire [MAX_K*ROW_W-1:0]       rows_flat_o,
    output wire [MAX_K*COL_W-1:0]       cols_flat_o,
    // Config activity is metadata only; it never owns another address array.
    output wire [MAX_K*NUM_CFG-1:0]     cfg_entry_valid_o
);
    reg [ROW_W-1:0] row_mem [0:MAX_K-1];
    reg [COL_W-1:0] col_mem [0:MAX_K-1];
    reg [MAX_K-1:0] valid_mem;
    /* verilator lint_off WIDTH */
    assign full_o = occupancy_o == MAX_K;
    /* verilator lint_on WIDTH */
    assign valid_o = valid_mem;

    genvar entry;
    generate
        for (entry = 0; entry < MAX_K; entry = entry + 1) begin : g_flat
            assign rows_flat_o[entry*ROW_W +: ROW_W] = row_mem[entry];
            assign cols_flat_o[entry*COL_W +: COL_W] = col_mem[entry];
        end
    endgenerate

    // Config order is CFG0..CFG6: K={4,3,5,4,3,5,4}.
    genvar cfg_idx;
    generate
        for (entry = 0; entry < MAX_K; entry = entry + 1) begin : g_cfg_valid
            for (cfg_idx = 0; cfg_idx < NUM_CFG; cfg_idx = cfg_idx + 1) begin : g_cfg
                if (cfg_idx == 0) assign cfg_entry_valid_o[entry*NUM_CFG + cfg_idx] = valid_mem[entry] && entry < 4;
                else if (cfg_idx == 1 || cfg_idx == 4) assign cfg_entry_valid_o[entry*NUM_CFG + cfg_idx] = valid_mem[entry] && entry < 3;
                else if (cfg_idx == 2 || cfg_idx == 5) assign cfg_entry_valid_o[entry*NUM_CFG + cfg_idx] = valid_mem[entry] && entry < 5;
                else assign cfg_entry_valid_o[entry*NUM_CFG + cfg_idx] = valid_mem[entry] && entry < 4;
            end
        end
    endgenerate

    always @(posedge clk_i) begin
        if (!rst_ni || clear_i) begin
            occupancy_o <= {COUNT_W{1'b0}};
            valid_mem <= {MAX_K{1'b0}};
            overflow_o <= 1'b0;
        end else if (pivot_write_i) begin
            if (!full_o) begin
                row_mem[occupancy_o] <= pivot_row_i;
                col_mem[occupancy_o] <= pivot_col_i;
                valid_mem[occupancy_o] <= 1'b1;
                occupancy_o <= occupancy_o + 1'b1;
            end else begin
                overflow_o <= 1'b1;
            end
        end
    end
endmodule

`default_nettype wire
