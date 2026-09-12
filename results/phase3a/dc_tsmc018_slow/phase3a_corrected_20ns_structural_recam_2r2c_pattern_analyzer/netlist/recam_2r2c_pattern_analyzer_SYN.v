/////////////////////////////////////////////////////////////
// Created by: Synopsys DC Expert(TM) in wire load mode
// Version   : W-2024.09-SP2
// Date      : Thu Sep 10 03:32:13 2026
/////////////////////////////////////////////////////////////


module recam_2r2c_pattern_analyzer ( matrix_flat_i, cam_overflow_i, 
        candidate_valid_o );
  input [15:0] matrix_flat_i;
  output [5:0] candidate_valid_o;
  input cam_overflow_i;
  wire   n7, n8, n9, n10, n11, n12;

  NOR3X1 U14 ( .A(n7), .B(matrix_flat_i[2]), .C(cam_overflow_i), .Y(
        candidate_valid_o[5]) );
  OR3XL U15 ( .A(matrix_flat_i[6]), .B(matrix_flat_i[7]), .C(matrix_flat_i[3]), 
        .Y(n7) );
  NOR3X1 U16 ( .A(n8), .B(matrix_flat_i[11]), .C(cam_overflow_i), .Y(
        candidate_valid_o[4]) );
  OR3XL U17 ( .A(matrix_flat_i[3]), .B(matrix_flat_i[9]), .C(matrix_flat_i[1]), 
        .Y(n8) );
  NOR3X1 U18 ( .A(n9), .B(matrix_flat_i[13]), .C(cam_overflow_i), .Y(
        candidate_valid_o[3]) );
  OR3XL U19 ( .A(matrix_flat_i[2]), .B(matrix_flat_i[1]), .C(matrix_flat_i[14]), .Y(n9) );
  NOR3X1 U20 ( .A(n10), .B(matrix_flat_i[11]), .C(cam_overflow_i), .Y(
        candidate_valid_o[2]) );
  OR3XL U21 ( .A(matrix_flat_i[8]), .B(matrix_flat_i[7]), .C(matrix_flat_i[4]), 
        .Y(n10) );
  NOR3X1 U22 ( .A(n11), .B(matrix_flat_i[12]), .C(cam_overflow_i), .Y(
        candidate_valid_o[1]) );
  OR3XL U23 ( .A(matrix_flat_i[14]), .B(matrix_flat_i[4]), .C(matrix_flat_i[6]), .Y(n11) );
  NOR3X1 U24 ( .A(n12), .B(matrix_flat_i[12]), .C(cam_overflow_i), .Y(
        candidate_valid_o[0]) );
  OR3XL U25 ( .A(matrix_flat_i[13]), .B(matrix_flat_i[8]), .C(matrix_flat_i[9]), .Y(n12) );
endmodule

