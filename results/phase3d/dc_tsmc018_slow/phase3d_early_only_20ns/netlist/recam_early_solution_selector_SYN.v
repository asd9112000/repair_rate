/////////////////////////////////////////////////////////////
// Created by: Synopsys DC Expert(TM) in wire load mode
// Version   : W-2024.09-SP2
// Date      : Thu Sep 10 11:53:31 2026
/////////////////////////////////////////////////////////////


module recam_early_solution_selector ( config_pattern_map_i, 
        early_solution_valid_o, selected_map_index_o, selected_config_id_o, 
        selected_pattern_id_o, selected_candidate_valid_o, 
        selected_repairable_o );
  input [79:0] config_pattern_map_i;
  output [1:0] selected_map_index_o;
  output [2:0] selected_config_id_o;
  output [3:0] selected_pattern_id_o;
  output [9:0] selected_candidate_valid_o;
  output early_solution_valid_o, selected_repairable_o;
  wire   n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12, n13, n14, n15, n16,
         n17, n18, n19, n20, n21, n22, n23, n24, n25, n26, n27, n28, n29, n30,
         n31, n32, n33, n34, n35, n36, n37, n38, n39, n40, n41, n42, n43, n44,
         n45, n46, n47, n48, n49, n50;

  INVX1 U68 ( .A(n8), .Y(n4) );
  NAND3X1 U69 ( .A(config_pattern_map_i[4]), .B(config_pattern_map_i[0]), .C(
        config_pattern_map_i[5]), .Y(n8) );
  BUFX3 U70 ( .A(n4), .Y(n48) );
  NAND4X1 U71 ( .A(config_pattern_map_i[25]), .B(config_pattern_map_i[24]), 
        .C(config_pattern_map_i[20]), .D(n8), .Y(n7) );
  BUFX3 U72 ( .A(n4), .Y(n49) );
  NAND4X1 U73 ( .A(n5), .B(n6), .C(n7), .D(n8), .Y(selected_repairable_o) );
  NAND2X1 U74 ( .A(n41), .B(n42), .Y(selected_candidate_valid_o[0]) );
  NAND2X1 U75 ( .A(n39), .B(n40), .Y(selected_candidate_valid_o[1]) );
  NAND2X1 U76 ( .A(n37), .B(n38), .Y(selected_candidate_valid_o[2]) );
  NAND2X1 U77 ( .A(n35), .B(n36), .Y(selected_candidate_valid_o[3]) );
  NAND2X1 U78 ( .A(n33), .B(n34), .Y(selected_candidate_valid_o[4]) );
  NAND2X1 U79 ( .A(n31), .B(n32), .Y(selected_candidate_valid_o[5]) );
  NAND2X1 U80 ( .A(n29), .B(n30), .Y(selected_candidate_valid_o[6]) );
  NAND2X1 U81 ( .A(n27), .B(n28), .Y(selected_candidate_valid_o[7]) );
  NAND2X1 U82 ( .A(n25), .B(n26), .Y(selected_candidate_valid_o[8]) );
  NAND2X1 U83 ( .A(n23), .B(n24), .Y(selected_candidate_valid_o[9]) );
  NAND2X1 U84 ( .A(n15), .B(n16), .Y(selected_pattern_id_o[0]) );
  NAND2X1 U85 ( .A(n13), .B(n14), .Y(selected_pattern_id_o[1]) );
  NAND2X1 U86 ( .A(n11), .B(n12), .Y(selected_pattern_id_o[2]) );
  NAND2X1 U87 ( .A(n9), .B(n10), .Y(selected_pattern_id_o[3]) );
  NAND2X1 U88 ( .A(n21), .B(n22), .Y(selected_config_id_o[0]) );
  NAND2X1 U89 ( .A(n19), .B(n20), .Y(selected_config_id_o[1]) );
  NAND2X1 U90 ( .A(n17), .B(n18), .Y(selected_config_id_o[2]) );
  NAND2X1 U91 ( .A(n5), .B(n7), .Y(selected_map_index_o[0]) );
  NAND2X1 U92 ( .A(n6), .B(n5), .Y(selected_map_index_o[1]) );
  OR3XL U93 ( .A(selected_map_index_o[1]), .B(n49), .C(n47), .Y(
        early_solution_valid_o) );
  NAND4X1 U94 ( .A(config_pattern_map_i[45]), .B(config_pattern_map_i[44]), 
        .C(n44), .D(config_pattern_map_i[40]), .Y(n6) );
  INVX1 U95 ( .A(n6), .Y(n2) );
  INVX1 U96 ( .A(n5), .Y(n1) );
  INVXL U97 ( .A(n6), .Y(n45) );
  INVXL U98 ( .A(n5), .Y(n46) );
  AOI22XL U99 ( .A0(config_pattern_map_i[10]), .A1(n48), .B0(
        config_pattern_map_i[70]), .B1(n46), .Y(n41) );
  AOI22XL U100 ( .A0(config_pattern_map_i[11]), .A1(n48), .B0(
        config_pattern_map_i[71]), .B1(n46), .Y(n39) );
  AOI22XL U101 ( .A0(config_pattern_map_i[12]), .A1(n48), .B0(
        config_pattern_map_i[72]), .B1(n46), .Y(n37) );
  AOI22XL U102 ( .A0(config_pattern_map_i[13]), .A1(n48), .B0(
        config_pattern_map_i[73]), .B1(n46), .Y(n35) );
  AOI22XL U103 ( .A0(config_pattern_map_i[14]), .A1(n48), .B0(
        config_pattern_map_i[74]), .B1(n46), .Y(n33) );
  AOI22XL U104 ( .A0(config_pattern_map_i[15]), .A1(n48), .B0(
        config_pattern_map_i[75]), .B1(n46), .Y(n31) );
  AOI22XL U105 ( .A0(config_pattern_map_i[16]), .A1(n48), .B0(
        config_pattern_map_i[76]), .B1(n46), .Y(n29) );
  AOI22XL U106 ( .A0(config_pattern_map_i[17]), .A1(n48), .B0(
        config_pattern_map_i[77]), .B1(n1), .Y(n27) );
  AOI22XL U107 ( .A0(config_pattern_map_i[18]), .A1(n48), .B0(
        config_pattern_map_i[78]), .B1(n1), .Y(n25) );
  AOI22XL U108 ( .A0(config_pattern_map_i[19]), .A1(n48), .B0(
        config_pattern_map_i[79]), .B1(n1), .Y(n23) );
  AOI22XL U109 ( .A0(config_pattern_map_i[1]), .A1(n49), .B0(
        config_pattern_map_i[61]), .B1(n1), .Y(n21) );
  AOI22XL U110 ( .A0(config_pattern_map_i[2]), .A1(n49), .B0(
        config_pattern_map_i[62]), .B1(n1), .Y(n19) );
  AOI22XL U111 ( .A0(config_pattern_map_i[3]), .A1(n49), .B0(
        config_pattern_map_i[63]), .B1(n1), .Y(n17) );
  AOI22XL U112 ( .A0(config_pattern_map_i[6]), .A1(n49), .B0(
        config_pattern_map_i[66]), .B1(n1), .Y(n15) );
  AOI22XL U113 ( .A0(config_pattern_map_i[7]), .A1(n49), .B0(
        config_pattern_map_i[67]), .B1(n1), .Y(n13) );
  AOI22XL U114 ( .A0(config_pattern_map_i[8]), .A1(n49), .B0(
        config_pattern_map_i[68]), .B1(n1), .Y(n11) );
  AOI22XL U115 ( .A0(config_pattern_map_i[9]), .A1(n49), .B0(
        config_pattern_map_i[69]), .B1(n1), .Y(n9) );
  NAND4X1 U116 ( .A(config_pattern_map_i[64]), .B(config_pattern_map_i[60]), 
        .C(config_pattern_map_i[65]), .D(n43), .Y(n5) );
  BUFX3 U117 ( .A(n3), .Y(n47) );
  AOI22XL U118 ( .A0(config_pattern_map_i[50]), .A1(n45), .B0(
        config_pattern_map_i[30]), .B1(n47), .Y(n42) );
  AOI22XL U119 ( .A0(config_pattern_map_i[51]), .A1(n2), .B0(
        config_pattern_map_i[31]), .B1(n47), .Y(n40) );
  AOI22XL U120 ( .A0(config_pattern_map_i[52]), .A1(n2), .B0(
        config_pattern_map_i[32]), .B1(n47), .Y(n38) );
  AOI22XL U121 ( .A0(config_pattern_map_i[53]), .A1(n2), .B0(
        config_pattern_map_i[33]), .B1(n47), .Y(n36) );
  AOI22XL U122 ( .A0(config_pattern_map_i[54]), .A1(n2), .B0(
        config_pattern_map_i[34]), .B1(n47), .Y(n34) );
  AOI22XL U123 ( .A0(config_pattern_map_i[55]), .A1(n2), .B0(
        config_pattern_map_i[35]), .B1(n47), .Y(n32) );
  AOI22XL U124 ( .A0(config_pattern_map_i[56]), .A1(n2), .B0(
        config_pattern_map_i[36]), .B1(n47), .Y(n30) );
  AOI22XL U125 ( .A0(config_pattern_map_i[57]), .A1(n2), .B0(
        config_pattern_map_i[37]), .B1(n47), .Y(n28) );
  AOI22XL U126 ( .A0(config_pattern_map_i[58]), .A1(n2), .B0(
        config_pattern_map_i[38]), .B1(n47), .Y(n26) );
  AOI22XL U127 ( .A0(config_pattern_map_i[59]), .A1(n2), .B0(
        config_pattern_map_i[39]), .B1(n50), .Y(n24) );
  AOI22XL U128 ( .A0(config_pattern_map_i[41]), .A1(n45), .B0(
        config_pattern_map_i[21]), .B1(n50), .Y(n22) );
  AOI22XL U129 ( .A0(config_pattern_map_i[42]), .A1(n45), .B0(
        config_pattern_map_i[22]), .B1(n50), .Y(n20) );
  AOI22XL U130 ( .A0(config_pattern_map_i[43]), .A1(n45), .B0(
        config_pattern_map_i[23]), .B1(n50), .Y(n18) );
  AOI22XL U131 ( .A0(config_pattern_map_i[46]), .A1(n45), .B0(
        config_pattern_map_i[26]), .B1(n50), .Y(n16) );
  AOI22XL U132 ( .A0(config_pattern_map_i[47]), .A1(n45), .B0(
        config_pattern_map_i[27]), .B1(n50), .Y(n14) );
  AOI22XL U133 ( .A0(config_pattern_map_i[48]), .A1(n45), .B0(
        config_pattern_map_i[28]), .B1(n50), .Y(n12) );
  AOI22XL U134 ( .A0(config_pattern_map_i[49]), .A1(n45), .B0(
        config_pattern_map_i[29]), .B1(n50), .Y(n10) );
  NOR3XL U135 ( .A(n45), .B(n49), .C(n50), .Y(n43) );
  NOR2XL U136 ( .A(n49), .B(n50), .Y(n44) );
  BUFX3 U137 ( .A(n3), .Y(n50) );
  INVX1 U138 ( .A(n7), .Y(n3) );
endmodule

