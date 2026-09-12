`default_nettype none

// Phase 3A fixed-geometry RECAM matrix builder.  The inputs are the logical
// Address/Hybrid CAM state after fault collection; this module owns no storage.
module recam_2r2c_matrix_builder #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer HYBRID_ENTRY_NUM = 7
) (
    input  wire [3:0] pivot_valid_i,
    input  wire [4*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input  wire [4*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input  wire [3:0] row_must_i,
    input  wire [3:0] col_must_i,
    input  wire [HYBRID_ENTRY_NUM-1:0] hybrid_valid_i,
    input  wire [HYBRID_ENTRY_NUM*ROW_ADDR_W-1:0] hybrid_rows_flat_i,
    input  wire [HYBRID_ENTRY_NUM*COL_ADDR_W-1:0] hybrid_cols_flat_i,
    output reg  [15:0] matrix_flat_o
);
    reg matrix [0:3][0:3];
    reg [ROW_ADDR_W-1:0] row_dict [0:3];
    reg [COL_ADDR_W-1:0] col_dict [0:3];
    reg [2:0] pivot_count, row_count, col_count;
    reg [1:0] row_index, col_index;
    reg row_match, col_match, extendable;
    integer entry, row, col, index;

    always @* begin
        pivot_count = 0;
        for (index = 0; index < 4; index = index + 1) begin
            row_dict[index] = '0;
            col_dict[index] = '0;
            if (pivot_valid_i[index]) begin
                row_dict[index] = pivot_rows_flat_i[index*ROW_ADDR_W +: ROW_ADDR_W];
                col_dict[index] = pivot_cols_flat_i[index*COL_ADDR_W +: COL_ADDR_W];
                pivot_count = pivot_count + 1'b1;
            end
            for (col = 0; col < 4; col = col + 1)
                matrix[index][col] = 1'b0;
        end

        row_count = pivot_count;
        col_count = pivot_count;
        for (row = 0; row < 4; row = row + 1)
            for (col = 0; col < 4; col = col + 1)
                matrix[row][col] = (row == col && row < pivot_count) ||
                                   (row < pivot_count && row_must_i[row]) ||
                                   (col < pivot_count && col_must_i[col]);

        extendable = pivot_count < 4;
        for (entry = 0; entry < HYBRID_ENTRY_NUM; entry = entry + 1) begin
            if (hybrid_valid_i[entry]) begin
                row_match = 1'b0;
                col_match = 1'b0;
                row_index = '0;
                col_index = '0;
                for (index = 0; index < 4; index = index + 1) begin
                    if (!row_match && index < row_count &&
                        hybrid_rows_flat_i[entry*ROW_ADDR_W +: ROW_ADDR_W] == row_dict[index]) begin
                        row_match = 1'b1;
                        row_index = index[1:0];
                    end
                    if (!col_match && index < col_count &&
                        hybrid_cols_flat_i[entry*COL_ADDR_W +: COL_ADDR_W] == col_dict[index]) begin
                        col_match = 1'b1;
                        col_index = index[1:0];
                    end
                end

                if (!extendable) begin
                    if (row_match && col_match)
                        matrix[row_index][col_index] = 1'b1;
                    else if (row_match)
                        for (index = 0; index < 4; index = index + 1)
                            matrix[row_index][index] = 1'b1;
                    else if (col_match)
                        for (index = 0; index < 4; index = index + 1)
                            matrix[index][col_index] = 1'b1;
                end else if (row_match && col_match) begin
                    matrix[row_index][col_index] = 1'b1;
                end else if (row_match && col_count < 4) begin
                    matrix[row_index][col_count[1:0]] = 1'b1;
                    col_dict[col_count[1:0]] = hybrid_cols_flat_i[entry*COL_ADDR_W +: COL_ADDR_W];
                    col_count = col_count + 1'b1;
                end else if (col_match && row_count < 4) begin
                    matrix[row_count[1:0]][col_index] = 1'b1;
                    row_dict[row_count[1:0]] = hybrid_rows_flat_i[entry*ROW_ADDR_W +: ROW_ADDR_W];
                    row_count = row_count + 1'b1;
                end else if (row_match) begin
                    for (index = 0; index < 4; index = index + 1)
                        matrix[row_index][index] = 1'b1;
                end else if (col_match) begin
                    for (index = 0; index < 4; index = index + 1)
                        matrix[index][col_index] = 1'b1;
                end

                if (row_count >= 4 && col_count >= 4)
                    extendable = 1'b0;
            end
        end

        matrix_flat_o = '0;
        for (row = 0; row < 4; row = row + 1)
            for (col = 0; col < 4; col = col + 1)
                matrix_flat_o[row*4 + col] = matrix[row][col];
    end
endmodule

`default_nettype wire
