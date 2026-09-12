`default_nettype none
/* verilator lint_off PINCONNECTEMPTY */
/* verilator lint_off UNUSED */
// Test-only parallel candidate evaluation; never instantiated by production RTL.
module dss_v2_early_single_sa_wrapper(
 input dss_v2_types_pkg::dss_v2_sa_id_t sa_id_i,input logic [3:0] local_accept_i,input logic [15:0] pattern_flat_i,
 input logic [3:0] released_i,borrowed_i,input logic [7:0] borrower_id_flat_i,
 output logic selected_valid_o,output logic [2:0] selected_config_o,output logic [3:0] selected_pattern_o,
 output logic donor_valid_o,output logic [1:0] donor_o,output logic release_o,output logic [1:0] release_resource_o,
 output logic [3:0] physical_feasible_o);
 import dss_v2_types_pkg::*;
 logic [2:0] cfg[0:3]; dss_v2_config_descriptor_t desc[0:3]; logic[3:0] feasible,dv,rv; logic[1:0] dn[0:3],rr[0:3]; logic[1:0] pick;
 // Stage 1: role alone determines stable canonical candidate slots.
 always_comb begin
  if(sa_id_i==0||sa_id_i==3) begin cfg[0]=0;cfg[1]=4;cfg[2]=5;cfg[3]=6;end else begin cfg[0]=0;cfg[1]=1;cfg[2]=2;cfg[3]=3;end
 end
 // Stage 2: selection consumes only completed feasibility results.
 always_comb begin
  selected_valid_o=0;pick=0;
  if(local_accept_i[0]&&feasible[0])begin selected_valid_o=1;pick=0;end
  else if((sa_id_i==0||sa_id_i==3)&&local_accept_i[1]&&feasible[1])begin selected_valid_o=1;pick=1;end
  else if(local_accept_i[2]&&feasible[2])begin selected_valid_o=1;pick=2;end
  else if((sa_id_i==1||sa_id_i==2)&&local_accept_i[1]&&feasible[1])begin selected_valid_o=1;pick=1;end
  else if(local_accept_i[3]&&feasible[3])begin selected_valid_o=1;pick=3;end
 selected_config_o=cfg[pick];selected_pattern_o=pattern_flat_i[pick*4+:4];donor_valid_o=dv[pick];donor_o=dn[pick];release_o=rv[pick];release_resource_o=rr[pick];
  physical_feasible_o=feasible;
 end
 genvar g; generate for(g=0;g<4;g=g+1)begin: C
  dss_legacy_config_adapter a(.legacy_config_id_i(cfg[g]),.config_descriptor_o(desc[g]),.legacy_config_valid_o(),.config_descriptor_i('0),.legacy_config_id_o(),.descriptor_valid_o());
  dss_v2_resource_feasibility f(.sa_id_i(sa_id_i),.sa_valid_i(1'b1),.config_descriptor_i(desc[g]),.resource_released_i(released_i),.resource_borrowed_i(borrowed_i),.resource_state_valid_o(),.descriptor_legal_o(),.physical_feasible_o(feasible[g]),.borrow_required_o(),.selected_donor_valid_o(dv[g]),.selected_donor_resource_o(dn[g]),.release_required_o(rv[g]),.release_resource_valid_o(),.release_resource_o(rr[g]));
 end endgenerate
endmodule
`default_nettype wire
/* verilator lint_on UNUSED */
/* verilator lint_on PINCONNECTEMPTY */
