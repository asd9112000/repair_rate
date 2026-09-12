/////////////////////////////////////////////////////////////
// Created by: Synopsys DC Expert(TM) in wire load mode
// Version   : W-2024.09-SP2
// Date      : Wed Sep  9 17:05:44 2026
/////////////////////////////////////////////////////////////


module recam_2r2c_pattern_analyzer ( matrix_flat_i, cam_overflow_i, 
        candidate_valid_o );
  input [15:0] matrix_flat_i;
  output [5:0] candidate_valid_o;
  input cam_overflow_i;
  wire   n6, n7, n8, n9, n10;

  NOR4BX1 U13 ( .AN(n6), .B(matrix_flat_i[4]), .C(matrix_flat_i[1]), .D(
        matrix_flat_i[0]), .Y(candidate_valid_o[5]) );
  NOR4X1 U14 ( .A(matrix_flat_i[10]), .B(n7), .C(matrix_flat_i[8]), .D(
        matrix_flat_i[2]), .Y(candidate_valid_o[4]) );
  NOR4X1 U15 ( .A(n7), .B(matrix_flat_i[12]), .C(matrix_flat_i[3]), .D(
        matrix_flat_i[15]), .Y(candidate_valid_o[3]) );
  NAND2X1 U16 ( .A(n8), .B(n9), .Y(n7) );
  INVX1 U17 ( .A(matrix_flat_i[0]), .Y(n9) );
  NOR4BX1 U18 ( .AN(n6), .B(matrix_flat_i[9]), .C(matrix_flat_i[6]), .D(
        matrix_flat_i[10]), .Y(candidate_valid_o[2]) );
  NOR4BX1 U19 ( .AN(n6), .B(matrix_flat_i[7]), .C(matrix_flat_i[15]), .D(
        matrix_flat_i[13]), .Y(candidate_valid_o[1]) );
  NOR2X1 U20 ( .A(matrix_flat_i[5]), .B(cam_overflow_i), .Y(n6) );
  NOR4X1 U21 ( .A(n10), .B(matrix_flat_i[11]), .C(matrix_flat_i[15]), .D(
        matrix_flat_i[14]), .Y(candidate_valid_o[0]) );
  NAND2BX1 U22 ( .AN(matrix_flat_i[10]), .B(n8), .Y(n10) );
  INVX1 U23 ( .A(cam_overflow_i), .Y(n8) );
endmodule

