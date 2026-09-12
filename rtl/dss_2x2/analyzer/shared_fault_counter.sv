`default_nettype none

// Shared physical-address counter table.  It is updated once for every
// accepted fault and is not replicated by ConfigID; threshold decode selects
// the relevant Rs/Cs interpretation afterwards.
module shared_fault_counter #(
    parameter integer ENTRY_NUM = 12,
    parameter integer ROW_W = 10,
    parameter integer COL_W = 10,
    parameter integer COUNT_W = (ENTRY_NUM <= 1) ? 1 : $clog2(ENTRY_NUM + 1)
) (
    input  wire clk_i, input wire rst_ni, input wire clear_i,
    input  wire fault_valid_i, input wire [ROW_W-1:0] fault_row_i,
    input  wire [COL_W-1:0] fault_col_i,
    output reg overflow_o,
    output wire [ENTRY_NUM-1:0] row_valid_o,
    output wire [ENTRY_NUM*ROW_W-1:0] row_addrs_flat_o,
    output wire [ENTRY_NUM*COUNT_W-1:0] row_counts_flat_o,
    output wire [ENTRY_NUM-1:0] col_valid_o,
    output wire [ENTRY_NUM*COL_W-1:0] col_addrs_flat_o,
    output wire [ENTRY_NUM*COUNT_W-1:0] col_counts_flat_o
);
    reg [ENTRY_NUM-1:0] row_valid_mem, col_valid_mem;
    reg [ROW_W-1:0] row_mem [0:ENTRY_NUM-1];
    reg [COL_W-1:0] col_mem [0:ENTRY_NUM-1];
    reg [COUNT_W-1:0] row_count_mem [0:ENTRY_NUM-1];
    reg [COUNT_W-1:0] col_count_mem [0:ENTRY_NUM-1];
    integer row_hit, col_hit, row_free, col_free, index;
    assign row_valid_o = row_valid_mem;
    assign col_valid_o = col_valid_mem;
    genvar entry;
    generate for (entry = 0; entry < ENTRY_NUM; entry = entry + 1) begin : g_flat
        assign row_addrs_flat_o[entry*ROW_W +: ROW_W] = row_mem[entry];
        assign row_counts_flat_o[entry*COUNT_W +: COUNT_W] = row_count_mem[entry];
        assign col_addrs_flat_o[entry*COL_W +: COL_W] = col_mem[entry];
        assign col_counts_flat_o[entry*COUNT_W +: COUNT_W] = col_count_mem[entry];
    end endgenerate
    always @(posedge clk_i) begin
        if (!rst_ni || clear_i) begin
            row_valid_mem <= {ENTRY_NUM{1'b0}}; col_valid_mem <= {ENTRY_NUM{1'b0}};
            overflow_o <= 1'b0;
            for (index = 0; index < ENTRY_NUM; index = index + 1) begin
                row_count_mem[index] <= {COUNT_W{1'b0}};
                col_count_mem[index] <= {COUNT_W{1'b0}};
            end
        end else if (fault_valid_i) begin
            /* verilator lint_off BLKSEQ */
            row_hit = -1; col_hit = -1; row_free = -1; col_free = -1;
            for (index = 0; index < ENTRY_NUM; index = index + 1) begin
                if (row_valid_mem[index] && row_mem[index] == fault_row_i && row_hit < 0) row_hit = index;
                if (!row_valid_mem[index] && row_free < 0) row_free = index;
                if (col_valid_mem[index] && col_mem[index] == fault_col_i && col_hit < 0) col_hit = index;
                if (!col_valid_mem[index] && col_free < 0) col_free = index;
            end
            /* verilator lint_on BLKSEQ */
            if (row_hit >= 0) row_count_mem[row_hit] <= row_count_mem[row_hit] + 1'b1;
            else if (row_free >= 0) begin
                row_valid_mem[row_free] <= 1'b1; row_mem[row_free] <= fault_row_i;
                row_count_mem[row_free] <= {{(COUNT_W-1){1'b0}}, 1'b1};
            end else overflow_o <= 1'b1;
            if (col_hit >= 0) col_count_mem[col_hit] <= col_count_mem[col_hit] + 1'b1;
            else if (col_free >= 0) begin
                col_valid_mem[col_free] <= 1'b1; col_mem[col_free] <= fault_col_i;
                col_count_mem[col_free] <= {{(COUNT_W-1){1'b0}}, 1'b1};
            end else overflow_o <= 1'b1;
        end
    end
endmodule

`default_nettype wire
