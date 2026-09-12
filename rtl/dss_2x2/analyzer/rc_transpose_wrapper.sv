`default_nettype none
module rc_transpose_wrapper #(
    parameter integer ROW_W = 10,
    parameter integer COL_W = 10,
    parameter integer MAX_K = 5,
    parameter integer COUNT_W = 4,
    parameter integer NORM_W = (ROW_W > COL_W) ? ROW_W : COL_W
) (
    input wire transpose_i,
    input wire [2:0] physical_rows_i,
    input wire [2:0] physical_cols_i,
    input wire [MAX_K*ROW_W-1:0] physical_pivot_rows_i,
    input wire [MAX_K*COL_W-1:0] physical_pivot_cols_i,
    input wire [MAX_K*COUNT_W-1:0] physical_row_counts_i,
    input wire [MAX_K*COUNT_W-1:0] physical_col_counts_i,
    input wire [MAX_K-1:0] physical_row_must_i,
    input wire [MAX_K-1:0] physical_col_must_i,
    output reg [2:0] canonical_rows_o,
    output reg [2:0] canonical_cols_o,
    output reg [MAX_K*NORM_W-1:0] canonical_pivot_rows_o,
    output reg [MAX_K*NORM_W-1:0] canonical_pivot_cols_o,
    output reg [MAX_K*COUNT_W-1:0] canonical_row_counts_o,
    output reg [MAX_K*COUNT_W-1:0] canonical_col_counts_o,
    output reg [MAX_K-1:0] canonical_row_must_o,
    output reg [MAX_K-1:0] canonical_col_must_o
);
    integer index;
    always @* begin
        canonical_rows_o = transpose_i ? physical_cols_i : physical_rows_i;
        canonical_cols_o = transpose_i ? physical_rows_i : physical_cols_i;
        canonical_pivot_rows_o = {(MAX_K*NORM_W){1'b0}};
        canonical_pivot_cols_o = {(MAX_K*NORM_W){1'b0}};
        for (index = 0; index < MAX_K; index = index + 1) begin
            if (transpose_i) begin
                canonical_pivot_rows_o[index*NORM_W +: COL_W] = physical_pivot_cols_i[index*COL_W +: COL_W];
                canonical_pivot_cols_o[index*NORM_W +: ROW_W] = physical_pivot_rows_i[index*ROW_W +: ROW_W];
            end else begin
                canonical_pivot_rows_o[index*NORM_W +: ROW_W] = physical_pivot_rows_i[index*ROW_W +: ROW_W];
                canonical_pivot_cols_o[index*NORM_W +: COL_W] = physical_pivot_cols_i[index*COL_W +: COL_W];
            end
        end
        canonical_row_counts_o = transpose_i ? physical_col_counts_i : physical_row_counts_i;
        canonical_col_counts_o = transpose_i ? physical_row_counts_i : physical_col_counts_i;
        canonical_row_must_o = transpose_i ? physical_col_must_i : physical_row_must_i;
        canonical_col_must_o = transpose_i ? physical_row_must_i : physical_col_must_i;
    end
endmodule
`default_nettype wire
