`default_nettype none
/* verilator lint_off UNUSED */
module recam_dss_v2_early_latency_test_top(
 input wire clk_i,rst_ni,start_i,input wire [4:0] pivot_valid_i,
 input wire [44:0] pivot_rows_flat_i,input wire [24:0] pivot_cols_flat_i,
 input wire [4:0] row_gt1_i,row_gt2_i,row_gt3_i,col_gt1_i,col_gt2_i,col_gt3_i,
 input wire [6:0] hybrid_valid_i,input wire [20:0] hybrid_pointer_flat_i,
 input wire [6:0] hybrid_descriptor_i,input wire [62:0] hybrid_differing_flat_i,
 input wire conventional_overflow_i,
 output wire legacy_done_o,legacy_group_repairable_o,output wire [11:0] legacy_ledger_o,
 output wire v2_done_o,v2_group_repairable_o,output wire [11:0] v2_ledger_o
);
 wire legacy_busy; wire v2_busy; wire [3:0] v2_commit,v2_borrow,v2_release;
 wire [11:0] v2_cfg; wire [15:0] v2_pattern; wire [7:0] v2_donor; wire [1:0] v2_failure;
 recam_phase3j_full_early_top legacy(.*,.busy_o(legacy_busy),.done_o(legacy_done_o),
  .group_repairable_o(legacy_group_repairable_o),.ledger_released_borrower_o(legacy_ledger_o));
 recam_dss_v2_early_top v2(.*,.busy_o(v2_busy),.done_o(v2_done_o),
  .group_repairable_o(v2_group_repairable_o),.sa_commit_valid_o(v2_commit),
  .ledger_released_borrower_o(v2_ledger_o),.selected_config_flat_o(v2_cfg),
  .selected_pattern_flat_o(v2_pattern),.selected_donor_flat_o(v2_donor),
  .borrow_flat_o(v2_borrow),.release_flat_o(v2_release),.failure_position_o(v2_failure));
endmodule
`default_nettype wire
/* verilator lint_on UNUSED */
