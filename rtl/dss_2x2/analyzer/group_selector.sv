`default_nettype none
module multi_config_group_selector #(
    parameter integer NUM_SA=4, NUM_CFG=7, PATTERN_ID_W=4,
    parameter integer SPARE_W=3, RS=2, CS=2,
    parameter integer SHARED_ROWS=1, SHARED_COLS=1, MAX_BORROWS=3
) (
    input wire clk_i, input wire rst_ni, input wire start_i,
    input wire [NUM_SA*NUM_CFG*PATTERN_ID_W-1:0] config_pattern_map_i,
    output reg busy_o, output reg done_o, output reg success_o,
    output reg [NUM_SA*3-1:0] selected_configs_o,
    output reg [NUM_SA*PATTERN_ID_W-1:0] selected_patterns_o,
    output reg [11:0] combinations_checked_o,
    output reg [11:0] combinations_pruned_o
);
    reg [2:0] cfg [0:NUM_SA-1];
    reg [NUM_SA*SPARE_W-1:0] demand_rows, demand_cols;
    wire feasible;
    wire [SPARE_W-1:0] unused_br, unused_bc;
    wire [7:0] unused_ur, unused_uc;
    reg all_valid, final_combo, better, lex_less, lex_decided;
    reg [PATTERN_ID_W-1:0] pattern [0:NUM_SA-1];
    reg [5:0] current_borrow, best_borrow;
    integer sa;
    wire [PATTERN_ID_W-1:0] pattern_map [0:NUM_SA-1][0:NUM_CFG-1];
    genvar map_sa,map_cfg;
    generate for(map_sa=0;map_sa<NUM_SA;map_sa=map_sa+1) begin:g_map_sa
        for(map_cfg=0;map_cfg<NUM_CFG;map_cfg=map_cfg+1) begin:g_map_cfg
            assign pattern_map[map_sa][map_cfg]=config_pattern_map_i[(map_sa*NUM_CFG+map_cfg)*PATTERN_ID_W +: PATTERN_ID_W];
        end
    end endgenerate
    function automatic [SPARE_W-1:0] cfg_rows(input [2:0] id);
        case(id) 0,1,5:cfg_rows=3'd2; 2,3:cfg_rows=3'd3; 4,6:cfg_rows=3'd1; default:cfg_rows=3'd0; endcase
    endfunction
    function automatic [SPARE_W-1:0] cfg_cols(input [2:0] id);
        case(id) 0,2,4:cfg_cols=3'd2; 1,3:cfg_cols=3'd1; 5,6:cfg_cols=3'd3; default:cfg_cols=3'd0; endcase
    endfunction
    always @* begin
        all_valid=1'b1; demand_rows=0; demand_cols=0; current_borrow=0;
        for(sa=0;sa<NUM_SA;sa=sa+1) begin
            pattern[sa]=pattern_map[sa][cfg[sa]];
            if(pattern[sa]==0) all_valid=1'b0;
            demand_rows[sa*SPARE_W +: SPARE_W]=cfg_rows(cfg[sa]);
            demand_cols[sa*SPARE_W +: SPARE_W]=cfg_cols(cfg[sa]);
            if(cfg_rows(cfg[sa])>RS) current_borrow=current_borrow+cfg_rows(cfg[sa])-RS;
            if(cfg_cols(cfg[sa])>CS) current_borrow=current_borrow+cfg_cols(cfg[sa])-CS;
        end
        final_combo=1'b1;
        for(sa=0;sa<NUM_SA;sa=sa+1) if(cfg[sa]!=NUM_CFG-1) final_combo=1'b0;
        lex_less=1'b0;lex_decided=1'b0;
        for(sa=0;sa<NUM_SA;sa=sa+1) begin
            if(!lex_decided && cfg[sa] != selected_configs_o[sa*3 +: 3]) begin
                lex_less = cfg[sa] < selected_configs_o[sa*3 +: 3];
                lex_decided = 1'b1;
            end
        end
        better=all_valid && feasible && (!success_o || current_borrow<best_borrow ||
            (current_borrow==best_borrow && lex_less));
    end
    physical_resource_ledger #(.RS(RS),.CS(CS),.SHARED_ROWS(SHARED_ROWS),.SHARED_COLS(SHARED_COLS),
        .MAX_BORROWS(MAX_BORROWS),.SPARE_WIDTH(SPARE_W)) ledger(
        .demand_rows_i(demand_rows),.demand_cols_i(demand_cols),.feasible_o(feasible),
        .borrowed_rows_o(unused_br),.borrowed_cols_o(unused_bc),.borrowed_rows_by_sa_o(),
        .borrowed_cols_by_sa_o(),.used_rows_o(unused_ur),.used_cols_o(unused_uc));
    always @(posedge clk_i) begin
        done_o<=1'b0;
        if(!rst_ni) begin busy_o<=0;success_o<=0;selected_configs_o<=0;selected_patterns_o<=0;
            combinations_checked_o<=0;combinations_pruned_o<=0;best_borrow<=0;
            for(sa=0;sa<NUM_SA;sa=sa+1) cfg[sa]<=0;
        end else if(start_i&&!busy_o) begin busy_o<=1;success_o<=0;selected_configs_o<=0;
            selected_patterns_o<=0;combinations_checked_o<=0;combinations_pruned_o<=0;best_borrow<=0;
            for(sa=0;sa<NUM_SA;sa=sa+1) cfg[sa]<=0;
        end else if(busy_o) begin
            combinations_checked_o<=combinations_checked_o+1'b1;
            if(!all_valid||!feasible) combinations_pruned_o<=combinations_pruned_o+1'b1;
            if(better) begin success_o<=1;best_borrow<=current_borrow;
                for(sa=0;sa<NUM_SA;sa=sa+1) begin
                    selected_configs_o[sa*3 +: 3]<=cfg[sa];
                    selected_patterns_o[sa*PATTERN_ID_W +: PATTERN_ID_W]<=pattern[sa];
                end
            end
            if(final_combo) begin busy_o<=0;done_o<=1; end
            else if(cfg[3]<NUM_CFG-1) cfg[3]<=cfg[3]+1'b1;
            else begin cfg[3]<=0; if(cfg[2]<NUM_CFG-1) cfg[2]<=cfg[2]+1'b1;
                else begin cfg[2]<=0;if(cfg[1]<NUM_CFG-1) cfg[1]<=cfg[1]+1'b1;
                    else begin cfg[1]<=0;cfg[0]<=cfg[0]+1'b1;end end end
        end
    end
endmodule
`default_nettype wire
