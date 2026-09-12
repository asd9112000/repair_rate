`default_nettype none
module repair_decode #(
    parameter integer MAX_K=5,ROW_W=10,COL_W=10,PATTERN_ID_W=4
) (
    input wire [2:0] config_id_i,input wire [PATTERN_ID_W-1:0] pattern_id_i,
    input wire [MAX_K-1:0] pivot_valid_i,
    input wire [MAX_K*ROW_W-1:0] pivot_rows_flat_i,
    input wire [MAX_K*COL_W-1:0] pivot_cols_flat_i,
    output reg decode_valid_o,output reg [MAX_K-1:0] repair_row_valid_o,
    output reg [MAX_K-1:0] repair_col_valid_o,
    output reg [MAX_K*ROW_W-1:0] repair_rows_flat_o,
    output reg [MAX_K*COL_W-1:0] repair_cols_flat_o
);
    integer rows,cols,k,mask,index,bits,candidate; reg [MAX_K-1:0] orientation; reg found;
    always @* begin
        case(config_id_i)
            0:begin rows=2;cols=2;end 1:begin rows=2;cols=1;end
            2:begin rows=3;cols=2;end 3:begin rows=3;cols=1;end
            4:begin rows=1;cols=2;end 5:begin rows=2;cols=3;end
            6:begin rows=1;cols=3;end default:begin rows=0;cols=0;end
        endcase
        k=rows+cols;candidate=1;found=1'b0;orientation=0;
        for(mask=0;mask<(1<<MAX_K);mask=mask+1) begin
            bits=0;
            if(mask<(1<<k)) begin
                for(index=0;index<MAX_K;index=index+1) if(index<k&&((mask>>(k-1-index))&1)!=0) bits=bits+1;
                if(bits==cols) begin
                    if(pattern_id_i==candidate&&!found) begin
                        found=1'b1;
                        for(index=0;index<MAX_K;index=index+1) if(index<k) orientation[index]=((mask>>(k-1-index))&1)!=0;
                    end
                    candidate=candidate+1;
                end
            end
        end
        decode_valid_o=found&&pattern_id_i!=0;repair_row_valid_o=0;repair_col_valid_o=0;
        repair_rows_flat_o=0;repair_cols_flat_o=0;
        for(index=0;index<MAX_K;index=index+1) if(index<k&&pivot_valid_i[index]&&decode_valid_o) begin
            if(orientation[index]) begin repair_col_valid_o[index]=1'b1;
                repair_cols_flat_o[index*COL_W +: COL_W]=pivot_cols_flat_i[index*COL_W +: COL_W];end
            else begin repair_row_valid_o[index]=1'b1;
                repair_rows_flat_o[index*ROW_W +: ROW_W]=pivot_rows_flat_i[index*ROW_W +: ROW_W];end
        end
    end
endmodule
`default_nettype wire
