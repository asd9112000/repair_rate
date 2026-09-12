/////////////////////////////////////////////////////////////
// Created by: Synopsys DC Expert(TM) in wire load mode
// Version   : W-2024.09-SP2
// Date      : Thu Sep 10 07:52:56 2026
/////////////////////////////////////////////////////////////


module recam_group_compressed_solution_selector ( sa_role_i, 
        config_pattern_map_i, group_solution_valid_o, selected_map_index_o, 
        selected_config_id_o, selected_pattern_id_o, 
        selected_candidate_valid_o, selected_repairable_o, selected_score_o );
  input [79:0] config_pattern_map_i;
  output [1:0] selected_map_index_o;
  output [2:0] selected_config_id_o;
  output [3:0] selected_pattern_id_o;
  output [9:0] selected_candidate_valid_o;
  output [1:0] selected_score_o;
  input sa_role_i;
  output group_solution_valid_o, selected_repairable_o;
  wire   n13, n14, n15, n16, n17, n18, n19, n20, n21, n22, n23, n24, n25, n26,
         n27, n28, n29, n30, n31, n32, n33, n34, n35, n36, n37, n38, n39, n40,
         n41, n42, n43, n44, n45, n46, n47, n48, n49, n50, n51, n52, n53, n54,
         n55, n56, n57, n58, n59, n60, n61, n62, n63, n64, n65, n66, n67, n68,
         n69, n70, n71, n72;

  NAND2X1 U79 ( .A(n16), .B(sa_role_i), .Y(n24) );
  INVX1 U80 ( .A(sa_role_i), .Y(n15) );
  NAND3X1 U81 ( .A(config_pattern_map_i[4]), .B(config_pattern_map_i[0]), .C(
        config_pattern_map_i[5]), .Y(n23) );
  AOI2BB2X1 U82 ( .B0(n15), .B1(n14), .A0N(n14), .A1N(n24), .Y(n18) );
  INVX1 U83 ( .A(n22), .Y(n14) );
  NAND4X1 U84 ( .A(config_pattern_map_i[25]), .B(config_pattern_map_i[24]), 
        .C(config_pattern_map_i[20]), .D(n65), .Y(n36) );
  NAND2X1 U85 ( .A(sa_role_i), .B(n17), .Y(n65) );
  NAND4X1 U86 ( .A(config_pattern_map_i[65]), .B(config_pattern_map_i[64]), 
        .C(config_pattern_map_i[60]), .D(n63), .Y(n19) );
  OAI31X1 U87 ( .A0(n14), .A1(n17), .A2(n16), .B0(n18), .Y(n63) );
  NAND4X1 U88 ( .A(config_pattern_map_i[45]), .B(config_pattern_map_i[44]), 
        .C(config_pattern_map_i[40]), .D(n64), .Y(n22) );
  OAI21XL U89 ( .A0(n17), .A1(n16), .B0(n15), .Y(n64) );
  INVX1 U90 ( .A(n23), .Y(n17) );
  INVX1 U91 ( .A(n36), .Y(n16) );
  AOI22X1 U92 ( .A0(n21), .A1(n22), .B0(n14), .B1(n15), .Y(n20) );
  OAI21XL U93 ( .A0(n16), .A1(n23), .B0(n24), .Y(n21) );
  NAND2X1 U94 ( .A(n18), .B(n19), .Y(selected_score_o[1]) );
  NAND2X1 U95 ( .A(n61), .B(n62), .Y(selected_candidate_valid_o[0]) );
  NAND2X1 U96 ( .A(n59), .B(n60), .Y(selected_candidate_valid_o[1]) );
  NAND2X1 U97 ( .A(n57), .B(n58), .Y(selected_candidate_valid_o[2]) );
  NAND2X1 U98 ( .A(n55), .B(n56), .Y(selected_candidate_valid_o[3]) );
  NAND2X1 U99 ( .A(n53), .B(n54), .Y(selected_candidate_valid_o[4]) );
  NAND2X1 U100 ( .A(n51), .B(n52), .Y(selected_candidate_valid_o[5]) );
  NAND2X1 U101 ( .A(n49), .B(n50), .Y(selected_candidate_valid_o[6]) );
  NAND2X1 U102 ( .A(n47), .B(n48), .Y(selected_candidate_valid_o[7]) );
  NAND2X1 U103 ( .A(n45), .B(n46), .Y(selected_candidate_valid_o[8]) );
  NAND2X1 U104 ( .A(n43), .B(n44), .Y(selected_candidate_valid_o[9]) );
  NAND2X1 U105 ( .A(n34), .B(n35), .Y(selected_pattern_id_o[0]) );
  NAND2X1 U106 ( .A(n32), .B(n33), .Y(selected_pattern_id_o[1]) );
  NAND2X1 U107 ( .A(n30), .B(n31), .Y(selected_pattern_id_o[2]) );
  NAND2X1 U108 ( .A(n28), .B(n29), .Y(selected_pattern_id_o[3]) );
  NAND2X1 U109 ( .A(n41), .B(n42), .Y(selected_config_id_o[0]) );
  NAND2X1 U110 ( .A(n39), .B(n40), .Y(selected_config_id_o[1]) );
  NAND2X1 U111 ( .A(n37), .B(n38), .Y(selected_config_id_o[2]) );
  OAI21XL U112 ( .A0(n14), .A1(n36), .B0(n19), .Y(selected_map_index_o[0]) );
  NAND2X1 U113 ( .A(n22), .B(n19), .Y(selected_map_index_o[1]) );
  OR3XL U114 ( .A(selected_map_index_o[1]), .B(n17), .C(n16), .Y(
        group_solution_valid_o) );
  NOR2X1 U115 ( .A(n22), .B(n72), .Y(n25) );
  NOR3X2 U116 ( .A(n16), .B(selected_map_index_o[1]), .C(n23), .Y(n27) );
  NOR2X1 U117 ( .A(n36), .B(selected_map_index_o[1]), .Y(n26) );
  INVXL U118 ( .A(n26), .Y(n66) );
  INVXL U119 ( .A(n66), .Y(n67) );
  INVXL U120 ( .A(n27), .Y(n68) );
  INVXL U121 ( .A(n68), .Y(n69) );
  BUFX1 U122 ( .A(n25), .Y(n70) );
  AOI22XL U123 ( .A0(config_pattern_map_i[50]), .A1(n70), .B0(
        config_pattern_map_i[30]), .B1(n67), .Y(n62) );
  AOI22XL U124 ( .A0(config_pattern_map_i[51]), .A1(n70), .B0(
        config_pattern_map_i[31]), .B1(n26), .Y(n60) );
  AOI22XL U125 ( .A0(config_pattern_map_i[52]), .A1(n70), .B0(
        config_pattern_map_i[32]), .B1(n26), .Y(n58) );
  AOI22XL U126 ( .A0(config_pattern_map_i[53]), .A1(n70), .B0(
        config_pattern_map_i[33]), .B1(n26), .Y(n56) );
  AOI22XL U127 ( .A0(config_pattern_map_i[54]), .A1(n70), .B0(
        config_pattern_map_i[34]), .B1(n26), .Y(n54) );
  AOI22XL U128 ( .A0(config_pattern_map_i[55]), .A1(n70), .B0(
        config_pattern_map_i[35]), .B1(n26), .Y(n52) );
  AOI22XL U129 ( .A0(config_pattern_map_i[56]), .A1(n70), .B0(
        config_pattern_map_i[36]), .B1(n26), .Y(n50) );
  AOI22XL U130 ( .A0(config_pattern_map_i[57]), .A1(n70), .B0(
        config_pattern_map_i[37]), .B1(n26), .Y(n48) );
  AOI22XL U131 ( .A0(config_pattern_map_i[58]), .A1(n70), .B0(
        config_pattern_map_i[38]), .B1(n26), .Y(n46) );
  AOI22XL U132 ( .A0(config_pattern_map_i[59]), .A1(n25), .B0(
        config_pattern_map_i[39]), .B1(n67), .Y(n44) );
  AOI22XL U133 ( .A0(config_pattern_map_i[41]), .A1(n25), .B0(
        config_pattern_map_i[21]), .B1(n67), .Y(n42) );
  AOI22XL U134 ( .A0(config_pattern_map_i[42]), .A1(n25), .B0(
        config_pattern_map_i[22]), .B1(n67), .Y(n40) );
  AOI22XL U135 ( .A0(config_pattern_map_i[43]), .A1(n25), .B0(
        config_pattern_map_i[23]), .B1(n67), .Y(n38) );
  AOI22XL U136 ( .A0(config_pattern_map_i[46]), .A1(n25), .B0(
        config_pattern_map_i[26]), .B1(n67), .Y(n35) );
  AOI22XL U137 ( .A0(config_pattern_map_i[47]), .A1(n25), .B0(
        config_pattern_map_i[27]), .B1(n67), .Y(n33) );
  AOI22XL U138 ( .A0(config_pattern_map_i[48]), .A1(n25), .B0(
        config_pattern_map_i[28]), .B1(n67), .Y(n31) );
  AOI22XL U139 ( .A0(config_pattern_map_i[49]), .A1(n25), .B0(
        config_pattern_map_i[29]), .B1(n67), .Y(n29) );
  BUFX3 U140 ( .A(n13), .Y(n71) );
  NOR2XL U141 ( .A(n72), .B(n20), .Y(selected_score_o[0]) );
  AOI22XL U142 ( .A0(config_pattern_map_i[10]), .A1(n69), .B0(
        config_pattern_map_i[70]), .B1(n71), .Y(n61) );
  AOI22XL U143 ( .A0(config_pattern_map_i[11]), .A1(n69), .B0(
        config_pattern_map_i[71]), .B1(n71), .Y(n59) );
  AOI22XL U144 ( .A0(config_pattern_map_i[12]), .A1(n69), .B0(
        config_pattern_map_i[72]), .B1(n71), .Y(n57) );
  AOI22XL U145 ( .A0(config_pattern_map_i[13]), .A1(n69), .B0(
        config_pattern_map_i[73]), .B1(n71), .Y(n55) );
  AOI22XL U146 ( .A0(config_pattern_map_i[14]), .A1(n69), .B0(
        config_pattern_map_i[74]), .B1(n71), .Y(n53) );
  AOI22XL U147 ( .A0(config_pattern_map_i[15]), .A1(n69), .B0(
        config_pattern_map_i[75]), .B1(n71), .Y(n51) );
  AOI22XL U148 ( .A0(config_pattern_map_i[16]), .A1(n69), .B0(
        config_pattern_map_i[76]), .B1(n71), .Y(n49) );
  AOI22XL U149 ( .A0(config_pattern_map_i[17]), .A1(n69), .B0(
        config_pattern_map_i[77]), .B1(n71), .Y(n47) );
  AOI22XL U150 ( .A0(config_pattern_map_i[18]), .A1(n27), .B0(
        config_pattern_map_i[78]), .B1(n71), .Y(n45) );
  AOI22XL U151 ( .A0(config_pattern_map_i[19]), .A1(n27), .B0(
        config_pattern_map_i[79]), .B1(n71), .Y(n43) );
  AOI22XL U152 ( .A0(config_pattern_map_i[1]), .A1(n27), .B0(
        config_pattern_map_i[61]), .B1(n72), .Y(n41) );
  AOI22XL U153 ( .A0(config_pattern_map_i[2]), .A1(n27), .B0(
        config_pattern_map_i[62]), .B1(n72), .Y(n39) );
  AOI22XL U154 ( .A0(config_pattern_map_i[3]), .A1(n27), .B0(
        config_pattern_map_i[63]), .B1(n72), .Y(n37) );
  AOI22XL U155 ( .A0(config_pattern_map_i[6]), .A1(n27), .B0(
        config_pattern_map_i[66]), .B1(n72), .Y(n34) );
  AOI22XL U156 ( .A0(config_pattern_map_i[7]), .A1(n27), .B0(
        config_pattern_map_i[67]), .B1(n72), .Y(n32) );
  AOI22XL U157 ( .A0(config_pattern_map_i[8]), .A1(n27), .B0(
        config_pattern_map_i[68]), .B1(n72), .Y(n30) );
  AOI22XL U158 ( .A0(config_pattern_map_i[9]), .A1(n27), .B0(
        config_pattern_map_i[69]), .B1(n72), .Y(n28) );
  OR4X1 U159 ( .A(n25), .B(n26), .C(n69), .D(n72), .Y(selected_repairable_o)
         );
  BUFX3 U160 ( .A(n13), .Y(n72) );
  INVX1 U161 ( .A(n19), .Y(n13) );
endmodule

