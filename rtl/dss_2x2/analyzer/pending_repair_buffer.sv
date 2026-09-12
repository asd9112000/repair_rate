`default_nettype none
module pending_repair_buffer #(
    parameter integer NUM_SA=4,MAX_K=5,ROW_W=10,COL_W=10,PATTERN_ID_W=4,
    parameter integer CAM_REUSE_ENTRIES=5,
    parameter integer CAM_REUSE_W=ROW_W+COL_W+7,
    parameter integer NUM_CFG=7,
    parameter integer HYBRID_ENTRIES=14,
    parameter integer HYBRID_PTR_W=$clog2(MAX_K)
) (
    input wire clk_i,input wire rst_ni,input wire clear_i,
    input wire capture_group_i,input wire group_success_i,
    input wire [NUM_SA*3-1:0] selected_configs_i,
    input wire [NUM_SA*PATTERN_ID_W-1:0] selected_patterns_i,
    input wire [NUM_SA*MAX_K-1:0] pivot_valid_i,
    input wire [NUM_SA*MAX_K*ROW_W-1:0] pivot_rows_flat_i,
    input wire [NUM_SA*MAX_K*COL_W-1:0] pivot_cols_flat_i,
    input wire [NUM_SA*CAM_REUSE_ENTRIES-1:0] cam_reuse_valid_i,
    input wire [NUM_SA*CAM_REUSE_ENTRIES*CAM_REUSE_W-1:0] cam_reuse_flat_i,
    input wire [NUM_SA*HYBRID_ENTRIES-1:0] hybrid_valid_i,
    input wire [NUM_SA*HYBRID_ENTRIES*ROW_W-1:0] hybrid_rows_flat_i,
    input wire [NUM_SA*HYBRID_ENTRIES*COL_W-1:0] hybrid_cols_flat_i,
    input wire [NUM_SA*HYBRID_ENTRIES*HYBRID_PTR_W-1:0] hybrid_ptrs_flat_i,
    input wire [NUM_SA*HYBRID_ENTRIES-1:0] hybrid_descriptors_i,
    input wire [NUM_SA*HYBRID_ENTRIES*NUM_CFG-1:0] hybrid_cfg_valid_flat_i,
    input wire [NUM_SA*NUM_CFG*MAX_K-1:0] row_must_by_cfg_i,
    input wire [NUM_SA*NUM_CFG*MAX_K-1:0] col_must_by_cfg_i,
    output reg pending_valid_o,output reg final_program_valid_o,
    output reg [NUM_SA*3-1:0] selected_configs_o,
    output reg [NUM_SA*PATTERN_ID_W-1:0] selected_patterns_o,
    output reg [NUM_SA*MAX_K-1:0] pivot_valid_o,
    output reg [NUM_SA*MAX_K*ROW_W-1:0] pivot_rows_flat_o,
    output reg [NUM_SA*MAX_K*COL_W-1:0] pivot_cols_flat_o,
    output reg [NUM_SA*CAM_REUSE_ENTRIES-1:0] cam_reuse_valid_o,
    output reg [NUM_SA*CAM_REUSE_ENTRIES*CAM_REUSE_W-1:0] cam_reuse_flat_o,
    output reg [NUM_SA*HYBRID_ENTRIES-1:0] hybrid_valid_o,
    output reg [NUM_SA*HYBRID_ENTRIES*ROW_W-1:0] hybrid_rows_flat_o,
    output reg [NUM_SA*HYBRID_ENTRIES*COL_W-1:0] hybrid_cols_flat_o,
    output reg [NUM_SA*HYBRID_ENTRIES*HYBRID_PTR_W-1:0] hybrid_ptrs_flat_o,
    output reg [NUM_SA*HYBRID_ENTRIES-1:0] hybrid_descriptors_o,
    output reg [NUM_SA*HYBRID_ENTRIES*NUM_CFG-1:0] hybrid_cfg_valid_flat_o,
    output reg [NUM_SA*NUM_CFG*MAX_K-1:0] row_must_by_cfg_o,
    output reg [NUM_SA*NUM_CFG*MAX_K-1:0] col_must_by_cfg_o
);
    always @(posedge clk_i) begin
        if(!rst_ni||clear_i) begin pending_valid_o<=0;final_program_valid_o<=0;selected_configs_o<=0;
            selected_patterns_o<=0;pivot_valid_o<=0;pivot_rows_flat_o<=0;pivot_cols_flat_o<=0;
            cam_reuse_valid_o<=0;cam_reuse_flat_o<=0;
            hybrid_valid_o<=0;hybrid_rows_flat_o<=0;hybrid_cols_flat_o<=0;
            hybrid_ptrs_flat_o<=0;hybrid_descriptors_o<=0;hybrid_cfg_valid_flat_o<=0;
            row_must_by_cfg_o<=0;col_must_by_cfg_o<=0;
        end else begin
            final_program_valid_o<=0;
            if(capture_group_i) begin
                if(group_success_i) begin pending_valid_o<=1;final_program_valid_o<=1;
                    selected_configs_o<=selected_configs_i;selected_patterns_o<=selected_patterns_i;
                    pivot_valid_o<=pivot_valid_i;pivot_rows_flat_o<=pivot_rows_flat_i;pivot_cols_flat_o<=pivot_cols_flat_i;
                    cam_reuse_valid_o<=cam_reuse_valid_i;cam_reuse_flat_o<=cam_reuse_flat_i;
                    hybrid_valid_o<=hybrid_valid_i;hybrid_rows_flat_o<=hybrid_rows_flat_i;
                    hybrid_cols_flat_o<=hybrid_cols_flat_i;hybrid_ptrs_flat_o<=hybrid_ptrs_flat_i;
                    hybrid_descriptors_o<=hybrid_descriptors_i;hybrid_cfg_valid_flat_o<=hybrid_cfg_valid_flat_i;
                    row_must_by_cfg_o<=row_must_by_cfg_i;col_must_by_cfg_o<=col_must_by_cfg_i;
                end else begin pending_valid_o<=0;selected_configs_o<=0;selected_patterns_o<=0;
                    pivot_valid_o<=0;pivot_rows_flat_o<=0;pivot_cols_flat_o<=0;cam_reuse_valid_o<=0;cam_reuse_flat_o<=0;
                    hybrid_valid_o<=0;hybrid_rows_flat_o<=0;hybrid_cols_flat_o<=0;hybrid_ptrs_flat_o<=0;
                    hybrid_descriptors_o<=0;hybrid_cfg_valid_flat_o<=0;row_must_by_cfg_o<=0;col_must_by_cfg_o<=0;end
            end
        end
    end
endmodule
`default_nettype wire
