`default_nettype none

// Looks up shared physical counters for each Pivot and derives all ConfigID
// Must bits combinationally.  No count or Must state is replicated.
module config_must_view #(
    parameter integer MAX_K=5,NUM_CFG=7,ENTRY_NUM=12,ROW_W=10,COL_W=10,
    parameter integer COUNT_W=(ENTRY_NUM<=1)?1:$clog2(ENTRY_NUM+1)
) (
    input wire [MAX_K-1:0] pivot_valid_i,
    input wire [MAX_K*ROW_W-1:0] pivot_rows_flat_i,
    input wire [MAX_K*COL_W-1:0] pivot_cols_flat_i,
    input wire [ENTRY_NUM-1:0] row_valid_i,
    input wire [ENTRY_NUM*ROW_W-1:0] row_addrs_flat_i,
    input wire [ENTRY_NUM*COUNT_W-1:0] row_counts_flat_i,
    input wire [ENTRY_NUM-1:0] col_valid_i,
    input wire [ENTRY_NUM*COL_W-1:0] col_addrs_flat_i,
    input wire [ENTRY_NUM*COUNT_W-1:0] col_counts_flat_i,
    output reg [NUM_CFG*MAX_K-1:0] row_must_by_cfg_o,
    output reg [NUM_CFG*MAX_K-1:0] col_must_by_cfg_o,
    output reg [MAX_K*COUNT_W-1:0] pivot_row_counts_o,
    output reg [MAX_K*COUNT_W-1:0] pivot_col_counts_o
);
    integer p,e,cfg,row_count,col_count;
    function automatic integer cfg_rows(input integer id);
        case(id)0,1,5:cfg_rows=2;2,3:cfg_rows=3;4,6:cfg_rows=1;default:cfg_rows=0;endcase
    endfunction
    function automatic integer cfg_cols(input integer id);
        case(id)0,2,4:cfg_cols=2;1,3:cfg_cols=1;5,6:cfg_cols=3;default:cfg_cols=0;endcase
    endfunction
    always @* begin
        row_must_by_cfg_o=0;col_must_by_cfg_o=0;pivot_row_counts_o=0;pivot_col_counts_o=0;
        for(p=0;p<MAX_K;p=p+1) begin
            row_count=0;col_count=0;
            if(pivot_valid_i[p]) begin
                for(e=0;e<ENTRY_NUM;e=e+1) begin
                    if(row_valid_i[e]&&row_addrs_flat_i[e*ROW_W +: ROW_W]==pivot_rows_flat_i[p*ROW_W +: ROW_W])
                        row_count=row_counts_flat_i[e*COUNT_W +: COUNT_W];
                    if(col_valid_i[e]&&col_addrs_flat_i[e*COL_W +: COL_W]==pivot_cols_flat_i[p*COL_W +: COL_W])
                        col_count=col_counts_flat_i[e*COUNT_W +: COUNT_W];
                end
                pivot_row_counts_o[p*COUNT_W +: COUNT_W]=row_count[COUNT_W-1:0];
                pivot_col_counts_o[p*COUNT_W +: COUNT_W]=col_count[COUNT_W-1:0];
                for(cfg=0;cfg<NUM_CFG;cfg=cfg+1) begin
                    row_must_by_cfg_o[cfg*MAX_K+p]=row_count>cfg_cols(cfg);
                    col_must_by_cfg_o[cfg*MAX_K+p]=col_count>cfg_rows(cfg);
                end
            end
        end
    end
endmodule

`default_nettype wire
