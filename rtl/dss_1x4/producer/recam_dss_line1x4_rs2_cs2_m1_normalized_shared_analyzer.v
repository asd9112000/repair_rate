`default_nettype none

// One combinational RECAM analyzer for the 1x4 RS2/CS2/m1 capacity attempts.
// It retains the C++ PatternID order for 2R2C, 3R2C, and 4R2C attempts.
module recam_dss_line1x4_rs2_cs2_m1_normalized_shared_analyzer #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 10
) (
    input wire [1:0] attempt_i,
    input wire [5:0] pivot_valid_i,
    input wire [6*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input wire [6*COL_ADDR_W-1:0] pivot_cols_flat_i,
    /* verilator lint_off UNUSED */
    input wire [5:0] row_gt1_i, input wire [5:0] row_gt2_i,
    input wire [5:0] row_gt3_i, input wire [5:0] row_gt4_i,
    input wire [5:0] col_gt1_i, input wire [5:0] col_gt2_i,
    input wire [5:0] col_gt3_i, input wire [5:0] col_gt4_i,
    /* verilator lint_on UNUSED */
    input wire [HYBRID_ENTRIES-1:0] hybrid_valid_i,
    input wire [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input wire [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,
    input wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,
    input wire conventional_overflow_i,
    output reg [14:0] candidate_valid_o,
    output reg [15*3-1:0] candidate_used_rows_flat_o,
    output reg [15*2-1:0] candidate_used_cols_flat_o,
    output reg dictionary_overflow_o
);
    reg [2:0] rows, cols, matrix_size, pivot_count, row_count, col_count;
    reg config_valid, differing_is_row, differing_match;
    reg [2:0] pointer, differing_index;
    reg [DIFF_ADDR_W-1:0] differing_address;
    reg [5:0] pattern;
    reg candidate_valid;
    reg [2:0] candidate_rows;
    reg [1:0] candidate_cols;
    reg matrix [0:5][0:5];
    reg [DIFF_ADDR_W-1:0] row_dictionary [0:5];
    reg [DIFF_ADDR_W-1:0] col_dictionary [0:5];
    integer pivot, row, col, hybrid, index, candidate;

    function automatic [5:0] candidate_pattern;
        input [1:0] capacity_attempt;
        input integer candidate_number;
        begin
            candidate_pattern = 6'b0;
            case (capacity_attempt)
                2'd0: case (candidate_number)
                    0: candidate_pattern = 6'b001100; 1: candidate_pattern = 6'b001010;
                    2: candidate_pattern = 6'b000110; 3: candidate_pattern = 6'b001001;
                    4: candidate_pattern = 6'b000101; 5: candidate_pattern = 6'b000011;
                endcase
                2'd1: case (candidate_number)
                    0: candidate_pattern = 6'b011000; 1: candidate_pattern = 6'b010100;
                    2: candidate_pattern = 6'b001100; 3: candidate_pattern = 6'b010010;
                    4: candidate_pattern = 6'b001010; 5: candidate_pattern = 6'b000110;
                    6: candidate_pattern = 6'b010001; 7: candidate_pattern = 6'b001001;
                    8: candidate_pattern = 6'b000101; 9: candidate_pattern = 6'b000011;
                endcase
                2'd2: case (candidate_number)
                    0: candidate_pattern = 6'b111100; 1: candidate_pattern = 6'b111010;
                    2: candidate_pattern = 6'b110110; 3: candidate_pattern = 6'b101110;
                    4: candidate_pattern = 6'b011110; 5: candidate_pattern = 6'b111001;
                    6: candidate_pattern = 6'b110101; 7: candidate_pattern = 6'b101101;
                    8: candidate_pattern = 6'b011101; 9: candidate_pattern = 6'b110011;
                    10: candidate_pattern = 6'b101011; 11: candidate_pattern = 6'b011011;
                    12: candidate_pattern = 6'b100111; 13: candidate_pattern = 6'b010111;
                    14: candidate_pattern = 6'b001111;
                    default: candidate_pattern = 6'b0;
                endcase
                default: candidate_pattern = 6'b0;
            endcase
        end
    endfunction

    function automatic integer candidate_count;
        input [1:0] capacity_attempt;
        begin
            case (capacity_attempt)
                2'd0: candidate_count = 6;
                2'd1: candidate_count = 10;
                2'd2: candidate_count = 15;
                default: candidate_count = 0;
            endcase
        end
    endfunction

    always @* begin
        case (attempt_i)
            2'd0: begin rows = 3'd2; cols = 3'd2; config_valid = 1'b1; end
            2'd1: begin rows = 3'd3; cols = 3'd2; config_valid = 1'b1; end
            2'd2: begin rows = 3'd4; cols = 3'd2; config_valid = 1'b1; end
            default: begin rows = 3'd0; cols = 3'd0; config_valid = 1'b0; end
        endcase
        matrix_size = rows + cols;
        pivot_count = 3'd0;
        row_count = 3'd0;
        col_count = 3'd0;
        candidate_valid_o = 15'd0;
        candidate_used_rows_flat_o = 45'd0;
        candidate_used_cols_flat_o = 30'd0;
        dictionary_overflow_o = 1'b0;

        for (pivot = 0; pivot < 6; pivot = pivot + 1) begin
            row_dictionary[pivot] = pivot_rows_flat_i[pivot*ROW_ADDR_W +: ROW_ADDR_W];
            col_dictionary[pivot] = {{(DIFF_ADDR_W-COL_ADDR_W){1'b0}},
                                     pivot_cols_flat_i[pivot*COL_ADDR_W +: COL_ADDR_W]};
            if ((pivot < matrix_size) && pivot_valid_i[pivot])
                pivot_count = pivot_count + 1'b1;
            for (col = 0; col < 6; col = col + 1)
                matrix[pivot][col] = 1'b0;
        end
        row_count = pivot_count;
        col_count = pivot_count;

        for (row = 0; row < 6; row = row + 1)
            for (col = 0; col < 6; col = col + 1)
                if ((row < matrix_size) && (col < matrix_size))
                    matrix[row][col] = ((row == col) && (row < pivot_count)) ||
                        ((row < pivot_count) && (cols == 3'd2 ? row_gt2_i[row] : 1'b0)) ||
                        ((col < pivot_count) &&
                         (rows == 3'd2 ? col_gt2_i[col] :
                          rows == 3'd3 ? col_gt3_i[col] : col_gt4_i[col]));

        for (hybrid = 0; hybrid < HYBRID_ENTRIES; hybrid = hybrid + 1) begin
            if (hybrid_valid_i[hybrid]) begin
                pointer = hybrid_pointer_flat_i[hybrid*3 +: 3];
                differing_is_row = hybrid_descriptor_i[hybrid];
                if (hybrid_descriptor_i[hybrid])
                    differing_address = hybrid_differing_flat_i[hybrid*DIFF_ADDR_W +: ROW_ADDR_W];
                else
                    differing_address = {{(DIFF_ADDR_W-COL_ADDR_W){1'b0}},
                        hybrid_differing_flat_i[hybrid*DIFF_ADDR_W +: COL_ADDR_W]};
                differing_match = 1'b0;
                differing_index = 3'd0;
                if (pointer < pivot_count) begin
                    if (differing_is_row) begin
                        for (index = 0; index < 6; index = index + 1)
                            if (!differing_match && index < row_count && differing_address == row_dictionary[index]) begin
                                differing_match = 1'b1; differing_index = index[2:0];
                            end
                        if (!differing_match && row_count < matrix_size) begin
                            differing_index = row_count; row_dictionary[row_count] = differing_address;
                            row_count = row_count + 1'b1; differing_match = 1'b1;
                        end
                        if (differing_match) matrix[differing_index][pointer] = 1'b1;
                        else for (index = 0; index < 6; index = index + 1)
                            if (index < row_count) matrix[index][pointer] = 1'b1;
                    end else begin
                        for (index = 0; index < 6; index = index + 1)
                            if (!differing_match && index < col_count && differing_address == col_dictionary[index]) begin
                                differing_match = 1'b1; differing_index = index[2:0];
                            end
                        if (!differing_match && col_count < matrix_size) begin
                            differing_index = col_count; col_dictionary[col_count] = differing_address;
                            col_count = col_count + 1'b1; differing_match = 1'b1;
                        end
                        if (differing_match) matrix[pointer][differing_index] = 1'b1;
                        else for (index = 0; index < 6; index = index + 1)
                            if (index < col_count) matrix[pointer][index] = 1'b1;
                    end
                end else dictionary_overflow_o = 1'b1;
            end
        end

        for (candidate = 0; candidate < 15; candidate = candidate + 1) begin
            pattern = candidate_pattern(attempt_i, candidate);
            candidate_valid = config_valid && candidate < candidate_count(attempt_i) &&
                              !conventional_overflow_i && !dictionary_overflow_o;
            candidate_rows = 3'd0;
            candidate_cols = 2'd0;
            for (row = 0; row < 6; row = row + 1) begin
                if (row < matrix_size && pattern[row]) begin
                    if (row < row_count) candidate_rows = candidate_rows + 1'b1;
                end else if (row < matrix_size && row < col_count) begin
                    candidate_cols = candidate_cols + 1'b1;
                end
            end
            for (row = 0; row < 6; row = row + 1)
                for (col = 0; col < 6; col = col + 1)
                    if (row < matrix_size && col < matrix_size && matrix[row][col] && pattern[row] && !pattern[col])
                        candidate_valid = 1'b0;
            candidate_valid_o[candidate] = candidate_valid;
            candidate_used_rows_flat_o[candidate*3 +: 3] = candidate_rows;
            candidate_used_cols_flat_o[candidate*2 +: 2] = candidate_cols;
        end
    end
endmodule

`default_nettype wire
