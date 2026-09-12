`default_nettype none
module multi_config_early_selector #(
    parameter integer NUM_SA=4, NUM_CFG=7, PATTERN_ID_W=4,
    parameter integer SPARE_W=3, RS=2, CS=2,
    parameter integer SHARED_ROWS=1, SHARED_COLS=1, MAX_BORROWS=3
) (
    input wire clk_i,input wire rst_ni,input wire start_i,
    input wire [NUM_SA*NUM_CFG*PATTERN_ID_W-1:0] config_pattern_map_i,
    output reg busy_o,output reg done_o,output reg success_o,
    output reg [NUM_SA*3-1:0] selected_configs_o,
    output reg [NUM_SA*PATTERN_ID_W-1:0] selected_patterns_o
);
    reg [1:0] active_sa;
    reg [NUM_SA*SPARE_W-1:0] committed_rows,committed_cols;
    wire [NUM_CFG-1:0] candidate_feasible;
    reg found;
    reg [2:0] chosen_cfg;
    reg [7:0] chosen_rank;
    integer cfg_idx;
    wire [PATTERN_ID_W-1:0] pattern_map [0:NUM_SA-1][0:NUM_CFG-1];
    genvar map_sa,map_cfg;
    generate for(map_sa=0;map_sa<NUM_SA;map_sa=map_sa+1) begin:g_map_sa
        for(map_cfg=0;map_cfg<NUM_CFG;map_cfg=map_cfg+1) begin:g_map_cfg
            assign pattern_map[map_sa][map_cfg]=config_pattern_map_i[(map_sa*NUM_CFG+map_cfg)*PATTERN_ID_W +: PATTERN_ID_W];
        end
    end endgenerate
    function automatic [SPARE_W-1:0] cfg_rows(input [2:0] id);
        case(id) 0,1,5:cfg_rows=3'd2;2,3:cfg_rows=3'd3;4,6:cfg_rows=3'd1;default:cfg_rows=3'd0;endcase
    endfunction
    function automatic [SPARE_W-1:0] cfg_cols(input [2:0] id);
        case(id) 0,2,4:cfg_cols=3'd2;1,3:cfg_cols=3'd1;5,6:cfg_cols=3'd3;default:cfg_cols=3'd0;endcase
    endfunction
    function automatic [7:0] cfg_rank(input [2:0] id);
        reg [3:0] borrowed,unused;
        begin
            borrowed=(cfg_rows(id)>RS?cfg_rows(id)-RS:0)+(cfg_cols(id)>CS?cfg_cols(id)-CS:0);
            unused=(RS>cfg_rows(id)?RS-cfg_rows(id):0)+(CS>cfg_cols(id)?CS-cfg_cols(id):0);
            cfg_rank=borrowed*64+(4-unused)*8+id;
        end
    endfunction
    genvar candidate,sa;
    generate for(candidate=0;candidate<NUM_CFG;candidate=candidate+1) begin:g_candidate
        wire [NUM_SA*SPARE_W-1:0] rows,cols;
        wire [SPARE_W-1:0] ubr,ubc; wire [7:0] uur,uuc;
        for(sa=0;sa<NUM_SA;sa=sa+1) begin:g_sa
            assign rows[sa*SPARE_W +: SPARE_W]=(active_sa==sa)?cfg_rows(candidate):committed_rows[sa*SPARE_W +: SPARE_W];
            assign cols[sa*SPARE_W +: SPARE_W]=(active_sa==sa)?cfg_cols(candidate):committed_cols[sa*SPARE_W +: SPARE_W];
        end
        physical_resource_ledger #(.RS(RS),.CS(CS),.SHARED_ROWS(SHARED_ROWS),.SHARED_COLS(SHARED_COLS),
            .MAX_BORROWS(MAX_BORROWS),.SPARE_WIDTH(SPARE_W)) ledger(
            .demand_rows_i(rows),.demand_cols_i(cols),.feasible_o(candidate_feasible[candidate]),
            .borrowed_rows_o(ubr),.borrowed_cols_o(ubc),.borrowed_rows_by_sa_o(),.borrowed_cols_by_sa_o(),
            .used_rows_o(uur),.used_cols_o(uuc));
    end endgenerate
    always @* begin
        found=1'b0;chosen_cfg=0;chosen_rank=255;
        for(cfg_idx=0;cfg_idx<NUM_CFG;cfg_idx=cfg_idx+1) begin
            if(pattern_map[active_sa][cfg_idx]!=0&&
               candidate_feasible[cfg_idx]&&cfg_rank(cfg_idx)<chosen_rank) begin
                found=1'b1;chosen_cfg=cfg_idx[2:0];chosen_rank=cfg_rank(cfg_idx);
            end
        end
    end
    always @(posedge clk_i) begin
        done_o<=1'b0;
        if(!rst_ni) begin busy_o<=0;success_o<=0;active_sa<=0;committed_rows<=0;committed_cols<=0;
            selected_configs_o<=0;selected_patterns_o<=0;
        end else if(start_i&&!busy_o) begin busy_o<=1;success_o<=0;active_sa<=0;committed_rows<=0;committed_cols<=0;
            selected_configs_o<=0;selected_patterns_o<=0;
        end else if(busy_o) begin
            if(!found) begin busy_o<=0;done_o<=1;success_o<=0;end
            else begin
                committed_rows[active_sa*SPARE_W +: SPARE_W]<=cfg_rows(chosen_cfg);
                committed_cols[active_sa*SPARE_W +: SPARE_W]<=cfg_cols(chosen_cfg);
                selected_configs_o[active_sa*3 +: 3]<=chosen_cfg;
                selected_patterns_o[active_sa*PATTERN_ID_W +: PATTERN_ID_W]<=
                    pattern_map[active_sa][chosen_cfg];
                if(active_sa==NUM_SA-1) begin busy_o<=0;done_o<=1;success_o<=1;end
                else active_sa<=active_sa+1'b1;
            end
        end
    end
endmodule
`default_nettype wire
