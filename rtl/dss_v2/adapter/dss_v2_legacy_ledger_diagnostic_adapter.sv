`default_nettype none
// Diagnostic-only canonical V2 ledger to frozen Phase 3J 12-bit encoding.
module dss_v2_legacy_ledger_diagnostic_adapter(
 input logic [3:0] resource_released_i,input logic [3:0] borrower_valid_i,input logic [7:0] borrower_id_flat_i,
 output logic [11:0] legacy_ledger_o,output logic canonical_state_valid_o);
 integer r; logic [1:0] id,code;
 always_comb begin
  legacy_ledger_o[3:0]=resource_released_i; legacy_ledger_o[11:4]='0; canonical_state_valid_o=1;
  for(r=0;r<4;r=r+1) begin
   id=borrower_id_flat_i[r*2+:2]; code=2'b00;
   if(borrower_valid_i[r]) begin
    if(!resource_released_i[r]) begin code=2'b11;canonical_state_valid_o=0; end
    else if(r<2) begin if(id==2'd1)code=2'b01; else if(id==2'd2)code=2'b10; else begin code=2'b11;canonical_state_valid_o=0;end end
    else begin if(id==2'd0)code=2'b01; else if(id==2'd3)code=2'b10; else begin code=2'b11;canonical_state_valid_o=0;end end
   end
   legacy_ledger_o[4+r*2+:2]=code;
  end
 end
endmodule
`default_nettype wire
