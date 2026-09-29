`default_nettype none
module tb_ca_live_group_directed(
 input logic clk_i,rst_ni,state_update_i,input logic[1:0]state_sa_i,input logic test_done_valid_i,input logic[1:0]test_done_sa_i,input logic candidate_valid_i,input logic[3:0]candidate_pattern_id_i,
 output logic scan_active_o,output logic[1:0]active_sa_o,scan_slot_o,output logic[2:0]scan_config_o,output logic[3:0]frozen_o,output logic ready_o,output logic[59:0]store_o,output logic repairable_o
);
 `CA_GROUP_CORE dut(.clk_i(clk_i),.rst_ni(rst_ni),.state_update_i(state_update_i),.state_sa_i(state_sa_i),.test_done_valid_i(test_done_valid_i),.test_done_sa_i(test_done_sa_i),.candidate_valid_i(candidate_valid_i),.candidate_pattern_id_i(candidate_pattern_id_i),.scan_active_o(scan_active_o),.active_sa_o(active_sa_o),.scan_slot_o(scan_slot_o),.scan_config_id_o(scan_config_o),.sa_result_frozen_o(frozen_o),.solution_ready_o(ready_o),.candidate_store_image_o(store_o),.group_repairable_o(repairable_o),.sa_commit_valid_o(),.ledger_released_borrower_o(),.selected_config_flat_o(),.selected_pattern_flat_o(),.selected_donor_flat_o(),.borrow_flat_o(),.release_flat_o(),.failure_position_o());
endmodule
`default_nettype wire
