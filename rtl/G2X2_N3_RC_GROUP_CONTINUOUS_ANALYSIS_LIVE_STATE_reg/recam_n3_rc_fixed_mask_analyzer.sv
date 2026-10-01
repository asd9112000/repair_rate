`default_nettype none

// Isolated production RS=CS=3, SHARE_M=1 shared analyzer.  One target-table descriptor
// is evaluated at a time; candidate lanes are a deterministic 35-wide superset.
module recam_n3_rc_fixed_mask_analyzer (
    input logic [2:0] row_count_i, col_count_i,
    input logic transpose_i,
    input logic [6:0] pivot_valid_i,
    input logic [62:0] pivot_rows_flat_i,
    input logic [90:0] pivot_cols_flat_i,
    input logic [6:0] row_gt1_i, row_gt2_i, row_gt3_i, row_gt4_i,
    input logic [6:0] col_gt1_i, col_gt2_i, col_gt3_i, col_gt4_i,
    input logic [16:0] hybrid_valid_i,
    input logic [50:0] hybrid_pointer_flat_i,
    input logic [16:0] hybrid_descriptor_i,
    input logic [220:0] hybrid_differing_flat_i,
    input logic conventional_overflow_i,
    output logic [34:0] candidate_valid_o,
    output logic [5:0] pattern_id_o,
    output logic solution_valid_o, repairable_o, dictionary_overflow_o
);
    logic [2:0] rows, cols, k, pivots, row_count, col_count;
    logic matrix [0:6][0:6];
    logic [12:0] row_dict [0:6];
    logic [12:0] col_dict [0:6];
    logic [6:0] pattern;
    logic [2:0] pointer, differing_index;
    logic [12:0] differing_address;
    logic differing_is_row, differing_match, candidate_is_valid;
    integer pivot, row, col, hybrid, index, candidate;

    function automatic integer candidate_count(input logic [2:0] r, input logic [2:0] c);
        begin
            case ({r,c})
                {3'd3,3'd3}: candidate_count = 20;
                {3'd3,3'd2}: candidate_count = 10;
                {3'd4,3'd3}: candidate_count = 35;
                {3'd4,3'd2}: candidate_count = 15;
                default: candidate_count = 0;
            endcase
        end
    endfunction

    function automatic logic [6:0] fixed_mask(
        input logic [2:0] r, input logic [2:0] c, input logic [5:0] pattern_id
    );
        begin
            fixed_mask = 7'b0;
            unique case ({r, c, pattern_id})
                {3'd3,3'd3,6'd1}: fixed_mask = 7'b0111000;
                {3'd3,3'd3,6'd2}: fixed_mask = 7'b0110100;
                {3'd3,3'd3,6'd3}: fixed_mask = 7'b0110010;
                {3'd3,3'd3,6'd4}: fixed_mask = 7'b0110001;
                {3'd3,3'd3,6'd5}: fixed_mask = 7'b0101100;
                {3'd3,3'd3,6'd6}: fixed_mask = 7'b0101010;
                {3'd3,3'd3,6'd7}: fixed_mask = 7'b0101001;
                {3'd3,3'd3,6'd8}: fixed_mask = 7'b0100110;
                {3'd3,3'd3,6'd9}: fixed_mask = 7'b0100101;
                {3'd3,3'd3,6'd10}: fixed_mask = 7'b0100011;
                {3'd3,3'd3,6'd11}: fixed_mask = 7'b0011100;
                {3'd3,3'd3,6'd12}: fixed_mask = 7'b0011010;
                {3'd3,3'd3,6'd13}: fixed_mask = 7'b0011001;
                {3'd3,3'd3,6'd14}: fixed_mask = 7'b0010110;
                {3'd3,3'd3,6'd15}: fixed_mask = 7'b0010101;
                {3'd3,3'd3,6'd16}: fixed_mask = 7'b0010011;
                {3'd3,3'd3,6'd17}: fixed_mask = 7'b0001110;
                {3'd3,3'd3,6'd18}: fixed_mask = 7'b0001101;
                {3'd3,3'd3,6'd19}: fixed_mask = 7'b0001011;
                {3'd3,3'd3,6'd20}: fixed_mask = 7'b0000111;
                {3'd3,3'd2,6'd1}: fixed_mask = 7'b0011100;
                {3'd3,3'd2,6'd2}: fixed_mask = 7'b0011010;
                {3'd3,3'd2,6'd3}: fixed_mask = 7'b0011001;
                {3'd3,3'd2,6'd4}: fixed_mask = 7'b0010110;
                {3'd3,3'd2,6'd5}: fixed_mask = 7'b0010101;
                {3'd3,3'd2,6'd6}: fixed_mask = 7'b0010011;
                {3'd3,3'd2,6'd7}: fixed_mask = 7'b0001110;
                {3'd3,3'd2,6'd8}: fixed_mask = 7'b0001101;
                {3'd3,3'd2,6'd9}: fixed_mask = 7'b0001011;
                {3'd3,3'd2,6'd10}: fixed_mask = 7'b0000111;
                {3'd4,3'd3,6'd1}: fixed_mask = 7'b1111000;
                {3'd4,3'd3,6'd2}: fixed_mask = 7'b1110100;
                {3'd4,3'd3,6'd3}: fixed_mask = 7'b1110010;
                {3'd4,3'd3,6'd4}: fixed_mask = 7'b1110001;
                {3'd4,3'd3,6'd5}: fixed_mask = 7'b1101100;
                {3'd4,3'd3,6'd6}: fixed_mask = 7'b1101010;
                {3'd4,3'd3,6'd7}: fixed_mask = 7'b1101001;
                {3'd4,3'd3,6'd8}: fixed_mask = 7'b1100110;
                {3'd4,3'd3,6'd9}: fixed_mask = 7'b1100101;
                {3'd4,3'd3,6'd10}: fixed_mask = 7'b1100011;
                {3'd4,3'd3,6'd11}: fixed_mask = 7'b1011100;
                {3'd4,3'd3,6'd12}: fixed_mask = 7'b1011010;
                {3'd4,3'd3,6'd13}: fixed_mask = 7'b1011001;
                {3'd4,3'd3,6'd14}: fixed_mask = 7'b1010110;
                {3'd4,3'd3,6'd15}: fixed_mask = 7'b1010101;
                {3'd4,3'd3,6'd16}: fixed_mask = 7'b1010011;
                {3'd4,3'd3,6'd17}: fixed_mask = 7'b1001110;
                {3'd4,3'd3,6'd18}: fixed_mask = 7'b1001101;
                {3'd4,3'd3,6'd19}: fixed_mask = 7'b1001011;
                {3'd4,3'd3,6'd20}: fixed_mask = 7'b1000111;
                {3'd4,3'd3,6'd21}: fixed_mask = 7'b0111100;
                {3'd4,3'd3,6'd22}: fixed_mask = 7'b0111010;
                {3'd4,3'd3,6'd23}: fixed_mask = 7'b0111001;
                {3'd4,3'd3,6'd24}: fixed_mask = 7'b0110110;
                {3'd4,3'd3,6'd25}: fixed_mask = 7'b0110101;
                {3'd4,3'd3,6'd26}: fixed_mask = 7'b0110011;
                {3'd4,3'd3,6'd27}: fixed_mask = 7'b0101110;
                {3'd4,3'd3,6'd28}: fixed_mask = 7'b0101101;
                {3'd4,3'd3,6'd29}: fixed_mask = 7'b0101011;
                {3'd4,3'd3,6'd30}: fixed_mask = 7'b0100111;
                {3'd4,3'd3,6'd31}: fixed_mask = 7'b0011110;
                {3'd4,3'd3,6'd32}: fixed_mask = 7'b0011101;
                {3'd4,3'd3,6'd33}: fixed_mask = 7'b0011011;
                {3'd4,3'd3,6'd34}: fixed_mask = 7'b0010111;
                {3'd4,3'd3,6'd35}: fixed_mask = 7'b0001111;
                {3'd4,3'd2,6'd1}: fixed_mask = 7'b0111100;
                {3'd4,3'd2,6'd2}: fixed_mask = 7'b0111010;
                {3'd4,3'd2,6'd3}: fixed_mask = 7'b0111001;
                {3'd4,3'd2,6'd4}: fixed_mask = 7'b0110110;
                {3'd4,3'd2,6'd5}: fixed_mask = 7'b0110101;
                {3'd4,3'd2,6'd6}: fixed_mask = 7'b0110011;
                {3'd4,3'd2,6'd7}: fixed_mask = 7'b0101110;
                {3'd4,3'd2,6'd8}: fixed_mask = 7'b0101101;
                {3'd4,3'd2,6'd9}: fixed_mask = 7'b0101011;
                {3'd4,3'd2,6'd10}: fixed_mask = 7'b0100111;
                {3'd4,3'd2,6'd11}: fixed_mask = 7'b0011110;
                {3'd4,3'd2,6'd12}: fixed_mask = 7'b0011101;
                {3'd4,3'd2,6'd13}: fixed_mask = 7'b0011011;
                {3'd4,3'd2,6'd14}: fixed_mask = 7'b0010111;
                {3'd4,3'd2,6'd15}: fixed_mask = 7'b0001111;
                default: fixed_mask = 7'b0;
            endcase
        end
    endfunction

    function automatic logic threshold_bit(
        input logic [6:0] gt1, gt2, gt3, gt4,
        input logic [2:0] which, input integer at
    );
        begin
            case (which)
                3'd1: threshold_bit = gt1[at];
                3'd2: threshold_bit = gt2[at];
                3'd3: threshold_bit = gt3[at];
                3'd4: threshold_bit = gt4[at];
                default: threshold_bit = 1'b0;
            endcase
        end
    endfunction

    always_comb begin
        rows = transpose_i ? col_count_i : row_count_i;
        cols = transpose_i ? row_count_i : col_count_i;
        k = rows + cols;
        pivots = '0;
        candidate_valid_o = '0;
        pattern_id_o = '0;
        solution_valid_o = 1'b0;
        repairable_o = 1'b0;
        dictionary_overflow_o = 1'b0;

        for (pivot = 0; pivot < 7; pivot = pivot + 1) begin
            row_dict[pivot] = '0;
            col_dict[pivot] = '0;
            if ((pivot < k) && pivot_valid_i[pivot]) begin
                if (transpose_i) begin
                    row_dict[pivot] = pivot_cols_flat_i[pivot*13 +: 13];
                    col_dict[pivot] = {{4{1'b0}}, pivot_rows_flat_i[pivot*9 +: 9]};
                end else begin
                    row_dict[pivot] = {{4{1'b0}}, pivot_rows_flat_i[pivot*9 +: 9]};
                    col_dict[pivot] = pivot_cols_flat_i[pivot*13 +: 13];
                end
                pivots = pivots + 1'b1;
            end
            for (col = 0; col < 7; col = col + 1) matrix[pivot][col] = 1'b0;
        end
        row_count = pivots;
        col_count = pivots;
        for (row = 0; row < 7; row = row + 1)
            for (col = 0; col < 7; col = col + 1)
                if ((row < k) && (col < k)) begin
                    matrix[row][col] = ((row == col) && (row < pivots)) ||
                        ((row < pivots) &&
                         (transpose_i ? threshold_bit(col_gt1_i,col_gt2_i,col_gt3_i,col_gt4_i,cols,row)
                                      : threshold_bit(row_gt1_i,row_gt2_i,row_gt3_i,row_gt4_i,cols,row))) ||
                        ((col < pivots) &&
                         (transpose_i ? threshold_bit(row_gt1_i,row_gt2_i,row_gt3_i,row_gt4_i,rows,col)
                                      : threshold_bit(col_gt1_i,col_gt2_i,col_gt3_i,col_gt4_i,rows,col)));
                end

        for (hybrid = 0; hybrid < 17; hybrid = hybrid + 1) if (hybrid_valid_i[hybrid]) begin
            pointer = hybrid_pointer_flat_i[hybrid*3 +: 3];
            differing_is_row = hybrid_descriptor_i[hybrid] ^ transpose_i;
            differing_address = hybrid_differing_flat_i[hybrid*13 +: 13];
            differing_match = 1'b0;
            differing_index = '0;
            if (pointer < pivots) begin
                if (differing_is_row) begin
                    for (index = 0; index < 7; index = index + 1)
                        if (!differing_match && index < row_count && differing_address == row_dict[index]) begin
                            differing_match = 1'b1; differing_index = index[2:0];
                        end
                    if (!differing_match && row_count < k) begin
                        differing_index = row_count; row_dict[row_count] = differing_address;
                        row_count = row_count + 1'b1; differing_match = 1'b1;
                    end
                    if (differing_match) matrix[differing_index][pointer] = 1'b1;
                    else for (index = 0; index < 7; index = index + 1)
                        if (index < row_count) matrix[index][pointer] = 1'b1;
                end else begin
                    for (index = 0; index < 7; index = index + 1)
                        if (!differing_match && index < col_count && differing_address == col_dict[index]) begin
                            differing_match = 1'b1; differing_index = index[2:0];
                        end
                    if (!differing_match && col_count < k) begin
                        differing_index = col_count; col_dict[col_count] = differing_address;
                        col_count = col_count + 1'b1; differing_match = 1'b1;
                    end
                    if (differing_match) matrix[pointer][differing_index] = 1'b1;
                    else for (index = 0; index < 7; index = index + 1)
                        if (index < col_count) matrix[pointer][index] = 1'b1;
                end
            end else dictionary_overflow_o = 1'b1;
        end

        for (candidate = 0; candidate < 35; candidate = candidate + 1) begin
            pattern = fixed_mask(rows, cols, candidate[5:0] + 6'd1);
            candidate_is_valid = (candidate < candidate_count(rows, cols)) &&
                                 !conventional_overflow_i && !dictionary_overflow_o;
            for (row = 0; row < 7; row = row + 1)
                for (col = 0; col < 7; col = col + 1)
                    if ((row < k) && (col < k) && matrix[row][col] && pattern[row] && !pattern[col])
                        candidate_is_valid = 1'b0;
            candidate_valid_o[candidate] = candidate_is_valid;
            if (candidate_is_valid && !solution_valid_o) begin
                solution_valid_o = 1'b1;
                repairable_o = 1'b1;
                pattern_id_o = candidate[5:0] + 1'b1;
            end
        end
    end
endmodule
`default_nettype wire
