`default_nettype none

// Six fixed C(4,2) candidates are evaluated concurrently.  A pattern bit of
// zero selects a spare row; one selects a spare column, matching RECAM's
// fixed-index candidate convention.
module recam_2r2c_pattern_analyzer (
    input  wire [15:0] matrix_flat_i,
    input  wire        cam_overflow_i,
    output wire [5:0]  candidate_valid_o
);
    function automatic candidate_valid;
        input [15:0] matrix;
        input [3:0] pattern;
        integer row, col;
        begin
            candidate_valid = !cam_overflow_i;
            for (row = 0; row < 4; row = row + 1)
                for (col = 0; col < 4; col = col + 1)
                    if (matrix[row*4 + col] && pattern[row] && !pattern[col])
                        candidate_valid = 1'b0;
        end
    endfunction

    // Candidate ordering is the fixed index ordering used by the RECAM model.
    assign candidate_valid_o[0] = candidate_valid(matrix_flat_i, 4'b1100);
    assign candidate_valid_o[1] = candidate_valid(matrix_flat_i, 4'b1010);
    assign candidate_valid_o[2] = candidate_valid(matrix_flat_i, 4'b0110);
    assign candidate_valid_o[3] = candidate_valid(matrix_flat_i, 4'b1001);
    assign candidate_valid_o[4] = candidate_valid(matrix_flat_i, 4'b0101);
    assign candidate_valid_o[5] = candidate_valid(matrix_flat_i, 4'b0011);
endmodule

`default_nettype wire
