`default_nettype none

// Single fault-collection engine reused for all four subarrays.  Address and
// fault payload are stored once; ConfigID-dependent roles are tag masks.
module shared_fault_collector #(
    parameter integer ROW_W=10,COL_W=10,MAX_FAULTS=12,MAX_K=5,NUM_CFG=7,
    parameter integer HYBRID_SHARED_ENTRIES=14,CAM_REUSE_ENTRIES=12,
    parameter integer FAULT_REF_W=(MAX_FAULTS<=1)?1:$clog2(MAX_FAULTS+1),
    parameter integer COUNT_W=(MAX_FAULTS<=1)?1:$clog2(MAX_FAULTS+1),
    parameter integer PIVOT_COUNT_W=$clog2(MAX_K+1)
) (
    input wire clk_i,input wire rst_ni,input wire clear_i,input wire fault_valid_i,
    input wire [ROW_W-1:0] fault_row_i,input wire [COL_W-1:0] fault_col_i,
    output wire fault_ready_o,output reg [FAULT_REF_W-1:0] fault_count_o,
    output wire [MAX_K-1:0] pivot_valid_o,
    output wire [MAX_K*ROW_W-1:0] pivot_rows_flat_o,
    output wire [MAX_K*COL_W-1:0] pivot_cols_flat_o,
    output wire [MAX_K*NUM_CFG-1:0] cfg_pivot_valid_o,
    output wire [PIVOT_COUNT_W-1:0] pivot_occupancy_o,
    output wire [HYBRID_SHARED_ENTRIES-1:0] hybrid_valid_o,
    output wire [HYBRID_SHARED_ENTRIES*ROW_W-1:0] hybrid_rows_flat_o,
    output wire [HYBRID_SHARED_ENTRIES*COL_W-1:0] hybrid_cols_flat_o,
    output wire [HYBRID_SHARED_ENTRIES*$clog2(MAX_K)-1:0] hybrid_ptrs_flat_o,
    output wire [HYBRID_SHARED_ENTRIES-1:0] hybrid_descriptors_o,
    output wire [HYBRID_SHARED_ENTRIES*NUM_CFG-1:0] hybrid_cfg_valid_flat_o,
    output wire [$clog2(HYBRID_SHARED_ENTRIES+1)-1:0] hybrid_occupancy_o,
    output wire [CAM_REUSE_ENTRIES-1:0] cam_reuse_valid_o,
    output wire [CAM_REUSE_ENTRIES*ROW_W-1:0] cam_reuse_rows_flat_o,
    output wire [CAM_REUSE_ENTRIES*COL_W-1:0] cam_reuse_cols_flat_o,
    output wire [CAM_REUSE_ENTRIES*NUM_CFG-1:0] cam_reuse_cfg_valid_flat_o,
    output wire [$clog2(CAM_REUSE_ENTRIES+1)-1:0] cam_reuse_occupancy_o,
    output wire counter_overflow_o,output wire hybrid_overflow_o,output wire cam_reuse_overflow_o,
    output wire [MAX_FAULTS-1:0] row_counter_valid_o,
    output wire [MAX_FAULTS*ROW_W-1:0] row_counter_addrs_flat_o,
    output wire [MAX_FAULTS*COUNT_W-1:0] row_counter_counts_flat_o,
    output wire [MAX_FAULTS-1:0] col_counter_valid_o,
    output wire [MAX_FAULTS*COL_W-1:0] col_counter_addrs_flat_o,
    output wire [MAX_FAULTS*COUNT_W-1:0] col_counter_counts_flat_o
);
    wire pivot_full,pivot_store_overflow,hybrid_full,reuse_full;
    wire accept,new_shared_pivot,new_hybrid,new_reuse;
    reg relation_found,descriptor;
    reg [$clog2(MAX_K)-1:0] relation_ptr;
    reg [NUM_CFG-1:0] hybrid_cfg_mask,reuse_cfg_mask;
    integer p,cfg,k;
    function automatic integer cfg_k(input integer id);
        case(id)0,3,6:cfg_k=4;1,4:cfg_k=3;2,5:cfg_k=5;default:cfg_k=0;endcase
    endfunction
    always @* begin
        relation_found=1'b0;descriptor=1'b0;relation_ptr=0;
        for(p=0;p<MAX_K;p=p+1)
            if(!relation_found&&pivot_valid_o[p]) begin
                if(fault_row_i==pivot_rows_flat_o[p*ROW_W +: ROW_W]) begin
                    relation_found=1'b1;descriptor=1'b0;relation_ptr=p[$clog2(MAX_K)-1:0];
                end else if(fault_col_i==pivot_cols_flat_o[p*COL_W +: COL_W]) begin
                    relation_found=1'b1;descriptor=1'b1;relation_ptr=p[$clog2(MAX_K)-1:0];
                end
            end
        hybrid_cfg_mask=0;reuse_cfg_mask=0;
        for(cfg=0;cfg<NUM_CFG;cfg=cfg+1) begin
            k=cfg_k(cfg);
            if(relation_found&&relation_ptr<k) hybrid_cfg_mask[cfg]=1'b1;
            else if(pivot_occupancy_o>=k) reuse_cfg_mask[cfg]=1'b1;
        end
    end
    assign fault_ready_o=fault_count_o<MAX_FAULTS;
    assign accept=fault_valid_i&&fault_ready_o;
    assign new_shared_pivot=accept&&!relation_found&&!pivot_full;
    assign new_hybrid=accept&&relation_found&&(hybrid_cfg_mask!=0);
    assign new_reuse=accept&&(reuse_cfg_mask!=0);
    always @(posedge clk_i) begin
        if(!rst_ni||clear_i) fault_count_o<={FAULT_REF_W{1'b0}};
        else if(accept) fault_count_o<=fault_count_o+1'b1;
    end
    shared_pivot_cam #(.ROW_W(ROW_W),.COL_W(COL_W),.MAX_K(MAX_K),.NUM_CFG(NUM_CFG)) pivots(
        .clk_i(clk_i),.rst_ni(rst_ni),.clear_i(clear_i),.pivot_write_i(new_shared_pivot),
        .pivot_row_i(fault_row_i),.pivot_col_i(fault_col_i),.full_o(pivot_full),
        .overflow_o(pivot_store_overflow),.occupancy_o(pivot_occupancy_o),.valid_o(pivot_valid_o),
        .rows_flat_o(pivot_rows_flat_o),.cols_flat_o(pivot_cols_flat_o),.cfg_entry_valid_o(cfg_pivot_valid_o));
    shared_fault_counter #(.ENTRY_NUM(MAX_FAULTS),.ROW_W(ROW_W),.COL_W(COL_W),.COUNT_W(COUNT_W)) counters(
        .clk_i(clk_i),.rst_ni(rst_ni),.clear_i(clear_i),.fault_valid_i(accept),.fault_row_i(fault_row_i),
        .fault_col_i(fault_col_i),.overflow_o(counter_overflow_o),.row_valid_o(row_counter_valid_o),
        .row_addrs_flat_o(row_counter_addrs_flat_o),.row_counts_flat_o(row_counter_counts_flat_o),
        .col_valid_o(col_counter_valid_o),.col_addrs_flat_o(col_counter_addrs_flat_o),
        .col_counts_flat_o(col_counter_counts_flat_o));
    tagged_hybrid_store #(.ENTRY_NUM(HYBRID_SHARED_ENTRIES),.FAULT_REF_W(FAULT_REF_W),.ROW_W(ROW_W),
        .COL_W(COL_W),.PIVOT_PTR_W($clog2(MAX_K)),.NUM_CFG(NUM_CFG)) hybrids(
        .clk_i(clk_i),.rst_ni(rst_ni),.clear_i(clear_i),.write_valid_i(new_hybrid),.fault_ref_i(fault_count_o),
        .fault_row_i(fault_row_i),.fault_col_i(fault_col_i),.descriptor_i(descriptor),.pivot_ptr_i(relation_ptr),
        .cfg_valid_i(hybrid_cfg_mask),.full_o(hybrid_full),.overflow_o(hybrid_overflow_o),
        .occupancy_o(hybrid_occupancy_o),.valid_o(hybrid_valid_o),.fault_refs_flat_o(),
        .rows_flat_o(hybrid_rows_flat_o),.cols_flat_o(hybrid_cols_flat_o),.descriptors_o(hybrid_descriptors_o),
        .pivot_ptrs_flat_o(hybrid_ptrs_flat_o),.cfg_valid_flat_o(hybrid_cfg_valid_flat_o));
    cam_reuse_temp_buffer #(.ENTRY_NUM(CAM_REUSE_ENTRIES),.ROW_W(ROW_W),.COL_W(COL_W),.NUM_CFG(NUM_CFG)) reuse(
        .clk_i(clk_i),.rst_ni(rst_ni),.clear_i(clear_i),.write_valid_i(new_reuse),.row_i(fault_row_i),
        .col_i(fault_col_i),.cfg_valid_i(reuse_cfg_mask),.full_o(reuse_full),.overflow_o(cam_reuse_overflow_o),
        .occupancy_o(cam_reuse_occupancy_o),.valid_o(cam_reuse_valid_o),.rows_flat_o(cam_reuse_rows_flat_o),
        .cols_flat_o(cam_reuse_cols_flat_o),.cfg_valid_flat_o(cam_reuse_cfg_valid_flat_o));
endmodule

`default_nettype wire
