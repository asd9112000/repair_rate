`default_nettype none

module candidate_analyzer #(
    parameter integer MAX_FAULTS = 12,
    parameter integer ROW_ADDR_WIDTH = 10,
    parameter integer COL_ADDR_WIDTH = 10,
    parameter integer MAX_RS = 3,
    parameter integer MAX_CS = 3,
    parameter integer BUFFER_ENTRY_NUM = 4,
    parameter integer MAX_K = MAX_RS + MAX_CS,
    parameter integer MASK_SPACE = (1 << MAX_K),
    parameter integer SPARE_WIDTH = (MAX_K <= 1) ? 1 : $clog2(MAX_K + 1),
    parameter integer CANDIDATE_ID_WIDTH = (MASK_SPACE <= 1) ? 1 : $clog2(MASK_SPACE),
    parameter integer FAULT_COUNT_WIDTH = (MAX_FAULTS <= 1) ? 1 : $clog2(MAX_FAULTS + 1)
) (
    input  wire [MAX_FAULTS-1:0]                    fault_valids_i,
    input  wire [MAX_FAULTS*ROW_ADDR_WIDTH-1:0]     fault_rows_flat_i,
    input  wire [MAX_FAULTS*COL_ADDR_WIDTH-1:0]     fault_cols_flat_i,
    input  wire [SPARE_WIDTH-1:0]                   active_rows_i,
    input  wire [SPARE_WIDTH-1:0]                   active_cols_i,
    output reg                                      repairable_o,
    output reg                                      storage_overflow_o,
    output reg  [MASK_SPACE-1:0]                    valid_candidates_o,
    output reg  [MASK_SPACE*SPARE_WIDTH-1:0]        used_rows_flat_o,
    output reg  [MASK_SPACE*SPARE_WIDTH-1:0]        used_cols_flat_o,
    output reg  [MASK_SPACE*MAX_K-1:0]              candidate_masks_flat_o,
    output reg  [CANDIDATE_ID_WIDTH-1:0]            first_candidate_id_o,
    output reg  [FAULT_COUNT_WIDTH-1:0]             pivot_count_o,
    output reg  [FAULT_COUNT_WIDTH-1:0]             hybrid_count_o,
    output reg  [FAULT_COUNT_WIDTH-1:0]             buffer_count_o,
    output reg  [31:0]                              attempt_cycles_o
);
    reg [ROW_ADDR_WIDTH-1:0] pivot_rows [0:MAX_K-1];
    reg [COL_ADDR_WIDTH-1:0] pivot_cols [0:MAX_K-1];
    integer pivot_row_count [0:MAX_K-1];
    integer pivot_col_count [0:MAX_K-1];
    reg pivot_row_must [0:MAX_K-1];
    reg pivot_col_must [0:MAX_K-1];

    reg [ROW_ADDR_WIDTH-1:0] nonpivot_rows [0:MAX_FAULTS-1];
    reg [COL_ADDR_WIDTH-1:0] nonpivot_cols [0:MAX_FAULTS-1];
    reg [ROW_ADDR_WIDTH-1:0] hybrid_rows [0:MAX_FAULTS-1];
    reg [COL_ADDR_WIDTH-1:0] hybrid_cols [0:MAX_FAULTS-1];
    integer hybrid_pointer [0:MAX_FAULTS-1];
    reg hybrid_row_is_diff [0:MAX_FAULTS-1];
    reg hybrid_valid [0:MAX_FAULTS-1];

    reg matrix [0:MAX_K-1][0:MAX_K-1];
    reg [ROW_ADDR_WIDTH-1:0] row_dictionary [0:MAX_K-1];
    reg [COL_ADDR_WIDTH-1:0] col_dictionary [0:MAX_K-1];
    reg row_dictionary_valid [0:MAX_K-1];
    reg col_dictionary_valid [0:MAX_K-1];
    reg [MAX_K-1:0] solution_bits;

    integer active_rows;
    integer active_cols;
    integer active_k;
    integer fault_count;
    integer pivot_count;
    integer nonpivot_count;
    integer buffer_count;
    integer hybrid_count;
    integer hybrid_live_count;
    integer hybrid_capacity;
    integer row_dictionary_count;
    integer col_dictionary_count;
    integer candidate_id;
    integer bit_count;
    integer used_rows;
    integer used_cols;
    integer mask_value;
    integer first_valid_seen;
    integer f;
    integer p;
    integer h;
    integer r;
    integer c;
    integer i;
    reg is_pivot;
    reg relation_found;
    reg row_match;
    reg col_match;
    reg candidate_valid;
    reg matrix_extendable;
    reg overflow;

    always @* begin
        valid_candidates_o = {MASK_SPACE{1'b0}};
        used_rows_flat_o = {(MASK_SPACE*SPARE_WIDTH){1'b0}};
        used_cols_flat_o = {(MASK_SPACE*SPARE_WIDTH){1'b0}};
        candidate_masks_flat_o = {(MASK_SPACE*MAX_K){1'b0}};
        first_candidate_id_o = {CANDIDATE_ID_WIDTH{1'b0}};
        pivot_count_o = {FAULT_COUNT_WIDTH{1'b0}};
        hybrid_count_o = {FAULT_COUNT_WIDTH{1'b0}};
        buffer_count_o = {FAULT_COUNT_WIDTH{1'b0}};
        attempt_cycles_o = 32'd0;
        repairable_o = 1'b0;
        storage_overflow_o = 1'b0;

        /* verilator lint_off WIDTH */
        active_rows = active_rows_i;
        active_cols = active_cols_i;
        /* verilator lint_on WIDTH */
        active_k = active_rows + active_cols;
        hybrid_capacity = (active_rows == 0 || active_cols == 0)
            ? 0
            : active_rows * (active_cols - 1) + active_cols * (active_rows - 1);
        fault_count = 0;
        pivot_count = 0;
        nonpivot_count = 0;
        buffer_count = 0;
        hybrid_count = 0;
        hybrid_live_count = 0;
        overflow = 1'b0;

        for (p = 0; p < MAX_K; p = p + 1) begin
            pivot_rows[p] = {ROW_ADDR_WIDTH{1'b0}};
            pivot_cols[p] = {COL_ADDR_WIDTH{1'b0}};
            pivot_row_count[p] = 1;
            pivot_col_count[p] = 1;
            pivot_row_must[p] = 1'b0;
            pivot_col_must[p] = 1'b0;
            row_dictionary[p] = {ROW_ADDR_WIDTH{1'b0}};
            col_dictionary[p] = {COL_ADDR_WIDTH{1'b0}};
            row_dictionary_valid[p] = 1'b0;
            col_dictionary_valid[p] = 1'b0;
            for (c = 0; c < MAX_K; c = c + 1)
                matrix[p][c] = 1'b0;
        end
        for (f = 0; f < MAX_FAULTS; f = f + 1) begin
            nonpivot_rows[f] = {ROW_ADDR_WIDTH{1'b0}};
            nonpivot_cols[f] = {COL_ADDR_WIDTH{1'b0}};
            hybrid_rows[f] = {ROW_ADDR_WIDTH{1'b0}};
            hybrid_cols[f] = {COL_ADDR_WIDTH{1'b0}};
            hybrid_pointer[f] = 0;
            hybrid_row_is_diff[f] = 1'b0;
            hybrid_valid[f] = 1'b0;
        end

        // Greedy input-order pivot classification, matching FaultList.
        for (f = 0; f < MAX_FAULTS; f = f + 1) begin
            if (fault_valids_i[f]) begin
                fault_count = fault_count + 1;
                is_pivot = 1'b1;
                for (p = 0; p < MAX_K; p = p + 1) begin
                    if (p < pivot_count &&
                        (fault_rows_flat_i[f*ROW_ADDR_WIDTH +: ROW_ADDR_WIDTH] == pivot_rows[p] ||
                         fault_cols_flat_i[f*COL_ADDR_WIDTH +: COL_ADDR_WIDTH] == pivot_cols[p]))
                        is_pivot = 1'b0;
                end
                if (is_pivot) begin
                    if (pivot_count < active_k) begin
                        pivot_rows[pivot_count] =
                            fault_rows_flat_i[f*ROW_ADDR_WIDTH +: ROW_ADDR_WIDTH];
                        pivot_cols[pivot_count] =
                            fault_cols_flat_i[f*COL_ADDR_WIDTH +: COL_ADDR_WIDTH];
                        pivot_row_count[pivot_count] = 1;
                        pivot_col_count[pivot_count] = 1;
                        pivot_count = pivot_count + 1;
                    end else if (buffer_count < BUFFER_ENTRY_NUM) begin
                        buffer_count = buffer_count + 1;
                    end else begin
                        overflow = 1'b1;
                    end
                end else begin
                    nonpivot_rows[nonpivot_count] =
                        fault_rows_flat_i[f*ROW_ADDR_WIDTH +: ROW_ADDR_WIDTH];
                    nonpivot_cols[nonpivot_count] =
                        fault_cols_flat_i[f*COL_ADDR_WIDTH +: COL_ADDR_WIDTH];
                    nonpivot_count = nonpivot_count + 1;
                end
            end
        end

        // Hybrid descriptors and must-line creation use first pivot match.
        for (f = 0; f < MAX_FAULTS; f = f + 1) begin
            if (f < nonpivot_count) begin
                relation_found = 1'b0;
                for (p = 0; p < MAX_K; p = p + 1) begin
                    if (!relation_found && p < pivot_count) begin
                        if (nonpivot_rows[f] == pivot_rows[p]) begin
                            relation_found = 1'b1;
                            if (!pivot_row_must[p]) begin
                                pivot_row_count[p] = pivot_row_count[p] + 1;
                                if (pivot_row_count[p] > active_cols) begin
                                    pivot_row_must[p] = 1'b1;
                                    for (h = 0; h < MAX_FAULTS; h = h + 1) begin
                                        if (h < hybrid_count && hybrid_valid[h] &&
                                            hybrid_pointer[h] == p && !hybrid_row_is_diff[h]) begin
                                            hybrid_valid[h] = 1'b0;
                                            hybrid_live_count = hybrid_live_count - 1;
                                        end
                                    end
                                end else if (hybrid_live_count < hybrid_capacity) begin
                                    hybrid_rows[hybrid_count] = nonpivot_rows[f];
                                    hybrid_cols[hybrid_count] = nonpivot_cols[f];
                                    hybrid_pointer[hybrid_count] = p;
                                    hybrid_row_is_diff[hybrid_count] = 1'b0;
                                    hybrid_valid[hybrid_count] = 1'b1;
                                    hybrid_count = hybrid_count + 1;
                                    hybrid_live_count = hybrid_live_count + 1;
                                end else begin
                                    overflow = 1'b1;
                                end
                            end
                        end else if (nonpivot_cols[f] == pivot_cols[p]) begin
                            relation_found = 1'b1;
                            if (!pivot_col_must[p]) begin
                                pivot_col_count[p] = pivot_col_count[p] + 1;
                                if (pivot_col_count[p] > active_rows) begin
                                    pivot_col_must[p] = 1'b1;
                                    for (h = 0; h < MAX_FAULTS; h = h + 1) begin
                                        if (h < hybrid_count && hybrid_valid[h] &&
                                            hybrid_pointer[h] == p && hybrid_row_is_diff[h]) begin
                                            hybrid_valid[h] = 1'b0;
                                            hybrid_live_count = hybrid_live_count - 1;
                                        end
                                    end
                                end else if (hybrid_live_count < hybrid_capacity) begin
                                    hybrid_rows[hybrid_count] = nonpivot_rows[f];
                                    hybrid_cols[hybrid_count] = nonpivot_cols[f];
                                    hybrid_pointer[hybrid_count] = p;
                                    hybrid_row_is_diff[hybrid_count] = 1'b1;
                                    hybrid_valid[hybrid_count] = 1'b1;
                                    hybrid_count = hybrid_count + 1;
                                    hybrid_live_count = hybrid_live_count + 1;
                                end else begin
                                    overflow = 1'b1;
                                end
                            end
                        end
                    end
                end
                if (!relation_found)
                    overflow = 1'b1;
            end
        end

        // Build the paper KxK matrix and its row/column address dictionaries.
        for (p = 0; p < MAX_K; p = p + 1) begin
            if (p < pivot_count) begin
                row_dictionary[p] = pivot_rows[p];
                col_dictionary[p] = pivot_cols[p];
                row_dictionary_valid[p] = 1'b1;
                col_dictionary_valid[p] = 1'b1;
            end
        end
        row_dictionary_count = pivot_count;
        col_dictionary_count = pivot_count;
        for (r = 0; r < MAX_K; r = r + 1) begin
            for (c = 0; c < MAX_K; c = c + 1) begin
                if (r < active_k && c < active_k)
                    matrix[r][c] =
                        (r == c && r < pivot_count) ||
                        (r < pivot_count && pivot_row_must[r]) ||
                        (c < pivot_count && pivot_col_must[c]);
            end
        end
        matrix_extendable = pivot_count < active_k;
        for (h = 0; h < MAX_FAULTS; h = h + 1) begin
            if (h < hybrid_count && hybrid_valid[h]) begin
                row_match = 1'b0;
                col_match = 1'b0;
                r = 0;
                c = 0;
                for (i = 0; i < MAX_K; i = i + 1) begin
                    if (!row_match && i < row_dictionary_count &&
                        hybrid_rows[h] == row_dictionary[i]) begin
                        row_match = 1'b1;
                        r = i;
                    end
                    if (!col_match && i < col_dictionary_count &&
                        hybrid_cols[h] == col_dictionary[i]) begin
                        col_match = 1'b1;
                        c = i;
                    end
                end
                if (!matrix_extendable) begin
                    if (row_match && col_match)
                        matrix[r][c] = 1'b1;
                    else if (row_match) begin
                        for (i = 0; i < MAX_K; i = i + 1)
                            if (i < active_k) matrix[r][i] = 1'b1;
                    end else if (col_match) begin
                        for (i = 0; i < MAX_K; i = i + 1)
                            if (i < active_k) matrix[i][c] = 1'b1;
                    end
                end else begin
                    if (row_match && col_match)
                        matrix[r][c] = 1'b1;
                    else if (row_match) begin
                        if (col_dictionary_count < active_k) begin
                            matrix[r][col_dictionary_count] = 1'b1;
                            col_dictionary[col_dictionary_count] = hybrid_cols[h];
                            col_dictionary_valid[col_dictionary_count] = 1'b1;
                            col_dictionary_count = col_dictionary_count + 1;
                        end else begin
                            for (i = 0; i < MAX_K; i = i + 1)
                                if (i < active_k) matrix[r][i] = 1'b1;
                        end
                    end else if (col_match) begin
                        if (row_dictionary_count < active_k) begin
                            matrix[row_dictionary_count][c] = 1'b1;
                            row_dictionary[row_dictionary_count] = hybrid_rows[h];
                            row_dictionary_valid[row_dictionary_count] = 1'b1;
                            row_dictionary_count = row_dictionary_count + 1;
                        end else begin
                            for (i = 0; i < MAX_K; i = i + 1)
                                if (i < active_k) matrix[i][c] = 1'b1;
                        end
                    end
                    if (row_dictionary_count >= active_k &&
                        col_dictionary_count >= active_k)
                        matrix_extendable = 1'b0;
                end
            end
        end

        // Enumerate Fig. 8 candidates in ascending lexicographic R/C order.
        candidate_id = 0;
        first_valid_seen = 0;
        for (mask_value = 0; mask_value < MASK_SPACE; mask_value = mask_value + 1) begin
            solution_bits = {MAX_K{1'b0}};
            bit_count = 0;
            if (mask_value < (1 << active_k)) begin
                for (i = 0; i < MAX_K; i = i + 1) begin
                    if (i < active_k) begin
                        solution_bits[i] =
                            ((mask_value >> (active_k - 1 - i)) & 1) != 0;
                        if (solution_bits[i])
                            bit_count = bit_count + 1;
                    end
                end
                if (bit_count == active_cols) begin
                    candidate_valid = !overflow;
                    used_rows = 0;
                    used_cols = 0;
                    for (r = 0; r < MAX_K; r = r + 1) begin
                        if (r < active_k) begin
                            candidate_masks_flat_o[candidate_id*MAX_K + r] =
                                solution_bits[r];
                            if (!solution_bits[r] && row_dictionary_valid[r])
                                used_rows = used_rows + 1;
                            if (solution_bits[r] && col_dictionary_valid[r])
                                used_cols = used_cols + 1;
                            for (c = 0; c < MAX_K; c = c + 1) begin
                                if (c < active_k && matrix[r][c] &&
                                    !(!solution_bits[r] || solution_bits[c]))
                                    candidate_valid = 1'b0;
                            end
                        end
                    end
                    used_rows_flat_o[candidate_id*SPARE_WIDTH +: SPARE_WIDTH] =
                        used_rows[SPARE_WIDTH-1:0];
                    used_cols_flat_o[candidate_id*SPARE_WIDTH +: SPARE_WIDTH] =
                        used_cols[SPARE_WIDTH-1:0];
                    if (candidate_valid) begin
                        valid_candidates_o[candidate_id] = 1'b1;
                        if (first_valid_seen == 0) begin
                            first_candidate_id_o =
                                candidate_id[CANDIDATE_ID_WIDTH-1:0];
                            first_valid_seen = 1;
                        end
                    end
                    candidate_id = candidate_id + 1;
                end
            end
        end

        storage_overflow_o = overflow;
        repairable_o = first_valid_seen != 0;
        pivot_count_o = pivot_count[FAULT_COUNT_WIDTH-1:0];
        hybrid_count_o = hybrid_live_count[FAULT_COUNT_WIDTH-1:0];
        buffer_count_o = buffer_count[FAULT_COUNT_WIDTH-1:0];
        attempt_cycles_o =
            fault_count * 2 + pivot_count + hybrid_live_count + buffer_count +
            active_k * active_k + candidate_id * 2;
    end

    initial begin
        if (MAX_FAULTS <= 0 || MAX_RS <= 0 || MAX_CS <= 0 ||
            BUFFER_ENTRY_NUM < 0 || MAX_K >= 30)
            $error("candidate_analyzer parameter range is invalid");
    end
endmodule

`default_nettype wire
