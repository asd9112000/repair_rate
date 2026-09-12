`default_nettype none

module dss_analyzer_top #(
    parameter integer ROW_W=10,COL_W=10,MAX_FAULTS=12,MAX_K=5,NUM_CFG=7,NUM_SA=4,
    parameter integer PATTERN_ID_W=4,HYBRID_SHARED_ENTRIES=14,
    parameter integer SHARED_ROWS=1,SHARED_COLS=1,MAX_BORROWS=3,
    parameter integer PARALLEL_ORIENTATION=1,
    parameter integer COUNT_W=(MAX_FAULTS<=1)?1:$clog2(MAX_FAULTS+1),
    parameter integer CAM_REUSE_W=ROW_W+COL_W+NUM_CFG
) (
    input wire clk_i,input wire rst_ni,input wire clear_group_i,
    input wire subarray_start_i,input wire [$clog2(NUM_SA)-1:0] subarray_i,
    input wire fault_valid_i,input wire [ROW_W-1:0] fault_row_i,input wire [COL_W-1:0] fault_col_i,
    output wire fault_ready_o,input wire subarray_commit_i,
    input wire policy_group_i,input wire selection_start_i,
    output wire selection_busy_o,output wire selection_done_o,output wire group_success_o,
    output wire [NUM_SA*NUM_CFG*PATTERN_ID_W-1:0] config_pattern_map_o,
    output wire [NUM_SA*3-1:0] selected_configs_o,
    output wire [NUM_SA*PATTERN_ID_W-1:0] selected_patterns_o,
    output wire [11:0] combinations_checked_o,output wire [11:0] combinations_pruned_o,
    output wire [2:0] borrowed_rows_o,output wire [2:0] borrowed_columns_o,
    output wire [7:0] used_rows_o,output wire [7:0] used_columns_o,
    output wire final_program_valid_o,
    output wire [2:0] pending_repair_occupancy_o,
    output wire [NUM_SA*MAX_K-1:0] repair_row_valid_o,
    output wire [NUM_SA*MAX_K-1:0] repair_col_valid_o,
    output wire [NUM_SA*MAX_K*ROW_W-1:0] repair_rows_flat_o,
    output wire [NUM_SA*MAX_K*COL_W-1:0] repair_cols_flat_o,
    output wire [$clog2(HYBRID_SHARED_ENTRIES+1)-1:0] hybrid_occupancy_o,
    output reg [$clog2(HYBRID_SHARED_ENTRIES+1)-1:0] maximum_hybrid_occupancy_o,
    output wire [$clog2(MAX_FAULTS+1)-1:0] cam_reuse_occupancy_o,
    output wire shared_counter_overflow_o,output wire shared_hybrid_overflow_o,
    output wire cam_reuse_overflow_o,
    output wire [$clog2(MAX_K+1)-1:0] shared_pivot_occupancy_o,
    output wire [MAX_K-1:0] shared_pivot_valid_o,
    output wire [MAX_K*ROW_W-1:0] shared_pivot_rows_flat_o,
    output wire [MAX_K*COL_W-1:0] shared_pivot_cols_flat_o
);
    wire collector_clear=clear_group_i||subarray_start_i;
    wire [MAX_K*NUM_CFG-1:0] cfg_pivot_valid;
    wire [$clog2(MAX_K+1)-1:0] pivot_occupancy;
    wire [HYBRID_SHARED_ENTRIES-1:0] hybrid_valid;
    wire [HYBRID_SHARED_ENTRIES*ROW_W-1:0] hybrid_rows;
    wire [HYBRID_SHARED_ENTRIES*COL_W-1:0] hybrid_cols;
    wire [HYBRID_SHARED_ENTRIES*$clog2(MAX_K)-1:0] hybrid_ptrs;
    wire [HYBRID_SHARED_ENTRIES-1:0] hybrid_descriptors;
    wire [HYBRID_SHARED_ENTRIES*NUM_CFG-1:0] hybrid_cfg_valid;
    wire [MAX_FAULTS-1:0] reuse_valid,row_counter_valid,col_counter_valid;
    wire [MAX_FAULTS*ROW_W-1:0] reuse_rows,row_counter_addrs;
    wire [MAX_FAULTS*COL_W-1:0] reuse_cols,col_counter_addrs;
    wire [MAX_FAULTS*NUM_CFG-1:0] reuse_cfg_valid;
    wire [MAX_FAULTS*COUNT_W-1:0] row_counter_counts,col_counter_counts;
    wire counter_overflow,hybrid_overflow,reuse_overflow;
    wire [$clog2(MAX_FAULTS+1)-1:0] unused_fault_count;
    shared_fault_collector #(.ROW_W(ROW_W),.COL_W(COL_W),.MAX_FAULTS(MAX_FAULTS),.MAX_K(MAX_K),
        .NUM_CFG(NUM_CFG),.HYBRID_SHARED_ENTRIES(HYBRID_SHARED_ENTRIES),.CAM_REUSE_ENTRIES(MAX_FAULTS)) collector(
        .clk_i(clk_i),.rst_ni(rst_ni),.clear_i(collector_clear),.fault_valid_i(fault_valid_i),
        .fault_row_i(fault_row_i),.fault_col_i(fault_col_i),.fault_ready_o(fault_ready_o),
        .fault_count_o(unused_fault_count),.pivot_valid_o(shared_pivot_valid_o),
        .pivot_rows_flat_o(shared_pivot_rows_flat_o),.pivot_cols_flat_o(shared_pivot_cols_flat_o),
        .cfg_pivot_valid_o(cfg_pivot_valid),.pivot_occupancy_o(pivot_occupancy),
        .hybrid_valid_o(hybrid_valid),.hybrid_rows_flat_o(hybrid_rows),.hybrid_cols_flat_o(hybrid_cols),
        .hybrid_ptrs_flat_o(hybrid_ptrs),.hybrid_descriptors_o(hybrid_descriptors),
        .hybrid_cfg_valid_flat_o(hybrid_cfg_valid),.hybrid_occupancy_o(hybrid_occupancy_o),
        .cam_reuse_valid_o(reuse_valid),.cam_reuse_rows_flat_o(reuse_rows),.cam_reuse_cols_flat_o(reuse_cols),
        .cam_reuse_cfg_valid_flat_o(reuse_cfg_valid),.cam_reuse_occupancy_o(cam_reuse_occupancy_o),
        .counter_overflow_o(counter_overflow),.hybrid_overflow_o(hybrid_overflow),
        .cam_reuse_overflow_o(reuse_overflow),.row_counter_valid_o(row_counter_valid),
        .row_counter_addrs_flat_o(row_counter_addrs),.row_counter_counts_flat_o(row_counter_counts),
        .col_counter_valid_o(col_counter_valid),.col_counter_addrs_flat_o(col_counter_addrs),
        .col_counter_counts_flat_o(col_counter_counts));
    assign shared_pivot_occupancy_o=pivot_occupancy;
    assign shared_counter_overflow_o=counter_overflow;
    assign shared_hybrid_overflow_o=hybrid_overflow;
    assign cam_reuse_overflow_o=reuse_overflow;

    wire [NUM_CFG*MAX_K-1:0] row_must,col_must;
    wire [MAX_K*COUNT_W-1:0] unused_pivot_row_counts,unused_pivot_col_counts;
    config_must_view #(.MAX_K(MAX_K),.NUM_CFG(NUM_CFG),.ENTRY_NUM(MAX_FAULTS),.ROW_W(ROW_W),.COL_W(COL_W),.COUNT_W(COUNT_W)) must_view(
        .pivot_valid_i(shared_pivot_valid_o),.pivot_rows_flat_i(shared_pivot_rows_flat_o),
        .pivot_cols_flat_i(shared_pivot_cols_flat_o),.row_valid_i(row_counter_valid),
        .row_addrs_flat_i(row_counter_addrs),.row_counts_flat_i(row_counter_counts),
        .col_valid_i(col_counter_valid),.col_addrs_flat_i(col_counter_addrs),.col_counts_flat_i(col_counter_counts),
        .row_must_by_cfg_o(row_must),.col_must_by_cfg_o(col_must),
        .pivot_row_counts_o(unused_pivot_row_counts),.pivot_col_counts_o(unused_pivot_col_counts));
    wire [NUM_CFG*PATTERN_ID_W-1:0] current_patterns;
    wire [NUM_CFG-1:0] unused_config_valid;
    wire result_capture;
    wire [$clog2(NUM_SA)-1:0] result_subarray;
    generate if(PARALLEL_ORIENTATION!=0) begin:g_parallel_orientation
        multi_config_analyzer_bank #(.MAX_K(MAX_K),.NUM_CFG(NUM_CFG),
            .HYBRID_ENTRY_NUM(HYBRID_SHARED_ENTRIES),.ROW_W(ROW_W),.COL_W(COL_W),
            .PATTERN_ID_W(PATTERN_ID_W)) analyzers(
            .cfg_pivot_valid_i(cfg_pivot_valid),.pivot_rows_flat_i(shared_pivot_rows_flat_o),
            .pivot_cols_flat_i(shared_pivot_cols_flat_o),.row_must_by_cfg_i(row_must),.col_must_by_cfg_i(col_must),
            .hybrid_valid_i(hybrid_valid),.hybrid_rows_flat_i(hybrid_rows),.hybrid_cols_flat_i(hybrid_cols),
            .hybrid_ptrs_flat_i(hybrid_ptrs),.hybrid_descriptors_i(hybrid_descriptors),
            .hybrid_cfg_valid_flat_i(hybrid_cfg_valid),.shared_storage_overflow_i(counter_overflow||hybrid_overflow),
            .config_patterns_o(current_patterns),.config_valid_o(unused_config_valid));
        assign result_capture=subarray_commit_i;
        assign result_subarray=subarray_i;
    end else begin:g_area_orientation
        reg [$clog2(NUM_SA)-1:0] area_subarray;
        wire area_busy,area_done;
        always @(posedge clk_i) begin
            if(!rst_ni || clear_group_i)
                area_subarray<=0;
            else if(subarray_commit_i && !area_busy)
                area_subarray<=subarray_i;
        end
        multi_config_analyzer_area_engine #(.MAX_K(MAX_K),.NUM_CFG(NUM_CFG),
            .HYBRID_ENTRY_NUM(HYBRID_SHARED_ENTRIES),.ADDR_W(ROW_W),
            .PATTERN_ID_W(PATTERN_ID_W)) analyzers(
            .clk_i(clk_i),.rst_ni(rst_ni),.start_i(subarray_commit_i),
            .cfg_pivot_valid_i(cfg_pivot_valid),.pivot_rows_flat_i(shared_pivot_rows_flat_o),
            .pivot_cols_flat_i(shared_pivot_cols_flat_o),.row_must_by_cfg_i(row_must),.col_must_by_cfg_i(col_must),
            .hybrid_valid_i(hybrid_valid),.hybrid_rows_flat_i(hybrid_rows),.hybrid_cols_flat_i(hybrid_cols),
            .hybrid_ptrs_flat_i(hybrid_ptrs),.hybrid_descriptors_i(hybrid_descriptors),
            .hybrid_cfg_valid_flat_i(hybrid_cfg_valid),.shared_storage_overflow_i(counter_overflow||hybrid_overflow),
            .busy_o(area_busy),.done_o(area_done),.config_patterns_o(current_patterns),
            .config_valid_o(unused_config_valid));
        assign result_capture=area_done;
        assign result_subarray=area_subarray;
    end endgenerate
    wire [NUM_SA-1:0] subarray_results_valid;
    config_result_bank #(.NUM_SA(NUM_SA),.NUM_CFG(NUM_CFG),.PATTERN_ID_W(PATTERN_ID_W)) result_bank(
        .clk_i(clk_i),.rst_ni(rst_ni),.clear_i(clear_group_i),.capture_i(result_capture),
        .subarray_i(result_subarray),.config_patterns_i(current_patterns),.config_pattern_map_o(config_pattern_map_o),
        .subarray_valid_o(subarray_results_valid));

    reg [NUM_SA*MAX_K-1:0] retained_pivot_valid;
    reg [NUM_SA*MAX_K*ROW_W-1:0] retained_pivot_rows;
    reg [NUM_SA*MAX_K*COL_W-1:0] retained_pivot_cols;
    reg [NUM_SA*MAX_FAULTS-1:0] retained_reuse_valid;
    reg [NUM_SA*MAX_FAULTS*CAM_REUSE_W-1:0] retained_reuse;
    wire [NUM_SA*HYBRID_SHARED_ENTRIES-1:0] retained_hybrid_valid;
    wire [NUM_SA*HYBRID_SHARED_ENTRIES*ROW_W-1:0] retained_hybrid_rows;
    wire [NUM_SA*HYBRID_SHARED_ENTRIES*COL_W-1:0] retained_hybrid_cols;
    wire [NUM_SA*HYBRID_SHARED_ENTRIES*$clog2(MAX_K)-1:0] retained_hybrid_ptrs;
    wire [NUM_SA*HYBRID_SHARED_ENTRIES-1:0] retained_hybrid_descriptors;
    wire [NUM_SA*HYBRID_SHARED_ENTRIES*NUM_CFG-1:0] retained_hybrid_cfg_valid;
    wire [NUM_SA*NUM_CFG*MAX_K-1:0] retained_row_must,retained_col_must;
    hybrid_delta_bank #(.NUM_SA(NUM_SA),.NUM_CFG(NUM_CFG),.MAX_K(MAX_K),
        .ENTRY_NUM(HYBRID_SHARED_ENTRIES),.ROW_W(ROW_W),.COL_W(COL_W)) hybrid_delta(
        .clk_i(clk_i),.rst_ni(rst_ni),.clear_i(clear_group_i),.capture_i(subarray_commit_i),.subarray_i(subarray_i),
        .hybrid_valid_i(hybrid_valid),.hybrid_rows_flat_i(hybrid_rows),.hybrid_cols_flat_i(hybrid_cols),
        .hybrid_ptrs_flat_i(hybrid_ptrs),.hybrid_descriptors_i(hybrid_descriptors),
        .hybrid_cfg_valid_flat_i(hybrid_cfg_valid),.row_must_by_cfg_i(row_must),.col_must_by_cfg_i(col_must),
        .hybrid_valid_o(retained_hybrid_valid),.hybrid_rows_flat_o(retained_hybrid_rows),
        .hybrid_cols_flat_o(retained_hybrid_cols),.hybrid_ptrs_flat_o(retained_hybrid_ptrs),
        .hybrid_descriptors_o(retained_hybrid_descriptors),.hybrid_cfg_valid_flat_o(retained_hybrid_cfg_valid),
        .row_must_by_cfg_o(retained_row_must),.col_must_by_cfg_o(retained_col_must));
    integer entry;
    always @(posedge clk_i) begin
        if(!rst_ni||clear_group_i) begin
            retained_pivot_valid<=0;retained_pivot_rows<=0;retained_pivot_cols<=0;
            retained_reuse_valid<=0;retained_reuse<=0;maximum_hybrid_occupancy_o<=0;
        end else if(subarray_commit_i) begin
            retained_pivot_valid[subarray_i*MAX_K +: MAX_K]<=shared_pivot_valid_o;
            retained_pivot_rows[subarray_i*MAX_K*ROW_W +: MAX_K*ROW_W]<=shared_pivot_rows_flat_o;
            retained_pivot_cols[subarray_i*MAX_K*COL_W +: MAX_K*COL_W]<=shared_pivot_cols_flat_o;
            retained_reuse_valid[subarray_i*MAX_FAULTS +: MAX_FAULTS]<=reuse_valid;
            for(entry=0;entry<MAX_FAULTS;entry=entry+1)
                retained_reuse[(subarray_i*MAX_FAULTS+entry)*CAM_REUSE_W +: CAM_REUSE_W]
                    <= {reuse_cfg_valid[entry*NUM_CFG +: NUM_CFG],
                        reuse_cols[entry*COL_W +: COL_W],reuse_rows[entry*ROW_W +: ROW_W]};
            if(hybrid_occupancy_o>maximum_hybrid_occupancy_o)
                maximum_hybrid_occupancy_o<=hybrid_occupancy_o;
        end
    end

    wire early_busy,early_done,early_success,group_busy,group_done,group_success;
    wire [NUM_SA*3-1:0] early_configs,group_configs;
    wire [NUM_SA*PATTERN_ID_W-1:0] early_patterns,group_patterns;
    multi_config_early_selector #(.NUM_SA(NUM_SA),.NUM_CFG(NUM_CFG),.PATTERN_ID_W(PATTERN_ID_W),
        .SHARED_ROWS(SHARED_ROWS),.SHARED_COLS(SHARED_COLS),.MAX_BORROWS(MAX_BORROWS)) early(
        .clk_i(clk_i),.rst_ni(rst_ni),.start_i(selection_start_i&&!policy_group_i),
        .config_pattern_map_i(config_pattern_map_o),.busy_o(early_busy),.done_o(early_done),.success_o(early_success),
        .selected_configs_o(early_configs),.selected_patterns_o(early_patterns));
    multi_config_group_selector #(.NUM_SA(NUM_SA),.NUM_CFG(NUM_CFG),.PATTERN_ID_W(PATTERN_ID_W),
        .SHARED_ROWS(SHARED_ROWS),.SHARED_COLS(SHARED_COLS),.MAX_BORROWS(MAX_BORROWS)) group_search(
        .clk_i(clk_i),.rst_ni(rst_ni),.start_i(selection_start_i&&policy_group_i),
        .config_pattern_map_i(config_pattern_map_o),.busy_o(group_busy),.done_o(group_done),.success_o(group_success),
        .selected_configs_o(group_configs),.selected_patterns_o(group_patterns),
        .combinations_checked_o(combinations_checked_o),.combinations_pruned_o(combinations_pruned_o));
    assign selection_busy_o=policy_group_i?group_busy:early_busy;
    assign selection_done_o=policy_group_i?group_done:early_done;
    assign group_success_o=policy_group_i?group_success:early_success;
    assign selected_configs_o=policy_group_i?group_configs:early_configs;
    assign selected_patterns_o=policy_group_i?group_patterns:early_patterns;
    wire [NUM_SA*3-1:0] selected_demand_rows,selected_demand_cols;
    genvar resource_sa;
    generate for(resource_sa=0;resource_sa<NUM_SA;resource_sa=resource_sa+1) begin:g_selected_resource
        wire [2:0] decoded_rows,decoded_cols;wire decoded_valid;
        config_resource_decode resource_decode(
            .config_id_i(selected_configs_o[resource_sa*3 +: 3]),.rows_o(decoded_rows),
            .cols_o(decoded_cols),.valid_o(decoded_valid));
        assign selected_demand_rows[resource_sa*3 +: 3]=(group_success_o&&decoded_valid)?decoded_rows:3'd0;
        assign selected_demand_cols[resource_sa*3 +: 3]=(group_success_o&&decoded_valid)?decoded_cols:3'd0;
    end endgenerate
    wire unused_final_feasible;wire [11:0] unused_borrow_rows_by_sa,unused_borrow_cols_by_sa;
    physical_resource_ledger #(.SHARED_ROWS(SHARED_ROWS),.SHARED_COLS(SHARED_COLS),
        .MAX_BORROWS(MAX_BORROWS),.SPARE_WIDTH(3)) final_ledger(
        .demand_rows_i(selected_demand_rows),.demand_cols_i(selected_demand_cols),.feasible_o(unused_final_feasible),
        .borrowed_rows_o(borrowed_rows_o),.borrowed_cols_o(borrowed_columns_o),
        .borrowed_rows_by_sa_o(unused_borrow_rows_by_sa),.borrowed_cols_by_sa_o(unused_borrow_cols_by_sa),
        .used_rows_o(used_rows_o),.used_cols_o(used_columns_o));

    wire pending_valid,pending_payload_ready;
    wire [NUM_SA*3-1:0] pending_configs;
    wire [NUM_SA*PATTERN_ID_W-1:0] pending_patterns;
    wire [NUM_SA*MAX_K-1:0] pending_pivot_valid;
    wire [NUM_SA*MAX_K*ROW_W-1:0] pending_pivot_rows;
    wire [NUM_SA*MAX_K*COL_W-1:0] pending_pivot_cols;
    wire [NUM_SA*HYBRID_SHARED_ENTRIES-1:0] pending_hybrid_valid;
    wire [NUM_SA*HYBRID_SHARED_ENTRIES*ROW_W-1:0] pending_hybrid_rows;
    wire [NUM_SA*HYBRID_SHARED_ENTRIES*COL_W-1:0] pending_hybrid_cols;
    wire [NUM_SA*HYBRID_SHARED_ENTRIES*$clog2(MAX_K)-1:0] pending_hybrid_ptrs;
    wire [NUM_SA*HYBRID_SHARED_ENTRIES-1:0] pending_hybrid_descriptors;
    wire [NUM_SA*HYBRID_SHARED_ENTRIES*NUM_CFG-1:0] pending_hybrid_cfg_valid;
    wire [NUM_SA*NUM_CFG*MAX_K-1:0] pending_row_must,pending_col_must;
    pending_repair_buffer #(.NUM_SA(NUM_SA),.MAX_K(MAX_K),.ROW_W(ROW_W),.COL_W(COL_W),
        .PATTERN_ID_W(PATTERN_ID_W),.CAM_REUSE_ENTRIES(MAX_FAULTS),.CAM_REUSE_W(CAM_REUSE_W),
        .NUM_CFG(NUM_CFG),.HYBRID_ENTRIES(HYBRID_SHARED_ENTRIES)) pending(
        .clk_i(clk_i),.rst_ni(rst_ni),.clear_i(clear_group_i),.capture_group_i(selection_done_o),
        .group_success_i(group_success_o),.selected_configs_i(selected_configs_o),
        .selected_patterns_i(selected_patterns_o),.pivot_valid_i(retained_pivot_valid),
        .pivot_rows_flat_i(retained_pivot_rows),.pivot_cols_flat_i(retained_pivot_cols),
        .cam_reuse_valid_i(retained_reuse_valid),.cam_reuse_flat_i(retained_reuse),
        .hybrid_valid_i(retained_hybrid_valid),.hybrid_rows_flat_i(retained_hybrid_rows),
        .hybrid_cols_flat_i(retained_hybrid_cols),.hybrid_ptrs_flat_i(retained_hybrid_ptrs),
        .hybrid_descriptors_i(retained_hybrid_descriptors),.hybrid_cfg_valid_flat_i(retained_hybrid_cfg_valid),
        .row_must_by_cfg_i(retained_row_must),.col_must_by_cfg_i(retained_col_must),
        .pending_valid_o(pending_valid),.final_program_valid_o(pending_payload_ready),
        .selected_configs_o(pending_configs),.selected_patterns_o(pending_patterns),
        .pivot_valid_o(pending_pivot_valid),.pivot_rows_flat_o(pending_pivot_rows),.pivot_cols_flat_o(pending_pivot_cols),
        .cam_reuse_valid_o(),.cam_reuse_flat_o(),.hybrid_valid_o(pending_hybrid_valid),
        .hybrid_rows_flat_o(pending_hybrid_rows),.hybrid_cols_flat_o(pending_hybrid_cols),
        .hybrid_ptrs_flat_o(pending_hybrid_ptrs),.hybrid_descriptors_o(pending_hybrid_descriptors),
        .hybrid_cfg_valid_flat_o(pending_hybrid_cfg_valid),.row_must_by_cfg_o(pending_row_must),
        .col_must_by_cfg_o(pending_col_must));
    assign pending_repair_occupancy_o=pending_valid?3'd4:3'd0;
    wire unused_decode_busy;
    repair_decode_area_engine #(.NUM_SA(NUM_SA),.MAX_K(MAX_K),.NUM_CFG(NUM_CFG),
        .ENTRY_NUM(HYBRID_SHARED_ENTRIES),.ROW_W(ROW_W),.COL_W(COL_W),
        .PATTERN_ID_W(PATTERN_ID_W)) final_decode(
        .clk_i(clk_i),.rst_ni(rst_ni),.clear_i(clear_group_i),.start_i(pending_payload_ready),
        .selected_configs_i(pending_configs),.selected_patterns_i(pending_patterns),
        .pivot_valid_i(pending_pivot_valid),.pivot_rows_flat_i(pending_pivot_rows),
        .pivot_cols_flat_i(pending_pivot_cols),.hybrid_valid_i(pending_hybrid_valid),
        .hybrid_rows_flat_i(pending_hybrid_rows),.hybrid_cols_flat_i(pending_hybrid_cols),
        .hybrid_ptrs_flat_i(pending_hybrid_ptrs),.hybrid_descriptors_i(pending_hybrid_descriptors),
        .hybrid_cfg_valid_flat_i(pending_hybrid_cfg_valid),.row_must_by_cfg_i(pending_row_must),
        .col_must_by_cfg_i(pending_col_must),.busy_o(unused_decode_busy),
        .final_program_valid_o(final_program_valid_o),.repair_row_valid_o(repair_row_valid_o),
        .repair_col_valid_o(repair_col_valid_o),.repair_rows_flat_o(repair_rows_flat_o),
        .repair_cols_flat_o(repair_cols_flat_o));
    wire unused_status=unused_final_feasible||(|unused_borrow_rows_by_sa)||(|unused_borrow_cols_by_sa)||(|subarray_results_valid);
endmodule

`default_nettype wire
