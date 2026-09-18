`default_nettype none
// Test-only candidate injector. Production target RTL receives analyzer output;
// this bridge feeds an identical canonical candidate corpus to exactly one core.
module recam_dss_v2_rs3cs3m1_policy_equivalence_top (
 input logic clk_i,rst_ni,start_i,policy_group_i,
 input logic [15:0] candidate_valid_i,input logic [95:0] candidate_pattern_flat_i,
 output logic busy_o,done_o,group_repairable_o,output logic [3:0] sa_commit_valid_o,
 output logic [11:0] ledger_o,selected_config_flat_o,output logic [23:0] selected_pattern_flat_o,
 output logic [7:0] selected_donor_flat_o,output logic [3:0] borrow_flat_o,release_flat_o,
 output logic [1:0] failure_position_o,output logic [1:0] current_sa_o,current_slot_o
);
 logic [1:0] early_sa,early_slot,group_sa,group_slot; logic [2:0] unused_cfg;
 logic early_busy,early_done,early_rep; logic [3:0] early_commit; logic [11:0] early_ledger,early_cfg; logic [23:0] early_pat; logic [7:0] early_donor; logic [3:0] early_borrow,early_release; logic [1:0] early_fail;
 logic group_busy,group_done,group_rep; logic [3:0] group_commit; logic [11:0] group_ledger,group_cfg; logic [23:0] group_pat; logic [7:0] group_donor; logic [3:0] group_borrow,group_release; logic [1:0] group_fail; logic [111:0] unused_store;
 logic early_valid,group_valid; logic [5:0] early_pattern,group_pattern;
 always_comb begin
   early_valid=candidate_valid_i[early_sa*4+early_slot]; early_pattern=candidate_pattern_flat_i[(early_sa*4+early_slot)*6+:6];
   group_valid=candidate_valid_i[group_sa*4+group_slot]; group_pattern=candidate_pattern_flat_i[(group_sa*4+group_slot)*6+:6];
   current_sa_o=policy_group_i?group_sa:early_sa; current_slot_o=policy_group_i?group_slot:early_slot;
   busy_o=policy_group_i?group_busy:early_busy; done_o=policy_group_i?group_done:early_done; group_repairable_o=policy_group_i?group_rep:early_rep;
   sa_commit_valid_o=policy_group_i?group_commit:early_commit; ledger_o=policy_group_i?group_ledger:early_ledger; selected_config_flat_o=policy_group_i?group_cfg:early_cfg; selected_pattern_flat_o=policy_group_i?group_pat:early_pat; selected_donor_flat_o=policy_group_i?group_donor:early_donor; borrow_flat_o=policy_group_i?group_borrow:early_borrow; release_flat_o=policy_group_i?group_release:early_release; failure_position_o=policy_group_i?group_fail:early_fail;
 end
 recam_dss_v2_rs3cs3m1_early_core early(.clk_i(clk_i),.rst_ni(rst_ni),.start_i(start_i&&!policy_group_i),.candidate_solution_valid_i(early_valid),.candidate_repairable_i(early_valid),.candidate_pattern_id_i(early_pattern),.current_sa_o(early_sa),.current_slot_o(early_slot),.current_config_id_o(unused_cfg),.busy_o(early_busy),.done_o(early_done),.group_repairable_o(early_rep),.sa_commit_valid_o(early_commit),.ledger_released_borrower_o(early_ledger),.selected_config_flat_o(early_cfg),.selected_pattern_flat_o(early_pat),.selected_donor_flat_o(early_donor),.borrow_flat_o(early_borrow),.release_flat_o(early_release),.failure_position_o(early_fail));
 recam_dss_v2_rs3cs3m1_group_core group(.clk_i(clk_i),.rst_ni(rst_ni),.start_i(start_i&&policy_group_i),.candidate_valid_i(group_valid),.candidate_pattern_id_i(group_pattern),.collection_active_o(),.allocation_active_o(),.current_sa_o(group_sa),.current_slot_o(group_slot),.current_config_id_o(),.candidate_store_image_o(unused_store),.busy_o(group_busy),.done_o(group_done),.group_repairable_o(group_rep),.sa_commit_valid_o(group_commit),.ledger_released_borrower_o(group_ledger),.selected_config_flat_o(group_cfg),.selected_pattern_flat_o(group_pat),.selected_donor_flat_o(group_donor),.borrow_flat_o(group_borrow),.release_flat_o(group_release),.failure_position_o(group_fail));
endmodule
`default_nettype wire
