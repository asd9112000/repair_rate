`default_nettype none

// Replays the selected tagged Hybrid view to reconstruct the RECAM row/column
// dictionaries before PatternID is decoded.  Candidate validity was already
// established by the analyzer, so this final stage does not rebuild its matrix.
// It consumes no CAM state beyond the temporary per-subarray Hybrid delta
// retained until group finalization.
module repair_decode_with_hybrid #(
    parameter integer MAX_K = 5,
    parameter integer NUM_CFG = 7,
    parameter integer ENTRY_NUM = 14,
    parameter integer ROW_W = 10,
    parameter integer COL_W = 10,
    parameter integer PTR_W = $clog2(MAX_K),
    parameter integer PATTERN_ID_W = 4
) (
    input wire [2:0] config_id_i,
    input wire [PATTERN_ID_W-1:0] pattern_id_i,
    input wire [MAX_K-1:0] pivot_valid_i,
    input wire [MAX_K*ROW_W-1:0] pivot_rows_flat_i,
    input wire [MAX_K*COL_W-1:0] pivot_cols_flat_i,
    input wire [ENTRY_NUM-1:0] hybrid_valid_i,
    input wire [ENTRY_NUM*ROW_W-1:0] hybrid_rows_flat_i,
    input wire [ENTRY_NUM*COL_W-1:0] hybrid_cols_flat_i,
    input wire [ENTRY_NUM*PTR_W-1:0] hybrid_ptrs_flat_i,
    input wire [ENTRY_NUM-1:0] hybrid_descriptors_i,
    input wire [ENTRY_NUM*NUM_CFG-1:0] hybrid_cfg_valid_flat_i,
    input wire [NUM_CFG*MAX_K-1:0] row_must_by_cfg_i,
    input wire [NUM_CFG*MAX_K-1:0] col_must_by_cfg_i,
    output reg decode_valid_o,
    output reg [MAX_K-1:0] repair_row_valid_o,
    output reg [MAX_K-1:0] repair_col_valid_o,
    output reg [MAX_K*ROW_W-1:0] repair_rows_flat_o,
    output reg [MAX_K*COL_W-1:0] repair_cols_flat_o
);
    reg [ROW_W-1:0] row_dict [0:MAX_K-1];
    reg [COL_W-1:0] col_dict [0:MAX_K-1];
    reg [MAX_K-1:0] orientation;
    localparam integer COUNT_W = (MAX_K <= 1) ? 1 : $clog2(MAX_K+1);
    reg [2:0] rows, cols, k;
    reg [COUNT_W-1:0] pivot_count, row_count, col_count;
    reg [PTR_W-1:0] ptr;
    reg [COUNT_W-1:0] bits;
    reg [PATTERN_ID_W-1:0] candidate;
    integer h, p, i, mask;
    reg row_match, col_match, extendable, selected_valid;

    always @* begin
        case (config_id_i)
            0: begin rows=2; cols=2; end
            1: begin rows=2; cols=1; end
            2: begin rows=3; cols=2; end
            3: begin rows=3; cols=1; end
            4: begin rows=1; cols=2; end
            5: begin rows=2; cols=3; end
            6: begin rows=1; cols=3; end
            default: begin rows=0; cols=0; end
        endcase
        k=rows+cols;
        pivot_count=0;
        repair_row_valid_o=0;
        repair_col_valid_o=0;
        repair_rows_flat_o=0;
        repair_cols_flat_o=0;
        orientation=0;
        for (p=0;p<MAX_K;p=p+1) begin
            row_dict[p]=0;
            col_dict[p]=0;
            if (p<k && pivot_valid_i[p]) begin
                row_dict[p]=pivot_rows_flat_i[p*ROW_W +: ROW_W];
                col_dict[p]=pivot_cols_flat_i[p*COL_W +: COL_W];
                pivot_count=pivot_count+1;
            end
        end
        row_count=pivot_count;
        col_count=pivot_count;
        extendable=pivot_count<k;
        for (h=0;h<ENTRY_NUM;h=h+1) begin
            ptr=hybrid_ptrs_flat_i[h*PTR_W +: PTR_W];
            if (hybrid_valid_i[h] &&
                hybrid_cfg_valid_flat_i[h*NUM_CFG+config_id_i] &&
                ptr<k &&
                !(hybrid_descriptors_i[h]
                    ? col_must_by_cfg_i[config_id_i*MAX_K+ptr]
                    : row_must_by_cfg_i[config_id_i*MAX_K+ptr])) begin
                row_match=0;
                col_match=0;
                for (i=0;i<MAX_K;i=i+1) begin
                    if (!row_match && i<row_count &&
                        hybrid_rows_flat_i[h*ROW_W +: ROW_W]==row_dict[i]) begin
                        row_match=1;
                    end
                    if (!col_match && i<col_count &&
                        hybrid_cols_flat_i[h*COL_W +: COL_W]==col_dict[i]) begin
                        col_match=1;
                    end
                end
                if (extendable && !col_match && row_match && col_count<k) begin
                    col_dict[col_count]=hybrid_cols_flat_i[h*COL_W +: COL_W];
                    col_count=col_count+1;
                end else if (extendable && !row_match && col_match && row_count<k) begin
                    row_dict[row_count]=hybrid_rows_flat_i[h*ROW_W +: ROW_W];
                    row_count=row_count+1;
                end
                if (row_count>=k && col_count>=k)
                    extendable=0;
            end
        end

        selected_valid=0;
        candidate=1;
        for (mask=0;mask<(1<<MAX_K);mask=mask+1) begin
            bits=0;
            if (mask<(1<<k)) begin
                for (i=0;i<MAX_K;i=i+1)
                    if (i<k && ((mask>>(k-1-i))&1)!=0)
                        bits=bits+1;
                if (bits==cols) begin
                    if (pattern_id_i==candidate) begin
                        selected_valid=1;
                        for (i=0;i<MAX_K;i=i+1)
                            if (i<k)
                                orientation[i]=((mask>>(k-1-i))&1)!=0;
                    end
                    candidate=candidate+1;
                end
            end
        end
        decode_valid_o=selected_valid && pattern_id_i!=0;
        for (i=0;i<MAX_K;i=i+1) begin
            if (decode_valid_o && i<k) begin
                if (!orientation[i] && i<row_count) begin
                    repair_row_valid_o[i]=1;
                    repair_rows_flat_o[i*ROW_W +: ROW_W]=row_dict[i];
                end else if (orientation[i] && i<col_count) begin
                    repair_col_valid_o[i]=1;
                    repair_cols_flat_o[i*COL_W +: COL_W]=col_dict[i];
                end
            end
        end
    end
endmodule

`default_nettype wire
