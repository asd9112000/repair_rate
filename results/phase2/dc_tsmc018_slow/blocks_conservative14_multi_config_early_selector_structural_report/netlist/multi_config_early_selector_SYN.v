/////////////////////////////////////////////////////////////
// Created by: Synopsys DC Expert(TM) in wire load mode
// Version   : W-2024.09-SP2
// Date      : Wed Sep  9 14:41:45 2026
/////////////////////////////////////////////////////////////



    module resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_6 ( 
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



    module physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_6 ( 
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

  resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_6 directional_rules ( 
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



    module resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_5 ( 
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



    module physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_5 ( 
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

  resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_5 directional_rules ( 
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



    module resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_4 ( 
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



    module physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_4 ( 
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

  resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_4 directional_rules ( 
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



    module resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_3 ( 
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



    module physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_3 ( 
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

  resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_3 directional_rules ( 
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



    module resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_2 ( 
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



    module physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_2 ( 
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

  resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_2 directional_rules ( 
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



    module resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_1 ( 
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



    module physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_1 ( 
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

  resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_1 directional_rules ( 
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



    module resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_0 ( 
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



    module physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_0 ( 
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

  resource_tracker_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_TOPOLOGY1_SPARE_WIDTH3_0 directional_rules ( 
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


module multi_config_early_selector ( clk_i, rst_ni, start_i, 
        config_pattern_map_i, busy_o, done_o, success_o, selected_configs_o, 
        selected_patterns_o );
  input [111:0] config_pattern_map_i;
  output [11:0] selected_configs_o;
  output [15:0] selected_patterns_o;
  input clk_i, rst_ni, start_i;
  output busy_o, done_o, success_o;
  wire   N158, N159, N304, N305, N306, N307, N308, N309, N310, N311, N312,
         N313, N314, N315, N316, N317, N318, N319, N320, N321, N322, N323,
         N324, N325, N326, N327, N328, N329, N330, N331, N369, N371, N372,
         N373, N374, N377, N382, N383, N400, N401, N402, N403, N408, N413,
         N418, N419, N420, N421, N422, N423, n111, n112, n113, n114, n115,
         n116, n117, n118, n119, n120, n121, n122, n123, n124, n125, n126,
         n127, n128, n129, n130, n131, n132, n133, n134, n135, n136, n137,
         n138, n139, n140, n141, n142, n143, n144, n145, n146, n147, n148,
         n149, n150, n151, n152, n153, n154, n155, n156, n157, n158, n159,
         n160, n161, n162, n163, n164, n165, n166, n167, n168, n169, n170,
         n171, n172, n173, n174, n175, n176, n177, n178, n179, n180, n181,
         n182, n183, n184, n185, n186, n187, n188, n189, n190, n191, n192,
         n193, n194, n195, n196, n197, n198, n199, n200, n201, n202, n203,
         n204, n205, n206, n207, n208, n209, n210, n211, n212, n213, n214,
         n215, n216, n217, n218, n219, n220, n221, n222, n223, n224, n225,
         n226, n227, n228, n229, n230, n231, n232, n233, n234, n235, n236,
         n237, n238, n239, n240, n241, n242, n243, n244, n245, n246, n247,
         n248, n249, n250, n251, n252, n253, n254, n255, n256, n257, n258,
         n259, n260, n261, n262, n263, n264, n265, n266, n267, n268, n269,
         n270, n271, n272, n273, n274, SYNOPSYS_UNCONNECTED_1,
         SYNOPSYS_UNCONNECTED_2, SYNOPSYS_UNCONNECTED_3,
         SYNOPSYS_UNCONNECTED_4, SYNOPSYS_UNCONNECTED_5,
         SYNOPSYS_UNCONNECTED_6, SYNOPSYS_UNCONNECTED_7,
         SYNOPSYS_UNCONNECTED_8, SYNOPSYS_UNCONNECTED_9,
         SYNOPSYS_UNCONNECTED_10, SYNOPSYS_UNCONNECTED_11,
         SYNOPSYS_UNCONNECTED_12, SYNOPSYS_UNCONNECTED_13,
         SYNOPSYS_UNCONNECTED_14, SYNOPSYS_UNCONNECTED_15,
         SYNOPSYS_UNCONNECTED_16, SYNOPSYS_UNCONNECTED_17,
         SYNOPSYS_UNCONNECTED_18, SYNOPSYS_UNCONNECTED_19,
         SYNOPSYS_UNCONNECTED_20, SYNOPSYS_UNCONNECTED_21,
         SYNOPSYS_UNCONNECTED_22, SYNOPSYS_UNCONNECTED_23,
         SYNOPSYS_UNCONNECTED_24, SYNOPSYS_UNCONNECTED_25,
         SYNOPSYS_UNCONNECTED_26, SYNOPSYS_UNCONNECTED_27,
         SYNOPSYS_UNCONNECTED_28, SYNOPSYS_UNCONNECTED_29,
         SYNOPSYS_UNCONNECTED_30, SYNOPSYS_UNCONNECTED_31,
         SYNOPSYS_UNCONNECTED_32, SYNOPSYS_UNCONNECTED_33,
         SYNOPSYS_UNCONNECTED_34, SYNOPSYS_UNCONNECTED_35,
         SYNOPSYS_UNCONNECTED_36, SYNOPSYS_UNCONNECTED_37,
         SYNOPSYS_UNCONNECTED_38, SYNOPSYS_UNCONNECTED_39,
         SYNOPSYS_UNCONNECTED_40, SYNOPSYS_UNCONNECTED_41,
         SYNOPSYS_UNCONNECTED_42, SYNOPSYS_UNCONNECTED_43,
         SYNOPSYS_UNCONNECTED_44, SYNOPSYS_UNCONNECTED_45,
         SYNOPSYS_UNCONNECTED_46, SYNOPSYS_UNCONNECTED_47,
         SYNOPSYS_UNCONNECTED_48, SYNOPSYS_UNCONNECTED_49,
         SYNOPSYS_UNCONNECTED_50, SYNOPSYS_UNCONNECTED_51,
         SYNOPSYS_UNCONNECTED_52, SYNOPSYS_UNCONNECTED_53,
         SYNOPSYS_UNCONNECTED_54, SYNOPSYS_UNCONNECTED_55,
         SYNOPSYS_UNCONNECTED_56, SYNOPSYS_UNCONNECTED_57,
         SYNOPSYS_UNCONNECTED_58, SYNOPSYS_UNCONNECTED_59,
         SYNOPSYS_UNCONNECTED_60, SYNOPSYS_UNCONNECTED_61,
         SYNOPSYS_UNCONNECTED_62, SYNOPSYS_UNCONNECTED_63,
         SYNOPSYS_UNCONNECTED_64, SYNOPSYS_UNCONNECTED_65,
         SYNOPSYS_UNCONNECTED_66, SYNOPSYS_UNCONNECTED_67,
         SYNOPSYS_UNCONNECTED_68, SYNOPSYS_UNCONNECTED_69,
         SYNOPSYS_UNCONNECTED_70, SYNOPSYS_UNCONNECTED_71,
         SYNOPSYS_UNCONNECTED_72, SYNOPSYS_UNCONNECTED_73,
         SYNOPSYS_UNCONNECTED_74, SYNOPSYS_UNCONNECTED_75,
         SYNOPSYS_UNCONNECTED_76, SYNOPSYS_UNCONNECTED_77,
         SYNOPSYS_UNCONNECTED_78, SYNOPSYS_UNCONNECTED_79,
         SYNOPSYS_UNCONNECTED_80, SYNOPSYS_UNCONNECTED_81,
         SYNOPSYS_UNCONNECTED_82, SYNOPSYS_UNCONNECTED_83,
         SYNOPSYS_UNCONNECTED_84, SYNOPSYS_UNCONNECTED_85,
         SYNOPSYS_UNCONNECTED_86, SYNOPSYS_UNCONNECTED_87,
         SYNOPSYS_UNCONNECTED_88, SYNOPSYS_UNCONNECTED_89,
         SYNOPSYS_UNCONNECTED_90, SYNOPSYS_UNCONNECTED_91,
         SYNOPSYS_UNCONNECTED_92, SYNOPSYS_UNCONNECTED_93,
         SYNOPSYS_UNCONNECTED_94, SYNOPSYS_UNCONNECTED_95,
         SYNOPSYS_UNCONNECTED_96, SYNOPSYS_UNCONNECTED_97,
         SYNOPSYS_UNCONNECTED_98, SYNOPSYS_UNCONNECTED_99,
         SYNOPSYS_UNCONNECTED_100, SYNOPSYS_UNCONNECTED_101,
         SYNOPSYS_UNCONNECTED_102, SYNOPSYS_UNCONNECTED_103,
         SYNOPSYS_UNCONNECTED_104, SYNOPSYS_UNCONNECTED_105,
         SYNOPSYS_UNCONNECTED_106, SYNOPSYS_UNCONNECTED_107,
         SYNOPSYS_UNCONNECTED_108, SYNOPSYS_UNCONNECTED_109,
         SYNOPSYS_UNCONNECTED_110, SYNOPSYS_UNCONNECTED_111,
         SYNOPSYS_UNCONNECTED_112, SYNOPSYS_UNCONNECTED_113,
         SYNOPSYS_UNCONNECTED_114, SYNOPSYS_UNCONNECTED_115,
         SYNOPSYS_UNCONNECTED_116, SYNOPSYS_UNCONNECTED_117,
         SYNOPSYS_UNCONNECTED_118, SYNOPSYS_UNCONNECTED_119,
         SYNOPSYS_UNCONNECTED_120, SYNOPSYS_UNCONNECTED_121,
         SYNOPSYS_UNCONNECTED_122, SYNOPSYS_UNCONNECTED_123,
         SYNOPSYS_UNCONNECTED_124, SYNOPSYS_UNCONNECTED_125,
         SYNOPSYS_UNCONNECTED_126, SYNOPSYS_UNCONNECTED_127,
         SYNOPSYS_UNCONNECTED_128, SYNOPSYS_UNCONNECTED_129,
         SYNOPSYS_UNCONNECTED_130, SYNOPSYS_UNCONNECTED_131,
         SYNOPSYS_UNCONNECTED_132, SYNOPSYS_UNCONNECTED_133,
         SYNOPSYS_UNCONNECTED_134, SYNOPSYS_UNCONNECTED_135,
         SYNOPSYS_UNCONNECTED_136, SYNOPSYS_UNCONNECTED_137,
         SYNOPSYS_UNCONNECTED_138, SYNOPSYS_UNCONNECTED_139,
         SYNOPSYS_UNCONNECTED_140, SYNOPSYS_UNCONNECTED_141,
         SYNOPSYS_UNCONNECTED_142, SYNOPSYS_UNCONNECTED_143,
         SYNOPSYS_UNCONNECTED_144, SYNOPSYS_UNCONNECTED_145,
         SYNOPSYS_UNCONNECTED_146, SYNOPSYS_UNCONNECTED_147,
         SYNOPSYS_UNCONNECTED_148, SYNOPSYS_UNCONNECTED_149,
         SYNOPSYS_UNCONNECTED_150, SYNOPSYS_UNCONNECTED_151,
         SYNOPSYS_UNCONNECTED_152, SYNOPSYS_UNCONNECTED_153,
         SYNOPSYS_UNCONNECTED_154, SYNOPSYS_UNCONNECTED_155,
         SYNOPSYS_UNCONNECTED_156, SYNOPSYS_UNCONNECTED_157,
         SYNOPSYS_UNCONNECTED_158, SYNOPSYS_UNCONNECTED_159,
         SYNOPSYS_UNCONNECTED_160, SYNOPSYS_UNCONNECTED_161,
         SYNOPSYS_UNCONNECTED_162, SYNOPSYS_UNCONNECTED_163,
         SYNOPSYS_UNCONNECTED_164, SYNOPSYS_UNCONNECTED_165,
         SYNOPSYS_UNCONNECTED_166, SYNOPSYS_UNCONNECTED_167,
         SYNOPSYS_UNCONNECTED_168, SYNOPSYS_UNCONNECTED_169,
         SYNOPSYS_UNCONNECTED_170, SYNOPSYS_UNCONNECTED_171,
         SYNOPSYS_UNCONNECTED_172, SYNOPSYS_UNCONNECTED_173,
         SYNOPSYS_UNCONNECTED_174, SYNOPSYS_UNCONNECTED_175,
         SYNOPSYS_UNCONNECTED_176, SYNOPSYS_UNCONNECTED_177,
         SYNOPSYS_UNCONNECTED_178, SYNOPSYS_UNCONNECTED_179,
         SYNOPSYS_UNCONNECTED_180, SYNOPSYS_UNCONNECTED_181,
         SYNOPSYS_UNCONNECTED_182, SYNOPSYS_UNCONNECTED_183,
         SYNOPSYS_UNCONNECTED_184, SYNOPSYS_UNCONNECTED_185,
         SYNOPSYS_UNCONNECTED_186, SYNOPSYS_UNCONNECTED_187,
         SYNOPSYS_UNCONNECTED_188, SYNOPSYS_UNCONNECTED_189,
         SYNOPSYS_UNCONNECTED_190, SYNOPSYS_UNCONNECTED_191,
         SYNOPSYS_UNCONNECTED_192, SYNOPSYS_UNCONNECTED_193,
         SYNOPSYS_UNCONNECTED_194, SYNOPSYS_UNCONNECTED_195,
         SYNOPSYS_UNCONNECTED_196, SYNOPSYS_UNCONNECTED_197,
         SYNOPSYS_UNCONNECTED_198, SYNOPSYS_UNCONNECTED_199,
         SYNOPSYS_UNCONNECTED_200, SYNOPSYS_UNCONNECTED_201,
         SYNOPSYS_UNCONNECTED_202, SYNOPSYS_UNCONNECTED_203,
         SYNOPSYS_UNCONNECTED_204, SYNOPSYS_UNCONNECTED_205,
         SYNOPSYS_UNCONNECTED_206, SYNOPSYS_UNCONNECTED_207,
         SYNOPSYS_UNCONNECTED_208, SYNOPSYS_UNCONNECTED_209,
         SYNOPSYS_UNCONNECTED_210, SYNOPSYS_UNCONNECTED_211,
         SYNOPSYS_UNCONNECTED_212, SYNOPSYS_UNCONNECTED_213,
         SYNOPSYS_UNCONNECTED_214, SYNOPSYS_UNCONNECTED_215,
         SYNOPSYS_UNCONNECTED_216, SYNOPSYS_UNCONNECTED_217,
         SYNOPSYS_UNCONNECTED_218, SYNOPSYS_UNCONNECTED_219,
         SYNOPSYS_UNCONNECTED_220, SYNOPSYS_UNCONNECTED_221,
         SYNOPSYS_UNCONNECTED_222, SYNOPSYS_UNCONNECTED_223,
         SYNOPSYS_UNCONNECTED_224, SYNOPSYS_UNCONNECTED_225,
         SYNOPSYS_UNCONNECTED_226, SYNOPSYS_UNCONNECTED_227,
         SYNOPSYS_UNCONNECTED_228, SYNOPSYS_UNCONNECTED_229,
         SYNOPSYS_UNCONNECTED_230, SYNOPSYS_UNCONNECTED_231,
         SYNOPSYS_UNCONNECTED_232, SYNOPSYS_UNCONNECTED_233,
         SYNOPSYS_UNCONNECTED_234, SYNOPSYS_UNCONNECTED_235,
         SYNOPSYS_UNCONNECTED_236, SYNOPSYS_UNCONNECTED_237,
         SYNOPSYS_UNCONNECTED_238, SYNOPSYS_UNCONNECTED_239,
         SYNOPSYS_UNCONNECTED_240, SYNOPSYS_UNCONNECTED_241,
         SYNOPSYS_UNCONNECTED_242, SYNOPSYS_UNCONNECTED_243,
         SYNOPSYS_UNCONNECTED_244, SYNOPSYS_UNCONNECTED_245,
         SYNOPSYS_UNCONNECTED_246, SYNOPSYS_UNCONNECTED_247,
         SYNOPSYS_UNCONNECTED_248, SYNOPSYS_UNCONNECTED_249,
         SYNOPSYS_UNCONNECTED_250, SYNOPSYS_UNCONNECTED_251,
         SYNOPSYS_UNCONNECTED_252, SYNOPSYS_UNCONNECTED_253,
         SYNOPSYS_UNCONNECTED_254, SYNOPSYS_UNCONNECTED_255,
         SYNOPSYS_UNCONNECTED_256, SYNOPSYS_UNCONNECTED_257,
         SYNOPSYS_UNCONNECTED_258, SYNOPSYS_UNCONNECTED_259,
         SYNOPSYS_UNCONNECTED_260, SYNOPSYS_UNCONNECTED_261,
         SYNOPSYS_UNCONNECTED_262, SYNOPSYS_UNCONNECTED_263,
         SYNOPSYS_UNCONNECTED_264, SYNOPSYS_UNCONNECTED_265,
         SYNOPSYS_UNCONNECTED_266, SYNOPSYS_UNCONNECTED_267,
         SYNOPSYS_UNCONNECTED_268, SYNOPSYS_UNCONNECTED_269,
         SYNOPSYS_UNCONNECTED_270, SYNOPSYS_UNCONNECTED_271,
         SYNOPSYS_UNCONNECTED_272, SYNOPSYS_UNCONNECTED_273,
         SYNOPSYS_UNCONNECTED_274, SYNOPSYS_UNCONNECTED_275,
         SYNOPSYS_UNCONNECTED_276, SYNOPSYS_UNCONNECTED_277,
         SYNOPSYS_UNCONNECTED_278, SYNOPSYS_UNCONNECTED_279,
         SYNOPSYS_UNCONNECTED_280, SYNOPSYS_UNCONNECTED_281,
         SYNOPSYS_UNCONNECTED_282, SYNOPSYS_UNCONNECTED_283,
         SYNOPSYS_UNCONNECTED_284, SYNOPSYS_UNCONNECTED_285,
         SYNOPSYS_UNCONNECTED_286, SYNOPSYS_UNCONNECTED_287,
         SYNOPSYS_UNCONNECTED_288, SYNOPSYS_UNCONNECTED_289,
         SYNOPSYS_UNCONNECTED_290, SYNOPSYS_UNCONNECTED_291,
         SYNOPSYS_UNCONNECTED_292, SYNOPSYS_UNCONNECTED_293,
         SYNOPSYS_UNCONNECTED_294, SYNOPSYS_UNCONNECTED_295,
         SYNOPSYS_UNCONNECTED_296, SYNOPSYS_UNCONNECTED_297,
         SYNOPSYS_UNCONNECTED_298, SYNOPSYS_UNCONNECTED_299,
         SYNOPSYS_UNCONNECTED_300, SYNOPSYS_UNCONNECTED_301,
         SYNOPSYS_UNCONNECTED_302, SYNOPSYS_UNCONNECTED_303,
         SYNOPSYS_UNCONNECTED_304, SYNOPSYS_UNCONNECTED_305,
         SYNOPSYS_UNCONNECTED_306, SYNOPSYS_UNCONNECTED_307,
         SYNOPSYS_UNCONNECTED_308, SYNOPSYS_UNCONNECTED_309,
         SYNOPSYS_UNCONNECTED_310, SYNOPSYS_UNCONNECTED_311,
         SYNOPSYS_UNCONNECTED_312, SYNOPSYS_UNCONNECTED_313,
         SYNOPSYS_UNCONNECTED_314, SYNOPSYS_UNCONNECTED_315,
         SYNOPSYS_UNCONNECTED_316, SYNOPSYS_UNCONNECTED_317,
         SYNOPSYS_UNCONNECTED_318, SYNOPSYS_UNCONNECTED_319,
         SYNOPSYS_UNCONNECTED_320, SYNOPSYS_UNCONNECTED_321,
         SYNOPSYS_UNCONNECTED_322;
  wire   [6:0] candidate_feasible;
  wire   [10:1] g_candidate_3__cols;
  wire   [9:0] g_candidate_4__cols;
  wire   [10:0] g_candidate_5__rows;
  wire   [11:0] g_candidate_6__rows;
  wire   [11:0] g_candidate_6__cols;

  EDFFX4 committed_cols_reg_0_ ( .D(N382), .E(N403), .CK(clk_i), .QN(n268) );
  DFFHQX4 done_o_reg ( .D(N423), .CK(clk_i), .Q(done_o) );
  EDFFX4 busy_o_reg ( .D(n274), .E(N369), .CK(clk_i), .Q(busy_o), .QN(n269) );
  EDFFX4 active_sa_reg_0_ ( .D(N373), .E(N372), .CK(clk_i), .Q(N158), .QN(n272) );
  EDFFX4 active_sa_reg_1_ ( .D(N374), .E(N372), .CK(clk_i), .Q(N159), .QN(n271) );
  EDFFX4 committed_cols_reg_1_ ( .D(N383), .E(N403), .CK(clk_i), .QN(n266) );
  EDFFX4 committed_cols_reg_10_ ( .D(N383), .E(N418), .CK(clk_i), .QN(n267) );
  EDFFX4 committed_cols_reg_7_ ( .D(N383), .E(N413), .CK(clk_i), .QN(n262) );
  EDFFX4 committed_cols_reg_4_ ( .D(N383), .E(N408), .CK(clk_i), .QN(n264) );
  EDFFX4 committed_rows_reg_1_ ( .D(N377), .E(N403), .CK(clk_i), .QN(n259) );
  EDFFX4 committed_rows_reg_10_ ( .D(N377), .E(N418), .CK(clk_i), .QN(n270) );
  EDFFX4 committed_rows_reg_7_ ( .D(N377), .E(N413), .CK(clk_i), .QN(n255) );
  EDFFX4 committed_rows_reg_4_ ( .D(N377), .E(N408), .CK(clk_i), .QN(n257) );
  EDFFX4 committed_rows_reg_3_ ( .D(n273), .E(N408), .CK(clk_i), .QN(n258) );
  EDFFX4 committed_rows_reg_6_ ( .D(n273), .E(N413), .CK(clk_i), .QN(n256) );
  EDFFX4 committed_rows_reg_9_ ( .D(n273), .E(N418), .CK(clk_i), .QN(n254) );
  EDFFX4 committed_rows_reg_0_ ( .D(n273), .E(N403), .CK(clk_i), .QN(n260) );
  EDFFX4 committed_cols_reg_9_ ( .D(N382), .E(N418), .CK(clk_i), .QN(n261) );
  EDFFX4 committed_cols_reg_6_ ( .D(N382), .E(N413), .CK(clk_i), .QN(n263) );
  EDFFX4 committed_cols_reg_3_ ( .D(N382), .E(N408), .CK(clk_i), .QN(n265) );
  EDFFX4 success_o_reg ( .D(N371), .E(N369), .CK(clk_i), .Q(success_o) );
  EDFFX4 selected_configs_o_reg_11_ ( .D(N402), .E(N418), .CK(clk_i), .Q(
        selected_configs_o[11]) );
  EDFFX4 selected_configs_o_reg_10_ ( .D(N401), .E(N418), .CK(clk_i), .Q(
        selected_configs_o[10]) );
  EDFFX4 selected_configs_o_reg_9_ ( .D(N400), .E(N418), .CK(clk_i), .Q(
        selected_configs_o[9]) );
  EDFFX4 selected_configs_o_reg_8_ ( .D(N402), .E(N413), .CK(clk_i), .Q(
        selected_configs_o[8]) );
  EDFFX4 selected_configs_o_reg_7_ ( .D(N401), .E(N413), .CK(clk_i), .Q(
        selected_configs_o[7]) );
  EDFFX4 selected_configs_o_reg_6_ ( .D(N400), .E(N413), .CK(clk_i), .Q(
        selected_configs_o[6]) );
  EDFFX4 selected_configs_o_reg_5_ ( .D(N402), .E(N408), .CK(clk_i), .Q(
        selected_configs_o[5]) );
  EDFFX4 selected_configs_o_reg_4_ ( .D(N401), .E(N408), .CK(clk_i), .Q(
        selected_configs_o[4]) );
  EDFFX4 selected_configs_o_reg_3_ ( .D(N400), .E(N408), .CK(clk_i), .Q(
        selected_configs_o[3]) );
  EDFFX4 selected_configs_o_reg_2_ ( .D(N402), .E(N403), .CK(clk_i), .Q(
        selected_configs_o[2]) );
  EDFFX4 selected_configs_o_reg_1_ ( .D(N401), .E(N403), .CK(clk_i), .Q(
        selected_configs_o[1]) );
  EDFFX4 selected_configs_o_reg_0_ ( .D(N400), .E(N403), .CK(clk_i), .Q(
        selected_configs_o[0]) );
  EDFFX4 selected_patterns_o_reg_15_ ( .D(N422), .E(N418), .CK(clk_i), .Q(
        selected_patterns_o[15]) );
  EDFFX4 selected_patterns_o_reg_14_ ( .D(N421), .E(N418), .CK(clk_i), .Q(
        selected_patterns_o[14]) );
  EDFFX4 selected_patterns_o_reg_13_ ( .D(N420), .E(N418), .CK(clk_i), .Q(
        selected_patterns_o[13]) );
  EDFFX4 selected_patterns_o_reg_12_ ( .D(N419), .E(N418), .CK(clk_i), .Q(
        selected_patterns_o[12]) );
  EDFFX4 selected_patterns_o_reg_11_ ( .D(N422), .E(N413), .CK(clk_i), .Q(
        selected_patterns_o[11]) );
  EDFFX4 selected_patterns_o_reg_10_ ( .D(N421), .E(N413), .CK(clk_i), .Q(
        selected_patterns_o[10]) );
  EDFFX4 selected_patterns_o_reg_9_ ( .D(N420), .E(N413), .CK(clk_i), .Q(
        selected_patterns_o[9]) );
  EDFFX4 selected_patterns_o_reg_8_ ( .D(N419), .E(N413), .CK(clk_i), .Q(
        selected_patterns_o[8]) );
  EDFFX4 selected_patterns_o_reg_7_ ( .D(N422), .E(N408), .CK(clk_i), .Q(
        selected_patterns_o[7]) );
  EDFFX4 selected_patterns_o_reg_6_ ( .D(N421), .E(N408), .CK(clk_i), .Q(
        selected_patterns_o[6]) );
  EDFFX4 selected_patterns_o_reg_5_ ( .D(N420), .E(N408), .CK(clk_i), .Q(
        selected_patterns_o[5]) );
  EDFFX4 selected_patterns_o_reg_4_ ( .D(N419), .E(N408), .CK(clk_i), .Q(
        selected_patterns_o[4]) );
  EDFFX4 selected_patterns_o_reg_3_ ( .D(N422), .E(N403), .CK(clk_i), .Q(
        selected_patterns_o[3]) );
  EDFFX4 selected_patterns_o_reg_2_ ( .D(N421), .E(N403), .CK(clk_i), .Q(
        selected_patterns_o[2]) );
  EDFFX4 selected_patterns_o_reg_1_ ( .D(N420), .E(N403), .CK(clk_i), .Q(
        selected_patterns_o[1]) );
  EDFFX4 selected_patterns_o_reg_0_ ( .D(N419), .E(N403), .CK(clk_i), .Q(
        selected_patterns_o[0]) );
  physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_6 g_candidate_0__ledger ( 
        .demand_rows_i({1'b0, g_candidate_5__rows[10:9], 1'b0, 
        g_candidate_5__rows[7:6], 1'b0, g_candidate_5__rows[4:3], 1'b0, 
        g_candidate_5__rows[1:0]}), .demand_cols_i({1'b0, 
        g_candidate_6__cols[10], g_candidate_4__cols[9], 1'b0, 
        g_candidate_6__cols[7], g_candidate_4__cols[6], 1'b0, 
        g_candidate_6__cols[4], g_candidate_4__cols[3], 1'b0, 
        g_candidate_6__cols[1], g_candidate_4__cols[0]}), .feasible_o(
        candidate_feasible[0]), .borrowed_rows_o({SYNOPSYS_UNCONNECTED_1, 
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
  physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_5 g_candidate_1__ledger ( 
        .demand_rows_i({1'b0, g_candidate_5__rows[10:9], 1'b0, 
        g_candidate_5__rows[7:6], 1'b0, g_candidate_5__rows[4:3], 1'b0, 
        g_candidate_5__rows[1:0]}), .demand_cols_i({1'b0, 
        g_candidate_3__cols[10], g_candidate_6__cols[9], 1'b0, 
        g_candidate_3__cols[7], g_candidate_6__cols[6], 1'b0, 
        g_candidate_3__cols[4], g_candidate_6__cols[3], 1'b0, 
        g_candidate_3__cols[1], g_candidate_6__cols[0]}), .feasible_o(
        candidate_feasible[1]), .borrowed_rows_o({SYNOPSYS_UNCONNECTED_47, 
        SYNOPSYS_UNCONNECTED_48, SYNOPSYS_UNCONNECTED_49}), .borrowed_cols_o({
        SYNOPSYS_UNCONNECTED_50, SYNOPSYS_UNCONNECTED_51, 
        SYNOPSYS_UNCONNECTED_52}), .borrowed_rows_by_sa_o({
        SYNOPSYS_UNCONNECTED_53, SYNOPSYS_UNCONNECTED_54, 
        SYNOPSYS_UNCONNECTED_55, SYNOPSYS_UNCONNECTED_56, 
        SYNOPSYS_UNCONNECTED_57, SYNOPSYS_UNCONNECTED_58, 
        SYNOPSYS_UNCONNECTED_59, SYNOPSYS_UNCONNECTED_60, 
        SYNOPSYS_UNCONNECTED_61, SYNOPSYS_UNCONNECTED_62, 
        SYNOPSYS_UNCONNECTED_63, SYNOPSYS_UNCONNECTED_64}), 
        .borrowed_cols_by_sa_o({SYNOPSYS_UNCONNECTED_65, 
        SYNOPSYS_UNCONNECTED_66, SYNOPSYS_UNCONNECTED_67, 
        SYNOPSYS_UNCONNECTED_68, SYNOPSYS_UNCONNECTED_69, 
        SYNOPSYS_UNCONNECTED_70, SYNOPSYS_UNCONNECTED_71, 
        SYNOPSYS_UNCONNECTED_72, SYNOPSYS_UNCONNECTED_73, 
        SYNOPSYS_UNCONNECTED_74, SYNOPSYS_UNCONNECTED_75, 
        SYNOPSYS_UNCONNECTED_76}), .used_rows_o({SYNOPSYS_UNCONNECTED_77, 
        SYNOPSYS_UNCONNECTED_78, SYNOPSYS_UNCONNECTED_79, 
        SYNOPSYS_UNCONNECTED_80, SYNOPSYS_UNCONNECTED_81, 
        SYNOPSYS_UNCONNECTED_82, SYNOPSYS_UNCONNECTED_83, 
        SYNOPSYS_UNCONNECTED_84}), .used_cols_o({SYNOPSYS_UNCONNECTED_85, 
        SYNOPSYS_UNCONNECTED_86, SYNOPSYS_UNCONNECTED_87, 
        SYNOPSYS_UNCONNECTED_88, SYNOPSYS_UNCONNECTED_89, 
        SYNOPSYS_UNCONNECTED_90, SYNOPSYS_UNCONNECTED_91, 
        SYNOPSYS_UNCONNECTED_92}) );
  physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_4 g_candidate_2__ledger ( 
        .demand_rows_i({1'b0, g_candidate_5__rows[10], g_candidate_6__rows[9], 
        1'b0, g_candidate_5__rows[7], g_candidate_6__rows[6], 1'b0, 
        g_candidate_5__rows[4], g_candidate_6__rows[3], 1'b0, 
        g_candidate_5__rows[1], g_candidate_6__rows[0]}), .demand_cols_i({1'b0, 
        g_candidate_6__cols[10], g_candidate_4__cols[9], 1'b0, 
        g_candidate_6__cols[7], g_candidate_4__cols[6], 1'b0, 
        g_candidate_6__cols[4], g_candidate_4__cols[3], 1'b0, 
        g_candidate_6__cols[1], g_candidate_4__cols[0]}), .feasible_o(
        candidate_feasible[2]), .borrowed_rows_o({SYNOPSYS_UNCONNECTED_93, 
        SYNOPSYS_UNCONNECTED_94, SYNOPSYS_UNCONNECTED_95}), .borrowed_cols_o({
        SYNOPSYS_UNCONNECTED_96, SYNOPSYS_UNCONNECTED_97, 
        SYNOPSYS_UNCONNECTED_98}), .borrowed_rows_by_sa_o({
        SYNOPSYS_UNCONNECTED_99, SYNOPSYS_UNCONNECTED_100, 
        SYNOPSYS_UNCONNECTED_101, SYNOPSYS_UNCONNECTED_102, 
        SYNOPSYS_UNCONNECTED_103, SYNOPSYS_UNCONNECTED_104, 
        SYNOPSYS_UNCONNECTED_105, SYNOPSYS_UNCONNECTED_106, 
        SYNOPSYS_UNCONNECTED_107, SYNOPSYS_UNCONNECTED_108, 
        SYNOPSYS_UNCONNECTED_109, SYNOPSYS_UNCONNECTED_110}), 
        .borrowed_cols_by_sa_o({SYNOPSYS_UNCONNECTED_111, 
        SYNOPSYS_UNCONNECTED_112, SYNOPSYS_UNCONNECTED_113, 
        SYNOPSYS_UNCONNECTED_114, SYNOPSYS_UNCONNECTED_115, 
        SYNOPSYS_UNCONNECTED_116, SYNOPSYS_UNCONNECTED_117, 
        SYNOPSYS_UNCONNECTED_118, SYNOPSYS_UNCONNECTED_119, 
        SYNOPSYS_UNCONNECTED_120, SYNOPSYS_UNCONNECTED_121, 
        SYNOPSYS_UNCONNECTED_122}), .used_rows_o({SYNOPSYS_UNCONNECTED_123, 
        SYNOPSYS_UNCONNECTED_124, SYNOPSYS_UNCONNECTED_125, 
        SYNOPSYS_UNCONNECTED_126, SYNOPSYS_UNCONNECTED_127, 
        SYNOPSYS_UNCONNECTED_128, SYNOPSYS_UNCONNECTED_129, 
        SYNOPSYS_UNCONNECTED_130}), .used_cols_o({SYNOPSYS_UNCONNECTED_131, 
        SYNOPSYS_UNCONNECTED_132, SYNOPSYS_UNCONNECTED_133, 
        SYNOPSYS_UNCONNECTED_134, SYNOPSYS_UNCONNECTED_135, 
        SYNOPSYS_UNCONNECTED_136, SYNOPSYS_UNCONNECTED_137, 
        SYNOPSYS_UNCONNECTED_138}) );
  physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_3 g_candidate_3__ledger ( 
        .demand_rows_i({1'b0, g_candidate_5__rows[10], g_candidate_6__rows[9], 
        1'b0, g_candidate_5__rows[7], g_candidate_6__rows[6], 1'b0, 
        g_candidate_5__rows[4], g_candidate_6__rows[3], 1'b0, 
        g_candidate_5__rows[1], g_candidate_6__rows[0]}), .demand_cols_i({1'b0, 
        g_candidate_3__cols[10], g_candidate_6__cols[9], 1'b0, 
        g_candidate_3__cols[7], g_candidate_6__cols[6], 1'b0, 
        g_candidate_3__cols[4], g_candidate_6__cols[3], 1'b0, 
        g_candidate_3__cols[1], g_candidate_6__cols[0]}), .feasible_o(
        candidate_feasible[3]), .borrowed_rows_o({SYNOPSYS_UNCONNECTED_139, 
        SYNOPSYS_UNCONNECTED_140, SYNOPSYS_UNCONNECTED_141}), 
        .borrowed_cols_o({SYNOPSYS_UNCONNECTED_142, SYNOPSYS_UNCONNECTED_143, 
        SYNOPSYS_UNCONNECTED_144}), .borrowed_rows_by_sa_o({
        SYNOPSYS_UNCONNECTED_145, SYNOPSYS_UNCONNECTED_146, 
        SYNOPSYS_UNCONNECTED_147, SYNOPSYS_UNCONNECTED_148, 
        SYNOPSYS_UNCONNECTED_149, SYNOPSYS_UNCONNECTED_150, 
        SYNOPSYS_UNCONNECTED_151, SYNOPSYS_UNCONNECTED_152, 
        SYNOPSYS_UNCONNECTED_153, SYNOPSYS_UNCONNECTED_154, 
        SYNOPSYS_UNCONNECTED_155, SYNOPSYS_UNCONNECTED_156}), 
        .borrowed_cols_by_sa_o({SYNOPSYS_UNCONNECTED_157, 
        SYNOPSYS_UNCONNECTED_158, SYNOPSYS_UNCONNECTED_159, 
        SYNOPSYS_UNCONNECTED_160, SYNOPSYS_UNCONNECTED_161, 
        SYNOPSYS_UNCONNECTED_162, SYNOPSYS_UNCONNECTED_163, 
        SYNOPSYS_UNCONNECTED_164, SYNOPSYS_UNCONNECTED_165, 
        SYNOPSYS_UNCONNECTED_166, SYNOPSYS_UNCONNECTED_167, 
        SYNOPSYS_UNCONNECTED_168}), .used_rows_o({SYNOPSYS_UNCONNECTED_169, 
        SYNOPSYS_UNCONNECTED_170, SYNOPSYS_UNCONNECTED_171, 
        SYNOPSYS_UNCONNECTED_172, SYNOPSYS_UNCONNECTED_173, 
        SYNOPSYS_UNCONNECTED_174, SYNOPSYS_UNCONNECTED_175, 
        SYNOPSYS_UNCONNECTED_176}), .used_cols_o({SYNOPSYS_UNCONNECTED_177, 
        SYNOPSYS_UNCONNECTED_178, SYNOPSYS_UNCONNECTED_179, 
        SYNOPSYS_UNCONNECTED_180, SYNOPSYS_UNCONNECTED_181, 
        SYNOPSYS_UNCONNECTED_182, SYNOPSYS_UNCONNECTED_183, 
        SYNOPSYS_UNCONNECTED_184}) );
  physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_2 g_candidate_4__ledger ( 
        .demand_rows_i({1'b0, g_candidate_6__rows[10:9], 1'b0, 
        g_candidate_6__rows[7:6], 1'b0, g_candidate_6__rows[4:3], 1'b0, 
        g_candidate_6__rows[1:0]}), .demand_cols_i({1'b0, 
        g_candidate_6__cols[10], g_candidate_4__cols[9], 1'b0, 
        g_candidate_6__cols[7], g_candidate_4__cols[6], 1'b0, 
        g_candidate_6__cols[4], g_candidate_4__cols[3], 1'b0, 
        g_candidate_6__cols[1], g_candidate_4__cols[0]}), .feasible_o(
        candidate_feasible[4]), .borrowed_rows_o({SYNOPSYS_UNCONNECTED_185, 
        SYNOPSYS_UNCONNECTED_186, SYNOPSYS_UNCONNECTED_187}), 
        .borrowed_cols_o({SYNOPSYS_UNCONNECTED_188, SYNOPSYS_UNCONNECTED_189, 
        SYNOPSYS_UNCONNECTED_190}), .borrowed_rows_by_sa_o({
        SYNOPSYS_UNCONNECTED_191, SYNOPSYS_UNCONNECTED_192, 
        SYNOPSYS_UNCONNECTED_193, SYNOPSYS_UNCONNECTED_194, 
        SYNOPSYS_UNCONNECTED_195, SYNOPSYS_UNCONNECTED_196, 
        SYNOPSYS_UNCONNECTED_197, SYNOPSYS_UNCONNECTED_198, 
        SYNOPSYS_UNCONNECTED_199, SYNOPSYS_UNCONNECTED_200, 
        SYNOPSYS_UNCONNECTED_201, SYNOPSYS_UNCONNECTED_202}), 
        .borrowed_cols_by_sa_o({SYNOPSYS_UNCONNECTED_203, 
        SYNOPSYS_UNCONNECTED_204, SYNOPSYS_UNCONNECTED_205, 
        SYNOPSYS_UNCONNECTED_206, SYNOPSYS_UNCONNECTED_207, 
        SYNOPSYS_UNCONNECTED_208, SYNOPSYS_UNCONNECTED_209, 
        SYNOPSYS_UNCONNECTED_210, SYNOPSYS_UNCONNECTED_211, 
        SYNOPSYS_UNCONNECTED_212, SYNOPSYS_UNCONNECTED_213, 
        SYNOPSYS_UNCONNECTED_214}), .used_rows_o({SYNOPSYS_UNCONNECTED_215, 
        SYNOPSYS_UNCONNECTED_216, SYNOPSYS_UNCONNECTED_217, 
        SYNOPSYS_UNCONNECTED_218, SYNOPSYS_UNCONNECTED_219, 
        SYNOPSYS_UNCONNECTED_220, SYNOPSYS_UNCONNECTED_221, 
        SYNOPSYS_UNCONNECTED_222}), .used_cols_o({SYNOPSYS_UNCONNECTED_223, 
        SYNOPSYS_UNCONNECTED_224, SYNOPSYS_UNCONNECTED_225, 
        SYNOPSYS_UNCONNECTED_226, SYNOPSYS_UNCONNECTED_227, 
        SYNOPSYS_UNCONNECTED_228, SYNOPSYS_UNCONNECTED_229, 
        SYNOPSYS_UNCONNECTED_230}) );
  physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_1 g_candidate_5__ledger ( 
        .demand_rows_i({1'b0, g_candidate_5__rows[10:9], 1'b0, 
        g_candidate_5__rows[7:6], 1'b0, g_candidate_5__rows[4:3], 1'b0, 
        g_candidate_5__rows[1:0]}), .demand_cols_i({1'b0, 
        g_candidate_6__cols[10:9], 1'b0, g_candidate_6__cols[7:6], 1'b0, 
        g_candidate_6__cols[4:3], 1'b0, g_candidate_6__cols[1:0]}), 
        .feasible_o(candidate_feasible[5]), .borrowed_rows_o({
        SYNOPSYS_UNCONNECTED_231, SYNOPSYS_UNCONNECTED_232, 
        SYNOPSYS_UNCONNECTED_233}), .borrowed_cols_o({SYNOPSYS_UNCONNECTED_234, 
        SYNOPSYS_UNCONNECTED_235, SYNOPSYS_UNCONNECTED_236}), 
        .borrowed_rows_by_sa_o({SYNOPSYS_UNCONNECTED_237, 
        SYNOPSYS_UNCONNECTED_238, SYNOPSYS_UNCONNECTED_239, 
        SYNOPSYS_UNCONNECTED_240, SYNOPSYS_UNCONNECTED_241, 
        SYNOPSYS_UNCONNECTED_242, SYNOPSYS_UNCONNECTED_243, 
        SYNOPSYS_UNCONNECTED_244, SYNOPSYS_UNCONNECTED_245, 
        SYNOPSYS_UNCONNECTED_246, SYNOPSYS_UNCONNECTED_247, 
        SYNOPSYS_UNCONNECTED_248}), .borrowed_cols_by_sa_o({
        SYNOPSYS_UNCONNECTED_249, SYNOPSYS_UNCONNECTED_250, 
        SYNOPSYS_UNCONNECTED_251, SYNOPSYS_UNCONNECTED_252, 
        SYNOPSYS_UNCONNECTED_253, SYNOPSYS_UNCONNECTED_254, 
        SYNOPSYS_UNCONNECTED_255, SYNOPSYS_UNCONNECTED_256, 
        SYNOPSYS_UNCONNECTED_257, SYNOPSYS_UNCONNECTED_258, 
        SYNOPSYS_UNCONNECTED_259, SYNOPSYS_UNCONNECTED_260}), .used_rows_o({
        SYNOPSYS_UNCONNECTED_261, SYNOPSYS_UNCONNECTED_262, 
        SYNOPSYS_UNCONNECTED_263, SYNOPSYS_UNCONNECTED_264, 
        SYNOPSYS_UNCONNECTED_265, SYNOPSYS_UNCONNECTED_266, 
        SYNOPSYS_UNCONNECTED_267, SYNOPSYS_UNCONNECTED_268}), .used_cols_o({
        SYNOPSYS_UNCONNECTED_269, SYNOPSYS_UNCONNECTED_270, 
        SYNOPSYS_UNCONNECTED_271, SYNOPSYS_UNCONNECTED_272, 
        SYNOPSYS_UNCONNECTED_273, SYNOPSYS_UNCONNECTED_274, 
        SYNOPSYS_UNCONNECTED_275, SYNOPSYS_UNCONNECTED_276}) );
  physical_resource_ledger_RS2_CS2_SHARED_ROWS1_SHARED_COLS1_MAX_BORROWS3_SPARE_WIDTH3_0 g_candidate_6__ledger ( 
        .demand_rows_i({1'b0, g_candidate_6__rows[10:9], 1'b0, 
        g_candidate_6__rows[7:6], 1'b0, g_candidate_6__rows[4:3], 1'b0, 
        g_candidate_6__rows[1:0]}), .demand_cols_i({1'b0, 
        g_candidate_6__cols[10:9], 1'b0, g_candidate_6__cols[7:6], 1'b0, 
        g_candidate_6__cols[4:3], 1'b0, g_candidate_6__cols[1:0]}), 
        .feasible_o(candidate_feasible[6]), .borrowed_rows_o({
        SYNOPSYS_UNCONNECTED_277, SYNOPSYS_UNCONNECTED_278, 
        SYNOPSYS_UNCONNECTED_279}), .borrowed_cols_o({SYNOPSYS_UNCONNECTED_280, 
        SYNOPSYS_UNCONNECTED_281, SYNOPSYS_UNCONNECTED_282}), 
        .borrowed_rows_by_sa_o({SYNOPSYS_UNCONNECTED_283, 
        SYNOPSYS_UNCONNECTED_284, SYNOPSYS_UNCONNECTED_285, 
        SYNOPSYS_UNCONNECTED_286, SYNOPSYS_UNCONNECTED_287, 
        SYNOPSYS_UNCONNECTED_288, SYNOPSYS_UNCONNECTED_289, 
        SYNOPSYS_UNCONNECTED_290, SYNOPSYS_UNCONNECTED_291, 
        SYNOPSYS_UNCONNECTED_292, SYNOPSYS_UNCONNECTED_293, 
        SYNOPSYS_UNCONNECTED_294}), .borrowed_cols_by_sa_o({
        SYNOPSYS_UNCONNECTED_295, SYNOPSYS_UNCONNECTED_296, 
        SYNOPSYS_UNCONNECTED_297, SYNOPSYS_UNCONNECTED_298, 
        SYNOPSYS_UNCONNECTED_299, SYNOPSYS_UNCONNECTED_300, 
        SYNOPSYS_UNCONNECTED_301, SYNOPSYS_UNCONNECTED_302, 
        SYNOPSYS_UNCONNECTED_303, SYNOPSYS_UNCONNECTED_304, 
        SYNOPSYS_UNCONNECTED_305, SYNOPSYS_UNCONNECTED_306}), .used_rows_o({
        SYNOPSYS_UNCONNECTED_307, SYNOPSYS_UNCONNECTED_308, 
        SYNOPSYS_UNCONNECTED_309, SYNOPSYS_UNCONNECTED_310, 
        SYNOPSYS_UNCONNECTED_311, SYNOPSYS_UNCONNECTED_312, 
        SYNOPSYS_UNCONNECTED_313, SYNOPSYS_UNCONNECTED_314}), .used_cols_o({
        SYNOPSYS_UNCONNECTED_315, SYNOPSYS_UNCONNECTED_316, 
        SYNOPSYS_UNCONNECTED_317, SYNOPSYS_UNCONNECTED_318, 
        SYNOPSYS_UNCONNECTED_319, SYNOPSYS_UNCONNECTED_320, 
        SYNOPSYS_UNCONNECTED_321, SYNOPSYS_UNCONNECTED_322}) );
  NOR2X1 U180 ( .A(n171), .B(N158), .Y(n166) );
  NOR2X1 U181 ( .A(n171), .B(n172), .Y(n165) );
  AOI22X1 U182 ( .A0(config_pattern_map_i[80]), .A1(n166), .B0(
        config_pattern_map_i[108]), .B1(n165), .Y(n112) );
  NOR2X1 U183 ( .A(N158), .B(N159), .Y(n168) );
  NOR2X1 U184 ( .A(n172), .B(N159), .Y(n167) );
  AOI22X1 U185 ( .A0(config_pattern_map_i[24]), .A1(n168), .B0(
        config_pattern_map_i[52]), .B1(n167), .Y(n111) );
  NAND2X1 U186 ( .A(n112), .B(n111), .Y(N331) );
  AOI22X1 U187 ( .A0(config_pattern_map_i[81]), .A1(n166), .B0(
        config_pattern_map_i[109]), .B1(n165), .Y(n114) );
  AOI22X1 U188 ( .A0(config_pattern_map_i[25]), .A1(n168), .B0(
        config_pattern_map_i[53]), .B1(n167), .Y(n113) );
  NAND2X1 U189 ( .A(n114), .B(n113), .Y(N330) );
  AOI22X1 U190 ( .A0(config_pattern_map_i[82]), .A1(n166), .B0(
        config_pattern_map_i[110]), .B1(n165), .Y(n116) );
  AOI22X1 U191 ( .A0(config_pattern_map_i[26]), .A1(n168), .B0(
        config_pattern_map_i[54]), .B1(n167), .Y(n115) );
  NAND2X1 U192 ( .A(n116), .B(n115), .Y(N329) );
  AOI22X1 U193 ( .A0(config_pattern_map_i[83]), .A1(n166), .B0(
        config_pattern_map_i[111]), .B1(n165), .Y(n118) );
  AOI22X1 U194 ( .A0(config_pattern_map_i[27]), .A1(n168), .B0(
        config_pattern_map_i[55]), .B1(n167), .Y(n117) );
  NAND2X1 U195 ( .A(n118), .B(n117), .Y(N328) );
  AOI22X1 U196 ( .A0(config_pattern_map_i[76]), .A1(n166), .B0(
        config_pattern_map_i[104]), .B1(n165), .Y(n120) );
  AOI22X1 U197 ( .A0(config_pattern_map_i[20]), .A1(n168), .B0(
        config_pattern_map_i[48]), .B1(n167), .Y(n119) );
  NAND2X1 U198 ( .A(n120), .B(n119), .Y(N327) );
  AOI22X1 U199 ( .A0(config_pattern_map_i[77]), .A1(n166), .B0(
        config_pattern_map_i[105]), .B1(n165), .Y(n122) );
  AOI22X1 U200 ( .A0(config_pattern_map_i[21]), .A1(n168), .B0(
        config_pattern_map_i[49]), .B1(n167), .Y(n121) );
  NAND2X1 U201 ( .A(n122), .B(n121), .Y(N326) );
  AOI22X1 U202 ( .A0(config_pattern_map_i[78]), .A1(n166), .B0(
        config_pattern_map_i[106]), .B1(n165), .Y(n124) );
  AOI22X1 U203 ( .A0(config_pattern_map_i[22]), .A1(n168), .B0(
        config_pattern_map_i[50]), .B1(n167), .Y(n123) );
  NAND2X1 U204 ( .A(n124), .B(n123), .Y(N325) );
  AOI22X1 U205 ( .A0(config_pattern_map_i[79]), .A1(n166), .B0(
        config_pattern_map_i[107]), .B1(n165), .Y(n126) );
  AOI22X1 U206 ( .A0(config_pattern_map_i[23]), .A1(n168), .B0(
        config_pattern_map_i[51]), .B1(n167), .Y(n125) );
  NAND2X1 U207 ( .A(n126), .B(n125), .Y(N324) );
  AOI22X1 U208 ( .A0(config_pattern_map_i[72]), .A1(n166), .B0(
        config_pattern_map_i[100]), .B1(n165), .Y(n128) );
  AOI22X1 U209 ( .A0(config_pattern_map_i[16]), .A1(n168), .B0(
        config_pattern_map_i[44]), .B1(n167), .Y(n127) );
  NAND2X1 U210 ( .A(n128), .B(n127), .Y(N323) );
  AOI22X1 U211 ( .A0(config_pattern_map_i[73]), .A1(n166), .B0(
        config_pattern_map_i[101]), .B1(n165), .Y(n130) );
  AOI22X1 U212 ( .A0(config_pattern_map_i[17]), .A1(n168), .B0(
        config_pattern_map_i[45]), .B1(n167), .Y(n129) );
  NAND2X1 U213 ( .A(n130), .B(n129), .Y(N322) );
  AOI22X1 U214 ( .A0(config_pattern_map_i[74]), .A1(n166), .B0(
        config_pattern_map_i[102]), .B1(n165), .Y(n132) );
  AOI22X1 U215 ( .A0(config_pattern_map_i[18]), .A1(n168), .B0(
        config_pattern_map_i[46]), .B1(n167), .Y(n131) );
  NAND2X1 U216 ( .A(n132), .B(n131), .Y(N321) );
  AOI22X1 U217 ( .A0(config_pattern_map_i[75]), .A1(n166), .B0(
        config_pattern_map_i[103]), .B1(n165), .Y(n134) );
  AOI22X1 U218 ( .A0(config_pattern_map_i[19]), .A1(n168), .B0(
        config_pattern_map_i[47]), .B1(n167), .Y(n133) );
  NAND2X1 U219 ( .A(n134), .B(n133), .Y(N320) );
  AOI22X1 U220 ( .A0(config_pattern_map_i[68]), .A1(n166), .B0(
        config_pattern_map_i[96]), .B1(n165), .Y(n136) );
  AOI22X1 U221 ( .A0(config_pattern_map_i[12]), .A1(n168), .B0(
        config_pattern_map_i[40]), .B1(n167), .Y(n135) );
  NAND2X1 U222 ( .A(n136), .B(n135), .Y(N319) );
  AOI22X1 U223 ( .A0(config_pattern_map_i[69]), .A1(n166), .B0(
        config_pattern_map_i[97]), .B1(n165), .Y(n138) );
  AOI22X1 U224 ( .A0(config_pattern_map_i[13]), .A1(n168), .B0(
        config_pattern_map_i[41]), .B1(n167), .Y(n137) );
  NAND2X1 U225 ( .A(n138), .B(n137), .Y(N318) );
  AOI22X1 U226 ( .A0(config_pattern_map_i[70]), .A1(n166), .B0(
        config_pattern_map_i[98]), .B1(n165), .Y(n140) );
  AOI22X1 U227 ( .A0(config_pattern_map_i[14]), .A1(n168), .B0(
        config_pattern_map_i[42]), .B1(n167), .Y(n139) );
  NAND2X1 U228 ( .A(n140), .B(n139), .Y(N317) );
  AOI22X1 U229 ( .A0(config_pattern_map_i[71]), .A1(n166), .B0(
        config_pattern_map_i[99]), .B1(n165), .Y(n142) );
  AOI22X1 U230 ( .A0(config_pattern_map_i[15]), .A1(n168), .B0(
        config_pattern_map_i[43]), .B1(n167), .Y(n141) );
  NAND2X1 U231 ( .A(n142), .B(n141), .Y(N316) );
  AOI22X1 U232 ( .A0(config_pattern_map_i[64]), .A1(n166), .B0(
        config_pattern_map_i[92]), .B1(n165), .Y(n144) );
  AOI22X1 U233 ( .A0(config_pattern_map_i[8]), .A1(n168), .B0(
        config_pattern_map_i[36]), .B1(n167), .Y(n143) );
  NAND2X1 U234 ( .A(n144), .B(n143), .Y(N315) );
  AOI22X1 U235 ( .A0(config_pattern_map_i[65]), .A1(n166), .B0(
        config_pattern_map_i[93]), .B1(n165), .Y(n146) );
  AOI22X1 U236 ( .A0(config_pattern_map_i[9]), .A1(n168), .B0(
        config_pattern_map_i[37]), .B1(n167), .Y(n145) );
  NAND2X1 U237 ( .A(n146), .B(n145), .Y(N314) );
  AOI22X1 U238 ( .A0(config_pattern_map_i[66]), .A1(n166), .B0(
        config_pattern_map_i[94]), .B1(n165), .Y(n148) );
  AOI22X1 U239 ( .A0(config_pattern_map_i[10]), .A1(n168), .B0(
        config_pattern_map_i[38]), .B1(n167), .Y(n147) );
  NAND2X1 U240 ( .A(n148), .B(n147), .Y(N313) );
  AOI22X1 U241 ( .A0(config_pattern_map_i[67]), .A1(n166), .B0(
        config_pattern_map_i[95]), .B1(n165), .Y(n150) );
  AOI22X1 U242 ( .A0(config_pattern_map_i[11]), .A1(n168), .B0(
        config_pattern_map_i[39]), .B1(n167), .Y(n149) );
  NAND2X1 U243 ( .A(n150), .B(n149), .Y(N312) );
  AOI22X1 U244 ( .A0(config_pattern_map_i[60]), .A1(n166), .B0(
        config_pattern_map_i[88]), .B1(n165), .Y(n152) );
  AOI22X1 U245 ( .A0(config_pattern_map_i[4]), .A1(n168), .B0(
        config_pattern_map_i[32]), .B1(n167), .Y(n151) );
  NAND2X1 U246 ( .A(n152), .B(n151), .Y(N311) );
  AOI22X1 U247 ( .A0(config_pattern_map_i[61]), .A1(n166), .B0(
        config_pattern_map_i[89]), .B1(n165), .Y(n154) );
  AOI22X1 U248 ( .A0(config_pattern_map_i[5]), .A1(n168), .B0(
        config_pattern_map_i[33]), .B1(n167), .Y(n153) );
  NAND2X1 U249 ( .A(n154), .B(n153), .Y(N310) );
  AOI22X1 U250 ( .A0(config_pattern_map_i[62]), .A1(n166), .B0(
        config_pattern_map_i[90]), .B1(n165), .Y(n156) );
  AOI22X1 U251 ( .A0(config_pattern_map_i[6]), .A1(n168), .B0(
        config_pattern_map_i[34]), .B1(n167), .Y(n155) );
  NAND2X1 U252 ( .A(n156), .B(n155), .Y(N309) );
  AOI22X1 U253 ( .A0(config_pattern_map_i[63]), .A1(n166), .B0(
        config_pattern_map_i[91]), .B1(n165), .Y(n158) );
  AOI22X1 U254 ( .A0(config_pattern_map_i[7]), .A1(n168), .B0(
        config_pattern_map_i[35]), .B1(n167), .Y(n157) );
  NAND2X1 U255 ( .A(n158), .B(n157), .Y(N308) );
  AOI22X1 U256 ( .A0(config_pattern_map_i[56]), .A1(n166), .B0(
        config_pattern_map_i[84]), .B1(n165), .Y(n160) );
  AOI22X1 U257 ( .A0(config_pattern_map_i[0]), .A1(n168), .B0(
        config_pattern_map_i[28]), .B1(n167), .Y(n159) );
  NAND2X1 U258 ( .A(n160), .B(n159), .Y(N307) );
  AOI22X1 U259 ( .A0(config_pattern_map_i[57]), .A1(n166), .B0(
        config_pattern_map_i[85]), .B1(n165), .Y(n162) );
  AOI22X1 U260 ( .A0(config_pattern_map_i[1]), .A1(n168), .B0(
        config_pattern_map_i[29]), .B1(n167), .Y(n161) );
  NAND2X1 U261 ( .A(n162), .B(n161), .Y(N306) );
  AOI22X1 U262 ( .A0(config_pattern_map_i[58]), .A1(n166), .B0(
        config_pattern_map_i[86]), .B1(n165), .Y(n164) );
  AOI22X1 U263 ( .A0(config_pattern_map_i[2]), .A1(n168), .B0(
        config_pattern_map_i[30]), .B1(n167), .Y(n163) );
  NAND2X1 U264 ( .A(n164), .B(n163), .Y(N305) );
  AOI22X1 U265 ( .A0(config_pattern_map_i[59]), .A1(n166), .B0(
        config_pattern_map_i[87]), .B1(n165), .Y(n170) );
  AOI22X1 U266 ( .A0(config_pattern_map_i[3]), .A1(n168), .B0(
        config_pattern_map_i[31]), .B1(n167), .Y(n169) );
  NAND2X1 U267 ( .A(n170), .B(n169), .Y(N304) );
  CLKINVX4 U268 ( .A(N159), .Y(n171) );
  CLKINVX4 U269 ( .A(N158), .Y(n172) );
  INVX1 U270 ( .A(n173), .Y(n273) );
  AOI21X1 U271 ( .A0(n174), .A1(N402), .B0(N401), .Y(n173) );
  AND3X1 U272 ( .A(start_i), .B(rst_ni), .C(n269), .Y(n274) );
  NAND2X1 U273 ( .A(n254), .B(n175), .Y(g_candidate_6__rows[9]) );
  NOR2X1 U274 ( .A(n255), .B(n176), .Y(g_candidate_6__rows[7]) );
  NAND2X1 U275 ( .A(n256), .B(n177), .Y(g_candidate_6__rows[6]) );
  NOR2X1 U276 ( .A(n257), .B(n178), .Y(g_candidate_6__rows[4]) );
  NAND2X1 U277 ( .A(n258), .B(n179), .Y(g_candidate_6__rows[3]) );
  NOR2X1 U278 ( .A(n259), .B(n180), .Y(g_candidate_6__rows[1]) );
  NOR2X1 U279 ( .A(n270), .B(n181), .Y(g_candidate_6__rows[10]) );
  NAND2X1 U280 ( .A(n260), .B(n182), .Y(g_candidate_6__rows[0]) );
  NAND2X1 U281 ( .A(n261), .B(n175), .Y(g_candidate_6__cols[9]) );
  NAND2X1 U282 ( .A(n262), .B(n177), .Y(g_candidate_6__cols[7]) );
  NAND2X1 U283 ( .A(n263), .B(n177), .Y(g_candidate_6__cols[6]) );
  NAND2X1 U284 ( .A(n264), .B(n179), .Y(g_candidate_6__cols[4]) );
  NAND2X1 U285 ( .A(n265), .B(n179), .Y(g_candidate_6__cols[3]) );
  NAND2X1 U286 ( .A(n266), .B(n182), .Y(g_candidate_6__cols[1]) );
  NAND2X1 U287 ( .A(n267), .B(n175), .Y(g_candidate_6__cols[10]) );
  NAND2X1 U288 ( .A(n268), .B(n182), .Y(g_candidate_6__cols[0]) );
  NOR2X1 U289 ( .A(n254), .B(n181), .Y(g_candidate_5__rows[9]) );
  NAND2X1 U290 ( .A(n255), .B(n177), .Y(g_candidate_5__rows[7]) );
  NOR2X1 U291 ( .A(n256), .B(n176), .Y(g_candidate_5__rows[6]) );
  NAND2X1 U292 ( .A(n257), .B(n179), .Y(g_candidate_5__rows[4]) );
  NOR2X1 U293 ( .A(n258), .B(n178), .Y(g_candidate_5__rows[3]) );
  NAND2X1 U294 ( .A(n259), .B(n182), .Y(g_candidate_5__rows[1]) );
  NAND2X1 U295 ( .A(n270), .B(n175), .Y(g_candidate_5__rows[10]) );
  NOR2X1 U296 ( .A(n260), .B(n180), .Y(g_candidate_5__rows[0]) );
  NOR2X1 U297 ( .A(n261), .B(n181), .Y(g_candidate_4__cols[9]) );
  NOR2X1 U298 ( .A(n263), .B(n176), .Y(g_candidate_4__cols[6]) );
  NOR2X1 U299 ( .A(n265), .B(n178), .Y(g_candidate_4__cols[3]) );
  NOR2X1 U300 ( .A(n268), .B(n180), .Y(g_candidate_4__cols[0]) );
  NOR2X1 U301 ( .A(n262), .B(n176), .Y(g_candidate_3__cols[7]) );
  NOR2X1 U302 ( .A(n264), .B(n178), .Y(g_candidate_3__cols[4]) );
  INVX1 U303 ( .A(n179), .Y(n178) );
  NOR2X1 U304 ( .A(n266), .B(n180), .Y(g_candidate_3__cols[1]) );
  INVX1 U305 ( .A(n182), .Y(n180) );
  NOR2X1 U306 ( .A(n267), .B(n181), .Y(g_candidate_3__cols[10]) );
  OAI221XL U307 ( .A0(n183), .A1(n184), .B0(n185), .B1(n186), .C0(n187), .Y(
        N422) );
  AOI22X1 U308 ( .A0(N328), .A1(n188), .B0(n189), .B1(n190), .Y(n187) );
  OAI221XL U309 ( .A0(n191), .A1(n192), .B0(n193), .B1(n194), .C0(n195), .Y(
        n190) );
  AOI22X1 U310 ( .A0(N304), .A1(n196), .B0(N312), .B1(n197), .Y(n195) );
  OAI221XL U311 ( .A0(n183), .A1(n198), .B0(n185), .B1(n199), .C0(n200), .Y(
        N421) );
  AOI22X1 U312 ( .A0(N329), .A1(n188), .B0(n189), .B1(n201), .Y(n200) );
  OAI221XL U313 ( .A0(n191), .A1(n202), .B0(n193), .B1(n203), .C0(n204), .Y(
        n201) );
  AOI22X1 U314 ( .A0(N305), .A1(n196), .B0(N313), .B1(n197), .Y(n204) );
  OAI221XL U315 ( .A0(n183), .A1(n205), .B0(n185), .B1(n206), .C0(n207), .Y(
        N420) );
  AOI22X1 U316 ( .A0(N330), .A1(n188), .B0(n189), .B1(n208), .Y(n207) );
  OAI221XL U317 ( .A0(n191), .A1(n209), .B0(n193), .B1(n210), .C0(n211), .Y(
        n208) );
  AOI22X1 U318 ( .A0(N306), .A1(n196), .B0(N314), .B1(n197), .Y(n211) );
  OAI221XL U319 ( .A0(n183), .A1(n212), .B0(n185), .B1(n213), .C0(n214), .Y(
        N419) );
  AOI22X1 U320 ( .A0(N331), .A1(n188), .B0(n189), .B1(n215), .Y(n214) );
  OAI221XL U321 ( .A0(n191), .A1(n216), .B0(n193), .B1(n217), .C0(n218), .Y(
        n215) );
  AOI22X1 U322 ( .A0(N307), .A1(n196), .B0(N315), .B1(n197), .Y(n218) );
  NOR3X1 U323 ( .A(n219), .B(n220), .C(n221), .Y(n197) );
  NOR3X1 U324 ( .A(n221), .B(n219), .C(n222), .Y(n196) );
  NAND3X1 U325 ( .A(n174), .B(n221), .C(n220), .Y(n193) );
  NAND3X1 U326 ( .A(n219), .B(n223), .C(n220), .Y(n191) );
  NAND2X1 U327 ( .A(n219), .B(N402), .Y(n185) );
  NAND2X1 U328 ( .A(N401), .B(n219), .Y(n183) );
  OAI21XL U329 ( .A0(n175), .A1(n224), .B0(n225), .Y(N418) );
  OAI21XL U330 ( .A0(n224), .A1(n177), .B0(n225), .Y(N413) );
  OAI21XL U331 ( .A0(n224), .A1(n179), .B0(n225), .Y(N408) );
  OAI21XL U332 ( .A0(n224), .A1(n182), .B0(n225), .Y(N403) );
  NAND2X1 U333 ( .A(n271), .B(n272), .Y(n182) );
  INVX1 U334 ( .A(n226), .Y(N402) );
  NOR2X1 U335 ( .A(n227), .B(n220), .Y(N401) );
  OAI21XL U336 ( .A0(n219), .A1(n227), .B0(n226), .Y(N383) );
  OR2X1 U337 ( .A(n188), .B(N400), .Y(N382) );
  INVX1 U338 ( .A(n228), .Y(N400) );
  NOR2X1 U339 ( .A(n226), .B(n220), .Y(n188) );
  INVX1 U340 ( .A(n222), .Y(n220) );
  OAI21XL U341 ( .A0(n229), .A1(n230), .B0(n231), .Y(n222) );
  INVX1 U342 ( .A(n232), .Y(n230) );
  NOR2BX1 U343 ( .AN(n233), .B(n234), .Y(n229) );
  NAND2X1 U344 ( .A(n189), .B(n221), .Y(n226) );
  OAI21XL U345 ( .A0(n227), .A1(n221), .B0(n228), .Y(N377) );
  NAND2X1 U346 ( .A(n219), .B(n189), .Y(n228) );
  INVX1 U347 ( .A(n174), .Y(n219) );
  OAI21XL U348 ( .A0(n235), .A1(n236), .B0(n231), .Y(n174) );
  AOI21X1 U349 ( .A0(n237), .A1(n238), .B0(n239), .Y(n236) );
  AOI21X1 U350 ( .A0(n177), .A1(n179), .B0(n227), .Y(N374) );
  NAND2X1 U351 ( .A(n271), .B(n240), .Y(n179) );
  INVX1 U352 ( .A(n176), .Y(n177) );
  NOR2X1 U353 ( .A(n240), .B(n271), .Y(n176) );
  NOR2X1 U354 ( .A(n227), .B(n240), .Y(N373) );
  INVX1 U355 ( .A(n272), .Y(n240) );
  OAI21XL U356 ( .A0(n181), .A1(n224), .B0(n225), .Y(N372) );
  INVX1 U357 ( .A(n224), .Y(N371) );
  NAND2X1 U358 ( .A(n189), .B(n241), .Y(n224) );
  NAND2BX1 U359 ( .AN(N423), .B(n225), .Y(N369) );
  OAI21XL U360 ( .A0(start_i), .A1(n242), .B0(n227), .Y(n225) );
  AOI21X1 U361 ( .A0(n241), .A1(n175), .B0(n227), .Y(N423) );
  INVX1 U362 ( .A(n189), .Y(n227) );
  NOR2X1 U363 ( .A(n242), .B(n269), .Y(n189) );
  INVX1 U364 ( .A(rst_ni), .Y(n242) );
  INVX1 U365 ( .A(n181), .Y(n175) );
  NOR2X1 U366 ( .A(n272), .B(n271), .Y(n181) );
  NAND3X1 U367 ( .A(n243), .B(n237), .C(n223), .Y(n241) );
  INVX1 U368 ( .A(n221), .Y(n223) );
  NAND2X1 U369 ( .A(n232), .B(n231), .Y(n221) );
  NAND4X1 U370 ( .A(candidate_feasible[6]), .B(n244), .C(n245), .D(n246), .Y(
        n231) );
  OR4X1 U371 ( .A(N328), .B(N329), .C(N330), .D(N331), .Y(n246) );
  NOR2X1 U372 ( .A(n239), .B(n235), .Y(n232) );
  AND4X1 U373 ( .A(candidate_feasible[5]), .B(n243), .C(n245), .D(n247), .Y(
        n235) );
  NAND4X1 U374 ( .A(n186), .B(n199), .C(n206), .D(n213), .Y(n247) );
  INVX1 U375 ( .A(N327), .Y(n213) );
  INVX1 U376 ( .A(N326), .Y(n206) );
  INVX1 U377 ( .A(N325), .Y(n199) );
  INVX1 U378 ( .A(N324), .Y(n186) );
  NOR2X1 U379 ( .A(n234), .B(n239), .Y(n245) );
  INVX1 U380 ( .A(n237), .Y(n234) );
  AND3X1 U381 ( .A(n248), .B(n238), .C(candidate_feasible[4]), .Y(n239) );
  NAND4X1 U382 ( .A(n194), .B(n203), .C(n210), .D(n217), .Y(n248) );
  INVX1 U383 ( .A(N323), .Y(n217) );
  INVX1 U384 ( .A(N322), .Y(n210) );
  INVX1 U385 ( .A(N321), .Y(n203) );
  INVX1 U386 ( .A(N320), .Y(n194) );
  NAND3X1 U387 ( .A(n244), .B(n249), .C(candidate_feasible[3]), .Y(n237) );
  NAND4X1 U388 ( .A(n184), .B(n198), .C(n205), .D(n212), .Y(n249) );
  INVX1 U389 ( .A(N319), .Y(n212) );
  INVX1 U390 ( .A(N318), .Y(n205) );
  INVX1 U391 ( .A(N317), .Y(n198) );
  INVX1 U392 ( .A(N316), .Y(n184) );
  AND2X1 U393 ( .A(n244), .B(n233), .Y(n243) );
  NAND3X1 U394 ( .A(n244), .B(n250), .C(candidate_feasible[2]), .Y(n233) );
  OR4X1 U395 ( .A(N312), .B(N313), .C(N314), .D(N315), .Y(n250) );
  AOI21X1 U396 ( .A0(candidate_feasible[0]), .A1(n251), .B0(n252), .Y(n244) );
  INVX1 U397 ( .A(n238), .Y(n252) );
  NAND2X1 U398 ( .A(candidate_feasible[1]), .B(n253), .Y(n238) );
  NAND4X1 U399 ( .A(n192), .B(n202), .C(n209), .D(n216), .Y(n253) );
  INVX1 U400 ( .A(N311), .Y(n216) );
  INVX1 U401 ( .A(N310), .Y(n209) );
  INVX1 U402 ( .A(N309), .Y(n202) );
  INVX1 U403 ( .A(N308), .Y(n192) );
  OR4X1 U404 ( .A(N304), .B(N305), .C(N306), .D(N307), .Y(n251) );
endmodule

