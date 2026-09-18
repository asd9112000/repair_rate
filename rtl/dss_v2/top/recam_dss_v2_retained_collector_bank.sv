`default_nettype none

// Frozen S1G-A2F collector-semantic state for one SA.  The register is the
// canonical 821-bit packing; all validity, membership, Must, full, and fault
// count signals below are reconstructed combinationally from it.
module recam_dss_v2_retained_collector_bank #(
    parameter int unsigned GENERATION_W = 4
) (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic clear_i,
    input  logic fault_valid_i,
    input  logic [9:0] fault_row_i,
    input  logic [9:0] fault_col_i,
    input  logic [2:0] config_id_i,
    input  logic analysis_complete_i,
    input  logic [GENERATION_W-1:0] analysis_generation_i,
    output logic fault_ready_o,
    output logic [820:0] retained_state_o,
    output logic [GENERATION_W-1:0] fault_generation_o,
    output logic analysis_valid_o,
    output logic [GENERATION_W-1:0] analysis_generation_o,
    output logic [4:0] pivot_valid_o,
    output logic [49:0] pivot_rows_flat_o,
    output logic [49:0] pivot_cols_flat_o,
    output logic [13:0] hybrid_valid_o,
    output logic [139:0] hybrid_rows_flat_o,
    output logic [139:0] hybrid_cols_flat_o,
    output logic [41:0] hybrid_pointer_flat_o,
    output logic [13:0] hybrid_descriptor_o,
    output logic [97:0] hybrid_cfg_valid_flat_o,
    output logic [11:0] reuse_valid_o,
    output logic [119:0] reuse_rows_flat_o,
    output logic [119:0] reuse_cols_flat_o,
    output logic [83:0] reuse_cfg_valid_flat_o,
    output logic [34:0] row_must_by_cfg_o,
    output logic [34:0] col_must_by_cfg_o,
    output logic [3:0] fault_count_o,
    output logic [231:0] analyzer_view_o
);
    localparam int unsigned RETAINED_COLLECTOR_BITS_PER_SA = 821;
    localparam int unsigned RETAINED_COLLECTOR_BITS_4SA = 3284;
    localparam int unsigned MAX_FAULTS = 12;
    localparam int unsigned MAX_K = 5;
    localparam int unsigned REACHABLE_HYBRIDS = 11;
    localparam int unsigned REACHABLE_REUSE = 11;
    localparam int unsigned PIVOT_COUNT_LSB = 0;
    localparam int unsigned PIVOT_DATA_LSB = 3;
    localparam int unsigned HYBRID_COUNT_LSB = 103;
    localparam int unsigned HYBRID_DATA_LSB = 107;
    localparam int unsigned REUSE_COUNT_LSB = 261;
    localparam int unsigned REUSE_DATA_LSB = 265;
    localparam int unsigned ROW_COUNTER_LSB = 485;
    localparam int unsigned COL_COUNTER_LSB = 653;

    logic [820:0] retained_q;
    logic [820:0] retained_next;
    logic [GENERATION_W-1:0] fault_generation_q;
    logic [GENERATION_W-1:0] analysis_generation_q;
    logic analysis_valid_q;
    logic [2:0] pivot_count;
    logic [3:0] hybrid_count;
    logic [3:0] reuse_count;
    logic [9:0] pivot_row [0:4];
    logic [9:0] pivot_col [0:4];
    logic [2:0] hybrid_pointer [0:10];
    logic hybrid_descriptor [0:10];
    logic [9:0] hybrid_differing [0:10];
    logic [9:0] reuse_row [0:10];
    logic [9:0] reuse_col [0:10];
    logic [9:0] row_counter_address [0:11];
    logic [3:0] row_counter_count [0:11];
    logic [9:0] col_counter_address [0:11];
    logic [3:0] col_counter_count [0:11];
    logic relation_found;
    logic relation_descriptor;
    logic [2:0] relation_pointer;
    logic reuse_relation_found;
    logic [2:0] reuse_relation_pointer;
    logic [6:0] relation_hybrid_cfg;
    logic [6:0] relation_reuse_cfg;
    integer index;
    integer cfg_loop;
    integer row_hit;
    integer col_hit;
    integer row_free;
    integer col_free;
    integer pivot_row_count;
    integer pivot_col_count;
    integer compact_slot;

    function automatic int cfg_rows(input int cfg);
        case (cfg)
            0, 1, 5: cfg_rows = 2;
            2, 3: cfg_rows = 3;
            4, 6: cfg_rows = 1;
            default: cfg_rows = 0;
        endcase
    endfunction

    function automatic int cfg_cols(input int cfg);
        case (cfg)
            0, 2, 4: cfg_cols = 2;
            1, 3: cfg_cols = 1;
            5, 6: cfg_cols = 3;
            default: cfg_cols = 0;
        endcase
    endfunction

    function automatic int cfg_k(input int cfg);
        cfg_k = cfg_rows(cfg) + cfg_cols(cfg);
    endfunction

    function automatic logic [6:0] hybrid_cfg_mask(input logic [2:0] pointer);
        integer cfg_index;
        begin
            hybrid_cfg_mask = '0;
            for (cfg_index = 0; cfg_index < 7; cfg_index = cfg_index + 1)
                hybrid_cfg_mask[cfg_index] = int'(pointer) < cfg_k(cfg_index);
        end
    endfunction

    function automatic logic hybrid_cfg_member(
        input logic [2:0] pointer,
        input logic [2:0] cfg_index
    );
        hybrid_cfg_member = int'(pointer) < cfg_k(int'(cfg_index));
    endfunction

    // Canonical unpack.  A valid prefix is represented only by its count.
    always_comb begin
        pivot_count = retained_q[PIVOT_COUNT_LSB +: 3];
        hybrid_count = retained_q[HYBRID_COUNT_LSB +: 4];
        reuse_count = retained_q[REUSE_COUNT_LSB +: 4];
        for (index = 0; index < 5; index = index + 1) begin
            pivot_row[index] = retained_q[PIVOT_DATA_LSB + index * 20 +: 10];
            pivot_col[index] = retained_q[PIVOT_DATA_LSB + index * 20 + 10 +: 10];
        end
        for (index = 0; index < 11; index = index + 1) begin
            hybrid_pointer[index] = retained_q[HYBRID_DATA_LSB + index * 14 +: 3];
            hybrid_descriptor[index] = retained_q[HYBRID_DATA_LSB + index * 14 + 3];
            hybrid_differing[index] = retained_q[HYBRID_DATA_LSB + index * 14 + 4 +: 10];
            reuse_row[index] = retained_q[REUSE_DATA_LSB + index * 20 +: 10];
            reuse_col[index] = retained_q[REUSE_DATA_LSB + index * 20 + 10 +: 10];
        end
        for (index = 0; index < 12; index = index + 1) begin
            row_counter_count[index] = retained_q[ROW_COUNTER_LSB + index * 14 +: 4];
            row_counter_address[index] = retained_q[ROW_COUNTER_LSB + index * 14 + 4 +: 10];
            col_counter_count[index] = retained_q[COL_COUNTER_LSB + index * 14 +: 4];
            col_counter_address[index] = retained_q[COL_COUNTER_LSB + index * 14 + 4 +: 10];
        end
    end

    always_comb begin
        pivot_valid_o = '0;
        pivot_rows_flat_o = '0;
        pivot_cols_flat_o = '0;
        hybrid_valid_o = '0;
        hybrid_rows_flat_o = '0;
        hybrid_cols_flat_o = '0;
        hybrid_pointer_flat_o = '0;
        hybrid_descriptor_o = '0;
        hybrid_cfg_valid_flat_o = '0;
        reuse_valid_o = '0;
        reuse_rows_flat_o = '0;
        reuse_cols_flat_o = '0;
        reuse_cfg_valid_flat_o = '0;
        row_must_by_cfg_o = '0;
        col_must_by_cfg_o = '0;
        fault_count_o = '0;
        for (index = 0; index < 12; index = index + 1)
            fault_count_o = fault_count_o + row_counter_count[index];
        for (index = 0; index < 5; index = index + 1) begin
            pivot_valid_o[index] = index < pivot_count;
            pivot_rows_flat_o[index * 10 +: 10] = pivot_row[index];
            pivot_cols_flat_o[index * 10 +: 10] = pivot_col[index];
        end
        for (index = 0; index < 11; index = index + 1) begin
            hybrid_valid_o[index] = index < hybrid_count;
            hybrid_pointer_flat_o[index * 3 +: 3] = hybrid_pointer[index];
            hybrid_descriptor_o[index] = hybrid_descriptor[index];
            hybrid_cfg_valid_flat_o[index * 7 +: 7] = hybrid_cfg_mask(hybrid_pointer[index]);
            if (hybrid_descriptor[index]) begin
                hybrid_rows_flat_o[index * 10 +: 10] = hybrid_differing[index];
                hybrid_cols_flat_o[index * 10 +: 10] = pivot_col[hybrid_pointer[index]];
            end else begin
                hybrid_rows_flat_o[index * 10 +: 10] = pivot_row[hybrid_pointer[index]];
                hybrid_cols_flat_o[index * 10 +: 10] = hybrid_differing[index];
            end
            reuse_valid_o[index] = index < reuse_count;
            reuse_rows_flat_o[index * 10 +: 10] = reuse_row[index];
            reuse_cols_flat_o[index * 10 +: 10] = reuse_col[index];
            reuse_relation_found = 1'b0;
            reuse_relation_pointer = '0;
            for (cfg_loop = 0; cfg_loop < 5; cfg_loop = cfg_loop + 1) begin
                if (!reuse_relation_found && cfg_loop < pivot_count) begin
                    if (reuse_row[index] == pivot_row[cfg_loop] || reuse_col[index] == pivot_col[cfg_loop]) begin
                        reuse_relation_found = 1'b1;
                        reuse_relation_pointer = cfg_loop[2:0];
                    end
                end
            end
            if (reuse_relation_found)
                for (cfg_loop = 0; cfg_loop < 7; cfg_loop = cfg_loop + 1)
                    reuse_cfg_valid_flat_o[index * 7 + cfg_loop] = int'(reuse_relation_pointer) >= cfg_k(cfg_loop);
            else if (int'(pivot_count) == MAX_K)
                reuse_cfg_valid_flat_o[index * 7 +: 7] = 7'h7f;
        end
        for (cfg_loop = 0; cfg_loop < 7; cfg_loop = cfg_loop + 1) begin
            for (index = 0; index < 5; index = index + 1) begin
                int row_count_value;
                int col_count_value;
                row_count_value = 0;
                col_count_value = 0;
                for (row_hit = 0; row_hit < 12; row_hit = row_hit + 1) begin
                    if (row_counter_count[row_hit] != 0 && row_counter_address[row_hit] == pivot_row[index])
                        row_count_value = int'(row_counter_count[row_hit]);
                    if (col_counter_count[row_hit] != 0 && col_counter_address[row_hit] == pivot_col[index])
                        col_count_value = int'(col_counter_count[row_hit]);
                end
                row_must_by_cfg_o[cfg_loop * 5 + index] =
                    index < pivot_count && row_count_value > cfg_cols(cfg_loop);
                col_must_by_cfg_o[cfg_loop * 5 + index] =
                    index < pivot_count && col_count_value > cfg_rows(cfg_loop);
            end
        end
    end

    // The update decisions intentionally read only the old coherent state.
    always_comb begin
        retained_next = retained_q;
        relation_found = 1'b0;
        relation_descriptor = 1'b0;
        relation_pointer = '0;
        for (index = 0; index < 5; index = index + 1) begin
            if (!relation_found && index < pivot_count) begin
                if (fault_row_i == pivot_row[index]) begin
                    relation_found = 1'b1;
                    relation_pointer = index[2:0];
                end else if (fault_col_i == pivot_col[index]) begin
                    relation_found = 1'b1;
                    relation_descriptor = 1'b1;
                    relation_pointer = index[2:0];
                end
            end
        end
        relation_hybrid_cfg = relation_found ? hybrid_cfg_mask(relation_pointer) : '0;
        relation_reuse_cfg = '0;
        for (cfg_loop = 0; cfg_loop < 7; cfg_loop = cfg_loop + 1) begin
            if (!(relation_found && int'(relation_pointer) < cfg_k(cfg_loop)) && int'(pivot_count) >= cfg_k(cfg_loop))
                relation_reuse_cfg[cfg_loop] = 1'b1;
        end
        if (fault_valid_i && int'(fault_count_o) < MAX_FAULTS) begin
            row_hit = -1; col_hit = -1; row_free = -1; col_free = -1;
            for (index = 0; index < 12; index = index + 1) begin
                if (row_counter_count[index] != 0 && row_counter_address[index] == fault_row_i && row_hit < 0) row_hit = index;
                if (row_counter_count[index] == 0 && row_free < 0) row_free = index;
                if (col_counter_count[index] != 0 && col_counter_address[index] == fault_col_i && col_hit < 0) col_hit = index;
                if (col_counter_count[index] == 0 && col_free < 0) col_free = index;
            end
            if (row_hit >= 0) retained_next[ROW_COUNTER_LSB + row_hit * 14 +: 4] = row_counter_count[row_hit] + 1'b1;
            else if (row_free >= 0) begin
                retained_next[ROW_COUNTER_LSB + row_free * 14 +: 4] = 4'd1;
                retained_next[ROW_COUNTER_LSB + row_free * 14 + 4 +: 10] = fault_row_i;
            end
            if (col_hit >= 0) retained_next[COL_COUNTER_LSB + col_hit * 14 +: 4] = col_counter_count[col_hit] + 1'b1;
            else if (col_free >= 0) begin
                retained_next[COL_COUNTER_LSB + col_free * 14 +: 4] = 4'd1;
                retained_next[COL_COUNTER_LSB + col_free * 14 + 4 +: 10] = fault_col_i;
            end
            if (!relation_found && int'(pivot_count) < MAX_K) begin
                retained_next[PIVOT_DATA_LSB + pivot_count * 20 +: 10] = fault_row_i;
                retained_next[PIVOT_DATA_LSB + pivot_count * 20 + 10 +: 10] = fault_col_i;
                retained_next[PIVOT_COUNT_LSB +: 3] = pivot_count + 1'b1;
            end else if (relation_found && relation_hybrid_cfg != 0) begin
                retained_next[HYBRID_DATA_LSB + hybrid_count * 14 +: 3] = relation_pointer;
                retained_next[HYBRID_DATA_LSB + hybrid_count * 14 + 3] = relation_descriptor;
                retained_next[HYBRID_DATA_LSB + hybrid_count * 14 + 4 +: 10] =
                    relation_descriptor ? fault_row_i : fault_col_i;
                retained_next[HYBRID_COUNT_LSB +: 4] = hybrid_count + 1'b1;
            end
            if (relation_reuse_cfg != 0) begin
                retained_next[REUSE_DATA_LSB + reuse_count * 20 +: 10] = fault_row_i;
                retained_next[REUSE_DATA_LSB + reuse_count * 20 + 10 +: 10] = fault_col_i;
                retained_next[REUSE_COUNT_LSB +: 4] = reuse_count + 1'b1;
            end
        end
    end

    assign fault_ready_o = int'(fault_count_o) < MAX_FAULTS;
    assign retained_state_o = retained_q;
    assign fault_generation_o = fault_generation_q;
    assign analysis_valid_o = analysis_valid_q;
    assign analysis_generation_o = analysis_generation_q;

    // Exact 232-bit post-Must boundary: base threshold view plus a stable
    // H0-to-H10 filtered subsequence in slots 0 through 8.
    always_comb begin
        analyzer_view_o = '0;
        compact_slot = 0;
        for (index = 0; index < 5; index = index + 1) begin
            // The frozen 232-bit boundary contains the selected Config's
            // K-prefix, not all physically retained pivots.
            analyzer_view_o[index] = index < pivot_count &&
                                     index < cfg_k(int'(config_id_i));
            analyzer_view_o[5 + index * 9 +: 9] = pivot_row[index][8:0];
            analyzer_view_o[50 + index * 5 +: 5] = pivot_col[index][4:0];
        end
        for (index = 0; index < 5; index = index + 1) begin
            pivot_row_count = 0;
            pivot_col_count = 0;
            for (row_hit = 0; row_hit < 12; row_hit = row_hit + 1) begin
                if (row_counter_count[row_hit] != 0 && row_counter_address[row_hit] == pivot_row[index])
                    pivot_row_count = int'(row_counter_count[row_hit]);
                if (col_counter_count[row_hit] != 0 && col_counter_address[row_hit] == pivot_col[index])
                    pivot_col_count = int'(col_counter_count[row_hit]);
            end
            analyzer_view_o[75 + index] = pivot_row_count > 1;
            analyzer_view_o[80 + index] = pivot_row_count > 2;
            analyzer_view_o[85 + index] = pivot_row_count > 3;
            analyzer_view_o[90 + index] = pivot_col_count > 1;
            analyzer_view_o[95 + index] = pivot_col_count > 2;
            analyzer_view_o[100 + index] = pivot_col_count > 3;
        end
        for (index = 0; index < 11; index = index + 1) begin
            if (index < hybrid_count && hybrid_pointer[index] < 5 &&
                hybrid_cfg_member(hybrid_pointer[index], config_id_i) &&
                !(hybrid_descriptor[index]
                    ? col_must_by_cfg_o[config_id_i * 5 + hybrid_pointer[index]]
                    : row_must_by_cfg_o[config_id_i * 5 + hybrid_pointer[index]])) begin
                if (compact_slot < 9) begin
                    analyzer_view_o[106 + compact_slot * 14] = 1'b1;
                    analyzer_view_o[106 + compact_slot * 14 + 1 +: 3] = hybrid_pointer[index];
                    analyzer_view_o[106 + compact_slot * 14 + 4] = hybrid_descriptor[index];
                    analyzer_view_o[106 + compact_slot * 14 + 5 +: 9] = hybrid_differing[index][8:0];
                end
                compact_slot = compact_slot + 1;
            end
        end
    end

    always_ff @(posedge clk_i) begin
        if (!rst_ni || clear_i) begin
            retained_q <= '0;
            fault_generation_q <= '0;
            analysis_generation_q <= '0;
            analysis_valid_q <= 1'b0;
        end else begin
            if (fault_valid_i && fault_ready_o) begin
                retained_q <= retained_next;
                fault_generation_q <= fault_generation_q + 1'b1;
                analysis_valid_q <= 1'b0;
            end else if (analysis_complete_i && analysis_generation_i == fault_generation_q) begin
                analysis_generation_q <= analysis_generation_i;
                analysis_valid_q <= 1'b1;
            end
        end
    end

    initial begin
        if (RETAINED_COLLECTOR_BITS_PER_SA != 821 || RETAINED_COLLECTOR_BITS_4SA != 3284)
            $fatal(1, "S1G-A2G retained collector packing width drift");
    end
endmodule

`default_nettype wire
