`default_nettype none
/* verilator lint_off PINCONNECTEMPTY */
/* verilator lint_off UNUSED */
module recam_dss_v2_early_top #(parameter integer ROW_ADDR_W=9,COL_ADDR_W=5,DIFF_ADDR_W=9,HYBRID_ENTRIES=7)(
 input wire clk_i,rst_ni,start_i,input wire [4:0] pivot_valid_i,input wire [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,input wire [5*COL_ADDR_W-1:0] pivot_cols_flat_i,input wire [4:0] row_gt1_i,row_gt2_i,row_gt3_i,col_gt1_i,col_gt2_i,col_gt3_i,input wire [HYBRID_ENTRIES-1:0] hybrid_valid_i,input wire [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,input wire [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,input wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,input wire conventional_overflow_i,
 output wire busy_o,done_o,group_repairable_o,output wire [3:0] sa_commit_valid_o,output wire [11:0] ledger_released_borrower_o,output wire [11:0] selected_config_flat_o,output wire [15:0] selected_pattern_flat_o,output wire [7:0] selected_donor_flat_o,output wire [3:0] borrow_flat_o,release_flat_o,output wire [1:0] failure_position_o);
 wire [2:0] cfg; wire [9:0] bitmap; wire [3:0] pat; wire sol,rep;
 // Core-only state diagnostics stay internal to the frozen production top.
 wire [1:0] core_sa; wire [3:0] core_released,core_borrowed; wire [7:0] core_borrower_ids;
 recam_dss_v2_early_core c(.clk_i(clk_i),.rst_ni(rst_ni),.start_i(start_i),.candidate_solution_valid_i(sol),.candidate_repairable_i(rep),.candidate_pattern_id_i(pat),.current_config_id_o(cfg),.current_sa_o(core_sa),.busy_o(busy_o),.done_o(done_o),.group_repairable_o(group_repairable_o),.sa_commit_valid_o(sa_commit_valid_o),.ledger_released_borrower_o(ledger_released_borrower_o),.selected_config_flat_o(selected_config_flat_o),.selected_pattern_flat_o(selected_pattern_flat_o),.selected_donor_flat_o(selected_donor_flat_o),.borrow_flat_o(borrow_flat_o),.release_flat_o(release_flat_o),.failure_position_o(failure_position_o),.canonical_released_o(core_released),.canonical_borrowed_o(core_borrowed),.canonical_borrower_id_flat_o(core_borrower_ids));
 recam_shared_config_analyzer #(.ROW_ADDR_W(ROW_ADDR_W),.COL_ADDR_W(COL_ADDR_W),.DIFF_ADDR_W(DIFF_ADDR_W),.HYBRID_ENTRIES(HYBRID_ENTRIES)) a(.config_id_i(cfg),.pivot_valid_i(pivot_valid_i),.pivot_rows_flat_i(pivot_rows_flat_i),.pivot_cols_flat_i(pivot_cols_flat_i),.row_gt1_i(row_gt1_i),.row_gt2_i(row_gt2_i),.row_gt3_i(row_gt3_i),.col_gt1_i(col_gt1_i),.col_gt2_i(col_gt2_i),.col_gt3_i(col_gt3_i),.hybrid_valid_i(hybrid_valid_i),.hybrid_pointer_flat_i(hybrid_pointer_flat_i),.hybrid_descriptor_i(hybrid_descriptor_i),.hybrid_differing_flat_i(hybrid_differing_flat_i),.conventional_overflow_i(conventional_overflow_i),.candidate_valid_o(bitmap),.pattern_id_o(pat),.solution_valid_o(sol),.repairable_o(rep),.dictionary_overflow_o());
endmodule
`default_nettype wire
/* verilator lint_on UNUSED */
/* verilator lint_on PINCONNECTEMPTY */
