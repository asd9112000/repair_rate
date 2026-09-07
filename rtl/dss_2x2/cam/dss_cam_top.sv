`default_nettype none

module dss_cam_top #(
    parameter integer ROW_ADDR_WIDTH = 10,
    parameter integer COL_ADDR_WIDTH = 10,
    parameter integer FAULT_ENTRY_NUM = 12,
    parameter integer RS = 2,
    parameter integer CS = 2,
    parameter integer SHARED_ROWS = 1,
    parameter integer SHARED_COLS = 1,
    parameter integer MAX_BORROWS = 3,
    parameter integer TOPOLOGY = 1,
    parameter integer BUFFER_ENTRY_NUM = RS + CS,
    parameter integer MAX_RS = RS + SHARED_ROWS,
    parameter integer MAX_CS = CS + SHARED_COLS,
    parameter integer MAX_K = RS + CS + ((SHARED_ROWS > SHARED_COLS) ? SHARED_ROWS : SHARED_COLS),
    parameter integer MASK_SPACE = (1 << MAX_K),
    parameter integer SPARE_WIDTH = (MAX_K <= 1) ? 1 : $clog2(MAX_K + 1),
    parameter integer CANDIDATE_ID_WIDTH = (MASK_SPACE <= 1) ? 1 : $clog2(MASK_SPACE),
    parameter integer FAULT_COUNT_WIDTH = (FAULT_ENTRY_NUM <= 1) ? 1 : $clog2(FAULT_ENTRY_NUM + 1)
) (
    input  wire                                  clk_i,
    input  wire                                  rst_ni,
    input  wire                                  clear_i,
    input  wire                                  fault_valid_i,
    output wire                                  fault_ready_o,
    input  wire [1:0]                            fault_subarray_i,
    input  wire [ROW_ADDR_WIDTH-1:0]             fault_row_i,
    input  wire [COL_ADDR_WIDTH-1:0]             fault_col_i,
    input  wire                                  analysis_start_i,
    output wire                                  analysis_busy_o,
    output wire                                  analysis_done_o,
    output wire                                  repairable_o,
    output wire                                  baseline_repairable_o,
    output wire                                  fault_storage_overflow_o,
    output wire [4*FAULT_COUNT_WIDTH-1:0]        fault_counts_o,
    output wire [3:0]                            selected_attempts_o,
    output wire [4*CANDIDATE_ID_WIDTH-1:0]       selected_candidates_o,
    output wire [4*SPARE_WIDTH-1:0]              selected_rows_o,
    output wire [4*SPARE_WIDTH-1:0]              selected_cols_o,
    output wire [4*MAX_K-1:0]                    selected_masks_o,
    output wire [SPARE_WIDTH-1:0]                borrowed_rows_o,
    output wire [SPARE_WIDTH-1:0]                borrowed_cols_o,
    output wire [31:0]                           analysis_cycles_o
);
    wire collector_ready;
    wire collector_overflow;
    wire [4*FAULT_ENTRY_NUM-1:0] fault_valids;
    wire [4*FAULT_ENTRY_NUM*ROW_ADDR_WIDTH-1:0] fault_rows;
    wire [4*FAULT_ENTRY_NUM*COL_ADDR_WIDTH-1:0] fault_cols;
    wire collector_enable;
    reg group_complete;

    wire [3:0] base_repairable_raw;
    wire [3:0] base_overflow;
    wire [4*MASK_SPACE-1:0] base_valid_candidates;
    wire [4*MASK_SPACE*SPARE_WIDTH-1:0] base_used_rows;
    wire [4*MASK_SPACE*SPARE_WIDTH-1:0] base_used_cols;
    wire [4*MASK_SPACE*MAX_K-1:0] base_candidate_masks;
    wire [4*32-1:0] base_attempt_cycles;

    wire [3:0] extended_repairable_raw;
    wire [3:0] extended_overflow;
    wire [4*MASK_SPACE-1:0] extended_valid_candidates_raw;
    wire [4*MASK_SPACE-1:0] extended_valid_candidates;
    wire [4*MASK_SPACE*SPARE_WIDTH-1:0] extended_used_rows;
    wire [4*MASK_SPACE*SPARE_WIDTH-1:0] extended_used_cols;
    wire [4*MASK_SPACE*MAX_K-1:0] extended_candidate_masks;
    wire [4*32-1:0] extended_attempt_cycles;

    wire selector_busy;
    wire selector_done;
    wire selector_repairable;
    wire [3:0] selector_attempts;
    wire [4*CANDIDATE_ID_WIDTH-1:0] selector_candidates;
    wire [4*SPARE_WIDTH-1:0] selector_rows;
    wire [4*SPARE_WIDTH-1:0] selector_cols;
    wire [4*MAX_K-1:0] selector_masks;
    wire [SPARE_WIDTH-1:0] selector_borrowed_rows;
    wire [SPARE_WIDTH-1:0] selector_borrowed_cols;
    wire [31:0] selector_cycles;
    wire stored_solution_valid;
    wire start_accepted;
    wire [4*CANDIDATE_ID_WIDTH-1:0] unused_base_first_candidate;
    wire [4*CANDIDATE_ID_WIDTH-1:0] unused_extended_first_candidate;
    wire [4*FAULT_COUNT_WIDTH-1:0] unused_base_pivot_count;
    wire [4*FAULT_COUNT_WIDTH-1:0] unused_base_hybrid_count;
    wire [4*FAULT_COUNT_WIDTH-1:0] unused_base_buffer_count;
    wire [4*FAULT_COUNT_WIDTH-1:0] unused_extended_pivot_count;
    wire [4*FAULT_COUNT_WIDTH-1:0] unused_extended_hybrid_count;
    wire [4*FAULT_COUNT_WIDTH-1:0] unused_extended_buffer_count;
    /* verilator lint_off WIDTH */
    localparam [SPARE_WIDTH-1:0] BASE_ROWS = RS;
    localparam [SPARE_WIDTH-1:0] BASE_COLS = CS;
    localparam [SPARE_WIDTH-1:0] EXTENDED_ROWS = RS + SHARED_ROWS;
    localparam [SPARE_WIDTH-1:0] EXTENDED_COLS = CS + SHARED_COLS;
    /* verilator lint_on WIDTH */

    assign collector_enable =
        !group_complete && !selector_busy && !selector_done && !stored_solution_valid;
    assign fault_ready_o = collector_enable && collector_ready;
    assign start_accepted = analysis_start_i && collector_enable && !fault_valid_i;
    assign analysis_busy_o = selector_busy || selector_done;
    assign analysis_done_o = stored_solution_valid;
    assign fault_storage_overflow_o = collector_overflow || |base_overflow || |extended_overflow;
    assign baseline_repairable_o = &(base_repairable_raw & {4{!collector_overflow}});
    assign extended_valid_candidates = TOPOLOGY == 0
        ? {(4*MASK_SPACE){1'b0}}
        : extended_valid_candidates_raw;

    always @(posedge clk_i) begin
        if (!rst_ni || clear_i)
            group_complete <= 1'b0;
        else if (stored_solution_valid)
            group_complete <= 1'b1;
    end

    fault_collector #(
        .ENTRY_NUM(FAULT_ENTRY_NUM),
        .ROW_ADDR_WIDTH(ROW_ADDR_WIDTH),
        .COL_ADDR_WIDTH(COL_ADDR_WIDTH),
        .COUNT_WIDTH(FAULT_COUNT_WIDTH)
    ) collector (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .clear_i(clear_i),
        .enable_i(collector_enable),
        .fault_valid_i(fault_valid_i),
        .fault_ready_o(collector_ready),
        .fault_subarray_i(fault_subarray_i),
        .fault_row_i(fault_row_i),
        .fault_col_i(fault_col_i),
        .overflow_o(collector_overflow),
        .valid_vectors_o(fault_valids),
        .rows_flat_o(fault_rows),
        .cols_flat_o(fault_cols),
        .fault_counts_o(fault_counts_o)
    );

    genvar subarray;
    generate
        for (subarray = 0; subarray < 4; subarray = subarray + 1) begin : gen_analyzers
            candidate_analyzer #(
                .MAX_FAULTS(FAULT_ENTRY_NUM),
                .ROW_ADDR_WIDTH(ROW_ADDR_WIDTH),
                .COL_ADDR_WIDTH(COL_ADDR_WIDTH),
                .MAX_RS(MAX_RS),
                .MAX_CS(MAX_CS),
                .BUFFER_ENTRY_NUM(BUFFER_ENTRY_NUM),
                .MAX_K(MAX_K),
                .MASK_SPACE(MASK_SPACE),
                .SPARE_WIDTH(SPARE_WIDTH),
                .CANDIDATE_ID_WIDTH(CANDIDATE_ID_WIDTH),
                .FAULT_COUNT_WIDTH(FAULT_COUNT_WIDTH)
            ) base_analyzer (
                .fault_valids_i(fault_valids[subarray*FAULT_ENTRY_NUM +: FAULT_ENTRY_NUM]),
                .fault_rows_flat_i(fault_rows[subarray*FAULT_ENTRY_NUM*ROW_ADDR_WIDTH +: FAULT_ENTRY_NUM*ROW_ADDR_WIDTH]),
                .fault_cols_flat_i(fault_cols[subarray*FAULT_ENTRY_NUM*COL_ADDR_WIDTH +: FAULT_ENTRY_NUM*COL_ADDR_WIDTH]),
                .active_rows_i(BASE_ROWS),
                .active_cols_i(BASE_COLS),
                .repairable_o(base_repairable_raw[subarray]),
                .storage_overflow_o(base_overflow[subarray]),
                .valid_candidates_o(base_valid_candidates[subarray*MASK_SPACE +: MASK_SPACE]),
                .used_rows_flat_o(base_used_rows[subarray*MASK_SPACE*SPARE_WIDTH +: MASK_SPACE*SPARE_WIDTH]),
                .used_cols_flat_o(base_used_cols[subarray*MASK_SPACE*SPARE_WIDTH +: MASK_SPACE*SPARE_WIDTH]),
                .candidate_masks_flat_o(base_candidate_masks[subarray*MASK_SPACE*MAX_K +: MASK_SPACE*MAX_K]),
                .first_candidate_id_o(unused_base_first_candidate[subarray*CANDIDATE_ID_WIDTH +: CANDIDATE_ID_WIDTH]),
                .pivot_count_o(unused_base_pivot_count[subarray*FAULT_COUNT_WIDTH +: FAULT_COUNT_WIDTH]),
                .hybrid_count_o(unused_base_hybrid_count[subarray*FAULT_COUNT_WIDTH +: FAULT_COUNT_WIDTH]),
                .buffer_count_o(unused_base_buffer_count[subarray*FAULT_COUNT_WIDTH +: FAULT_COUNT_WIDTH]),
                .attempt_cycles_o(base_attempt_cycles[subarray*32 +: 32])
            );

            if (subarray == 0 || subarray == 3) begin : gen_column_borrower
                candidate_analyzer #(
                    .MAX_FAULTS(FAULT_ENTRY_NUM),
                    .ROW_ADDR_WIDTH(ROW_ADDR_WIDTH),
                    .COL_ADDR_WIDTH(COL_ADDR_WIDTH),
                    .MAX_RS(MAX_RS),
                    .MAX_CS(MAX_CS),
                    .BUFFER_ENTRY_NUM(BUFFER_ENTRY_NUM),
                    .MAX_K(MAX_K),
                    .MASK_SPACE(MASK_SPACE),
                    .SPARE_WIDTH(SPARE_WIDTH),
                    .CANDIDATE_ID_WIDTH(CANDIDATE_ID_WIDTH),
                    .FAULT_COUNT_WIDTH(FAULT_COUNT_WIDTH)
                ) extended_analyzer (
                    .fault_valids_i(fault_valids[subarray*FAULT_ENTRY_NUM +: FAULT_ENTRY_NUM]),
                    .fault_rows_flat_i(fault_rows[subarray*FAULT_ENTRY_NUM*ROW_ADDR_WIDTH +: FAULT_ENTRY_NUM*ROW_ADDR_WIDTH]),
                    .fault_cols_flat_i(fault_cols[subarray*FAULT_ENTRY_NUM*COL_ADDR_WIDTH +: FAULT_ENTRY_NUM*COL_ADDR_WIDTH]),
                    .active_rows_i(BASE_ROWS),
                    .active_cols_i(EXTENDED_COLS),
                    .repairable_o(extended_repairable_raw[subarray]),
                    .storage_overflow_o(extended_overflow[subarray]),
                    .valid_candidates_o(extended_valid_candidates_raw[subarray*MASK_SPACE +: MASK_SPACE]),
                    .used_rows_flat_o(extended_used_rows[subarray*MASK_SPACE*SPARE_WIDTH +: MASK_SPACE*SPARE_WIDTH]),
                    .used_cols_flat_o(extended_used_cols[subarray*MASK_SPACE*SPARE_WIDTH +: MASK_SPACE*SPARE_WIDTH]),
                    .candidate_masks_flat_o(extended_candidate_masks[subarray*MASK_SPACE*MAX_K +: MASK_SPACE*MAX_K]),
                    .first_candidate_id_o(unused_extended_first_candidate[subarray*CANDIDATE_ID_WIDTH +: CANDIDATE_ID_WIDTH]),
                    .pivot_count_o(unused_extended_pivot_count[subarray*FAULT_COUNT_WIDTH +: FAULT_COUNT_WIDTH]),
                    .hybrid_count_o(unused_extended_hybrid_count[subarray*FAULT_COUNT_WIDTH +: FAULT_COUNT_WIDTH]),
                    .buffer_count_o(unused_extended_buffer_count[subarray*FAULT_COUNT_WIDTH +: FAULT_COUNT_WIDTH]),
                    .attempt_cycles_o(extended_attempt_cycles[subarray*32 +: 32])
                );
            end else begin : gen_row_borrower
                candidate_analyzer #(
                    .MAX_FAULTS(FAULT_ENTRY_NUM),
                    .ROW_ADDR_WIDTH(ROW_ADDR_WIDTH),
                    .COL_ADDR_WIDTH(COL_ADDR_WIDTH),
                    .MAX_RS(MAX_RS),
                    .MAX_CS(MAX_CS),
                    .BUFFER_ENTRY_NUM(BUFFER_ENTRY_NUM),
                    .MAX_K(MAX_K),
                    .MASK_SPACE(MASK_SPACE),
                    .SPARE_WIDTH(SPARE_WIDTH),
                    .CANDIDATE_ID_WIDTH(CANDIDATE_ID_WIDTH),
                    .FAULT_COUNT_WIDTH(FAULT_COUNT_WIDTH)
                ) extended_analyzer (
                    .fault_valids_i(fault_valids[subarray*FAULT_ENTRY_NUM +: FAULT_ENTRY_NUM]),
                    .fault_rows_flat_i(fault_rows[subarray*FAULT_ENTRY_NUM*ROW_ADDR_WIDTH +: FAULT_ENTRY_NUM*ROW_ADDR_WIDTH]),
                    .fault_cols_flat_i(fault_cols[subarray*FAULT_ENTRY_NUM*COL_ADDR_WIDTH +: FAULT_ENTRY_NUM*COL_ADDR_WIDTH]),
                    .active_rows_i(EXTENDED_ROWS),
                    .active_cols_i(BASE_COLS),
                    .repairable_o(extended_repairable_raw[subarray]),
                    .storage_overflow_o(extended_overflow[subarray]),
                    .valid_candidates_o(extended_valid_candidates_raw[subarray*MASK_SPACE +: MASK_SPACE]),
                    .used_rows_flat_o(extended_used_rows[subarray*MASK_SPACE*SPARE_WIDTH +: MASK_SPACE*SPARE_WIDTH]),
                    .used_cols_flat_o(extended_used_cols[subarray*MASK_SPACE*SPARE_WIDTH +: MASK_SPACE*SPARE_WIDTH]),
                    .candidate_masks_flat_o(extended_candidate_masks[subarray*MASK_SPACE*MAX_K +: MASK_SPACE*MAX_K]),
                    .first_candidate_id_o(unused_extended_first_candidate[subarray*CANDIDATE_ID_WIDTH +: CANDIDATE_ID_WIDTH]),
                    .pivot_count_o(unused_extended_pivot_count[subarray*FAULT_COUNT_WIDTH +: FAULT_COUNT_WIDTH]),
                    .hybrid_count_o(unused_extended_hybrid_count[subarray*FAULT_COUNT_WIDTH +: FAULT_COUNT_WIDTH]),
                    .buffer_count_o(unused_extended_buffer_count[subarray*FAULT_COUNT_WIDTH +: FAULT_COUNT_WIDTH]),
                    .attempt_cycles_o(extended_attempt_cycles[subarray*32 +: 32])
                );
            end
        end
    endgenerate

    solution_selector #(
        .RS(RS),
        .CS(CS),
        .SHARED_ROWS(SHARED_ROWS),
        .SHARED_COLS(SHARED_COLS),
        .MAX_BORROWS(MAX_BORROWS),
        .TOPOLOGY(TOPOLOGY),
        .MAX_RS(MAX_RS),
        .MAX_CS(MAX_CS),
        .MAX_K(MAX_K),
        .MASK_SPACE(MASK_SPACE),
        .SPARE_WIDTH(SPARE_WIDTH),
        .CANDIDATE_ID_WIDTH(CANDIDATE_ID_WIDTH)
    ) selector (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(start_accepted),
        .base_repairable_i(base_repairable_raw & {4{!collector_overflow}}),
        .base_valid_candidates_i(base_valid_candidates),
        .base_used_rows_i(base_used_rows),
        .base_used_cols_i(base_used_cols),
        .base_candidate_masks_i(base_candidate_masks),
        .base_attempt_cycles_i(base_attempt_cycles),
        .extended_repairable_i(extended_repairable_raw & {4{!collector_overflow}}),
        .extended_valid_candidates_i(extended_valid_candidates),
        .extended_used_rows_i(extended_used_rows),
        .extended_used_cols_i(extended_used_cols),
        .extended_candidate_masks_i(extended_candidate_masks),
        .extended_attempt_cycles_i(extended_attempt_cycles),
        .busy_o(selector_busy),
        .done_o(selector_done),
        .repairable_o(selector_repairable),
        .selected_attempts_o(selector_attempts),
        .selected_candidates_o(selector_candidates),
        .selected_rows_o(selector_rows),
        .selected_cols_o(selector_cols),
        .selected_masks_o(selector_masks),
        .borrowed_rows_o(selector_borrowed_rows),
        .borrowed_cols_o(selector_borrowed_cols),
        .analysis_cycles_o(selector_cycles)
    );

    solution_store #(
        .SPARE_WIDTH(SPARE_WIDTH),
        .CANDIDATE_ID_WIDTH(CANDIDATE_ID_WIDTH),
        .MAX_K(MAX_K)
    ) store (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .clear_i(clear_i),
        .capture_i(selector_done),
        .repairable_i(selector_repairable),
        .selected_attempts_i(selector_attempts),
        .selected_candidates_i(selector_candidates),
        .selected_rows_i(selector_rows),
        .selected_cols_i(selector_cols),
        .selected_masks_i(selector_masks),
        .borrowed_rows_i(selector_borrowed_rows),
        .borrowed_cols_i(selector_borrowed_cols),
        .analysis_cycles_i(selector_cycles),
        .solution_valid_o(stored_solution_valid),
        .repairable_o(repairable_o),
        .selected_attempts_o(selected_attempts_o),
        .selected_candidates_o(selected_candidates_o),
        .selected_rows_o(selected_rows_o),
        .selected_cols_o(selected_cols_o),
        .selected_masks_o(selected_masks_o),
        .borrowed_rows_o(borrowed_rows_o),
        .borrowed_cols_o(borrowed_cols_o),
        .analysis_cycles_o(analysis_cycles_o)
    );

    initial begin
        if (TOPOLOGY != 0 && TOPOLOGY != 1)
            $error("Phase 2 dss_cam_top supports no-sharing and directional topology only");
        if (RS <= 0 || CS <= 0 || SHARED_ROWS < 0 || SHARED_COLS < 0 ||
            SHARED_ROWS > RS || SHARED_COLS > CS)
            $error("dss_cam_top spare parameters are invalid");
    end
endmodule

`default_nettype wire
