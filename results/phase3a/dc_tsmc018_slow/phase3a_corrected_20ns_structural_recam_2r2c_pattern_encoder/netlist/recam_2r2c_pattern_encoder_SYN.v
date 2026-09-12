/////////////////////////////////////////////////////////////
// Created by: Synopsys DC Expert(TM) in wire load mode
// Version   : W-2024.09-SP2
// Date      : Thu Sep 10 03:32:17 2026
/////////////////////////////////////////////////////////////


module recam_2r2c_pattern_encoder ( candidate_valid_i, repairable_o, 
        pattern_id_o );
  input [5:0] candidate_valid_i;
  output [3:0] pattern_id_o;
  output repairable_o;
  wire   n9, n10, n11, n12, n13, n14, n15, n16, n17, n18;
  assign pattern_id_o[3] = 1'b0;

  NAND3X1 U15 ( .A(n9), .B(n10), .C(n11), .Y(repairable_o) );
  INVX1 U16 ( .A(n9), .Y(pattern_id_o[2]) );
  NAND2X1 U17 ( .A(n11), .B(n12), .Y(n9) );
  OAI2BB1X1 U18 ( .A0N(n10), .A1N(candidate_valid_i[3]), .B0(n13), .Y(n12) );
  OAI21XL U19 ( .A0(candidate_valid_i[4]), .A1(candidate_valid_i[5]), .B0(n14), 
        .Y(n13) );
  INVX1 U20 ( .A(candidate_valid_i[0]), .Y(n10) );
  AOI21X1 U21 ( .A0(n11), .A1(n15), .B0(n16), .Y(pattern_id_o[1]) );
  INVX1 U22 ( .A(n14), .Y(n16) );
  NAND2BX1 U23 ( .AN(candidate_valid_i[4]), .B(candidate_valid_i[5]), .Y(n15)
         );
  OAI21XL U24 ( .A0(candidate_valid_i[1]), .A1(n17), .B0(n18), .Y(
        pattern_id_o[0]) );
  AOI31X1 U25 ( .A0(n14), .A1(n11), .A2(candidate_valid_i[4]), .B0(
        candidate_valid_i[0]), .Y(n18) );
  AOI21X1 U26 ( .A0(n11), .A1(candidate_valid_i[3]), .B0(candidate_valid_i[0]), 
        .Y(n14) );
  NOR2X1 U27 ( .A(candidate_valid_i[1]), .B(candidate_valid_i[2]), .Y(n11) );
  INVX1 U28 ( .A(candidate_valid_i[2]), .Y(n17) );
endmodule

