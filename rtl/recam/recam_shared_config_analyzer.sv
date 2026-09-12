`default_nettype none

// Phase 3B shared, fully combinational analyzer. One ConfigID is presented at
// a time. Synthesis unrolls the fixed loops into parallel candidate logic.
module recam_shared_config_analyzer #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 7
) (
    input  wire [2:0] config_id_i,
    input  wire [4:0] pivot_valid_i,
    input  wire [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input  wire [5*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input  wire [4:0] row_gt1_i,
    input  wire [4:0] row_gt2_i,
    input  wire [4:0] row_gt3_i,
    input  wire [4:0] col_gt1_i,
    input  wire [4:0] col_gt2_i,
    input  wire [4:0] col_gt3_i,
    input  wire [HYBRID_ENTRIES-1:0] hybrid_valid_i,
    input  wire [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input  wire [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,
    input  wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,
    input  wire conventional_overflow_i,
    output reg  [9:0] candidate_valid_o,
    output reg  [3:0] pattern_id_o,
    output reg        solution_valid_o,
    output reg        repairable_o,
    output reg        dictionary_overflow_o
);
    localparam [2:0] CFG_2R2C = 3'd0;
    localparam [2:0] CFG_2R1C = 3'd1;
    localparam [2:0] CFG_3R2C = 3'd2;
    localparam [2:0] CFG_3R1C = 3'd3;
    localparam [2:0] CFG_1R2C = 3'd4;
    localparam [2:0] CFG_2R3C = 3'd5;
    localparam [2:0] CFG_1R3C = 3'd6;

    reg [2:0] canonical_rows, canonical_cols, matrix_size;
    reg [2:0] pivot_count, row_count, col_count;
    reg [2:0] pointer, differing_index;
    reg [DIFF_ADDR_W-1:0] differing_address;
    reg transpose, config_valid, differing_match, differing_is_row;
    reg candidate_is_valid;
    reg [4:0] pattern;
    reg matrix [0:4][0:4];
    // Both dictionaries use the 9-bit differing-address representation.
    // ColumnWord values are explicitly zero-extended on every insertion.
    reg [DIFF_ADDR_W-1:0] row_dictionary [0:4];
    reg [DIFF_ADDR_W-1:0] col_dictionary [0:4];
    integer pivot, row, col, hybrid, index, candidate;

    // Pattern bit 0 selects a row; bit 1 selects a column. Candidate ordering
    // matches the Phase 3A ascending-mask order.
    function automatic [4:0] candidate_pattern;
        input [2:0] rows;
        input [2:0] cols;
        input integer candidate_number;
        begin
            candidate_pattern = 5'b00000;
            case ({rows, cols})
                {3'd2, 3'd1}: case (candidate_number)
                    0: candidate_pattern = 5'b00100;
                    1: candidate_pattern = 5'b00010;
                    2: candidate_pattern = 5'b00001;
                    default: candidate_pattern = 5'b00000;
                endcase
                {3'd2, 3'd2}: case (candidate_number)
                    0: candidate_pattern = 5'b01100;
                    1: candidate_pattern = 5'b01010;
                    2: candidate_pattern = 5'b00110;
                    3: candidate_pattern = 5'b01001;
                    4: candidate_pattern = 5'b00101;
                    5: candidate_pattern = 5'b00011;
                    default: candidate_pattern = 5'b00000;
                endcase
                {3'd3, 3'd1}: case (candidate_number)
                    0: candidate_pattern = 5'b01000;
                    1: candidate_pattern = 5'b00100;
                    2: candidate_pattern = 5'b00010;
                    3: candidate_pattern = 5'b00001;
                    default: candidate_pattern = 5'b00000;
                endcase
                {3'd3, 3'd2}: case (candidate_number)
                    0: candidate_pattern = 5'b11000;
                    1: candidate_pattern = 5'b10100;
                    2: candidate_pattern = 5'b01100;
                    3: candidate_pattern = 5'b10010;
                    4: candidate_pattern = 5'b01010;
                    5: candidate_pattern = 5'b00110;
                    6: candidate_pattern = 5'b10001;
                    7: candidate_pattern = 5'b01001;
                    8: candidate_pattern = 5'b00101;
                    9: candidate_pattern = 5'b00011;
                    default: candidate_pattern = 5'b00000;
                endcase
                default: candidate_pattern = 5'b00000;
            endcase
        end
    endfunction

    function automatic integer candidate_count;
        input [2:0] rows;
        input [2:0] cols;
        begin
            case ({rows, cols})
                {3'd2, 3'd1}: candidate_count = 3;
                {3'd2, 3'd2}: candidate_count = 6;
                {3'd3, 3'd1}: candidate_count = 4;
                {3'd3, 3'd2}: candidate_count = 10;
                default: candidate_count = 0;
            endcase
        end
    endfunction

    always @* begin
        canonical_rows = 3'd0;
        canonical_cols = 3'd0;
        transpose = 1'b0;
        config_valid = 1'b1;
        case (config_id_i)
            CFG_2R2C: begin canonical_rows = 3'd2; canonical_cols = 3'd2; end
            CFG_2R1C: begin canonical_rows = 3'd2; canonical_cols = 3'd1; end
            CFG_3R2C: begin canonical_rows = 3'd3; canonical_cols = 3'd2; end
            CFG_3R1C: begin canonical_rows = 3'd3; canonical_cols = 3'd1; end
            CFG_1R2C: begin canonical_rows = 3'd2; canonical_cols = 3'd1; transpose = 1'b1; end
            CFG_2R3C: begin canonical_rows = 3'd3; canonical_cols = 3'd2; transpose = 1'b1; end
            CFG_1R3C: begin canonical_rows = 3'd3; canonical_cols = 3'd1; transpose = 1'b1; end
            default: config_valid = 1'b0;
        endcase

        matrix_size = canonical_rows + canonical_cols;
        pivot_count = 3'd0;
        candidate_valid_o = 10'b0;
        pattern_id_o = 4'd0;
        solution_valid_o = 1'b0;
        repairable_o = 1'b0;
        dictionary_overflow_o = 1'b0;

        // Upstream guarantees a compact valid prefix. Verification checks the
        // contract, so this datapath does not synthesize a redundant checker.
        for (pivot = 0; pivot < 5; pivot = pivot + 1) begin
            row_dictionary[pivot] = {DIFF_ADDR_W{1'b0}};
            col_dictionary[pivot] = {DIFF_ADDR_W{1'b0}};
            if ((pivot < matrix_size) && pivot_valid_i[pivot]) begin
                if (transpose) begin
                    row_dictionary[pivot] = {{(DIFF_ADDR_W-COL_ADDR_W){1'b0}},
                        pivot_cols_flat_i[pivot*COL_ADDR_W +: COL_ADDR_W]};
                    col_dictionary[pivot] = pivot_rows_flat_i[pivot*ROW_ADDR_W +: ROW_ADDR_W];
                end else begin
                    row_dictionary[pivot] = pivot_rows_flat_i[pivot*ROW_ADDR_W +: ROW_ADDR_W];
                    col_dictionary[pivot] = {{(DIFF_ADDR_W-COL_ADDR_W){1'b0}},
                        pivot_cols_flat_i[pivot*COL_ADDR_W +: COL_ADDR_W]};
                end
                pivot_count = pivot_count + 3'd1;
            end
            for (col = 0; col < 5; col = col + 1)
                matrix[pivot][col] = 1'b0;
        end

        row_count = pivot_count;
        col_count = pivot_count;
        for (row = 0; row < 5; row = row + 1)
            for (col = 0; col < 5; col = col + 1)
                if ((row < matrix_size) && (col < matrix_size))
                    matrix[row][col] =
                        ((row == col) && (row < pivot_count)) ||
                        ((row < pivot_count) &&
                         ((canonical_cols == 3'd1) ?
                            (transpose ? col_gt1_i[row] : row_gt1_i[row]) :
                          (canonical_cols == 3'd2) ?
                            (transpose ? col_gt2_i[row] : row_gt2_i[row]) :
                            (transpose ? col_gt3_i[row] : row_gt3_i[row]))) ||
                        ((col < pivot_count) &&
                         ((canonical_rows == 3'd1) ?
                            (transpose ? row_gt1_i[col] : col_gt1_i[col]) :
                          (canonical_rows == 3'd2) ?
                            (transpose ? row_gt2_i[col] : col_gt2_i[col]) :
                            (transpose ? row_gt3_i[col] : col_gt3_i[col])));

        // Collector convention: descriptor 0 means same pivot row and a
        // differing column; descriptor 1 means same pivot column and a
        // differing row. Transposition swaps the differing dimension.
        for (hybrid = 0; hybrid < HYBRID_ENTRIES; hybrid = hybrid + 1) begin
            if (hybrid_valid_i[hybrid]) begin
                pointer = hybrid_pointer_flat_i[hybrid*3 +: 3];
                differing_is_row = hybrid_descriptor_i[hybrid] ^ transpose;
                if (hybrid_descriptor_i[hybrid])
                    differing_address = hybrid_differing_flat_i[hybrid*DIFF_ADDR_W +: ROW_ADDR_W];
                else
                    differing_address = {{(DIFF_ADDR_W-COL_ADDR_W){1'b0}},
                        hybrid_differing_flat_i[hybrid*DIFF_ADDR_W +: COL_ADDR_W]};
                differing_match = 1'b0;
                differing_index = 3'd0;
                if (pointer < pivot_count) begin
                    if (differing_is_row) begin
                        for (index = 0; index < 5; index = index + 1)
                            if (!differing_match && (index < row_count) &&
                                (differing_address == row_dictionary[index])) begin
                                differing_match = 1'b1;
                                differing_index = index[2:0];
                            end
                        if (!differing_match && (row_count < matrix_size)) begin
                            differing_index = row_count;
                            row_dictionary[row_count] = differing_address;
                            row_count = row_count + 3'd1;
                            differing_match = 1'b1;
                        end
                        if (differing_match)
                            matrix[differing_index][pointer] = 1'b1;
                        else
                            // A known pivot column with an unseen row and a
                            // full row dictionary is a RECAM line constraint,
                            // not a global analyzer failure.
                            for (index = 0; index < 5; index = index + 1)
                                if (index < row_count)
                                    matrix[index][pointer] = 1'b1;
                    end else begin
                        for (index = 0; index < 5; index = index + 1)
                            if (!differing_match && (index < col_count) &&
                                (differing_address == col_dictionary[index])) begin
                                differing_match = 1'b1;
                                differing_index = index[2:0];
                            end
                        if (!differing_match && (col_count < matrix_size)) begin
                            differing_index = col_count;
                            col_dictionary[col_count] = differing_address;
                            col_count = col_count + 3'd1;
                            differing_match = 1'b1;
                        end
                        if (differing_match)
                            matrix[pointer][differing_index] = 1'b1;
                        else
                            // A known pivot row with an unseen column and a
                            // full column dictionary is a RECAM line
                            // constraint, not a global analyzer failure.
                            for (index = 0; index < 5; index = index + 1)
                                if (index < col_count)
                                    matrix[pointer][index] = 1'b1;
                    end
                end else begin
                    dictionary_overflow_o = 1'b1;
                end
            end
        end

        // All ten candidate cones remain present and combinational; unused
        // candidate slots for a smaller canonical class stay zero.
        for (candidate = 0; candidate < 10; candidate = candidate + 1) begin
            pattern = candidate_pattern(canonical_rows, canonical_cols, candidate);
            candidate_is_valid = config_valid &&
                                 (candidate < candidate_count(canonical_rows, canonical_cols)) &&
                                 !conventional_overflow_i && !dictionary_overflow_o;
            for (row = 0; row < 5; row = row + 1)
                for (col = 0; col < 5; col = col + 1)
                    if ((row < matrix_size) && (col < matrix_size) &&
                        matrix[row][col] && pattern[row] && !pattern[col])
                        candidate_is_valid = 1'b0;
            candidate_valid_o[candidate] = candidate_is_valid;
            if (candidate_is_valid && !solution_valid_o) begin
                solution_valid_o = 1'b1;
                repairable_o = 1'b1;
                pattern_id_o = candidate[3:0] + 4'd1;
            end
        end
    end
endmodule

`default_nettype wire
