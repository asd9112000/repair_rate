`default_nettype none
module recam_phase3d_integration_top(
 input wire clk_i,rst_ni,start_i,sa_role_i,input wire [4:0] pivot_valid_i,
 input wire [44:0] pivot_rows_flat_i,input wire [24:0] pivot_cols_flat_i,
 input wire [4:0] row_gt1_i,row_gt2_i,row_gt3_i,col_gt1_i,col_gt2_i,col_gt3_i,
 input wire [6:0] hybrid_valid_i,input wire [20:0] hybrid_pointer_flat_i,
 input wire [6:0] hybrid_descriptor_i,input wire [62:0] hybrid_differing_flat_i,input wire conventional_overflow_i,
 output wire busy_o,done_o,early_solution_valid_o,output wire [1:0] selected_map_index_o,output wire [2:0] selected_config_id_o,output wire [3:0] selected_pattern_id_o,output wire [9:0] selected_candidate_valid_o);
 wire [79:0] map; wire [2:0] cfg; wire [9:0] cand;wire [3:0] pat;wire sol,rep;
 recam_role_aware_config_scheduler s(.*,.current_config_id_o(cfg),.config_pattern_map_o(map),.current_candidate_valid_o(cand),.current_pattern_id_o(pat),.current_solution_valid_o(sol),.current_repairable_o(rep));
 recam_early_solution_selector e(.config_pattern_map_i(map),.early_solution_valid_o(early_solution_valid_o),.selected_map_index_o(selected_map_index_o),.selected_config_id_o(selected_config_id_o),.selected_pattern_id_o(selected_pattern_id_o),.selected_candidate_valid_o(selected_candidate_valid_o),.selected_repairable_o());
endmodule
