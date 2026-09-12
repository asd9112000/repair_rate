`default_nettype none

// One temporary decoder walks the persistent PendingRepairBuffer A -> D.
module repair_decode_area_engine #(
    parameter integer NUM_SA=4,MAX_K=5,NUM_CFG=7,ENTRY_NUM=14,
    parameter integer ROW_W=10,COL_W=10,PATTERN_ID_W=4,
    parameter integer PTR_W=$clog2(MAX_K),
    parameter integer SA_W=(NUM_SA<=1)?1:$clog2(NUM_SA)
) (
    input wire clk_i,input wire rst_ni,input wire clear_i,input wire start_i,
    input wire [NUM_SA*3-1:0] selected_configs_i,
    input wire [NUM_SA*PATTERN_ID_W-1:0] selected_patterns_i,
    input wire [NUM_SA*MAX_K-1:0] pivot_valid_i,
    input wire [NUM_SA*MAX_K*ROW_W-1:0] pivot_rows_flat_i,
    input wire [NUM_SA*MAX_K*COL_W-1:0] pivot_cols_flat_i,
    input wire [NUM_SA*ENTRY_NUM-1:0] hybrid_valid_i,
    input wire [NUM_SA*ENTRY_NUM*ROW_W-1:0] hybrid_rows_flat_i,
    input wire [NUM_SA*ENTRY_NUM*COL_W-1:0] hybrid_cols_flat_i,
    input wire [NUM_SA*ENTRY_NUM*PTR_W-1:0] hybrid_ptrs_flat_i,
    input wire [NUM_SA*ENTRY_NUM-1:0] hybrid_descriptors_i,
    input wire [NUM_SA*ENTRY_NUM*NUM_CFG-1:0] hybrid_cfg_valid_flat_i,
    input wire [NUM_SA*NUM_CFG*MAX_K-1:0] row_must_by_cfg_i,
    input wire [NUM_SA*NUM_CFG*MAX_K-1:0] col_must_by_cfg_i,
    output reg busy_o,output reg final_program_valid_o,
    output reg [NUM_SA*MAX_K-1:0] repair_row_valid_o,
    output reg [NUM_SA*MAX_K-1:0] repair_col_valid_o,
    output reg [NUM_SA*MAX_K*ROW_W-1:0] repair_rows_flat_o,
    output reg [NUM_SA*MAX_K*COL_W-1:0] repair_cols_flat_o
);
    reg [SA_W-1:0] active_sa;
    reg decode_failed;
    wire decode_valid;
    wire [MAX_K-1:0] decoded_row_valid,decoded_col_valid;
    wire [MAX_K*ROW_W-1:0] decoded_rows;
    wire [MAX_K*COL_W-1:0] decoded_cols;

    repair_decode_with_hybrid #(.MAX_K(MAX_K),.NUM_CFG(NUM_CFG),.ENTRY_NUM(ENTRY_NUM),
        .ROW_W(ROW_W),.COL_W(COL_W),.PATTERN_ID_W(PATTERN_ID_W)) decoder(
        .config_id_i(selected_configs_i[active_sa*3 +: 3]),
        .pattern_id_i(selected_patterns_i[active_sa*PATTERN_ID_W +: PATTERN_ID_W]),
        .pivot_valid_i(pivot_valid_i[active_sa*MAX_K +: MAX_K]),
        .pivot_rows_flat_i(pivot_rows_flat_i[active_sa*MAX_K*ROW_W +: MAX_K*ROW_W]),
        .pivot_cols_flat_i(pivot_cols_flat_i[active_sa*MAX_K*COL_W +: MAX_K*COL_W]),
        .hybrid_valid_i(hybrid_valid_i[active_sa*ENTRY_NUM +: ENTRY_NUM]),
        .hybrid_rows_flat_i(hybrid_rows_flat_i[active_sa*ENTRY_NUM*ROW_W +: ENTRY_NUM*ROW_W]),
        .hybrid_cols_flat_i(hybrid_cols_flat_i[active_sa*ENTRY_NUM*COL_W +: ENTRY_NUM*COL_W]),
        .hybrid_ptrs_flat_i(hybrid_ptrs_flat_i[active_sa*ENTRY_NUM*PTR_W +: ENTRY_NUM*PTR_W]),
        .hybrid_descriptors_i(hybrid_descriptors_i[active_sa*ENTRY_NUM +: ENTRY_NUM]),
        .hybrid_cfg_valid_flat_i(hybrid_cfg_valid_flat_i[active_sa*ENTRY_NUM*NUM_CFG +: ENTRY_NUM*NUM_CFG]),
        .row_must_by_cfg_i(row_must_by_cfg_i[active_sa*NUM_CFG*MAX_K +: NUM_CFG*MAX_K]),
        .col_must_by_cfg_i(col_must_by_cfg_i[active_sa*NUM_CFG*MAX_K +: NUM_CFG*MAX_K]),
        .decode_valid_o(decode_valid),.repair_row_valid_o(decoded_row_valid),
        .repair_col_valid_o(decoded_col_valid),.repair_rows_flat_o(decoded_rows),
        .repair_cols_flat_o(decoded_cols));

    always @(posedge clk_i) begin
        final_program_valid_o<=1'b0;
        if(!rst_ni || clear_i) begin
            busy_o<=1'b0;active_sa<=0;decode_failed<=1'b0;
            repair_row_valid_o<=0;repair_col_valid_o<=0;
            repair_rows_flat_o<=0;repair_cols_flat_o<=0;
        end else if(start_i && !busy_o) begin
            busy_o<=1'b1;active_sa<=0;decode_failed<=1'b0;
            repair_row_valid_o<=0;repair_col_valid_o<=0;
            repair_rows_flat_o<=0;repair_cols_flat_o<=0;
        end else if(busy_o) begin
            repair_row_valid_o[active_sa*MAX_K +: MAX_K]<=decoded_row_valid;
            repair_col_valid_o[active_sa*MAX_K +: MAX_K]<=decoded_col_valid;
            repair_rows_flat_o[active_sa*MAX_K*ROW_W +: MAX_K*ROW_W]<=decoded_rows;
            repair_cols_flat_o[active_sa*MAX_K*COL_W +: MAX_K*COL_W]<=decoded_cols;
            decode_failed<=decode_failed || !decode_valid;
            if(active_sa==NUM_SA-1) begin
                busy_o<=1'b0;
                final_program_valid_o<=decode_valid && !decode_failed;
            end else active_sa<=active_sa+1'b1;
        end
    end
endmodule

`default_nettype wire
