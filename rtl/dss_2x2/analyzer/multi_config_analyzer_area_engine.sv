`default_nettype none

// One canonical candidate analyzer is reused over all seven physical ConfigID
// views. Inputs are snapshotted once; no CAM or physical counter is duplicated.
module multi_config_analyzer_area_engine #(
    parameter integer MAX_K=5,
    parameter integer NUM_CFG=7,
    parameter integer HYBRID_ENTRY_NUM=14,
    parameter integer ADDR_W=10,
    parameter integer PTR_W=$clog2(MAX_K),
    parameter integer PATTERN_ID_W=4
) (
    input wire clk_i,
    input wire rst_ni,
    input wire start_i,
    input wire [MAX_K*NUM_CFG-1:0] cfg_pivot_valid_i,
    input wire [MAX_K*ADDR_W-1:0] pivot_rows_flat_i,
    input wire [MAX_K*ADDR_W-1:0] pivot_cols_flat_i,
    input wire [NUM_CFG*MAX_K-1:0] row_must_by_cfg_i,
    input wire [NUM_CFG*MAX_K-1:0] col_must_by_cfg_i,
    input wire [HYBRID_ENTRY_NUM-1:0] hybrid_valid_i,
    input wire [HYBRID_ENTRY_NUM*ADDR_W-1:0] hybrid_rows_flat_i,
    input wire [HYBRID_ENTRY_NUM*ADDR_W-1:0] hybrid_cols_flat_i,
    input wire [HYBRID_ENTRY_NUM*PTR_W-1:0] hybrid_ptrs_flat_i,
    input wire [HYBRID_ENTRY_NUM-1:0] hybrid_descriptors_i,
    input wire [HYBRID_ENTRY_NUM*NUM_CFG-1:0] hybrid_cfg_valid_flat_i,
    input wire shared_storage_overflow_i,
    output reg busy_o,
    output reg done_o,
    output reg [NUM_CFG*PATTERN_ID_W-1:0] config_patterns_o,
    output reg [NUM_CFG-1:0] config_valid_o
);
    reg [2:0] active_cfg;
    reg [MAX_K*NUM_CFG-1:0] cfg_pivot_valid_reg;
    reg [MAX_K*ADDR_W-1:0] pivot_rows_reg,pivot_cols_reg;
    reg [NUM_CFG*MAX_K-1:0] row_must_reg,col_must_reg;
    reg [HYBRID_ENTRY_NUM-1:0] hybrid_valid_reg,hybrid_descriptors_reg;
    reg [HYBRID_ENTRY_NUM*ADDR_W-1:0] hybrid_rows_reg,hybrid_cols_reg;
    reg [HYBRID_ENTRY_NUM*PTR_W-1:0] hybrid_ptrs_reg;
    reg [HYBRID_ENTRY_NUM*NUM_CFG-1:0] hybrid_cfg_valid_reg;
    reg shared_storage_overflow_reg;

    wire view_transpose=(active_cfg>=3'd4);
    reg [2:0] canonical_rows,canonical_cols;
    always @* begin
        case(active_cfg)
            3'd0: begin canonical_rows=3'd2;canonical_cols=3'd2;end
            3'd1,3'd4: begin canonical_rows=3'd2;canonical_cols=3'd1;end
            3'd2,3'd5: begin canonical_rows=3'd3;canonical_cols=3'd2;end
            default: begin canonical_rows=3'd3;canonical_cols=3'd1;end
        endcase
    end

    wire [MAX_K-1:0] view_pivot_valid,view_row_must,view_col_must;
    wire [MAX_K*ADDR_W-1:0] view_pivot_rows,view_pivot_cols;
    wire [HYBRID_ENTRY_NUM-1:0] view_hybrid_valid;
    wire [HYBRID_ENTRY_NUM*ADDR_W-1:0] view_hybrid_rows,view_hybrid_cols;
    reg [PATTERN_ID_W-1:0] candidate_index,selected_pattern;
    reg candidate_found;
    reg [PATTERN_ID_W-1:0] pattern_count;

    config_logical_view #(.MAX_K(MAX_K),.NUM_CFG(NUM_CFG),
        .HYBRID_ENTRY_NUM(HYBRID_ENTRY_NUM),.ADDR_W(ADDR_W),.PTR_W(PTR_W)) view(
        .config_id_i(active_cfg),.transpose_i(view_transpose),
        .cfg_pivot_valid_i(cfg_pivot_valid_reg),.pivot_rows_flat_i(pivot_rows_reg),
        .pivot_cols_flat_i(pivot_cols_reg),.row_must_by_cfg_i(row_must_reg),
        .col_must_by_cfg_i(col_must_reg),.hybrid_valid_i(hybrid_valid_reg),
        .hybrid_rows_flat_i(hybrid_rows_reg),.hybrid_cols_flat_i(hybrid_cols_reg),
        .hybrid_ptrs_flat_i(hybrid_ptrs_reg),.hybrid_descriptors_i(hybrid_descriptors_reg),
        .hybrid_cfg_valid_flat_i(hybrid_cfg_valid_reg),.pivot_valid_o(view_pivot_valid),
        .pivot_rows_flat_o(view_pivot_rows),.pivot_cols_flat_o(view_pivot_cols),
        .row_must_o(view_row_must),.col_must_o(view_col_must),
        .hybrid_valid_o(view_hybrid_valid),.hybrid_rows_flat_o(view_hybrid_rows),
        .hybrid_cols_flat_o(view_hybrid_cols));

    wire candidate_valid;
    config_candidate_evaluator #(.MAX_K(MAX_K),.HYBRID_ENTRY_NUM(HYBRID_ENTRY_NUM),
        .ROW_W(ADDR_W),.COL_W(ADDR_W),.CANDIDATE_W(PATTERN_ID_W)) analyzer(
        .active_rows_i(canonical_rows),.active_cols_i(canonical_cols),
        .candidate_index_i(candidate_index),
        .pivot_valid_i(view_pivot_valid),.pivot_rows_flat_i(view_pivot_rows),
        .pivot_cols_flat_i(view_pivot_cols),.row_must_i(view_row_must),
        .col_must_i(view_col_must),.hybrid_valid_i(view_hybrid_valid),
        .hybrid_rows_flat_i(view_hybrid_rows),.hybrid_cols_flat_i(view_hybrid_cols),
        .shared_storage_overflow_i(shared_storage_overflow_reg),
        .candidate_valid_o(candidate_valid));

    always @* begin
        case(active_cfg)
            3'd1,3'd4:pattern_count=4'd3;
            3'd0:pattern_count=4'd6;
            3'd3,3'd6:pattern_count=4'd4;
            default:pattern_count=4'd10;
        endcase
    end

    always @(posedge clk_i) begin
        done_o<=1'b0;
        if(!rst_ni) begin
            busy_o<=1'b0;active_cfg<=3'd0;candidate_index<=0;
            selected_pattern<=0;candidate_found<=1'b0;
            config_patterns_o<=0;config_valid_o<=0;
            cfg_pivot_valid_reg<=0;pivot_rows_reg<=0;pivot_cols_reg<=0;
            row_must_reg<=0;col_must_reg<=0;hybrid_valid_reg<=0;
            hybrid_descriptors_reg<=0;hybrid_rows_reg<=0;hybrid_cols_reg<=0;
            hybrid_ptrs_reg<=0;hybrid_cfg_valid_reg<=0;
            shared_storage_overflow_reg<=1'b0;
        end else if(start_i && !busy_o) begin
            busy_o<=1'b1;active_cfg<=3'd0;candidate_index<=0;
            selected_pattern<=0;candidate_found<=1'b0;
            config_patterns_o<=0;config_valid_o<=0;
            cfg_pivot_valid_reg<=cfg_pivot_valid_i;
            pivot_rows_reg<=pivot_rows_flat_i;pivot_cols_reg<=pivot_cols_flat_i;
            row_must_reg<=row_must_by_cfg_i;col_must_reg<=col_must_by_cfg_i;
            hybrid_valid_reg<=hybrid_valid_i;hybrid_descriptors_reg<=hybrid_descriptors_i;
            hybrid_rows_reg<=hybrid_rows_flat_i;hybrid_cols_reg<=hybrid_cols_flat_i;
            hybrid_ptrs_reg<=hybrid_ptrs_flat_i;hybrid_cfg_valid_reg<=hybrid_cfg_valid_flat_i;
            shared_storage_overflow_reg<=shared_storage_overflow_i;
        end else if(busy_o) begin
            if(candidate_valid) begin
                candidate_found<=1'b1;
                if(view_transpose)
                    selected_pattern<=pattern_count-candidate_index;
                else if(!candidate_found)
                    selected_pattern<=candidate_index+1'b1;
            end
            if(candidate_index==pattern_count-1'b1) begin
                config_valid_o[active_cfg]<=candidate_found||candidate_valid;
                if(candidate_valid&&(view_transpose||!candidate_found)) begin
                    if(view_transpose)
                        config_patterns_o[active_cfg*PATTERN_ID_W +: PATTERN_ID_W]
                            <=pattern_count-candidate_index;
                    else config_patterns_o[active_cfg*PATTERN_ID_W +: PATTERN_ID_W]
                            <=candidate_index+1'b1;
                end else config_patterns_o[active_cfg*PATTERN_ID_W +: PATTERN_ID_W]
                        <=selected_pattern;
                candidate_index<=0;selected_pattern<=0;candidate_found<=1'b0;
                if(active_cfg==3'd6) begin busy_o<=1'b0;done_o<=1'b1;end
                else active_cfg<=active_cfg+1'b1;
            end else candidate_index<=candidate_index+1'b1;
        end
    end
endmodule

`default_nettype wire
