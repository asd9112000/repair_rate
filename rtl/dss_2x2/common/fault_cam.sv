`default_nettype none

module fault_cam #(
    parameter integer ENTRY_NUM = 12,
    parameter integer ROW_ADDR_WIDTH = 10,
    parameter integer COL_ADDR_WIDTH = 10,
    parameter integer COUNT_WIDTH = (ENTRY_NUM <= 1) ? 1 : $clog2(ENTRY_NUM + 1),
    parameter integer INDEX_WIDTH = (ENTRY_NUM <= 1) ? 1 : $clog2(ENTRY_NUM)
) (
    input  wire                                  clk_i,
    input  wire                                  rst_ni,
    input  wire                                  clear_i,
    input  wire                                  fault_valid_i,
    output wire                                  fault_ready_o,
    input  wire [ROW_ADDR_WIDTH-1:0]             fault_row_i,
    input  wire [COL_ADDR_WIDTH-1:0]             fault_col_i,
    output reg                                   overflow_o,
    output reg  [COUNT_WIDTH-1:0]                fault_count_o,
    output wire [ENTRY_NUM-1:0]                  valid_vector_o,
    output wire [ENTRY_NUM*ROW_ADDR_WIDTH-1:0]   rows_flat_o,
    output wire [ENTRY_NUM*COL_ADDR_WIDTH-1:0]   cols_flat_o
);
    localparam integer KEY_WIDTH = ROW_ADDR_WIDTH + COL_ADDR_WIDTH;
    wire [ENTRY_NUM*KEY_WIDTH-1:0] keys_flat;
    wire [ENTRY_NUM-1:0] unused_data_flat;
    wire [ENTRY_NUM-1:0] unused_match_vector;
    wire unused_read_valid;
    wire [KEY_WIDTH-1:0] unused_read_key;
    wire unused_read_data;
    wire unused_search_match;
    wire [INDEX_WIDTH-1:0] unused_search_index;
    wire unused_search_data;
    wire accepted_fault;

    // Count uses one additional representable value for ENTRY_NUM itself.
    /* verilator lint_off WIDTH */
    localparam [COUNT_WIDTH-1:0] ENTRY_COUNT = ENTRY_NUM;
    /* verilator lint_on WIDTH */

    assign fault_ready_o = fault_count_o < ENTRY_COUNT;
    assign accepted_fault = fault_valid_i && fault_ready_o;

    generic_cam #(
        .ENTRY_NUM(ENTRY_NUM),
        .KEY_WIDTH(KEY_WIDTH),
        .DATA_WIDTH(1),
        .INDEX_WIDTH(INDEX_WIDTH)
    ) storage (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .clear_i(clear_i),
        .write_valid_i(accepted_fault),
        .write_index_i(fault_count_o[INDEX_WIDTH-1:0]),
        .write_key_i({fault_row_i, fault_col_i}),
        .write_data_i(1'b0),
        .invalidate_valid_i(1'b0),
        .invalidate_index_i({INDEX_WIDTH{1'b0}}),
        .read_valid_i(1'b0),
        .read_index_i({INDEX_WIDTH{1'b0}}),
        .read_entry_valid_o(unused_read_valid),
        .read_key_o(unused_read_key),
        .read_data_o(unused_read_data),
        .search_valid_i(1'b0),
        .search_key_i({KEY_WIDTH{1'b0}}),
        .search_match_o(unused_search_match),
        .search_index_o(unused_search_index),
        .search_data_o(unused_search_data),
        .match_vector_o(unused_match_vector),
        .valid_vector_o(valid_vector_o),
        .keys_flat_o(keys_flat),
        .data_flat_o(unused_data_flat)
    );

    genvar entry;
    generate
        for (entry = 0; entry < ENTRY_NUM; entry = entry + 1) begin : gen_address_split
            assign cols_flat_o[entry*COL_ADDR_WIDTH +: COL_ADDR_WIDTH] =
                keys_flat[entry*KEY_WIDTH +: COL_ADDR_WIDTH];
            assign rows_flat_o[entry*ROW_ADDR_WIDTH +: ROW_ADDR_WIDTH] =
                keys_flat[entry*KEY_WIDTH + COL_ADDR_WIDTH +: ROW_ADDR_WIDTH];
        end
    endgenerate

    always @(posedge clk_i) begin
        if (!rst_ni || clear_i) begin
            fault_count_o <= {COUNT_WIDTH{1'b0}};
            overflow_o <= 1'b0;
        end else begin
            if (accepted_fault)
                fault_count_o <= fault_count_o + 1'b1;
            if (fault_valid_i && !fault_ready_o)
                overflow_o <= 1'b1;
        end
    end
endmodule

`default_nettype wire
