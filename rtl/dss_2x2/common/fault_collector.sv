`default_nettype none

module fault_collector #(
    parameter integer ENTRY_NUM = 12,
    parameter integer ROW_ADDR_WIDTH = 10,
    parameter integer COL_ADDR_WIDTH = 10,
    parameter integer COUNT_WIDTH = (ENTRY_NUM <= 1) ? 1 : $clog2(ENTRY_NUM + 1)
) (
    input  wire                                      clk_i,
    input  wire                                      rst_ni,
    input  wire                                      clear_i,
    input  wire                                      enable_i,
    input  wire                                      fault_valid_i,
    output reg                                       fault_ready_o,
    input  wire [1:0]                                fault_subarray_i,
    input  wire [ROW_ADDR_WIDTH-1:0]                 fault_row_i,
    input  wire [COL_ADDR_WIDTH-1:0]                 fault_col_i,
    output wire                                      overflow_o,
    output wire [4*ENTRY_NUM-1:0]                    valid_vectors_o,
    output wire [4*ENTRY_NUM*ROW_ADDR_WIDTH-1:0]     rows_flat_o,
    output wire [4*ENTRY_NUM*COL_ADDR_WIDTH-1:0]     cols_flat_o,
    output wire [4*COUNT_WIDTH-1:0]                  fault_counts_o
);
    wire [3:0] cam_ready;
    wire [3:0] cam_overflow;
    wire [3:0] cam_write_valid;

    always @* begin
        fault_ready_o = 1'b0;
        if (enable_i) begin
            case (fault_subarray_i)
                2'd0: fault_ready_o = cam_ready[0];
                2'd1: fault_ready_o = cam_ready[1];
                2'd2: fault_ready_o = cam_ready[2];
                2'd3: fault_ready_o = cam_ready[3];
                default: fault_ready_o = 1'b0;
            endcase
        end
    end

    assign cam_write_valid[0] = enable_i && fault_valid_i && fault_subarray_i == 2'd0;
    assign cam_write_valid[1] = enable_i && fault_valid_i && fault_subarray_i == 2'd1;
    assign cam_write_valid[2] = enable_i && fault_valid_i && fault_subarray_i == 2'd2;
    assign cam_write_valid[3] = enable_i && fault_valid_i && fault_subarray_i == 2'd3;
    assign overflow_o = |cam_overflow;

    genvar subarray;
    generate
        for (subarray = 0; subarray < 4; subarray = subarray + 1) begin : gen_fault_cam
            fault_cam #(
                .ENTRY_NUM(ENTRY_NUM),
                .ROW_ADDR_WIDTH(ROW_ADDR_WIDTH),
                .COL_ADDR_WIDTH(COL_ADDR_WIDTH),
                .COUNT_WIDTH(COUNT_WIDTH)
            ) cam (
                .clk_i(clk_i),
                .rst_ni(rst_ni),
                .clear_i(clear_i),
                .fault_valid_i(cam_write_valid[subarray]),
                .fault_ready_o(cam_ready[subarray]),
                .fault_row_i(fault_row_i),
                .fault_col_i(fault_col_i),
                .overflow_o(cam_overflow[subarray]),
                .fault_count_o(fault_counts_o[subarray*COUNT_WIDTH +: COUNT_WIDTH]),
                .valid_vector_o(valid_vectors_o[subarray*ENTRY_NUM +: ENTRY_NUM]),
                .rows_flat_o(rows_flat_o[subarray*ENTRY_NUM*ROW_ADDR_WIDTH +: ENTRY_NUM*ROW_ADDR_WIDTH]),
                .cols_flat_o(cols_flat_o[subarray*ENTRY_NUM*COL_ADDR_WIDTH +: ENTRY_NUM*COL_ADDR_WIDTH])
            );
        end
    endgenerate
endmodule

`default_nettype wire
