/////////////////////////////////////////////////////////////
// Created by: Synopsys DC Ultra(TM) in wire load mode
// Version   : W-2024.09-SP2
// Date      : Wed Sep  9 20:27:23 2026
/////////////////////////////////////////////////////////////


module recam_2r2c_pattern_analyzer ( matrix_flat_i, cam_overflow_i, 
        candidate_valid_o );
  input [15:0] matrix_flat_i;
  output [5:0] candidate_valid_o;
  input cam_overflow_i;
  wire   n4, n5, n6;

  OR4X2 U11 ( .A(cam_overflow_i), .B(matrix_flat_i[14]), .C(matrix_flat_i[11]), 
        .D(matrix_flat_i[10]), .Y(n4) );
  NOR2XL U12 ( .A(matrix_flat_i[15]), .B(n4), .Y(candidate_valid_o[0]) );
  NOR2XL U13 ( .A(matrix_flat_i[0]), .B(cam_overflow_i), .Y(n6) );
  NOR4BXL U14 ( .AN(n6), .B(matrix_flat_i[12]), .C(matrix_flat_i[15]), .D(
        matrix_flat_i[3]), .Y(candidate_valid_o[3]) );
  NOR2XL U15 ( .A(cam_overflow_i), .B(matrix_flat_i[5]), .Y(n5) );
  NOR4BXL U16 ( .AN(n5), .B(matrix_flat_i[15]), .C(matrix_flat_i[7]), .D(
        matrix_flat_i[13]), .Y(candidate_valid_o[1]) );
  NOR4BXL U17 ( .AN(n6), .B(matrix_flat_i[8]), .C(matrix_flat_i[10]), .D(
        matrix_flat_i[2]), .Y(candidate_valid_o[4]) );
  NOR4BXL U18 ( .AN(n5), .B(matrix_flat_i[9]), .C(matrix_flat_i[10]), .D(
        matrix_flat_i[6]), .Y(candidate_valid_o[2]) );
  NOR4BXL U19 ( .AN(n6), .B(matrix_flat_i[4]), .C(matrix_flat_i[5]), .D(
        matrix_flat_i[1]), .Y(candidate_valid_o[5]) );
endmodule

