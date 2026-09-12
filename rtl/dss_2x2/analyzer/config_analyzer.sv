`default_nettype none
module config_analyzer #(
    parameter integer MAX_K=5,HYBRID_ENTRY_NUM=14,ROW_W=10,COL_W=10,PATTERN_ID_W=4,MAX_PATTERNS=10
) (
    input wire [2:0] active_rows_i,input wire [2:0] active_cols_i,input wire [MAX_K-1:0] pivot_valid_i,
    input wire [MAX_K*ROW_W-1:0] pivot_rows_flat_i,input wire [MAX_K*COL_W-1:0] pivot_cols_flat_i,
    input wire [MAX_K-1:0] row_must_i,input wire [MAX_K-1:0] col_must_i,
    input wire [HYBRID_ENTRY_NUM-1:0] hybrid_valid_i,
    input wire [HYBRID_ENTRY_NUM*ROW_W-1:0] hybrid_rows_flat_i,
    input wire [HYBRID_ENTRY_NUM*COL_W-1:0] hybrid_cols_flat_i,input wire shared_storage_overflow_i,
    output reg repairable_o,output reg [MAX_PATTERNS-1:0] candidate_valid_o,
    output reg [PATTERN_ID_W-1:0] lowest_pattern_id_o,output reg [MAX_K*MAX_K-1:0] matrix_flat_o
);
    // Combinational scratch only; no fault payload survives outside the one
    // shared collector.
    reg matrix[0:MAX_K-1][0:MAX_K-1];reg [ROW_W-1:0] rows_dict[0:MAX_K-1];reg [COL_W-1:0] cols_dict[0:MAX_K-1];
    localparam integer COUNT_W=(MAX_K<=1)?1:$clog2(MAX_K+1);
    reg [MAX_K-1:0] pattern;
    reg [2:0] rows,cols,k;
    reg [COUNT_W-1:0] np,nr,nc,ri,ci,bits;
    reg [PATTERN_ID_W-1:0] cand;
    integer h,p,r,c,i,mask;
    reg rm,cm,extendable,valid;
    always @* begin
        rows=active_rows_i;cols=active_cols_i;k=rows+cols;np=0;candidate_valid_o=0;lowest_pattern_id_o=0;repairable_o=0;matrix_flat_o=0;
        for(p=0;p<MAX_K;p=p+1)begin rows_dict[p]=0;cols_dict[p]=0;if(p<k&&pivot_valid_i[p])begin rows_dict[p]=pivot_rows_flat_i[p*ROW_W+:ROW_W];cols_dict[p]=pivot_cols_flat_i[p*COL_W+:COL_W];np=np+1;end for(c=0;c<MAX_K;c=c+1)matrix[p][c]=0;end
        nr=np;nc=np;for(r=0;r<MAX_K;r=r+1)for(c=0;c<MAX_K;c=c+1)if(r<k&&c<k)matrix[r][c]=(r==c&&r<np)||(r<np&&row_must_i[r])||(c<np&&col_must_i[c]);extendable=np<k;
        for(h=0;h<HYBRID_ENTRY_NUM;h=h+1)if(hybrid_valid_i[h])begin
            rm=0;cm=0;ri=0;ci=0;for(i=0;i<MAX_K;i=i+1)begin
                if(!rm&&i<nr&&hybrid_rows_flat_i[h*ROW_W+:ROW_W]==rows_dict[i])begin rm=1;ri=i;end
                if(!cm&&i<nc&&hybrid_cols_flat_i[h*COL_W+:COL_W]==cols_dict[i])begin cm=1;ci=i;end end
            if(!extendable)begin
                if(rm&&cm) matrix[ri][ci]=1;
                else if(rm) begin for(i=0;i<MAX_K;i=i+1) if(i<k) matrix[ri][i]=1; end
                else if(cm) begin for(i=0;i<MAX_K;i=i+1) if(i<k) matrix[i][ci]=1; end
            end
            else if(rm&&cm)matrix[ri][ci]=1;
            else if(rm&&nc<k)begin matrix[ri][nc]=1;cols_dict[nc]=hybrid_cols_flat_i[h*COL_W+:COL_W];nc=nc+1;end
            else if(cm&&nr<k)begin matrix[nr][ci]=1;rows_dict[nr]=hybrid_rows_flat_i[h*ROW_W+:ROW_W];nr=nr+1;end
            else if(rm) begin for(i=0;i<MAX_K;i=i+1) if(i<k) matrix[ri][i]=1; end
            else if(cm) begin for(i=0;i<MAX_K;i=i+1) if(i<k) matrix[i][ci]=1; end
            if(nr>=k&&nc>=k)extendable=0;
        end
        for(r=0;r<MAX_K;r=r+1)for(c=0;c<MAX_K;c=c+1)matrix_flat_o[r*MAX_K+c]=matrix[r][c];cand=0;
        for(mask=0;mask<(1<<MAX_K);mask=mask+1)begin bits=0;pattern=0;if(mask<(1<<k))begin
            for(i=0;i<MAX_K;i=i+1)if(i<k)begin pattern[i]=((mask>>(k-1-i))&1)!=0;if(pattern[i])bits=bits+1;end
            if(bits==cols&&cand<MAX_PATTERNS)begin valid=!shared_storage_overflow_i;
                for(r=0;r<MAX_K;r=r+1)for(c=0;c<MAX_K;c=c+1)if(r<k&&c<k&&matrix[r][c]&&!(!pattern[r]||pattern[c]))valid=0;
                if(valid)begin candidate_valid_o[cand]=1;if(!repairable_o)lowest_pattern_id_o=cand[PATTERN_ID_W-1:0]+1'b1;repairable_o=1;end cand=cand+1;end end end
    end
endmodule
`default_nettype wire
