`default_nettype none

// One physical Hybrid payload store.  Multiple entries may reference one
// fault when configuration-dependent descriptor/pointer metadata differs.
module tagged_hybrid_store #(
    parameter integer ENTRY_NUM = 14,
    parameter integer FAULT_REF_W = 8,
    parameter integer ROW_W = 10,
    parameter integer COL_W = 10,
    parameter integer PIVOT_PTR_W = 3,
    parameter integer NUM_CFG = 7,
    parameter integer INDEX_W = (ENTRY_NUM <= 1) ? 1 : $clog2(ENTRY_NUM),
    parameter integer COUNT_W = (ENTRY_NUM <= 1) ? 1 : $clog2(ENTRY_NUM + 1)
) (
    input  wire                         clk_i,
    input  wire                         rst_ni,
    input  wire                         clear_i,
    input  wire                         write_valid_i,
    input  wire [FAULT_REF_W-1:0]       fault_ref_i,
    input  wire [ROW_W-1:0]             fault_row_i,
    input  wire [COL_W-1:0]             fault_col_i,
    input  wire                         descriptor_i,
    input  wire [PIVOT_PTR_W-1:0]       pivot_ptr_i,
    input  wire [NUM_CFG-1:0]           cfg_valid_i,
    output wire                         full_o,
    output reg                          overflow_o,
    output reg  [COUNT_W-1:0]           occupancy_o,
    output wire [ENTRY_NUM-1:0]         valid_o,
    output wire [ENTRY_NUM*FAULT_REF_W-1:0] fault_refs_flat_o,
    output wire [ENTRY_NUM*ROW_W-1:0] rows_flat_o,
    output wire [ENTRY_NUM*COL_W-1:0] cols_flat_o,
    output wire [ENTRY_NUM-1:0]         descriptors_o,
    output wire [ENTRY_NUM*PIVOT_PTR_W-1:0] pivot_ptrs_flat_o,
    output wire [ENTRY_NUM*NUM_CFG-1:0] cfg_valid_flat_o
);
    reg [ENTRY_NUM-1:0] valid_mem;
    reg [FAULT_REF_W-1:0] fault_ref_mem [0:ENTRY_NUM-1];
    reg [ROW_W-1:0] row_mem [0:ENTRY_NUM-1];
    reg [COL_W-1:0] col_mem [0:ENTRY_NUM-1];
    reg descriptor_mem [0:ENTRY_NUM-1];
    reg [PIVOT_PTR_W-1:0] pivot_ptr_mem [0:ENTRY_NUM-1];
    reg [NUM_CFG-1:0] cfg_valid_mem [0:ENTRY_NUM-1];

    /* verilator lint_off WIDTH */
    assign full_o = occupancy_o == ENTRY_NUM;
    /* verilator lint_on WIDTH */
    assign valid_o = valid_mem;
    genvar entry;
    generate
        for (entry = 0; entry < ENTRY_NUM; entry = entry + 1) begin : g_flat
            assign fault_refs_flat_o[entry*FAULT_REF_W +: FAULT_REF_W] = fault_ref_mem[entry];
            assign rows_flat_o[entry*ROW_W +: ROW_W] = row_mem[entry];
            assign cols_flat_o[entry*COL_W +: COL_W] = col_mem[entry];
            assign descriptors_o[entry] = descriptor_mem[entry];
            assign pivot_ptrs_flat_o[entry*PIVOT_PTR_W +: PIVOT_PTR_W] = pivot_ptr_mem[entry];
            assign cfg_valid_flat_o[entry*NUM_CFG +: NUM_CFG] = cfg_valid_mem[entry];
        end
    endgenerate

    always @(posedge clk_i) begin
        if (!rst_ni || clear_i) begin
            occupancy_o <= {COUNT_W{1'b0}};
            valid_mem <= {ENTRY_NUM{1'b0}};
            overflow_o <= 1'b0;
        end else if (write_valid_i) begin
            if (!full_o) begin
                valid_mem[occupancy_o] <= 1'b1;
                fault_ref_mem[occupancy_o] <= fault_ref_i;
                row_mem[occupancy_o] <= fault_row_i;
                col_mem[occupancy_o] <= fault_col_i;
                descriptor_mem[occupancy_o] <= descriptor_i;
                pivot_ptr_mem[occupancy_o] <= pivot_ptr_i;
                cfg_valid_mem[occupancy_o] <= cfg_valid_i;
                occupancy_o <= occupancy_o + 1'b1;
            end else begin
                overflow_o <= 1'b1;
            end
        end
    end
endmodule

`default_nettype wire
