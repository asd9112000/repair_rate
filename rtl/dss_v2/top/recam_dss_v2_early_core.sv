`default_nettype none
/* verilator lint_off PINCONNECTEMPTY */
// Streaming EARLY control/resource core.  One analyzer result is consumed per
// candidate evaluation; no candidate-map or candidate-history storage exists.
module recam_dss_v2_early_core(
 input wire clk_i,rst_ni,start_i,input wire candidate_solution_valid_i,candidate_repairable_i,
 input wire [3:0] candidate_pattern_id_i,output reg [2:0] current_config_id_o,
 output wire [1:0] current_sa_o,
 output reg busy_o,done_o,group_repairable_o,output reg [3:0] sa_commit_valid_o,
 output wire [11:0] ledger_released_borrower_o,output reg [11:0] selected_config_flat_o,
 output reg [15:0] selected_pattern_flat_o,output reg [7:0] selected_donor_flat_o,
 output reg [3:0] borrow_flat_o,release_flat_o,output reg [1:0] failure_position_o,
 // Core-only diagnostic view used by the deterministic Phase 4E-2 harness.
 // It exposes registered Phase 4C state; it adds neither allocation state nor
 // a production-top interface.
 output wire [3:0] canonical_released_o,canonical_borrowed_o,
 output wire [7:0] canonical_borrower_id_flat_o);
 import dss_v2_types_pkg::*;
 reg [1:0] sa_q,rank_q; wire role=(sa_q==1||sa_q==2); dss_v2_config_descriptor_t desc; wire desc_ok;
 wire [3:0] released,borrowed,bvalid; wire [7:0] bid; wire feasible,borrow,dvalid,rel,rvalid,state_ok; wire [1:0] donor,rres;
 assign current_sa_o=sa_q;
 assign canonical_released_o=released;
 assign canonical_borrowed_o=borrowed;
 assign canonical_borrower_id_flat_o=bid;
 always @* begin if(!role)case(rank_q)0:current_config_id_o=0;1:current_config_id_o=4;2:current_config_id_o=5;default:current_config_id_o=6;endcase else case(rank_q)0:current_config_id_o=0;1:current_config_id_o=2;2:current_config_id_o=1;default:current_config_id_o=3;endcase end
 dss_legacy_config_adapter ca(.legacy_config_id_i(current_config_id_o),.config_descriptor_o(desc),.legacy_config_valid_o(desc_ok),.config_descriptor_i('0),.legacy_config_id_o(),.descriptor_valid_o());
 dss_v2_resource_feasibility f(.sa_id_i(sa_q),.sa_valid_i(1'b1),.config_descriptor_i(desc),.resource_released_i(released),.resource_borrowed_i(borrowed),.resource_state_valid_o(state_ok),.descriptor_legal_o(),.physical_feasible_o(feasible),.borrow_required_o(borrow),.selected_donor_valid_o(dvalid),.selected_donor_resource_o(donor),.release_required_o(rel),.release_resource_valid_o(rvalid),.release_resource_o(rres));
 wire accept=candidate_solution_valid_i&&candidate_repairable_i&&desc_ok&&feasible&&state_ok;
 dss_v2_resource_ledger l(.clk_i(clk_i),.rst_ni(rst_ni),.commit_i(busy_o&&accept),.transaction_valid_i(accept),.requester_sa_i(sa_q),.release_required_i(rel),.release_resource_valid_i(rvalid),.release_resource_id_i(rres),.borrow_required_i(borrow),.selected_donor_valid_i(dvalid),.selected_donor_resource_id_i(donor),.resource_released_o(released),.resource_borrowed_o(borrowed),.borrower_valid_o(bvalid),.borrower_id_flat_o(bid),.commit_accepted_o(),.commit_error_o());
 dss_v2_legacy_ledger_diagnostic_adapter p(.resource_released_i(released),.borrower_valid_i(bvalid),.borrower_id_flat_i(bid),.legacy_ledger_o(ledger_released_borrower_o),.canonical_state_valid_o());
 always @(posedge clk_i) begin done_o<=0;if(!rst_ni)begin busy_o<=0;group_repairable_o<=0;sa_q<=0;rank_q<=0;sa_commit_valid_o<=0;selected_config_flat_o<=0;selected_pattern_flat_o<=0;selected_donor_flat_o<=0;borrow_flat_o<=0;release_flat_o<=0;failure_position_o<=3;end else if(start_i&&!busy_o)begin busy_o<=1;group_repairable_o<=0;sa_q<=0;rank_q<=0;sa_commit_valid_o<=0;end else if(busy_o)if(accept)begin sa_commit_valid_o[sa_q]<=1;selected_config_flat_o[sa_q*3+:3]<=current_config_id_o;selected_pattern_flat_o[sa_q*4+:4]<=candidate_pattern_id_i;selected_donor_flat_o[sa_q*2+:2]<=donor;borrow_flat_o[sa_q]<=borrow;release_flat_o[sa_q]<=rel;if(sa_q==3)begin busy_o<=0;done_o<=1;group_repairable_o<=1;end else begin sa_q<=sa_q+1;rank_q<=0;end end else if(rank_q==3)begin busy_o<=0;done_o<=1;group_repairable_o<=0;failure_position_o<=sa_q;end else rank_q<=rank_q+1;end
endmodule
`default_nettype wire
/* verilator lint_on PINCONNECTEMPTY */
