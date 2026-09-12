`default_nettype none

// Phase 4F-3 GROUP-NoScratch core: collect 16 compressed candidates first,
// then perform frozen greedy A->B->C->D allocation with the live ledger.
module recam_dss_v2_group_core(
 input logic clk_i,rst_ni,start_i,input logic candidate_valid_i,input logic [3:0] candidate_pattern_id_i,
 output logic collection_active_o,allocation_active_o,output logic [1:0] current_sa_o,current_slot_o,
 output logic [2:0] current_config_id_o,output logic [79:0] candidate_store_image_o,
 output logic busy_o,done_o,group_repairable_o,output logic [3:0] sa_commit_valid_o,
 output logic [11:0] ledger_released_borrower_o,output logic [11:0] selected_config_flat_o,
 output logic [15:0] selected_pattern_flat_o,output logic [7:0] selected_donor_flat_o,
 output logic [3:0] borrow_flat_o,release_flat_o,output logic [1:0] failure_position_o
);
 import dss_v2_types_pkg::*;
 typedef enum logic [1:0] {IDLE,COLLECT,ALLOCATE} state_t;
 state_t state_q;
 // The same SA/index pair is reused: [SA, slot] in COLLECT and [SA, rank]
 // in ALLOCATE.  No per-SA candidate-control arrays are retained.
 logic [1:0] sa_q,index_q;
 logic [1:0] priority_slot;
 logic [7:0] priority_slot_flat;
 logic [3:0] priority_valid;
 logic [15:0] priority_pattern;
 dss_v2_config_descriptor_t descriptor; logic [2:0] decoded_config;
 logic [3:0] released,borrowed,bvalid; logic [7:0] borrower_ids;
 logic feasible,borrow,donor_valid,release_required,release_valid,state_valid;
 logic [1:0] donor,release_resource;
 wire stored_valid=priority_valid[index_q]; wire [3:0] stored_pattern=priority_pattern[index_q*4+:4];
 wire accept=(state_q==ALLOCATE)&&stored_valid&&state_valid&&feasible;
 assign collection_active_o=(state_q==COLLECT); assign allocation_active_o=(state_q==ALLOCATE);
 assign current_sa_o=sa_q;
 assign current_slot_o=(state_q==COLLECT)?index_q:priority_slot;
 assign current_config_id_o=decoded_config;
 /* verilator lint_off PINCONNECTEMPTY */
 dss_v2_group_candidate_store store(.clk_i(clk_i),.rst_ni(rst_ni),.write_enable_i(state_q==COLLECT),
  .write_sa_i(sa_q),.write_slot_i(index_q),.write_candidate_valid_i(candidate_valid_i),.write_pattern_id_i(candidate_pattern_id_i),
  .read_sa_i(2'd0),.read_slot_i(2'd0),.read_candidate_valid_o(),.read_pattern_id_o(),.candidate_store_image_o(candidate_store_image_o));
 dss_v2_group_priority_reader pr(.sa_id_i(sa_q),.candidate_store_image_i(candidate_store_image_o),
  .priority_slot_flat_o(priority_slot_flat),.priority_valid_o(priority_valid),.priority_pattern_id_flat_o(priority_pattern));
 always_comb priority_slot = priority_slot_flat[index_q*2 +: 2];
 dss_v2_group_slot_decode dec(.sa_id_i(current_sa_o),.canonical_slot_i(current_slot_o),.legacy_config_id_o(decoded_config),.config_descriptor_o(descriptor));
 dss_v2_resource_feasibility feas(.sa_id_i(sa_q),.sa_valid_i(1'b1),.config_descriptor_i(descriptor),.resource_released_i(released),.resource_borrowed_i(borrowed),.resource_state_valid_o(state_valid),.descriptor_legal_o(),.physical_feasible_o(feasible),.borrow_required_o(borrow),.selected_donor_valid_o(donor_valid),.selected_donor_resource_o(donor),.release_required_o(release_required),.release_resource_valid_o(release_valid),.release_resource_o(release_resource));
 dss_v2_resource_ledger ledger(.clk_i(clk_i),.rst_ni(rst_ni),.commit_i(accept),.transaction_valid_i(accept),.requester_sa_i(sa_q),.release_required_i(release_required),.release_resource_valid_i(release_valid),.release_resource_id_i(release_resource),.borrow_required_i(borrow),.selected_donor_valid_i(donor_valid),.selected_donor_resource_id_i(donor),.resource_released_o(released),.resource_borrowed_o(borrowed),.borrower_valid_o(bvalid),.borrower_id_flat_o(borrower_ids),.commit_accepted_o(),.commit_error_o());
 dss_v2_legacy_ledger_diagnostic_adapter diag(.resource_released_i(released),.borrower_valid_i(bvalid),.borrower_id_flat_i(borrower_ids),.legacy_ledger_o(ledger_released_borrower_o),.canonical_state_valid_o());
 /* verilator lint_on PINCONNECTEMPTY */
 always_ff @(posedge clk_i) begin
  done_o<=0;
  if(!rst_ni) begin state_q<=IDLE;busy_o<=0;group_repairable_o<=0;sa_q<=0;index_q<=0;sa_commit_valid_o<=0;selected_config_flat_o<=0;selected_pattern_flat_o<=0;selected_donor_flat_o<=0;borrow_flat_o<=0;release_flat_o<=0;failure_position_o<=3;end
  else if(start_i&&state_q==IDLE) begin state_q<=COLLECT;busy_o<=1;group_repairable_o<=0;sa_q<=0;index_q<=0;sa_commit_valid_o<=0;selected_config_flat_o<=0;selected_pattern_flat_o<=0;selected_donor_flat_o<=0;borrow_flat_o<=0;release_flat_o<=0;end
  else if(state_q==COLLECT) begin if(index_q==3) begin index_q<=0;if(sa_q==3) begin state_q<=ALLOCATE;sa_q<=0;end else sa_q<=sa_q+1;end else index_q<=index_q+1;end
  else if(state_q==ALLOCATE) begin if(accept) begin sa_commit_valid_o[sa_q]<=1;selected_config_flat_o[sa_q*3+:3]<=decoded_config;selected_pattern_flat_o[sa_q*4+:4]<=stored_pattern;selected_donor_flat_o[sa_q*2+:2]<=donor;borrow_flat_o[sa_q]<=borrow;release_flat_o[sa_q]<=release_required;if(sa_q==3)begin state_q<=IDLE;busy_o<=0;done_o<=1;group_repairable_o<=1;end else begin sa_q<=sa_q+1;index_q<=0;end end else if(index_q==3)begin state_q<=IDLE;busy_o<=0;done_o<=1;group_repairable_o<=0;failure_position_o<=sa_q;end else index_q<=index_q+1;end
 end
endmodule
`default_nettype wire
