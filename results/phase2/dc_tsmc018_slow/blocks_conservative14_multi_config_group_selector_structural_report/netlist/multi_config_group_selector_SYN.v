/////////////////////////////////////////////////////////////
// Created by: Synopsys DC Expert(TM) in wire load mode
// Version   : W-2024.09-SP2
// Date      : Wed Sep  9 14:41:40 2026
/////////////////////////////////////////////////////////////



    module resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3 ( 
        demands_rows_i, demands_cols_i, allocation_success_o, borrowed_rows_o, 
        borrowed_cols_o, borrowed_rows_by_sa_o, borrowed_cols_by_sa_o, 
        used_rows_o, used_cols_o );
  input [11:0] demands_rows_i;
  input [11:0] demands_cols_i;
  output [2:0] borrowed_rows_o;
  output [2:0] borrowed_cols_o;
  output [11:0] borrowed_rows_by_sa_o;
  output [11:0] borrowed_cols_by_sa_o;
  output [7:0] used_rows_o;
  output [7:0] used_cols_o;
  output allocation_success_o;
  wire   n13, n14, n15, n16, n17, n18, n19, n20, n21, n22, n23, n24, n25, n26,
         n27, n28, n29, n30, n31, n32, n33, n34, n35, n36, n37, n38, n39, n40,
         n41, n42, n43, n44, n45, n46, n47, n48, n49, n50, n51, n52, n53, n54,
         n55, n56, n57, n58, n59, n60, n61, n62, n63, n64, n65, n66, n67, n68,
         n69, n70, n71, n72, n73, n74, n75, n76, n77, n78, n79, n80, n81, n82,
         n83, n84, n85, n86;

  INVX1 U3 ( .A(1'b1), .Y(used_rows_o[7]) );
  INVX1 U5 ( .A(1'b1), .Y(used_rows_o[6]) );
  INVX1 U7 ( .A(1'b1), .Y(used_rows_o[5]) );
  INVX1 U9 ( .A(1'b1), .Y(used_cols_o[7]) );
  INVX1 U11 ( .A(1'b1), .Y(used_cols_o[6]) );
  INVX1 U13 ( .A(1'b1), .Y(used_cols_o[5]) );
  XOR2X1 U15 ( .A(n13), .B(n14), .Y(borrowed_rows_o[0]) );
  INVX1 U16 ( .A(n15), .Y(borrowed_rows_by_sa_o[9]) );
  XOR2X1 U17 ( .A(n16), .B(n17), .Y(borrowed_cols_o[0]) );
  INVX1 U18 ( .A(n18), .Y(borrowed_cols_by_sa_o[6]) );
  NOR3X1 U19 ( .A(n19), .B(n20), .C(n21), .Y(allocation_success_o) );
  NAND4BXL U20 ( .AN(borrowed_cols_o[2]), .B(n22), .C(n23), .D(n24), .Y(n21)
         );
  XNOR2X1 U21 ( .A(n24), .B(n25), .Y(borrowed_cols_o[2]) );
  XOR2X1 U22 ( .A(n26), .B(n27), .Y(n25) );
  AOI21X1 U23 ( .A0(n28), .A1(n29), .B0(n30), .Y(n27) );
  INVX1 U24 ( .A(n31), .Y(n30) );
  OAI21XL U25 ( .A0(n29), .A1(n28), .B0(n32), .Y(n31) );
  AOI31X1 U26 ( .A0(n33), .A1(n34), .A2(borrowed_cols_by_sa_o[5]), .B0(n35), 
        .Y(n26) );
  MXI2X1 U27 ( .A(n36), .B(n37), .S0(n33), .Y(n35) );
  NAND2X1 U28 ( .A(borrowed_cols_by_sa_o[2]), .B(n38), .Y(n37) );
  NAND2X1 U29 ( .A(n34), .B(n38), .Y(n36) );
  INVX1 U30 ( .A(n38), .Y(borrowed_cols_by_sa_o[5]) );
  NAND2X1 U31 ( .A(demands_cols_i[4]), .B(demands_cols_i[5]), .Y(n38) );
  AOI32X1 U32 ( .A0(borrowed_cols_by_sa_o[0]), .A1(n39), .A2(
        borrowed_cols_by_sa_o[3]), .B0(borrowed_cols_by_sa_o[1]), .B1(
        borrowed_cols_by_sa_o[4]), .Y(n33) );
  INVX1 U33 ( .A(n23), .Y(n39) );
  NOR2X1 U34 ( .A(borrowed_cols_by_sa_o[4]), .B(borrowed_cols_by_sa_o[1]), .Y(
        n23) );
  XNOR2X1 U35 ( .A(n40), .B(n41), .Y(n24) );
  AOI22X1 U36 ( .A0(borrowed_cols_by_sa_o[7]), .A1(n42), .B0(
        borrowed_cols_by_sa_o[10]), .B1(n43), .Y(n41) );
  NAND2BX1 U37 ( .AN(borrowed_cols_by_sa_o[10]), .B(n44), .Y(n42) );
  INVX1 U38 ( .A(n43), .Y(n44) );
  XNOR2X1 U39 ( .A(borrowed_cols_by_sa_o[8]), .B(borrowed_cols_by_sa_o[11]), 
        .Y(n40) );
  AND2X1 U40 ( .A(demands_cols_i[7]), .B(demands_cols_i[8]), .Y(
        borrowed_cols_by_sa_o[8]) );
  NAND3BX1 U41 ( .AN(borrowed_rows_o[2]), .B(n45), .C(n46), .Y(n20) );
  AOI211X1 U42 ( .A0(borrowed_cols_o[1]), .A1(borrowed_rows_o[1]), .B0(n47), 
        .C0(n48), .Y(n46) );
  AOI2BB1X1 U43 ( .A0N(demands_rows_i[0]), .A1N(borrowed_rows_by_sa_o[6]), 
        .B0(n49), .Y(n48) );
  AOI2BB1X1 U44 ( .A0N(demands_rows_i[9]), .A1N(borrowed_rows_by_sa_o[3]), 
        .B0(n50), .Y(n47) );
  XNOR2X1 U45 ( .A(n51), .B(n52), .Y(borrowed_rows_o[1]) );
  XNOR2X1 U46 ( .A(n53), .B(n54), .Y(n51) );
  XNOR2X1 U47 ( .A(n55), .B(n32), .Y(borrowed_cols_o[1]) );
  XOR2X1 U48 ( .A(n43), .B(n56), .Y(n32) );
  XOR2X1 U49 ( .A(borrowed_cols_by_sa_o[7]), .B(borrowed_cols_by_sa_o[10]), 
        .Y(n56) );
  NOR2X1 U50 ( .A(n57), .B(n18), .Y(n43) );
  XNOR2X1 U51 ( .A(n28), .B(n29), .Y(n55) );
  XNOR2X1 U52 ( .A(n58), .B(n59), .Y(n29) );
  XOR2X1 U53 ( .A(borrowed_cols_by_sa_o[4]), .B(borrowed_cols_by_sa_o[1]), .Y(
        n59) );
  NOR2BX1 U54 ( .AN(demands_cols_i[2]), .B(demands_cols_i[1]), .Y(
        borrowed_cols_by_sa_o[1]) );
  NOR2BX1 U55 ( .AN(demands_cols_i[5]), .B(demands_cols_i[4]), .Y(
        borrowed_cols_by_sa_o[4]) );
  NAND2X1 U56 ( .A(borrowed_cols_by_sa_o[0]), .B(borrowed_cols_by_sa_o[3]), 
        .Y(n58) );
  INVX1 U57 ( .A(n60), .Y(borrowed_cols_by_sa_o[3]) );
  AND2X1 U58 ( .A(n17), .B(n16), .Y(n28) );
  XNOR2X1 U59 ( .A(n18), .B(borrowed_cols_by_sa_o[9]), .Y(n16) );
  OAI21XL U60 ( .A0(demands_cols_i[7]), .A1(demands_cols_i[8]), .B0(
        demands_cols_i[6]), .Y(n18) );
  XNOR2X1 U61 ( .A(n60), .B(borrowed_cols_by_sa_o[0]), .Y(n17) );
  OAI21XL U62 ( .A0(demands_cols_i[5]), .A1(demands_cols_i[4]), .B0(
        demands_cols_i[3]), .Y(n60) );
  XOR2X1 U63 ( .A(n61), .B(n62), .Y(borrowed_rows_o[2]) );
  XOR2X1 U64 ( .A(n63), .B(n64), .Y(n62) );
  XOR2X1 U65 ( .A(borrowed_rows_by_sa_o[5]), .B(borrowed_rows_by_sa_o[2]), .Y(
        n64) );
  AND2X1 U66 ( .A(demands_rows_i[4]), .B(demands_rows_i[5]), .Y(
        borrowed_rows_by_sa_o[5]) );
  AOI32X1 U67 ( .A0(borrowed_rows_by_sa_o[0]), .A1(n65), .A2(
        borrowed_rows_by_sa_o[3]), .B0(borrowed_rows_by_sa_o[4]), .B1(
        borrowed_rows_by_sa_o[1]), .Y(n63) );
  INVX1 U68 ( .A(n22), .Y(n65) );
  NOR2X1 U69 ( .A(borrowed_rows_by_sa_o[4]), .B(borrowed_rows_by_sa_o[1]), .Y(
        n22) );
  XNOR2X1 U70 ( .A(n45), .B(n66), .Y(n61) );
  AOI21X1 U71 ( .A0(n53), .A1(n54), .B0(n67), .Y(n66) );
  INVX1 U72 ( .A(n68), .Y(n67) );
  OAI21XL U73 ( .A0(n54), .A1(n53), .B0(n52), .Y(n68) );
  XOR2X1 U74 ( .A(n69), .B(n70), .Y(n52) );
  XNOR2X1 U75 ( .A(borrowed_rows_by_sa_o[7]), .B(n71), .Y(n70) );
  XNOR2X1 U76 ( .A(n72), .B(n73), .Y(n54) );
  XOR2X1 U77 ( .A(borrowed_rows_by_sa_o[4]), .B(borrowed_rows_by_sa_o[1]), .Y(
        n73) );
  AND2X1 U78 ( .A(demands_rows_i[2]), .B(n49), .Y(borrowed_rows_by_sa_o[1]) );
  INVX1 U79 ( .A(demands_rows_i[1]), .Y(n49) );
  NOR2BX1 U80 ( .AN(demands_rows_i[5]), .B(demands_rows_i[4]), .Y(
        borrowed_rows_by_sa_o[4]) );
  NAND2X1 U81 ( .A(borrowed_rows_by_sa_o[3]), .B(borrowed_rows_by_sa_o[0]), 
        .Y(n72) );
  INVX1 U82 ( .A(n74), .Y(borrowed_rows_by_sa_o[0]) );
  AND2X1 U83 ( .A(n14), .B(n13), .Y(n53) );
  XNOR2X1 U84 ( .A(n15), .B(borrowed_rows_by_sa_o[6]), .Y(n13) );
  INVX1 U85 ( .A(n75), .Y(borrowed_rows_by_sa_o[6]) );
  XNOR2X1 U86 ( .A(n74), .B(borrowed_rows_by_sa_o[3]), .Y(n14) );
  INVX1 U87 ( .A(n76), .Y(borrowed_rows_by_sa_o[3]) );
  OAI21XL U88 ( .A0(demands_rows_i[5]), .A1(demands_rows_i[4]), .B0(
        demands_rows_i[3]), .Y(n76) );
  OAI21XL U89 ( .A0(demands_rows_i[2]), .A1(demands_rows_i[1]), .B0(
        demands_rows_i[0]), .Y(n74) );
  XNOR2X1 U90 ( .A(n77), .B(n78), .Y(n45) );
  AOI22X1 U91 ( .A0(borrowed_rows_by_sa_o[7]), .A1(n79), .B0(n69), .B1(
        borrowed_rows_by_sa_o[10]), .Y(n78) );
  NAND2X1 U92 ( .A(n71), .B(n80), .Y(n79) );
  INVX1 U93 ( .A(n69), .Y(n80) );
  NOR2X1 U94 ( .A(n75), .B(n15), .Y(n69) );
  OAI21XL U95 ( .A0(demands_rows_i[10]), .A1(demands_rows_i[11]), .B0(
        demands_rows_i[9]), .Y(n15) );
  OAI21XL U96 ( .A0(demands_rows_i[7]), .A1(demands_rows_i[8]), .B0(
        demands_rows_i[6]), .Y(n75) );
  XNOR2X1 U97 ( .A(borrowed_rows_by_sa_o[11]), .B(borrowed_rows_by_sa_o[8]), 
        .Y(n77) );
  AND2X1 U98 ( .A(demands_rows_i[7]), .B(demands_rows_i[8]), .Y(
        borrowed_rows_by_sa_o[8]) );
  NAND4X1 U99 ( .A(n81), .B(n82), .C(n83), .D(n84), .Y(n19) );
  NOR3X1 U100 ( .A(n85), .B(borrowed_cols_by_sa_o[11]), .C(
        borrowed_cols_by_sa_o[10]), .Y(n84) );
  NOR2BX1 U101 ( .AN(demands_cols_i[11]), .B(demands_cols_i[10]), .Y(
        borrowed_cols_by_sa_o[10]) );
  AND2X1 U102 ( .A(demands_cols_i[10]), .B(demands_cols_i[11]), .Y(
        borrowed_cols_by_sa_o[11]) );
  OR3XL U103 ( .A(borrowed_rows_by_sa_o[10]), .B(borrowed_rows_by_sa_o[7]), 
        .C(borrowed_cols_by_sa_o[7]), .Y(n85) );
  NOR2BX1 U104 ( .AN(demands_cols_i[8]), .B(demands_cols_i[7]), .Y(
        borrowed_cols_by_sa_o[7]) );
  NOR2BX1 U105 ( .AN(demands_rows_i[8]), .B(demands_rows_i[7]), .Y(
        borrowed_rows_by_sa_o[7]) );
  INVX1 U106 ( .A(n71), .Y(borrowed_rows_by_sa_o[10]) );
  NAND2X1 U107 ( .A(demands_rows_i[11]), .B(n50), .Y(n71) );
  NOR3X1 U108 ( .A(borrowed_cols_by_sa_o[2]), .B(borrowed_rows_by_sa_o[11]), 
        .C(borrowed_rows_by_sa_o[2]), .Y(n83) );
  AND2X1 U109 ( .A(demands_rows_i[2]), .B(demands_rows_i[1]), .Y(
        borrowed_rows_by_sa_o[2]) );
  NOR2BX1 U110 ( .AN(demands_rows_i[11]), .B(n50), .Y(
        borrowed_rows_by_sa_o[11]) );
  INVX1 U111 ( .A(demands_rows_i[10]), .Y(n50) );
  INVX1 U112 ( .A(n34), .Y(borrowed_cols_by_sa_o[2]) );
  NAND2X1 U113 ( .A(demands_cols_i[1]), .B(demands_cols_i[2]), .Y(n34) );
  OAI21XL U114 ( .A0(borrowed_cols_by_sa_o[0]), .A1(demands_cols_i[3]), .B0(
        demands_cols_i[4]), .Y(n82) );
  INVX1 U115 ( .A(n86), .Y(borrowed_cols_by_sa_o[0]) );
  OAI21XL U116 ( .A0(demands_cols_i[2]), .A1(demands_cols_i[1]), .B0(
        demands_cols_i[0]), .Y(n86) );
  OAI21XL U117 ( .A0(borrowed_cols_by_sa_o[9]), .A1(demands_cols_i[6]), .B0(
        demands_cols_i[7]), .Y(n81) );
  INVX1 U118 ( .A(n57), .Y(borrowed_cols_by_sa_o[9]) );
  OAI21XL U119 ( .A0(demands_cols_i[10]), .A1(demands_cols_i[11]), .B0(
        demands_cols_i[9]), .Y(n57) );
endmodule



    module physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3 ( 
        demand_rows_i, demand_cols_i, feasible_o, borrowed_rows_o, 
        borrowed_cols_o, borrowed_rows_by_sa_o, borrowed_cols_by_sa_o, 
        used_rows_o, used_cols_o );
  input [11:0] demand_rows_i;
  input [11:0] demand_cols_i;
  output [2:0] borrowed_rows_o;
  output [2:0] borrowed_cols_o;
  output [11:0] borrowed_rows_by_sa_o;
  output [11:0] borrowed_cols_by_sa_o;
  output [7:0] used_rows_o;
  output [7:0] used_cols_o;
  output feasible_o;
  wire   SYNOPSYS_UNCONNECTED_1, SYNOPSYS_UNCONNECTED_2,
         SYNOPSYS_UNCONNECTED_3, SYNOPSYS_UNCONNECTED_4,
         SYNOPSYS_UNCONNECTED_5, SYNOPSYS_UNCONNECTED_6;

  resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3 directional_rules ( 
        .demands_rows_i(demand_rows_i), .demands_cols_i(demand_cols_i), 
        .allocation_success_o(feasible_o), .borrowed_rows_o(borrowed_rows_o), 
        .borrowed_cols_o(borrowed_cols_o), .borrowed_rows_by_sa_o(
        borrowed_rows_by_sa_o), .borrowed_cols_by_sa_o(borrowed_cols_by_sa_o), 
        .used_rows_o({SYNOPSYS_UNCONNECTED_1, SYNOPSYS_UNCONNECTED_2, 
        SYNOPSYS_UNCONNECTED_3, used_rows_o[4:0]}), .used_cols_o({
        SYNOPSYS_UNCONNECTED_4, SYNOPSYS_UNCONNECTED_5, SYNOPSYS_UNCONNECTED_6, 
        used_cols_o[4:0]}) );
  INVX1 U1 ( .A(1'b1), .Y(used_rows_o[7]) );
  INVX1 U3 ( .A(1'b1), .Y(used_rows_o[6]) );
  INVX1 U5 ( .A(1'b1), .Y(used_rows_o[5]) );
  INVX1 U7 ( .A(1'b1), .Y(used_cols_o[7]) );
  INVX1 U9 ( .A(1'b1), .Y(used_cols_o[6]) );
  INVX1 U11 ( .A(1'b1), .Y(used_cols_o[5]) );
endmodule


module multi_config_group_selector_DW01_inc_0_DW01_inc_4 ( A, SUM );
  input [11:0] A;
  output [11:0] SUM;

  wire   [11:2] carry;

  ADDHX2 U1_1_10 ( .A(A[10]), .B(carry[10]), .CO(carry[11]), .S(SUM[10]) );
  ADDHX2 U1_1_9 ( .A(A[9]), .B(carry[9]), .CO(carry[10]), .S(SUM[9]) );
  ADDHX2 U1_1_8 ( .A(A[8]), .B(carry[8]), .CO(carry[9]), .S(SUM[8]) );
  ADDHX2 U1_1_7 ( .A(A[7]), .B(carry[7]), .CO(carry[8]), .S(SUM[7]) );
  ADDHX2 U1_1_6 ( .A(A[6]), .B(carry[6]), .CO(carry[7]), .S(SUM[6]) );
  ADDHX2 U1_1_5 ( .A(A[5]), .B(carry[5]), .CO(carry[6]), .S(SUM[5]) );
  ADDHX2 U1_1_4 ( .A(A[4]), .B(carry[4]), .CO(carry[5]), .S(SUM[4]) );
  ADDHX2 U1_1_3 ( .A(A[3]), .B(carry[3]), .CO(carry[4]), .S(SUM[3]) );
  ADDHX2 U1_1_2 ( .A(A[2]), .B(carry[2]), .CO(carry[3]), .S(SUM[2]) );
  ADDHX2 U1_1_1 ( .A(A[1]), .B(A[0]), .CO(carry[2]), .S(SUM[1]) );
  XOR2X1 U1 ( .A(carry[11]), .B(A[11]), .Y(SUM[11]) );
  INVX1 U2 ( .A(A[0]), .Y(SUM[0]) );
endmodule


module multi_config_group_selector_DW01_inc_1_DW01_inc_5 ( A, SUM );
  input [11:0] A;
  output [11:0] SUM;

  wire   [11:2] carry;

  ADDHX2 U1_1_10 ( .A(A[10]), .B(carry[10]), .CO(carry[11]), .S(SUM[10]) );
  ADDHX2 U1_1_9 ( .A(A[9]), .B(carry[9]), .CO(carry[10]), .S(SUM[9]) );
  ADDHX2 U1_1_8 ( .A(A[8]), .B(carry[8]), .CO(carry[9]), .S(SUM[8]) );
  ADDHX2 U1_1_7 ( .A(A[7]), .B(carry[7]), .CO(carry[8]), .S(SUM[7]) );
  ADDHX2 U1_1_6 ( .A(A[6]), .B(carry[6]), .CO(carry[7]), .S(SUM[6]) );
  ADDHX2 U1_1_5 ( .A(A[5]), .B(carry[5]), .CO(carry[6]), .S(SUM[5]) );
  ADDHX2 U1_1_4 ( .A(A[4]), .B(carry[4]), .CO(carry[5]), .S(SUM[4]) );
  ADDHX2 U1_1_3 ( .A(A[3]), .B(carry[3]), .CO(carry[4]), .S(SUM[3]) );
  ADDHX2 U1_1_2 ( .A(A[2]), .B(carry[2]), .CO(carry[3]), .S(SUM[2]) );
  ADDHX2 U1_1_1 ( .A(A[1]), .B(A[0]), .CO(carry[2]), .S(SUM[1]) );
  XOR2X1 U1 ( .A(carry[11]), .B(A[11]), .Y(SUM[11]) );
  INVX1 U2 ( .A(A[0]), .Y(SUM[0]) );
endmodule


module multi_config_group_selector ( clk_i, rst_ni, start_i, 
        config_pattern_map_i, busy_o, done_o, success_o, selected_configs_o, 
        selected_patterns_o, combinations_checked_o, combinations_pruned_o );
  input [111:0] config_pattern_map_i;
  output [11:0] selected_configs_o;
  output [15:0] selected_patterns_o;
  output [11:0] combinations_checked_o;
  output [11:0] combinations_pruned_o;
  input clk_i, rst_ni, start_i;
  output busy_o, done_o, success_o;
  wire   best_borrow_3_, demand_rows_7, demand_rows_6, demand_rows_4,
         demand_rows_3, demand_rows_1, demand_rows_0, demand_cols_7,
         demand_cols_6, demand_cols_4, demand_cols_3, demand_cols_1,
         demand_cols_0, feasible, N890, N891, N892, N893, N894, N895, N896,
         N897, N898, N899, N900, N901, N904, N905, N906, N907, N908, N909,
         N910, N911, N912, N913, N914, N915, N958, N963, N965, N967, N969,
         N971, N973, N975, N977, N979, N981, N983, N985, N987, N989, N991,
         N993, N995, N997, N998, N999, N1001, N1003, N1005, N1007, N1009,
         N1011, N1013, N1015, N1017, N1018, N1019, N1020, N1021, N1022, N1023,
         N1024, N1025, N1026, N1027, N1028, N1029, N1030, N1032, N1034, N1036,
         N1038, N1040, N1042, N1044, N1046, N1048, N1050, N1052, N1053, N1054,
         N1056, N1058, N1060, N1067, N1068, N1069, N1070, N1071, N1072, N1073,
         N1074, N1075, N1076, N1077, N1078, N1079, N1080, N1081, N1083, n379,
         n380, n381, n382, n383, n384, n385, n386, n387, n388, n389, n390,
         n391, n392, n393, n394, n395, n396, n397, n398, n399, n400, n401,
         n402, n403, n404, n405, n406, n407, n408, n409, n410, n411, n412,
         n413, n414, n415, n416, n417, n418, n419, n420, n421, n422, n423,
         n424, n425, n426, n427, n428, n429, n430, n431, n432, n433, n434,
         n435, n436, n437, n438, n439, n440, n441, n442, n443, n444, n445,
         n446, n447, n448, n449, n450, n451, n452, n453, n454, n455, n456,
         n457, n458, n459, n460, n461, n462, n463, n464, n465, n466, n467,
         n468, n469, n470, n471, n472, n473, n474, n475, n476, n477, n478,
         n479, n480, n481, n482, n483, n484, n485, n486, n487, n488, n489,
         n490, n491, n492, n493, n494, n495, n496, n497, n498, n499, n500,
         n501, n502, n503, n504, n505, n506, n507, n508, n509, n510, n511,
         n512, n513, n514, n515, n516, n517, n518, n519, n520, n521, n522,
         n523, n524, n525, n526, n527, n528, n529, n530, n531, n532, n533,
         n534, n535, n536, n537, n538, n539, n540, n541, n542, n543, n544,
         n545, n546, n547, n548, n549, n550, n551, n552, n553, n554, n555,
         n556, n557, n558, n559, n560, n561, n562, n563, n564, n565, n566,
         n567, n568, n569, n570, n571, n572, n573, n574, n575, n576, n577,
         n578, n579, n580, n581, n582, n583, n584, n585, n586, n587, n588,
         n589, n590, n591, n592, n593, n594, n595, n596,
         SYNOPSYS_UNCONNECTED_1, SYNOPSYS_UNCONNECTED_2,
         SYNOPSYS_UNCONNECTED_3, SYNOPSYS_UNCONNECTED_4,
         SYNOPSYS_UNCONNECTED_5, SYNOPSYS_UNCONNECTED_6,
         SYNOPSYS_UNCONNECTED_7, SYNOPSYS_UNCONNECTED_8,
         SYNOPSYS_UNCONNECTED_9, SYNOPSYS_UNCONNECTED_10,
         SYNOPSYS_UNCONNECTED_11, SYNOPSYS_UNCONNECTED_12,
         SYNOPSYS_UNCONNECTED_13, SYNOPSYS_UNCONNECTED_14,
         SYNOPSYS_UNCONNECTED_15, SYNOPSYS_UNCONNECTED_16,
         SYNOPSYS_UNCONNECTED_17, SYNOPSYS_UNCONNECTED_18,
         SYNOPSYS_UNCONNECTED_19, SYNOPSYS_UNCONNECTED_20,
         SYNOPSYS_UNCONNECTED_21, SYNOPSYS_UNCONNECTED_22,
         SYNOPSYS_UNCONNECTED_23, SYNOPSYS_UNCONNECTED_24,
         SYNOPSYS_UNCONNECTED_25, SYNOPSYS_UNCONNECTED_26,
         SYNOPSYS_UNCONNECTED_27, SYNOPSYS_UNCONNECTED_28,
         SYNOPSYS_UNCONNECTED_29, SYNOPSYS_UNCONNECTED_30,
         SYNOPSYS_UNCONNECTED_31, SYNOPSYS_UNCONNECTED_32,
         SYNOPSYS_UNCONNECTED_33, SYNOPSYS_UNCONNECTED_34,
         SYNOPSYS_UNCONNECTED_35, SYNOPSYS_UNCONNECTED_36,
         SYNOPSYS_UNCONNECTED_37, SYNOPSYS_UNCONNECTED_38,
         SYNOPSYS_UNCONNECTED_39, SYNOPSYS_UNCONNECTED_40,
         SYNOPSYS_UNCONNECTED_41, SYNOPSYS_UNCONNECTED_42,
         SYNOPSYS_UNCONNECTED_43, SYNOPSYS_UNCONNECTED_44,
         SYNOPSYS_UNCONNECTED_45, SYNOPSYS_UNCONNECTED_46;
  wire   [10:9] demand_rows;
  wire   [10:9] demand_cols;

  EDFFX4 busy_o_reg ( .D(n596), .E(N958), .CK(clk_i), .Q(busy_o), .QN(n575) );
  EDFFX4 cfg_reg_2__2_ ( .D(N1074), .E(N1071), .CK(clk_i), .QN(n588) );
  EDFFX4 cfg_reg_2__0_ ( .D(N1072), .E(N1071), .CK(clk_i), .QN(n587) );
  EDFFX4 cfg_reg_2__1_ ( .D(N1073), .E(N1071), .CK(clk_i), .QN(n586) );
  EDFFX4 cfg_reg_3__2_ ( .D(N1070), .E(N1067), .CK(clk_i), .QN(n585) );
  EDFFX4 cfg_reg_3__0_ ( .D(N1068), .E(N1067), .CK(clk_i), .QN(n584) );
  EDFFX4 cfg_reg_3__1_ ( .D(N1069), .E(N1067), .CK(clk_i), .QN(n583) );
  DFFHQX4 done_o_reg ( .D(N1083), .CK(clk_i), .Q(done_o) );
  EDFFX4 cfg_reg_1__1_ ( .D(N1077), .E(N1075), .CK(clk_i), .QN(n582) );
  EDFFX4 cfg_reg_0__0_ ( .D(N1080), .E(N1079), .CK(clk_i), .QN(n581) );
  EDFFX4 cfg_reg_0__1_ ( .D(N1081), .E(N1079), .CK(clk_i), .QN(n580) );
  EDFFX4 cfg_reg_0__2_ ( .D(n595), .E(N1079), .CK(clk_i), .QN(n579) );
  EDFFX4 cfg_reg_1__0_ ( .D(N1076), .E(N1075), .CK(clk_i), .QN(n578) );
  EDFFX4 cfg_reg_1__2_ ( .D(N1078), .E(N1075), .CK(clk_i), .QN(n577) );
  EDFFX4 combinations_checked_o_reg_0_ ( .D(N1019), .E(N1018), .CK(clk_i), .Q(
        combinations_checked_o[0]) );
  EDFFX4 combinations_checked_o_reg_1_ ( .D(N1020), .E(N1018), .CK(clk_i), .Q(
        combinations_checked_o[1]) );
  EDFFX4 combinations_checked_o_reg_2_ ( .D(N1021), .E(N1018), .CK(clk_i), .Q(
        combinations_checked_o[2]) );
  EDFFX4 combinations_checked_o_reg_3_ ( .D(N1022), .E(N1018), .CK(clk_i), .Q(
        combinations_checked_o[3]) );
  EDFFX4 combinations_checked_o_reg_4_ ( .D(N1023), .E(N1018), .CK(clk_i), .Q(
        combinations_checked_o[4]) );
  EDFFX4 combinations_checked_o_reg_5_ ( .D(N1024), .E(N1018), .CK(clk_i), .Q(
        combinations_checked_o[5]) );
  EDFFX4 combinations_checked_o_reg_6_ ( .D(N1025), .E(N1018), .CK(clk_i), .Q(
        combinations_checked_o[6]) );
  EDFFX4 combinations_checked_o_reg_7_ ( .D(N1026), .E(N1018), .CK(clk_i), .Q(
        combinations_checked_o[7]) );
  EDFFX4 combinations_checked_o_reg_8_ ( .D(N1027), .E(N1018), .CK(clk_i), .Q(
        combinations_checked_o[8]) );
  EDFFX4 combinations_checked_o_reg_9_ ( .D(N1028), .E(N1018), .CK(clk_i), .Q(
        combinations_checked_o[9]) );
  EDFFX4 combinations_checked_o_reg_10_ ( .D(N1029), .E(N1018), .CK(clk_i), 
        .Q(combinations_checked_o[10]) );
  EDFFX4 combinations_checked_o_reg_11_ ( .D(N1030), .E(N1018), .CK(clk_i), 
        .Q(combinations_checked_o[11]) );
  EDFFX4 combinations_pruned_o_reg_0_ ( .D(N1032), .E(N1053), .CK(clk_i), .Q(
        combinations_pruned_o[0]) );
  EDFFX4 combinations_pruned_o_reg_1_ ( .D(N1034), .E(N1053), .CK(clk_i), .Q(
        combinations_pruned_o[1]) );
  EDFFX4 combinations_pruned_o_reg_2_ ( .D(N1036), .E(N1053), .CK(clk_i), .Q(
        combinations_pruned_o[2]) );
  EDFFX4 combinations_pruned_o_reg_3_ ( .D(N1038), .E(N1053), .CK(clk_i), .Q(
        combinations_pruned_o[3]) );
  EDFFX4 combinations_pruned_o_reg_4_ ( .D(N1040), .E(N1053), .CK(clk_i), .Q(
        combinations_pruned_o[4]) );
  EDFFX4 combinations_pruned_o_reg_5_ ( .D(N1042), .E(N1053), .CK(clk_i), .Q(
        combinations_pruned_o[5]) );
  EDFFX4 combinations_pruned_o_reg_6_ ( .D(N1044), .E(N1053), .CK(clk_i), .Q(
        combinations_pruned_o[6]) );
  EDFFX4 combinations_pruned_o_reg_7_ ( .D(N1046), .E(N1053), .CK(clk_i), .Q(
        combinations_pruned_o[7]) );
  EDFFX4 combinations_pruned_o_reg_8_ ( .D(N1048), .E(N1053), .CK(clk_i), .Q(
        combinations_pruned_o[8]) );
  EDFFX4 combinations_pruned_o_reg_9_ ( .D(N1050), .E(N1053), .CK(clk_i), .Q(
        combinations_pruned_o[9]) );
  EDFFX4 combinations_pruned_o_reg_10_ ( .D(N1052), .E(N1053), .CK(clk_i), .Q(
        combinations_pruned_o[10]) );
  EDFFX4 combinations_pruned_o_reg_11_ ( .D(N1054), .E(N1053), .CK(clk_i), .Q(
        combinations_pruned_o[11]) );
  EDFFX4 best_borrow_reg_0_ ( .D(N1056), .E(N998), .CK(clk_i), .QN(n591) );
  EDFFX4 selected_patterns_o_reg_6_ ( .D(N999), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[6]) );
  EDFFX4 selected_patterns_o_reg_4_ ( .D(N995), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[4]) );
  EDFFX4 selected_patterns_o_reg_2_ ( .D(N991), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[2]) );
  EDFFX4 selected_patterns_o_reg_0_ ( .D(N987), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[0]) );
  EDFFX4 selected_configs_o_reg_10_ ( .D(N983), .E(N998), .CK(clk_i), .Q(
        selected_configs_o[10]) );
  EDFFX4 selected_configs_o_reg_8_ ( .D(N979), .E(N998), .CK(clk_i), .Q(
        selected_configs_o[8]) );
  EDFFX4 selected_configs_o_reg_6_ ( .D(N975), .E(N998), .CK(clk_i), .Q(
        selected_configs_o[6]) );
  EDFFX4 selected_configs_o_reg_4_ ( .D(N971), .E(N998), .CK(clk_i), .Q(
        selected_configs_o[4]) );
  EDFFX4 selected_configs_o_reg_2_ ( .D(N967), .E(N998), .CK(clk_i), .Q(
        selected_configs_o[2]) );
  EDFFX4 selected_configs_o_reg_0_ ( .D(N963), .E(N998), .CK(clk_i), .Q(
        selected_configs_o[0]) );
  EDFFX4 best_borrow_reg_5_ ( .D(1'b0), .E(N998), .CK(clk_i), .QN(n593) );
  EDFFX4 best_borrow_reg_3_ ( .D(1'b0), .E(N998), .CK(clk_i), .Q(
        best_borrow_3_) );
  EDFFX4 best_borrow_reg_1_ ( .D(N1058), .E(N998), .CK(clk_i), .QN(n590) );
  EDFFX4 selected_patterns_o_reg_15_ ( .D(N1017), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[15]) );
  EDFFX4 selected_patterns_o_reg_13_ ( .D(N1013), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[13]) );
  EDFFX4 selected_patterns_o_reg_11_ ( .D(N1009), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[11]) );
  EDFFX4 selected_patterns_o_reg_9_ ( .D(N1005), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[9]) );
  EDFFX4 selected_patterns_o_reg_7_ ( .D(N1001), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[7]) );
  EDFFX4 selected_patterns_o_reg_8_ ( .D(N1003), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[8]) );
  EDFFX4 selected_patterns_o_reg_10_ ( .D(N1007), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[10]) );
  EDFFX4 selected_patterns_o_reg_12_ ( .D(N1011), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[12]) );
  EDFFX4 selected_patterns_o_reg_14_ ( .D(N1015), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[14]) );
  EDFFX4 best_borrow_reg_2_ ( .D(N1060), .E(N998), .CK(clk_i), .QN(n589) );
  EDFFX4 best_borrow_reg_4_ ( .D(1'b0), .E(N998), .CK(clk_i), .QN(n592) );
  EDFFX4 success_o_reg ( .D(n594), .E(N998), .CK(clk_i), .Q(success_o) );
  EDFFX4 selected_configs_o_reg_1_ ( .D(N965), .E(N998), .CK(clk_i), .Q(
        selected_configs_o[1]) );
  EDFFX4 selected_configs_o_reg_3_ ( .D(N969), .E(N998), .CK(clk_i), .Q(
        selected_configs_o[3]), .QN(n576) );
  EDFFX4 selected_configs_o_reg_5_ ( .D(N973), .E(N998), .CK(clk_i), .Q(
        selected_configs_o[5]) );
  EDFFX4 selected_configs_o_reg_7_ ( .D(N977), .E(N998), .CK(clk_i), .Q(
        selected_configs_o[7]) );
  EDFFX4 selected_configs_o_reg_9_ ( .D(N981), .E(N998), .CK(clk_i), .Q(
        selected_configs_o[9]) );
  EDFFX4 selected_configs_o_reg_11_ ( .D(N985), .E(N998), .CK(clk_i), .Q(
        selected_configs_o[11]) );
  EDFFX4 selected_patterns_o_reg_1_ ( .D(N989), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[1]) );
  EDFFX4 selected_patterns_o_reg_3_ ( .D(N993), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[3]) );
  EDFFX4 selected_patterns_o_reg_5_ ( .D(N997), .E(N998), .CK(clk_i), .Q(
        selected_patterns_o[5]) );
  physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3 ledger ( 
        .demand_rows_i({1'b0, demand_rows, 1'b0, demand_rows_7, demand_rows_6, 
        1'b0, demand_rows_4, demand_rows_3, 1'b0, demand_rows_1, demand_rows_0}), .demand_cols_i({1'b0, demand_cols, 1'b0, demand_cols_7, demand_cols_6, 1'b0, 
        demand_cols_4, demand_cols_3, 1'b0, demand_cols_1, demand_cols_0}), 
        .feasible_o(feasible), .borrowed_rows_o({SYNOPSYS_UNCONNECTED_1, 
        SYNOPSYS_UNCONNECTED_2, SYNOPSYS_UNCONNECTED_3}), .borrowed_cols_o({
        SYNOPSYS_UNCONNECTED_4, SYNOPSYS_UNCONNECTED_5, SYNOPSYS_UNCONNECTED_6}), .borrowed_rows_by_sa_o({SYNOPSYS_UNCONNECTED_7, SYNOPSYS_UNCONNECTED_8, 
        SYNOPSYS_UNCONNECTED_9, SYNOPSYS_UNCONNECTED_10, 
        SYNOPSYS_UNCONNECTED_11, SYNOPSYS_UNCONNECTED_12, 
        SYNOPSYS_UNCONNECTED_13, SYNOPSYS_UNCONNECTED_14, 
        SYNOPSYS_UNCONNECTED_15, SYNOPSYS_UNCONNECTED_16, 
        SYNOPSYS_UNCONNECTED_17, SYNOPSYS_UNCONNECTED_18}), 
        .borrowed_cols_by_sa_o({SYNOPSYS_UNCONNECTED_19, 
        SYNOPSYS_UNCONNECTED_20, SYNOPSYS_UNCONNECTED_21, 
        SYNOPSYS_UNCONNECTED_22, SYNOPSYS_UNCONNECTED_23, 
        SYNOPSYS_UNCONNECTED_24, SYNOPSYS_UNCONNECTED_25, 
        SYNOPSYS_UNCONNECTED_26, SYNOPSYS_UNCONNECTED_27, 
        SYNOPSYS_UNCONNECTED_28, SYNOPSYS_UNCONNECTED_29, 
        SYNOPSYS_UNCONNECTED_30}), .used_rows_o({SYNOPSYS_UNCONNECTED_31, 
        SYNOPSYS_UNCONNECTED_32, SYNOPSYS_UNCONNECTED_33, 
        SYNOPSYS_UNCONNECTED_34, SYNOPSYS_UNCONNECTED_35, 
        SYNOPSYS_UNCONNECTED_36, SYNOPSYS_UNCONNECTED_37, 
        SYNOPSYS_UNCONNECTED_38}), .used_cols_o({SYNOPSYS_UNCONNECTED_39, 
        SYNOPSYS_UNCONNECTED_40, SYNOPSYS_UNCONNECTED_41, 
        SYNOPSYS_UNCONNECTED_42, SYNOPSYS_UNCONNECTED_43, 
        SYNOPSYS_UNCONNECTED_44, SYNOPSYS_UNCONNECTED_45, 
        SYNOPSYS_UNCONNECTED_46}) );
  multi_config_group_selector_DW01_inc_0_DW01_inc_4 add_74_S2 ( .A(
        combinations_pruned_o), .SUM({N915, N914, N913, N912, N911, N910, N909, 
        N908, N907, N906, N905, N904}) );
  multi_config_group_selector_DW01_inc_1_DW01_inc_5 add_73 ( .A(
        combinations_checked_o), .SUM({N901, N900, N899, N898, N897, N896, 
        N895, N894, N893, N892, N891, N890}) );
  MXI2X1 U367 ( .A(n379), .B(n380), .S0(n381), .Y(n595) );
  NAND2X1 U368 ( .A(n579), .B(n594), .Y(n380) );
  INVX1 U369 ( .A(N967), .Y(n379) );
  AND3X1 U370 ( .A(start_i), .B(rst_ni), .C(n575), .Y(n596) );
  NOR2X1 U371 ( .A(n382), .B(n383), .Y(N999) );
  OAI31X1 U372 ( .A0(n384), .A1(n385), .A2(n383), .B0(n386), .Y(N998) );
  NOR4BX1 U373 ( .AN(n387), .B(n388), .C(n389), .D(best_borrow_3_), .Y(n385)
         );
  AOI211X1 U374 ( .A0(n591), .A1(n390), .B0(n391), .C0(n392), .Y(n389) );
  AOI211X1 U375 ( .A0(n393), .A1(n394), .B0(n395), .C0(n396), .Y(n392) );
  MXI2X1 U376 ( .A(n397), .B(n398), .S0(n399), .Y(n396) );
  NAND3X1 U377 ( .A(n400), .B(n401), .C(n402), .Y(n398) );
  OAI32X1 U378 ( .A0(n403), .A1(n404), .A2(n405), .B0(n406), .B1(n407), .Y(
        n400) );
  INVX1 U379 ( .A(selected_configs_o[11]), .Y(n407) );
  AOI31X1 U380 ( .A0(n583), .A1(n584), .A2(selected_configs_o[9]), .B0(
        selected_configs_o[10]), .Y(n405) );
  OAI22X1 U381 ( .A0(selected_configs_o[9]), .A1(n583), .B0(
        selected_configs_o[11]), .B1(n585), .Y(n403) );
  AOI21X1 U382 ( .A0(selected_configs_o[5]), .A1(n577), .B0(n408), .Y(n397) );
  AOI211X1 U383 ( .A0(n409), .A1(n410), .B0(n411), .C0(n412), .Y(n408) );
  AOI21X1 U384 ( .A0(n578), .A1(n582), .B0(selected_configs_o[4]), .Y(n412) );
  INVX1 U385 ( .A(selected_configs_o[5]), .Y(n410) );
  OAI32X1 U386 ( .A0(n413), .A1(n414), .A2(n415), .B0(n416), .B1(n417), .Y(
        n395) );
  INVX1 U387 ( .A(selected_configs_o[2]), .Y(n417) );
  AOI31X1 U388 ( .A0(n581), .A1(n580), .A2(selected_configs_o[0]), .B0(
        selected_configs_o[1]), .Y(n414) );
  AOI21X1 U389 ( .A0(selected_configs_o[0]), .A1(n581), .B0(n580), .Y(n413) );
  OAI32X1 U390 ( .A0(n418), .A1(n419), .A2(n420), .B0(n421), .B1(n422), .Y(
        n394) );
  INVX1 U391 ( .A(selected_configs_o[8]), .Y(n422) );
  AOI21X1 U392 ( .A0(n587), .A1(n586), .B0(selected_configs_o[7]), .Y(n420) );
  INVX1 U393 ( .A(n401), .Y(n393) );
  NAND3X1 U394 ( .A(n423), .B(n399), .C(n402), .Y(n401) );
  OAI31X1 U395 ( .A0(n424), .A1(n425), .A2(n426), .B0(n402), .Y(n399) );
  INVX1 U396 ( .A(n427), .Y(n402) );
  OAI221XL U397 ( .A0(selected_configs_o[0]), .A1(n581), .B0(
        selected_configs_o[1]), .B1(n580), .C0(n428), .Y(n427) );
  INVX1 U398 ( .A(n415), .Y(n428) );
  NOR2X1 U399 ( .A(selected_configs_o[2]), .B(n579), .Y(n415) );
  XNOR2X1 U400 ( .A(n576), .B(n429), .Y(n426) );
  XNOR2X1 U401 ( .A(selected_configs_o[5]), .B(n577), .Y(n425) );
  XNOR2X1 U402 ( .A(selected_configs_o[4]), .B(n582), .Y(n424) );
  NAND3X1 U403 ( .A(n430), .B(n431), .C(n432), .Y(n423) );
  AOI21X1 U404 ( .A0(selected_configs_o[8]), .A1(n588), .B0(n418), .Y(n432) );
  NOR2X1 U405 ( .A(selected_configs_o[8]), .B(n588), .Y(n418) );
  XNOR2X1 U406 ( .A(selected_configs_o[6]), .B(n433), .Y(n431) );
  XNOR2X1 U407 ( .A(selected_configs_o[7]), .B(n434), .Y(n430) );
  INVX1 U408 ( .A(n435), .Y(n391) );
  AOI21X1 U409 ( .A0(n590), .A1(n436), .B0(n437), .Y(n435) );
  NAND3X1 U410 ( .A(n593), .B(n592), .C(success_o), .Y(n388) );
  AOI21X1 U411 ( .A0(n438), .A1(n439), .B0(n440), .Y(n387) );
  AOI211X1 U412 ( .A0(n436), .A1(n441), .B0(n437), .C0(n442), .Y(n440) );
  INVX1 U413 ( .A(n443), .Y(n442) );
  OAI21XL U414 ( .A0(n441), .A1(n436), .B0(n590), .Y(n443) );
  NOR2X1 U415 ( .A(n439), .B(n438), .Y(n437) );
  OR2X1 U416 ( .A(n591), .B(n390), .Y(n441) );
  INVX1 U417 ( .A(n589), .Y(n439) );
  NOR2X1 U418 ( .A(n444), .B(n383), .Y(N997) );
  NOR2X1 U419 ( .A(n445), .B(n383), .Y(N995) );
  NOR2X1 U420 ( .A(n446), .B(n383), .Y(N993) );
  NOR2X1 U421 ( .A(n447), .B(n383), .Y(N991) );
  AND2X1 U422 ( .A(n448), .B(n594), .Y(N989) );
  AND2X1 U423 ( .A(n449), .B(n594), .Y(N987) );
  NOR2X1 U424 ( .A(n583), .B(n383), .Y(N983) );
  NOR2X1 U425 ( .A(n586), .B(n383), .Y(N977) );
  NOR2X1 U426 ( .A(n582), .B(n383), .Y(N971) );
  NOR2X1 U427 ( .A(n383), .B(n579), .Y(N967) );
  NAND2BX1 U428 ( .AN(N1083), .B(n386), .Y(N958) );
  NOR2X1 U429 ( .A(n450), .B(n383), .Y(N1083) );
  INVX1 U430 ( .A(n451), .Y(N1081) );
  AOI22X1 U431 ( .A0(n581), .A1(N965), .B0(n580), .B1(N963), .Y(n451) );
  NOR2X1 U432 ( .A(n383), .B(n581), .Y(N963) );
  NOR2X1 U433 ( .A(n383), .B(n580), .Y(N965) );
  NOR2X1 U434 ( .A(n383), .B(n452), .Y(N1080) );
  OAI31X1 U435 ( .A0(n453), .A1(n454), .A2(n455), .B0(n386), .Y(N1079) );
  INVX1 U436 ( .A(n456), .Y(n455) );
  NOR2X1 U437 ( .A(n456), .B(n457), .Y(N1078) );
  AOI21X1 U438 ( .A0(n411), .A1(n594), .B0(N973), .Y(n457) );
  NOR2X1 U439 ( .A(n383), .B(n577), .Y(N973) );
  MX2X1 U440 ( .A(N1076), .B(N969), .S0(n582), .Y(N1077) );
  NOR2X1 U441 ( .A(n383), .B(n578), .Y(N969) );
  NOR3X1 U442 ( .A(n383), .B(n456), .C(n429), .Y(N1076) );
  OAI21XL U443 ( .A0(n454), .A1(n453), .B0(n386), .Y(N1075) );
  INVX1 U444 ( .A(n458), .Y(n454) );
  NOR2X1 U445 ( .A(n458), .B(n459), .Y(N1074) );
  AOI21X1 U446 ( .A0(n419), .A1(n594), .B0(N979), .Y(n459) );
  NOR2X1 U447 ( .A(n383), .B(n588), .Y(N979) );
  MX2X1 U448 ( .A(N1072), .B(N975), .S0(n586), .Y(N1073) );
  NOR2X1 U449 ( .A(n383), .B(n587), .Y(N975) );
  NOR3X1 U450 ( .A(n383), .B(n458), .C(n433), .Y(N1072) );
  NAND2X1 U451 ( .A(n386), .B(n453), .Y(N1071) );
  NAND3X1 U452 ( .A(n594), .B(n450), .C(n460), .Y(n453) );
  INVX1 U453 ( .A(n461), .Y(N1070) );
  AOI22X1 U454 ( .A0(n462), .A1(N985), .B0(n404), .B1(n463), .Y(n461) );
  NOR2X1 U455 ( .A(n383), .B(n585), .Y(N985) );
  MX2X1 U456 ( .A(N1068), .B(N981), .S0(n583), .Y(N1069) );
  NOR2X1 U457 ( .A(n383), .B(n584), .Y(N981) );
  NOR2BX1 U458 ( .AN(n463), .B(n464), .Y(N1068) );
  NOR2X1 U459 ( .A(n383), .B(n460), .Y(n463) );
  OAI2BB1X1 U460 ( .A0N(n450), .A1N(n594), .B0(n386), .Y(N1067) );
  NAND4X1 U461 ( .A(n456), .B(n458), .C(n465), .D(n466), .Y(n450) );
  NOR4BX1 U462 ( .AN(n584), .B(n452), .C(n429), .D(n433), .Y(n466) );
  NOR2X1 U463 ( .A(n467), .B(n462), .Y(n465) );
  INVX1 U464 ( .A(n460), .Y(n462) );
  NOR2X1 U466 ( .A(n383), .B(n438), .Y(N1060) );
  MX2X1 U467 ( .A(n468), .B(n469), .S0(n470), .Y(n438) );
  NAND2BX1 U468 ( .AN(n471), .B(n472), .Y(n469) );
  NAND3BX1 U469 ( .AN(n473), .B(n474), .C(n475), .Y(n468) );
  NOR2BX1 U470 ( .AN(n436), .B(n383), .Y(N1058) );
  MXI2X1 U471 ( .A(n471), .B(n472), .S0(n470), .Y(n436) );
  XNOR2X1 U472 ( .A(n471), .B(n476), .Y(n472) );
  MXI2X1 U473 ( .A(n474), .B(n473), .S0(n475), .Y(n471) );
  XOR2X1 U474 ( .A(n477), .B(n474), .Y(n473) );
  OAI21XL U475 ( .A0(n478), .A1(n479), .B0(n480), .Y(n474) );
  OAI22X1 U476 ( .A0(n481), .A1(n482), .B0(n483), .B1(n482), .Y(n480) );
  OAI22X1 U477 ( .A0(n484), .A1(n485), .B0(n486), .B1(n487), .Y(n482) );
  NOR2BX1 U478 ( .AN(n390), .B(n383), .Y(N1056) );
  XNOR2X1 U479 ( .A(n476), .B(n470), .Y(n390) );
  AND2X1 U480 ( .A(demand_cols[9]), .B(demand_cols[10]), .Y(n470) );
  OAI21XL U481 ( .A0(n585), .A1(n488), .B0(n464), .Y(demand_cols[10]) );
  XNOR2X1 U482 ( .A(n584), .B(n460), .Y(demand_cols[9]) );
  XNOR2X1 U483 ( .A(n477), .B(n475), .Y(n476) );
  AND2X1 U484 ( .A(demand_rows[9]), .B(demand_rows[10]), .Y(n475) );
  OAI21XL U485 ( .A0(n584), .A1(n488), .B0(n406), .Y(demand_rows[10]) );
  MXI2X1 U486 ( .A(n464), .B(n583), .S0(n585), .Y(demand_rows[9]) );
  XOR2X1 U487 ( .A(n478), .B(n479), .Y(n477) );
  NAND2X1 U488 ( .A(demand_cols_6), .B(demand_cols_7), .Y(n479) );
  OAI21XL U489 ( .A0(n588), .A1(n434), .B0(n433), .Y(demand_cols_7) );
  XNOR2X1 U490 ( .A(n587), .B(n458), .Y(demand_cols_6) );
  XNOR2X1 U491 ( .A(n483), .B(n481), .Y(n478) );
  AND2X1 U492 ( .A(demand_rows_6), .B(demand_rows_7), .Y(n481) );
  OAI21XL U493 ( .A0(n587), .A1(n434), .B0(n421), .Y(demand_rows_7) );
  MXI2X1 U494 ( .A(n433), .B(n586), .S0(n588), .Y(demand_rows_6) );
  XOR2X1 U495 ( .A(n484), .B(n485), .Y(n483) );
  NAND2X1 U496 ( .A(demand_cols_3), .B(demand_cols_4), .Y(n485) );
  OAI21XL U497 ( .A0(n577), .A1(n489), .B0(n429), .Y(demand_cols_4) );
  XNOR2X1 U498 ( .A(n578), .B(n456), .Y(demand_cols_3) );
  XNOR2X1 U499 ( .A(n486), .B(n487), .Y(n484) );
  NAND2X1 U500 ( .A(demand_rows_3), .B(demand_rows_4), .Y(n487) );
  OAI21XL U501 ( .A0(n578), .A1(n489), .B0(n409), .Y(demand_rows_4) );
  MXI2X1 U502 ( .A(n429), .B(n582), .S0(n577), .Y(demand_rows_3) );
  AOI22X1 U503 ( .A0(demand_cols_1), .A1(demand_cols_0), .B0(demand_rows_1), 
        .B1(demand_rows_0), .Y(n486) );
  MXI2X1 U504 ( .A(n452), .B(n580), .S0(n579), .Y(demand_rows_0) );
  OAI21XL U505 ( .A0(n581), .A1(n490), .B0(n416), .Y(demand_rows_1) );
  XNOR2X1 U506 ( .A(n452), .B(n467), .Y(demand_cols_0) );
  INVX1 U507 ( .A(n491), .Y(n467) );
  OAI21XL U508 ( .A0(n579), .A1(n490), .B0(n452), .Y(demand_cols_1) );
  AND2X1 U509 ( .A(N915), .B(n594), .Y(N1054) );
  OAI2BB1X1 U510 ( .A0N(n384), .A1N(n594), .B0(n386), .Y(N1053) );
  NAND4X1 U511 ( .A(feasible), .B(n492), .C(n493), .D(n494), .Y(n384) );
  NAND4X1 U512 ( .A(n382), .B(n444), .C(n445), .D(n495), .Y(n494) );
  AND3X1 U513 ( .A(n496), .B(n497), .C(n498), .Y(n445) );
  AOI222X1 U514 ( .A0(config_pattern_map_i[52]), .A1(n456), .B0(
        config_pattern_map_i[48]), .B1(n499), .C0(config_pattern_map_i[44]), 
        .C1(n500), .Y(n498) );
  AOI22X1 U515 ( .A0(config_pattern_map_i[32]), .A1(n501), .B0(
        config_pattern_map_i[36]), .B1(n502), .Y(n497) );
  AOI22X1 U516 ( .A0(config_pattern_map_i[28]), .A1(n503), .B0(
        config_pattern_map_i[40]), .B1(n411), .Y(n496) );
  AND3X1 U517 ( .A(n504), .B(n505), .C(n506), .Y(n444) );
  AOI222X1 U518 ( .A0(config_pattern_map_i[53]), .A1(n456), .B0(
        config_pattern_map_i[49]), .B1(n499), .C0(config_pattern_map_i[45]), 
        .C1(n500), .Y(n506) );
  AOI22X1 U519 ( .A0(config_pattern_map_i[33]), .A1(n501), .B0(
        config_pattern_map_i[37]), .B1(n502), .Y(n505) );
  AOI22X1 U520 ( .A0(config_pattern_map_i[29]), .A1(n503), .B0(
        config_pattern_map_i[41]), .B1(n411), .Y(n504) );
  AND3X1 U521 ( .A(n507), .B(n508), .C(n509), .Y(n382) );
  AOI222X1 U522 ( .A0(config_pattern_map_i[54]), .A1(n456), .B0(
        config_pattern_map_i[50]), .B1(n499), .C0(config_pattern_map_i[46]), 
        .C1(n500), .Y(n509) );
  AOI22X1 U523 ( .A0(config_pattern_map_i[34]), .A1(n501), .B0(
        config_pattern_map_i[38]), .B1(n502), .Y(n508) );
  AOI22X1 U524 ( .A0(config_pattern_map_i[30]), .A1(n503), .B0(
        config_pattern_map_i[42]), .B1(n411), .Y(n507) );
  AOI33X1 U525 ( .A0(n510), .A1(n511), .A2(n512), .B0(n446), .B1(n447), .B2(
        n513), .Y(n493) );
  NOR2X1 U526 ( .A(n449), .B(n448), .Y(n513) );
  NAND3X1 U527 ( .A(n514), .B(n515), .C(n516), .Y(n448) );
  AOI222X1 U528 ( .A0(config_pattern_map_i[25]), .A1(n491), .B0(
        config_pattern_map_i[13]), .B1(n381), .C0(config_pattern_map_i[17]), 
        .C1(n517), .Y(n516) );
  AOI22X1 U529 ( .A0(config_pattern_map_i[5]), .A1(n518), .B0(
        config_pattern_map_i[9]), .B1(n519), .Y(n515) );
  AOI22X1 U530 ( .A0(config_pattern_map_i[1]), .A1(n520), .B0(
        config_pattern_map_i[21]), .B1(n521), .Y(n514) );
  NAND3X1 U531 ( .A(n522), .B(n523), .C(n524), .Y(n449) );
  AOI222X1 U532 ( .A0(config_pattern_map_i[24]), .A1(n491), .B0(
        config_pattern_map_i[12]), .B1(n381), .C0(config_pattern_map_i[16]), 
        .C1(n517), .Y(n524) );
  AOI22X1 U533 ( .A0(config_pattern_map_i[4]), .A1(n518), .B0(
        config_pattern_map_i[8]), .B1(n519), .Y(n523) );
  AOI22X1 U534 ( .A0(config_pattern_map_i[0]), .A1(n520), .B0(
        config_pattern_map_i[20]), .B1(n521), .Y(n522) );
  AND3X1 U535 ( .A(n525), .B(n526), .C(n527), .Y(n447) );
  AOI222X1 U536 ( .A0(config_pattern_map_i[26]), .A1(n491), .B0(
        config_pattern_map_i[14]), .B1(n381), .C0(config_pattern_map_i[18]), 
        .C1(n517), .Y(n527) );
  AOI22X1 U537 ( .A0(config_pattern_map_i[6]), .A1(n518), .B0(
        config_pattern_map_i[10]), .B1(n519), .Y(n526) );
  AOI22X1 U538 ( .A0(config_pattern_map_i[2]), .A1(n520), .B0(
        config_pattern_map_i[22]), .B1(n521), .Y(n525) );
  AND3X1 U539 ( .A(n528), .B(n529), .C(n530), .Y(n446) );
  AOI222X1 U540 ( .A0(config_pattern_map_i[27]), .A1(n491), .B0(
        config_pattern_map_i[15]), .B1(n381), .C0(config_pattern_map_i[19]), 
        .C1(n517), .Y(n530) );
  NOR3X1 U541 ( .A(n490), .B(n579), .C(n452), .Y(n517) );
  NOR2X1 U542 ( .A(n581), .B(n580), .Y(n381) );
  NOR2X1 U543 ( .A(n580), .B(n579), .Y(n491) );
  AOI22X1 U544 ( .A0(config_pattern_map_i[7]), .A1(n518), .B0(
        config_pattern_map_i[11]), .B1(n519), .Y(n529) );
  NOR3X1 U545 ( .A(n416), .B(n580), .C(n452), .Y(n519) );
  NOR3X1 U546 ( .A(n416), .B(n581), .C(n490), .Y(n518) );
  AOI22X1 U547 ( .A0(config_pattern_map_i[3]), .A1(n520), .B0(
        config_pattern_map_i[23]), .B1(n521), .Y(n528) );
  NOR2X1 U548 ( .A(n581), .B(n579), .Y(n521) );
  NOR3X1 U549 ( .A(n490), .B(n416), .C(n452), .Y(n520) );
  INVX1 U550 ( .A(n581), .Y(n452) );
  INVX1 U551 ( .A(n579), .Y(n416) );
  INVX1 U552 ( .A(n580), .Y(n490) );
  NOR2X1 U553 ( .A(n531), .B(n532), .Y(n512) );
  NAND4X1 U554 ( .A(n533), .B(n534), .C(n535), .D(n536), .Y(n492) );
  AND2X1 U555 ( .A(N914), .B(n594), .Y(N1052) );
  AND2X1 U556 ( .A(N913), .B(n594), .Y(N1050) );
  AND2X1 U557 ( .A(N912), .B(n594), .Y(N1048) );
  AND2X1 U558 ( .A(N911), .B(n594), .Y(N1046) );
  AND2X1 U559 ( .A(N910), .B(n594), .Y(N1044) );
  AND2X1 U560 ( .A(N909), .B(n594), .Y(N1042) );
  AND2X1 U561 ( .A(N908), .B(n594), .Y(N1040) );
  AND2X1 U562 ( .A(N907), .B(n594), .Y(N1038) );
  AND2X1 U563 ( .A(N906), .B(n594), .Y(N1036) );
  AND2X1 U564 ( .A(N905), .B(n594), .Y(N1034) );
  AND2X1 U565 ( .A(N904), .B(n594), .Y(N1032) );
  AND2X1 U566 ( .A(N901), .B(n594), .Y(N1030) );
  AND2X1 U567 ( .A(N900), .B(n594), .Y(N1029) );
  AND2X1 U568 ( .A(N899), .B(n594), .Y(N1028) );
  AND2X1 U569 ( .A(N898), .B(n594), .Y(N1027) );
  AND2X1 U570 ( .A(N897), .B(n594), .Y(N1026) );
  AND2X1 U571 ( .A(N896), .B(n594), .Y(N1025) );
  AND2X1 U572 ( .A(N895), .B(n594), .Y(N1024) );
  AND2X1 U573 ( .A(N894), .B(n594), .Y(N1023) );
  AND2X1 U574 ( .A(N893), .B(n594), .Y(N1022) );
  AND2X1 U575 ( .A(N892), .B(n594), .Y(N1021) );
  AND2X1 U576 ( .A(N891), .B(n594), .Y(N1020) );
  AND2X1 U577 ( .A(N890), .B(n594), .Y(N1019) );
  NAND2X1 U578 ( .A(n383), .B(n386), .Y(N1018) );
  OAI21XL U579 ( .A0(start_i), .A1(n537), .B0(n383), .Y(n386) );
  AND2X1 U580 ( .A(n531), .B(n594), .Y(N1017) );
  NAND3X1 U581 ( .A(n538), .B(n539), .C(n540), .Y(n531) );
  AOI222X1 U582 ( .A0(config_pattern_map_i[99]), .A1(n404), .B0(
        config_pattern_map_i[111]), .B1(n460), .C0(config_pattern_map_i[103]), 
        .C1(n541), .Y(n540) );
  AOI22X1 U583 ( .A0(config_pattern_map_i[107]), .A1(n542), .B0(
        config_pattern_map_i[95]), .B1(n543), .Y(n539) );
  AOI22X1 U584 ( .A0(config_pattern_map_i[91]), .A1(n544), .B0(
        config_pattern_map_i[87]), .B1(n545), .Y(n538) );
  AND2X1 U585 ( .A(n532), .B(n594), .Y(N1015) );
  NAND3X1 U586 ( .A(n546), .B(n547), .C(n548), .Y(n532) );
  AOI222X1 U587 ( .A0(config_pattern_map_i[98]), .A1(n404), .B0(
        config_pattern_map_i[110]), .B1(n460), .C0(config_pattern_map_i[102]), 
        .C1(n541), .Y(n548) );
  AOI22X1 U588 ( .A0(config_pattern_map_i[106]), .A1(n542), .B0(
        config_pattern_map_i[94]), .B1(n543), .Y(n547) );
  AOI22X1 U589 ( .A0(config_pattern_map_i[90]), .A1(n544), .B0(
        config_pattern_map_i[86]), .B1(n545), .Y(n546) );
  NOR2X1 U590 ( .A(n511), .B(n383), .Y(N1013) );
  AND3X1 U591 ( .A(n549), .B(n550), .C(n551), .Y(n511) );
  AOI222X1 U592 ( .A0(config_pattern_map_i[97]), .A1(n404), .B0(
        config_pattern_map_i[109]), .B1(n460), .C0(config_pattern_map_i[101]), 
        .C1(n541), .Y(n551) );
  AOI22X1 U593 ( .A0(config_pattern_map_i[105]), .A1(n542), .B0(
        config_pattern_map_i[93]), .B1(n543), .Y(n550) );
  AOI22X1 U594 ( .A0(config_pattern_map_i[89]), .A1(n544), .B0(
        config_pattern_map_i[85]), .B1(n545), .Y(n549) );
  NOR2X1 U595 ( .A(n510), .B(n383), .Y(N1011) );
  AND3X1 U596 ( .A(n552), .B(n553), .C(n554), .Y(n510) );
  AOI222X1 U597 ( .A0(config_pattern_map_i[96]), .A1(n404), .B0(
        config_pattern_map_i[108]), .B1(n460), .C0(config_pattern_map_i[100]), 
        .C1(n541), .Y(n554) );
  NOR3X1 U598 ( .A(n464), .B(n585), .C(n488), .Y(n541) );
  NOR2X1 U599 ( .A(n585), .B(n583), .Y(n460) );
  NOR2X1 U600 ( .A(n584), .B(n583), .Y(n404) );
  AOI22X1 U601 ( .A0(config_pattern_map_i[104]), .A1(n542), .B0(
        config_pattern_map_i[92]), .B1(n543), .Y(n553) );
  NOR3X1 U602 ( .A(n464), .B(n583), .C(n406), .Y(n543) );
  NOR2X1 U603 ( .A(n585), .B(n584), .Y(n542) );
  AOI22X1 U604 ( .A0(config_pattern_map_i[88]), .A1(n544), .B0(
        config_pattern_map_i[84]), .B1(n545), .Y(n552) );
  NOR3X1 U605 ( .A(n406), .B(n464), .C(n488), .Y(n545) );
  INVX1 U606 ( .A(n584), .Y(n464) );
  NOR3X1 U607 ( .A(n406), .B(n584), .C(n488), .Y(n544) );
  INVX1 U608 ( .A(n583), .Y(n488) );
  INVX1 U609 ( .A(n585), .Y(n406) );
  NOR2X1 U610 ( .A(n534), .B(n383), .Y(N1009) );
  AND3X1 U611 ( .A(n555), .B(n556), .C(n557), .Y(n534) );
  AOI222X1 U612 ( .A0(config_pattern_map_i[83]), .A1(n458), .B0(
        config_pattern_map_i[79]), .B1(n558), .C0(config_pattern_map_i[75]), 
        .C1(n559), .Y(n557) );
  AOI22X1 U613 ( .A0(config_pattern_map_i[63]), .A1(n560), .B0(
        config_pattern_map_i[67]), .B1(n561), .Y(n556) );
  AOI22X1 U614 ( .A0(config_pattern_map_i[59]), .A1(n562), .B0(
        config_pattern_map_i[71]), .B1(n419), .Y(n555) );
  NOR2X1 U615 ( .A(n533), .B(n383), .Y(N1007) );
  AND3X1 U616 ( .A(n563), .B(n564), .C(n565), .Y(n533) );
  AOI222X1 U617 ( .A0(config_pattern_map_i[82]), .A1(n458), .B0(
        config_pattern_map_i[78]), .B1(n558), .C0(config_pattern_map_i[74]), 
        .C1(n559), .Y(n565) );
  AOI22X1 U618 ( .A0(config_pattern_map_i[62]), .A1(n560), .B0(
        config_pattern_map_i[66]), .B1(n561), .Y(n564) );
  AOI22X1 U619 ( .A0(config_pattern_map_i[58]), .A1(n562), .B0(
        config_pattern_map_i[70]), .B1(n419), .Y(n563) );
  NOR2X1 U620 ( .A(n535), .B(n383), .Y(N1005) );
  AND3X1 U621 ( .A(n566), .B(n567), .C(n568), .Y(n535) );
  AOI222X1 U622 ( .A0(config_pattern_map_i[81]), .A1(n458), .B0(
        config_pattern_map_i[77]), .B1(n558), .C0(config_pattern_map_i[73]), 
        .C1(n559), .Y(n568) );
  AOI22X1 U623 ( .A0(config_pattern_map_i[61]), .A1(n560), .B0(
        config_pattern_map_i[65]), .B1(n561), .Y(n567) );
  AOI22X1 U624 ( .A0(config_pattern_map_i[57]), .A1(n562), .B0(
        config_pattern_map_i[69]), .B1(n419), .Y(n566) );
  NOR2X1 U625 ( .A(n536), .B(n383), .Y(N1003) );
  AND3X1 U626 ( .A(n569), .B(n570), .C(n571), .Y(n536) );
  AOI222X1 U627 ( .A0(config_pattern_map_i[80]), .A1(n458), .B0(
        config_pattern_map_i[76]), .B1(n558), .C0(config_pattern_map_i[72]), 
        .C1(n559), .Y(n571) );
  NOR3X1 U628 ( .A(n434), .B(n588), .C(n433), .Y(n559) );
  NOR2X1 U629 ( .A(n588), .B(n587), .Y(n558) );
  NOR2X1 U630 ( .A(n588), .B(n586), .Y(n458) );
  AOI22X1 U631 ( .A0(config_pattern_map_i[60]), .A1(n560), .B0(
        config_pattern_map_i[64]), .B1(n561), .Y(n570) );
  NOR3X1 U632 ( .A(n433), .B(n586), .C(n421), .Y(n561) );
  NOR3X1 U633 ( .A(n434), .B(n587), .C(n421), .Y(n560) );
  AOI22X1 U634 ( .A0(config_pattern_map_i[56]), .A1(n562), .B0(
        config_pattern_map_i[68]), .B1(n419), .Y(n569) );
  NOR2X1 U635 ( .A(n587), .B(n586), .Y(n419) );
  NOR3X1 U636 ( .A(n433), .B(n434), .C(n421), .Y(n562) );
  INVX1 U637 ( .A(n588), .Y(n421) );
  INVX1 U638 ( .A(n586), .Y(n434) );
  INVX1 U639 ( .A(n587), .Y(n433) );
  NOR2X1 U640 ( .A(n495), .B(n383), .Y(N1001) );
  INVX1 U641 ( .A(n594), .Y(n383) );
  NOR2X1 U642 ( .A(n537), .B(n575), .Y(n594) );
  INVX1 U643 ( .A(rst_ni), .Y(n537) );
  AND3X1 U644 ( .A(n572), .B(n573), .C(n574), .Y(n495) );
  AOI222X1 U645 ( .A0(config_pattern_map_i[55]), .A1(n456), .B0(
        config_pattern_map_i[51]), .B1(n499), .C0(config_pattern_map_i[47]), 
        .C1(n500), .Y(n574) );
  NOR3X1 U646 ( .A(n489), .B(n577), .C(n429), .Y(n500) );
  NOR2X1 U647 ( .A(n578), .B(n577), .Y(n499) );
  NOR2X1 U648 ( .A(n582), .B(n577), .Y(n456) );
  AOI22X1 U649 ( .A0(config_pattern_map_i[35]), .A1(n501), .B0(
        config_pattern_map_i[39]), .B1(n502), .Y(n573) );
  NOR3X1 U650 ( .A(n409), .B(n582), .C(n429), .Y(n502) );
  NOR3X1 U651 ( .A(n489), .B(n578), .C(n409), .Y(n501) );
  AOI22X1 U652 ( .A0(config_pattern_map_i[31]), .A1(n503), .B0(
        config_pattern_map_i[43]), .B1(n411), .Y(n572) );
  NOR2X1 U653 ( .A(n582), .B(n578), .Y(n411) );
  NOR3X1 U654 ( .A(n409), .B(n489), .C(n429), .Y(n503) );
  INVX1 U655 ( .A(n578), .Y(n429) );
  INVX1 U656 ( .A(n582), .Y(n489) );
  INVX1 U657 ( .A(n577), .Y(n409) );
endmodule

