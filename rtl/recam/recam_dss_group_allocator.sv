`default_nettype none
// Phase 3H/3I greedy sequential allocator. Maps must remain stable while busy.
module recam_dss_group_allocator(
 input wire clk_i,rst_ni,start_i,policy_i,input wire [79:0] map_a_i,map_b_i,map_c_i,map_d_i,
 output reg busy_o,done_o,group_repairable_o,output reg [3:0] sa_commit_valid_o,
 output wire [11:0] ledger_released_borrower_o,output reg [11:0] selected_config_flat_o,
 output reg [15:0] selected_pattern_flat_o,output reg [7:0] selected_map_index_flat_o,
 output reg [7:0] selected_donor_flat_o,output reg [3:0] borrow_flat_o,release_flat_o);
 reg [1:0] current_sa; reg [3:0] released; reg [7:0] borrowers;
 assign ledger_released_borrower_o={borrowers,released};
 reg [79:0] current_map; wire physical_valid,borrow,donor_valid,release_action; wire [2:0] cfg; wire [1:0] idx,donor,release_res; wire [3:0] pat; wire [9:0] cand; wire rep,ledger_valid; wire [3:0] feasible;
 always @* begin case(current_sa) 0:current_map=map_a_i;1:current_map=map_b_i;2:current_map=map_c_i;default:current_map=map_d_i; endcase end
 recam_resource_aware_fallback_selector f(.sa_id_i(current_sa),.policy_i(policy_i),.config_pattern_map_i(current_map),.resource_released_i(released),.resource_borrower_flat_i(borrowers),.ledger_state_valid_o(ledger_valid),.physical_feasible_bitmap_o(feasible),.physical_solution_valid_o(physical_valid),.selected_map_index_o(idx),.selected_config_id_o(cfg),.selected_pattern_id_o(pat),.selected_candidate_valid_o(cand),.selected_repairable_o(rep),.borrow_required_o(borrow),.selected_donor_valid_o(donor_valid),.selected_donor_resource_o(donor),.release_required_o(release_action),.release_resource_o(release_res),.selected_policy_rank_o());
 function [1:0] borrower_code; input [1:0] resource; input [1:0] sa; begin
   if(resource<2) borrower_code=(sa==1)?1:2; else borrower_code=(sa==0)?1:2;
 end endfunction
 always @(posedge clk_i) begin
   done_o<=0;
   if(!rst_ni) begin busy_o<=0;done_o<=0;group_repairable_o<=0;current_sa<=0;released<=0;borrowers<=0;sa_commit_valid_o<=0;selected_config_flat_o<=0;selected_pattern_flat_o<=0;selected_map_index_flat_o<=0;selected_donor_flat_o<=0;borrow_flat_o<=0;release_flat_o<=0; end
   else if(start_i&&!busy_o) begin busy_o<=1;group_repairable_o<=0;current_sa<=0;released<=0;borrowers<=0;sa_commit_valid_o<=0;selected_config_flat_o<=0;selected_pattern_flat_o<=0;selected_map_index_flat_o<=0;selected_donor_flat_o<=0;borrow_flat_o<=0;release_flat_o<=0; end
   else if(busy_o) begin
     if(!physical_valid||!ledger_valid) begin busy_o<=0;group_repairable_o<=0;done_o<=1; end
     else begin
       sa_commit_valid_o[current_sa]<=1; selected_config_flat_o[current_sa*3+:3]<=cfg; selected_pattern_flat_o[current_sa*4+:4]<=pat; selected_map_index_flat_o[current_sa*2+:2]<=idx; selected_donor_flat_o[current_sa*2+:2]<=donor; borrow_flat_o[current_sa]<=borrow; release_flat_o[current_sa]<=release_action;
       if(release_action) released[release_res]<=1;
       if(borrow&&donor_valid) borrowers[donor*2+:2]<=borrower_code(donor,current_sa);
       if(current_sa==3) begin busy_o<=0;done_o<=1;group_repairable_o<=1; end else current_sa<=current_sa+1;
     end
   end
 end
endmodule
