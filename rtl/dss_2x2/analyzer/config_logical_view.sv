`default_nettype none

// Stateless ConfigID tag selection and complete row/column transpose for one
// canonical analyzer input view.
module config_logical_view #(
    parameter integer MAX_K=5,
    parameter integer NUM_CFG=7,
    parameter integer HYBRID_ENTRY_NUM=14,
    parameter integer ADDR_W=10,
    parameter integer PTR_W=$clog2(MAX_K)
) (
    input wire [2:0] config_id_i,
    input wire transpose_i,
    input wire [MAX_K*NUM_CFG-1:0] cfg_pivot_valid_i,
    input wire [MAX_K*ADDR_W-1:0] pivot_rows_flat_i,
    input wire [MAX_K*ADDR_W-1:0] pivot_cols_flat_i,
    input wire [NUM_CFG*MAX_K-1:0] row_must_by_cfg_i,
    input wire [NUM_CFG*MAX_K-1:0] col_must_by_cfg_i,
    input wire [HYBRID_ENTRY_NUM-1:0] hybrid_valid_i,
    input wire [HYBRID_ENTRY_NUM*ADDR_W-1:0] hybrid_rows_flat_i,
    input wire [HYBRID_ENTRY_NUM*ADDR_W-1:0] hybrid_cols_flat_i,
    input wire [HYBRID_ENTRY_NUM*PTR_W-1:0] hybrid_ptrs_flat_i,
    input wire [HYBRID_ENTRY_NUM-1:0] hybrid_descriptors_i,
    input wire [HYBRID_ENTRY_NUM*NUM_CFG-1:0] hybrid_cfg_valid_flat_i,
    output reg [MAX_K-1:0] pivot_valid_o,
    output reg [MAX_K*ADDR_W-1:0] pivot_rows_flat_o,
    output reg [MAX_K*ADDR_W-1:0] pivot_cols_flat_o,
    output reg [MAX_K-1:0] row_must_o,
    output reg [MAX_K-1:0] col_must_o,
    output reg [HYBRID_ENTRY_NUM-1:0] hybrid_valid_o,
    output reg [HYBRID_ENTRY_NUM*ADDR_W-1:0] hybrid_rows_flat_o,
    output reg [HYBRID_ENTRY_NUM*ADDR_W-1:0] hybrid_cols_flat_o
);
    integer entry;
    integer ptr;
    reg retired_by_must;

    always @* begin
        pivot_valid_o=0;
        pivot_rows_flat_o=0;
        pivot_cols_flat_o=0;
        row_must_o=0;
        col_must_o=0;
        hybrid_valid_o=0;
        hybrid_rows_flat_o=0;
        hybrid_cols_flat_o=0;

        for(entry=0;entry<MAX_K;entry=entry+1) begin
            pivot_valid_o[entry]=cfg_pivot_valid_i[entry*NUM_CFG+config_id_i];
            row_must_o[entry]=transpose_i
                ? col_must_by_cfg_i[config_id_i*MAX_K+entry]
                : row_must_by_cfg_i[config_id_i*MAX_K+entry];
            col_must_o[entry]=transpose_i
                ? row_must_by_cfg_i[config_id_i*MAX_K+entry]
                : col_must_by_cfg_i[config_id_i*MAX_K+entry];
            if(transpose_i) begin
                pivot_rows_flat_o[entry*ADDR_W +: ADDR_W]=pivot_cols_flat_i[entry*ADDR_W +: ADDR_W];
                pivot_cols_flat_o[entry*ADDR_W +: ADDR_W]=pivot_rows_flat_i[entry*ADDR_W +: ADDR_W];
            end else begin
                pivot_rows_flat_o[entry*ADDR_W +: ADDR_W]=pivot_rows_flat_i[entry*ADDR_W +: ADDR_W];
                pivot_cols_flat_o[entry*ADDR_W +: ADDR_W]=pivot_cols_flat_i[entry*ADDR_W +: ADDR_W];
            end
        end

        for(entry=0;entry<HYBRID_ENTRY_NUM;entry=entry+1) begin
            ptr=hybrid_ptrs_flat_i[entry*PTR_W +: PTR_W];
            retired_by_must=1'b1;
            if(ptr<MAX_K)
                retired_by_must=hybrid_descriptors_i[entry]
                    ? col_must_by_cfg_i[config_id_i*MAX_K+ptr]
                    : row_must_by_cfg_i[config_id_i*MAX_K+ptr];
            hybrid_valid_o[entry]=hybrid_valid_i[entry] &&
                hybrid_cfg_valid_flat_i[entry*NUM_CFG+config_id_i] &&
                ptr<MAX_K && !retired_by_must;
            if(transpose_i) begin
                hybrid_rows_flat_o[entry*ADDR_W +: ADDR_W]=hybrid_cols_flat_i[entry*ADDR_W +: ADDR_W];
                hybrid_cols_flat_o[entry*ADDR_W +: ADDR_W]=hybrid_rows_flat_i[entry*ADDR_W +: ADDR_W];
            end else begin
                hybrid_rows_flat_o[entry*ADDR_W +: ADDR_W]=hybrid_rows_flat_i[entry*ADDR_W +: ADDR_W];
                hybrid_cols_flat_o[entry*ADDR_W +: ADDR_W]=hybrid_cols_flat_i[entry*ADDR_W +: ADDR_W];
            end
        end
    end
endmodule

`default_nettype wire
