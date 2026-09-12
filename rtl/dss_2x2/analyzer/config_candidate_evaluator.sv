`default_nettype none

// Combinational scratch for the AREA engine.  One physical evaluator is reused
// for every candidate of every ConfigID; no fault payload is retained here.
module config_candidate_evaluator #(
    parameter integer MAX_K=5,HYBRID_ENTRY_NUM=14,ROW_W=10,COL_W=10,
    parameter integer CANDIDATE_W=4
) (
    input wire [2:0] active_rows_i,input wire [2:0] active_cols_i,
    input wire [CANDIDATE_W-1:0] candidate_index_i,
    input wire [MAX_K-1:0] pivot_valid_i,
    input wire [MAX_K*ROW_W-1:0] pivot_rows_flat_i,
    input wire [MAX_K*COL_W-1:0] pivot_cols_flat_i,
    input wire [MAX_K-1:0] row_must_i,input wire [MAX_K-1:0] col_must_i,
    input wire [HYBRID_ENTRY_NUM-1:0] hybrid_valid_i,
    input wire [HYBRID_ENTRY_NUM*ROW_W-1:0] hybrid_rows_flat_i,
    input wire [HYBRID_ENTRY_NUM*COL_W-1:0] hybrid_cols_flat_i,
    input wire shared_storage_overflow_i,
    output reg candidate_valid_o
);
    localparam integer COUNT_W=(MAX_K<=1)?1:$clog2(MAX_K+1);
    reg matrix[0:MAX_K-1][0:MAX_K-1];
    reg [ROW_W-1:0] rows_dict[0:MAX_K-1];
    reg [COL_W-1:0] cols_dict[0:MAX_K-1];
    reg [MAX_K-1:0] pattern_mask,pattern;
    reg [2:0] rows,cols,k;
    reg [COUNT_W-1:0] np,nr,nc,ri,ci;
    reg rm,cm,extendable;
    integer h,p,r,c,i;

    always @* begin
        rows=active_rows_i;cols=active_cols_i;k=rows+cols;np=0;
        for(p=0;p<MAX_K;p=p+1) begin
            rows_dict[p]=0;cols_dict[p]=0;
            if(p<k&&pivot_valid_i[p]) begin
                rows_dict[p]=pivot_rows_flat_i[p*ROW_W+:ROW_W];
                cols_dict[p]=pivot_cols_flat_i[p*COL_W+:COL_W];
                np=np+1'b1;
            end
            for(c=0;c<MAX_K;c=c+1) matrix[p][c]=1'b0;
        end
        nr=np;nc=np;
        for(r=0;r<MAX_K;r=r+1)
            for(c=0;c<MAX_K;c=c+1)
                if(r<k&&c<k)
                    matrix[r][c]=(r==c&&r<np)||(r<np&&row_must_i[r])||
                        (c<np&&col_must_i[c]);
        extendable=np<k;
        for(h=0;h<HYBRID_ENTRY_NUM;h=h+1) if(hybrid_valid_i[h]) begin
            rm=0;cm=0;ri=0;ci=0;
            for(i=0;i<MAX_K;i=i+1) begin
                if(!rm&&i<nr&&hybrid_rows_flat_i[h*ROW_W+:ROW_W]==rows_dict[i]) begin
                    rm=1;ri=i;
                end
                if(!cm&&i<nc&&hybrid_cols_flat_i[h*COL_W+:COL_W]==cols_dict[i]) begin
                    cm=1;ci=i;
                end
            end
            if(!extendable) begin
                if(rm&&cm) matrix[ri][ci]=1'b1;
                else if(rm) begin for(i=0;i<MAX_K;i=i+1) if(i<k) matrix[ri][i]=1'b1; end
                else if(cm) begin for(i=0;i<MAX_K;i=i+1) if(i<k) matrix[i][ci]=1'b1; end
            end else if(rm&&cm) matrix[ri][ci]=1'b1;
            else if(rm&&nc<k) begin
                matrix[ri][nc]=1'b1;cols_dict[nc]=hybrid_cols_flat_i[h*COL_W+:COL_W];nc=nc+1'b1;
            end else if(cm&&nr<k) begin
                matrix[nr][ci]=1'b1;rows_dict[nr]=hybrid_rows_flat_i[h*ROW_W+:ROW_W];nr=nr+1'b1;
            end else if(rm) begin for(i=0;i<MAX_K;i=i+1) if(i<k) matrix[ri][i]=1'b1; end
            else if(cm) begin for(i=0;i<MAX_K;i=i+1) if(i<k) matrix[i][ci]=1'b1; end
            if(nr>=k&&nc>=k) extendable=1'b0;
        end

        pattern_mask=0;
        case({rows,cols})
            {3'd2,3'd1}: case(candidate_index_i)
                0:pattern_mask=5'd1;1:pattern_mask=5'd2;default:pattern_mask=5'd4;
            endcase
            {3'd2,3'd2}: case(candidate_index_i)
                0:pattern_mask=5'd3;1:pattern_mask=5'd5;2:pattern_mask=5'd6;
                3:pattern_mask=5'd9;4:pattern_mask=5'd10;default:pattern_mask=5'd12;
            endcase
            {3'd3,3'd1}: case(candidate_index_i)
                0:pattern_mask=5'd1;1:pattern_mask=5'd2;2:pattern_mask=5'd4;default:pattern_mask=5'd8;
            endcase
            default: case(candidate_index_i)
                0:pattern_mask=5'd3;1:pattern_mask=5'd5;2:pattern_mask=5'd6;
                3:pattern_mask=5'd9;4:pattern_mask=5'd10;5:pattern_mask=5'd12;
                6:pattern_mask=5'd17;7:pattern_mask=5'd18;8:pattern_mask=5'd20;
                default:pattern_mask=5'd24;
            endcase
        endcase
        pattern=0;
        for(i=0;i<MAX_K;i=i+1) if(i<k) pattern[i]=pattern_mask[k-1-i];
        candidate_valid_o=!shared_storage_overflow_i;
        for(r=0;r<MAX_K;r=r+1)
            for(c=0;c<MAX_K;c=c+1)
                if(r<k&&c<k&&matrix[r][c]&&!(!pattern[r]||pattern[c]))
                    candidate_valid_o=1'b0;
    end
endmodule

`default_nettype wire
