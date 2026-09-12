/////////////////////////////////////////////////////////////
// Created by: Synopsys DC Expert(TM) in wire load mode
// Version   : W-2024.09-SP2
// Date      : Thu Sep 10 05:09:12 2026
/////////////////////////////////////////////////////////////


module recam_role_aware_config_control ( clk_i, rst_ni, start_i, sa_role_i, 
        analyzer_candidate_valid_i, analyzer_pattern_id_i, 
        analyzer_solution_valid_i, analyzer_repairable_i, busy_o, done_o, 
        current_config_id_o, config_pattern_map_o );
  input [9:0] analyzer_candidate_valid_i;
  input [3:0] analyzer_pattern_id_i;
  output [2:0] current_config_id_o;
  output [79:0] config_pattern_map_o;
  input clk_i, rst_ni, start_i, sa_role_i, analyzer_solution_valid_i,
         analyzer_repairable_i;
  output busy_o, done_o;
  wire   sa_role_reg, N92, n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12,
         n13, n14, n15, n16, n17, n18, n19, n21, n22, n23, n24, n25, n26, n27,
         n28, n29, n30, n31, n32, n33, n34, n35, n36, n37, n38, n39, n40, n41,
         n42, n43, n44, n45, n46, n47, n48, n49, n50, n51, n52, n53, n54, n55,
         n56, n57, n58, n59, n60, n61, n62, n63, n64, n65, n66, n67, n68, n69,
         n70, n71, n72, n73, n74, n75, n76, n77, n78, n79, n80, n81, n82, n83,
         n84, n85, n86, n87, n88, n89, n90, n91, n92, n93, n94, n95, n96, n97,
         n98, n99, n100, n101, n102, n103, n104, n105, n106, n107, n108, n109,
         n110, n111, n112, n113, n114, n115, n116, n117, n118, n119, n120,
         n121, n122, n123, n124, n125, n126, n127, n128, n129, n130, n131,
         n132, n133, n134, n135, n136, n137, n138, n139, n140, n141, n142,
         n143, n144, n145, n146, n147, n148, n149, n150, n151, n152, n153,
         n154, n155, n156, n157, n158, n159, n160, n161, n162, n163, n164,
         n165, n166, n167, n168, n169, n170, n171, n172, n173, n174, n175,
         n176, n177, n178, n179, n180, n181, n182, n183, n184, n185, n186,
         n187, n188, n189, n190, n191, n192, n193, n194, n195, n196, n197,
         n198, n199, n200, n201, n202, n203, n204, n205, n206, n207, n208,
         n209, n210, n211, n212, n213;
  wire   [1:0] scan_index_reg;

  DFFHQX1 scan_index_reg_reg_0_ ( .D(n181), .CK(n203), .Q(scan_index_reg[0])
         );
  DFFHQX1 done_o_reg ( .D(N92), .CK(n196), .Q(done_o) );
  DFFX1 busy_o_reg ( .D(n179), .CK(n196), .Q(busy_o), .QN(n79) );
  DFFHQX1 sa_role_reg_reg ( .D(n178), .CK(n196), .Q(sa_role_reg) );
  DFFHQX1 scan_index_reg_reg_1_ ( .D(n180), .CK(n196), .Q(scan_index_reg[1])
         );
  DFFHQX1 config_pattern_map_o_reg_79_ ( .D(n1), .CK(n196), .Q(
        config_pattern_map_o[79]) );
  DFFHQX1 config_pattern_map_o_reg_78_ ( .D(n2), .CK(n197), .Q(
        config_pattern_map_o[78]) );
  DFFHQX1 config_pattern_map_o_reg_77_ ( .D(n3), .CK(n197), .Q(
        config_pattern_map_o[77]) );
  DFFHQX1 config_pattern_map_o_reg_76_ ( .D(n4), .CK(n197), .Q(
        config_pattern_map_o[76]) );
  DFFHQX1 config_pattern_map_o_reg_75_ ( .D(n5), .CK(n197), .Q(
        config_pattern_map_o[75]) );
  DFFHQX1 config_pattern_map_o_reg_74_ ( .D(n6), .CK(n197), .Q(
        config_pattern_map_o[74]) );
  DFFHQX1 config_pattern_map_o_reg_73_ ( .D(n7), .CK(n197), .Q(
        config_pattern_map_o[73]) );
  DFFHQX1 config_pattern_map_o_reg_72_ ( .D(n8), .CK(n198), .Q(
        config_pattern_map_o[72]) );
  DFFHQX1 config_pattern_map_o_reg_71_ ( .D(n9), .CK(n198), .Q(
        config_pattern_map_o[71]) );
  DFFHQX1 config_pattern_map_o_reg_70_ ( .D(n10), .CK(n198), .Q(
        config_pattern_map_o[70]) );
  DFFHQX1 config_pattern_map_o_reg_69_ ( .D(n11), .CK(n198), .Q(
        config_pattern_map_o[69]) );
  DFFHQX1 config_pattern_map_o_reg_68_ ( .D(n12), .CK(n198), .Q(
        config_pattern_map_o[68]) );
  DFFHQX1 config_pattern_map_o_reg_67_ ( .D(n13), .CK(n198), .Q(
        config_pattern_map_o[67]) );
  DFFHQX1 config_pattern_map_o_reg_66_ ( .D(n14), .CK(n199), .Q(
        config_pattern_map_o[66]) );
  DFFHQX1 config_pattern_map_o_reg_65_ ( .D(n15), .CK(n199), .Q(
        config_pattern_map_o[65]) );
  DFFHQX1 config_pattern_map_o_reg_64_ ( .D(n16), .CK(n199), .Q(
        config_pattern_map_o[64]) );
  DFFHQX1 config_pattern_map_o_reg_63_ ( .D(n17), .CK(n199), .Q(
        config_pattern_map_o[63]) );
  DFFHQX1 config_pattern_map_o_reg_62_ ( .D(n177), .CK(n199), .Q(
        config_pattern_map_o[62]) );
  DFFHQX1 config_pattern_map_o_reg_61_ ( .D(n18), .CK(n199), .Q(
        config_pattern_map_o[61]) );
  DFFHQX1 config_pattern_map_o_reg_60_ ( .D(n176), .CK(n200), .Q(
        config_pattern_map_o[60]) );
  DFFHQX1 config_pattern_map_o_reg_59_ ( .D(n59), .CK(n200), .Q(
        config_pattern_map_o[59]) );
  DFFHQX1 config_pattern_map_o_reg_58_ ( .D(n60), .CK(n200), .Q(
        config_pattern_map_o[58]) );
  DFFHQX1 config_pattern_map_o_reg_57_ ( .D(n61), .CK(n200), .Q(
        config_pattern_map_o[57]) );
  DFFHQX1 config_pattern_map_o_reg_56_ ( .D(n62), .CK(n200), .Q(
        config_pattern_map_o[56]) );
  DFFHQX1 config_pattern_map_o_reg_55_ ( .D(n63), .CK(n200), .Q(
        config_pattern_map_o[55]) );
  DFFHQX1 config_pattern_map_o_reg_54_ ( .D(n64), .CK(n201), .Q(
        config_pattern_map_o[54]) );
  DFFHQX1 config_pattern_map_o_reg_53_ ( .D(n65), .CK(n201), .Q(
        config_pattern_map_o[53]) );
  DFFHQX1 config_pattern_map_o_reg_52_ ( .D(n66), .CK(n201), .Q(
        config_pattern_map_o[52]) );
  DFFHQX1 config_pattern_map_o_reg_51_ ( .D(n67), .CK(n201), .Q(
        config_pattern_map_o[51]) );
  DFFHQX1 config_pattern_map_o_reg_50_ ( .D(n68), .CK(n201), .Q(
        config_pattern_map_o[50]) );
  DFFHQX1 config_pattern_map_o_reg_49_ ( .D(n69), .CK(n201), .Q(
        config_pattern_map_o[49]) );
  DFFHQX1 config_pattern_map_o_reg_48_ ( .D(n70), .CK(n202), .Q(
        config_pattern_map_o[48]) );
  DFFHQX1 config_pattern_map_o_reg_47_ ( .D(n71), .CK(n202), .Q(
        config_pattern_map_o[47]) );
  DFFHQX1 config_pattern_map_o_reg_46_ ( .D(n72), .CK(n202), .Q(
        config_pattern_map_o[46]) );
  DFFHQX1 config_pattern_map_o_reg_45_ ( .D(n73), .CK(n202), .Q(
        config_pattern_map_o[45]) );
  DFFHQX1 config_pattern_map_o_reg_44_ ( .D(n74), .CK(n202), .Q(
        config_pattern_map_o[44]) );
  DFFHQX1 config_pattern_map_o_reg_43_ ( .D(n75), .CK(n202), .Q(
        config_pattern_map_o[43]) );
  DFFHQX1 config_pattern_map_o_reg_42_ ( .D(n175), .CK(n203), .Q(
        config_pattern_map_o[42]) );
  DFFHQX1 config_pattern_map_o_reg_41_ ( .D(n174), .CK(n203), .Q(
        config_pattern_map_o[41]) );
  DFFHQX1 config_pattern_map_o_reg_40_ ( .D(n173), .CK(n203), .Q(
        config_pattern_map_o[40]) );
  DFFHQX1 config_pattern_map_o_reg_39_ ( .D(n21), .CK(n203), .Q(
        config_pattern_map_o[39]) );
  DFFHQX1 config_pattern_map_o_reg_38_ ( .D(n22), .CK(n203), .Q(
        config_pattern_map_o[38]) );
  DFFHQX1 config_pattern_map_o_reg_37_ ( .D(n23), .CK(n204), .Q(
        config_pattern_map_o[37]) );
  DFFHQX1 config_pattern_map_o_reg_36_ ( .D(n24), .CK(n204), .Q(
        config_pattern_map_o[36]) );
  DFFHQX1 config_pattern_map_o_reg_35_ ( .D(n25), .CK(n204), .Q(
        config_pattern_map_o[35]) );
  DFFHQX1 config_pattern_map_o_reg_34_ ( .D(n26), .CK(n204), .Q(
        config_pattern_map_o[34]) );
  DFFHQX1 config_pattern_map_o_reg_33_ ( .D(n27), .CK(n204), .Q(
        config_pattern_map_o[33]) );
  DFFHQX1 config_pattern_map_o_reg_32_ ( .D(n28), .CK(n204), .Q(
        config_pattern_map_o[32]) );
  DFFHQX1 config_pattern_map_o_reg_31_ ( .D(n29), .CK(n205), .Q(
        config_pattern_map_o[31]) );
  DFFHQX1 config_pattern_map_o_reg_30_ ( .D(n30), .CK(n205), .Q(
        config_pattern_map_o[30]) );
  DFFHQX1 config_pattern_map_o_reg_29_ ( .D(n31), .CK(n205), .Q(
        config_pattern_map_o[29]) );
  DFFHQX1 config_pattern_map_o_reg_28_ ( .D(n32), .CK(n205), .Q(
        config_pattern_map_o[28]) );
  DFFHQX1 config_pattern_map_o_reg_27_ ( .D(n33), .CK(n205), .Q(
        config_pattern_map_o[27]) );
  DFFHQX1 config_pattern_map_o_reg_26_ ( .D(n34), .CK(n205), .Q(
        config_pattern_map_o[26]) );
  DFFHQX1 config_pattern_map_o_reg_25_ ( .D(n35), .CK(n206), .Q(
        config_pattern_map_o[25]) );
  DFFHQX1 config_pattern_map_o_reg_24_ ( .D(n36), .CK(n206), .Q(
        config_pattern_map_o[24]) );
  DFFHQX1 config_pattern_map_o_reg_23_ ( .D(n37), .CK(n206), .Q(
        config_pattern_map_o[23]) );
  DFFHQX1 config_pattern_map_o_reg_22_ ( .D(n38), .CK(n206), .Q(
        config_pattern_map_o[22]) );
  DFFHQX1 config_pattern_map_o_reg_21_ ( .D(n172), .CK(n206), .Q(
        config_pattern_map_o[21]) );
  DFFHQX1 config_pattern_map_o_reg_20_ ( .D(n171), .CK(n206), .Q(
        config_pattern_map_o[20]) );
  DFFHQX1 config_pattern_map_o_reg_19_ ( .D(n41), .CK(n207), .Q(
        config_pattern_map_o[19]) );
  DFFHQX1 config_pattern_map_o_reg_18_ ( .D(n42), .CK(n207), .Q(
        config_pattern_map_o[18]) );
  DFFHQX1 config_pattern_map_o_reg_17_ ( .D(n43), .CK(n207), .Q(
        config_pattern_map_o[17]) );
  DFFHQX1 config_pattern_map_o_reg_16_ ( .D(n44), .CK(n207), .Q(
        config_pattern_map_o[16]) );
  DFFHQX1 config_pattern_map_o_reg_15_ ( .D(n45), .CK(n207), .Q(
        config_pattern_map_o[15]) );
  DFFHQX1 config_pattern_map_o_reg_14_ ( .D(n46), .CK(n207), .Q(
        config_pattern_map_o[14]) );
  DFFHQX1 config_pattern_map_o_reg_13_ ( .D(n47), .CK(n208), .Q(
        config_pattern_map_o[13]) );
  DFFHQX1 config_pattern_map_o_reg_12_ ( .D(n48), .CK(n208), .Q(
        config_pattern_map_o[12]) );
  DFFHQX1 config_pattern_map_o_reg_11_ ( .D(n49), .CK(n208), .Q(
        config_pattern_map_o[11]) );
  DFFHQX1 config_pattern_map_o_reg_10_ ( .D(n50), .CK(n208), .Q(
        config_pattern_map_o[10]) );
  DFFHQX1 config_pattern_map_o_reg_9_ ( .D(n51), .CK(n208), .Q(
        config_pattern_map_o[9]) );
  DFFHQX1 config_pattern_map_o_reg_8_ ( .D(n52), .CK(n208), .Q(
        config_pattern_map_o[8]) );
  DFFHQX1 config_pattern_map_o_reg_7_ ( .D(n53), .CK(n209), .Q(
        config_pattern_map_o[7]) );
  DFFHQX1 config_pattern_map_o_reg_6_ ( .D(n54), .CK(n209), .Q(
        config_pattern_map_o[6]) );
  DFFHQX1 config_pattern_map_o_reg_5_ ( .D(n55), .CK(n209), .Q(
        config_pattern_map_o[5]) );
  DFFHQX1 config_pattern_map_o_reg_4_ ( .D(n56), .CK(n209), .Q(
        config_pattern_map_o[4]) );
  DFFHQX1 config_pattern_map_o_reg_3_ ( .D(n170), .CK(n209), .Q(
        config_pattern_map_o[3]) );
  DFFHQX1 config_pattern_map_o_reg_2_ ( .D(n169), .CK(n209), .Q(
        config_pattern_map_o[2]) );
  DFFHQX1 config_pattern_map_o_reg_1_ ( .D(n168), .CK(n197), .Q(
        config_pattern_map_o[1]) );
  DFFHQX1 config_pattern_map_o_reg_0_ ( .D(n167), .CK(n196), .Q(
        config_pattern_map_o[0]) );
  OAI21XL U187 ( .A0(scan_index_reg[0]), .A1(n103), .B0(n78), .Y(n101) );
  INVX1 U188 ( .A(n101), .Y(n58) );
  OAI21XL U189 ( .A0(n103), .A1(n82), .B0(n78), .Y(n123) );
  INVX1 U190 ( .A(n123), .Y(n40) );
  OAI21XL U191 ( .A0(n143), .A1(n81), .B0(n78), .Y(n142) );
  INVX1 U192 ( .A(n142), .Y(n77) );
  INVX1 U193 ( .A(n145), .Y(n19) );
  INVX1 U194 ( .A(n164), .Y(n78) );
  BUFX3 U195 ( .A(n40), .Y(n192) );
  BUFX3 U196 ( .A(n40), .Y(n191) );
  BUFX3 U197 ( .A(n19), .Y(n195) );
  BUFX3 U198 ( .A(n19), .Y(n194) );
  INVX1 U199 ( .A(scan_index_reg[1]), .Y(n81) );
  NAND2X1 U200 ( .A(rst_ni), .B(n165), .Y(n164) );
  INVX1 U201 ( .A(sa_role_reg), .Y(n80) );
  NAND3X1 U202 ( .A(rst_ni), .B(n79), .C(start_i), .Y(n165) );
  INVX1 U203 ( .A(scan_index_reg[0]), .Y(n82) );
  NAND2X1 U204 ( .A(n78), .B(n103), .Y(n166) );
  AOI21X1 U205 ( .A0(n80), .A1(scan_index_reg[0]), .B0(n81), .Y(
        current_config_id_o[0]) );
  AOI21X1 U206 ( .A0(n80), .A1(n81), .B0(n82), .Y(current_config_id_o[1]) );
  AOI21X1 U207 ( .A0(n82), .A1(n81), .B0(sa_role_reg), .Y(
        current_config_id_o[2]) );
  INVX1 U208 ( .A(n85), .Y(n56) );
  INVX1 U209 ( .A(n86), .Y(n55) );
  INVX1 U210 ( .A(n87), .Y(n54) );
  INVX1 U211 ( .A(n210), .Y(n209) );
  INVX1 U212 ( .A(n88), .Y(n53) );
  INVX1 U213 ( .A(n89), .Y(n52) );
  INVX1 U214 ( .A(n90), .Y(n51) );
  INVX1 U215 ( .A(n91), .Y(n50) );
  INVX1 U216 ( .A(n92), .Y(n49) );
  INVX1 U217 ( .A(n93), .Y(n48) );
  INVX1 U218 ( .A(n211), .Y(n208) );
  INVX1 U219 ( .A(n94), .Y(n47) );
  INVX1 U220 ( .A(n95), .Y(n46) );
  INVX1 U221 ( .A(n96), .Y(n45) );
  INVX1 U222 ( .A(n97), .Y(n44) );
  INVX1 U223 ( .A(n98), .Y(n43) );
  INVX1 U224 ( .A(n99), .Y(n42) );
  INVX1 U225 ( .A(n213), .Y(n207) );
  INVX1 U226 ( .A(n100), .Y(n41) );
  INVX1 U227 ( .A(n105), .Y(n38) );
  INVX1 U228 ( .A(n106), .Y(n37) );
  INVX1 U229 ( .A(n107), .Y(n36) );
  INVX1 U230 ( .A(n211), .Y(n206) );
  INVX1 U231 ( .A(n108), .Y(n35) );
  INVX1 U232 ( .A(n109), .Y(n34) );
  INVX1 U233 ( .A(n110), .Y(n33) );
  INVX1 U234 ( .A(n111), .Y(n32) );
  INVX1 U235 ( .A(n112), .Y(n31) );
  INVX1 U236 ( .A(n113), .Y(n30) );
  INVX1 U237 ( .A(n211), .Y(n205) );
  INVX1 U238 ( .A(n114), .Y(n29) );
  INVX1 U239 ( .A(n115), .Y(n28) );
  INVX1 U240 ( .A(n116), .Y(n27) );
  INVX1 U241 ( .A(n117), .Y(n26) );
  INVX1 U242 ( .A(n118), .Y(n25) );
  INVX1 U243 ( .A(n119), .Y(n24) );
  INVX1 U244 ( .A(n210), .Y(n204) );
  INVX1 U245 ( .A(n120), .Y(n23) );
  INVX1 U246 ( .A(n121), .Y(n22) );
  INVX1 U247 ( .A(n122), .Y(n21) );
  INVX1 U248 ( .A(n125), .Y(n75) );
  INVX1 U249 ( .A(n126), .Y(n74) );
  INVX1 U250 ( .A(n127), .Y(n73) );
  INVX1 U251 ( .A(n128), .Y(n72) );
  INVX1 U252 ( .A(n129), .Y(n71) );
  INVX1 U253 ( .A(n212), .Y(n202) );
  INVX1 U254 ( .A(n130), .Y(n70) );
  INVX1 U255 ( .A(n131), .Y(n69) );
  INVX1 U256 ( .A(n132), .Y(n68) );
  INVX1 U257 ( .A(n133), .Y(n67) );
  INVX1 U258 ( .A(n134), .Y(n66) );
  INVX1 U259 ( .A(n135), .Y(n65) );
  INVX1 U260 ( .A(n212), .Y(n201) );
  INVX1 U261 ( .A(n136), .Y(n64) );
  INVX1 U262 ( .A(n137), .Y(n63) );
  INVX1 U263 ( .A(n138), .Y(n62) );
  INVX1 U264 ( .A(n139), .Y(n61) );
  INVX1 U265 ( .A(n140), .Y(n60) );
  INVX1 U266 ( .A(n141), .Y(n59) );
  INVX1 U267 ( .A(n211), .Y(n200) );
  OAI2BB1X1 U268 ( .A0N(config_pattern_map_o[60]), .A1N(n184), .B0(n145), .Y(
        n176) );
  INVX1 U269 ( .A(n146), .Y(n18) );
  OAI2BB1X1 U270 ( .A0N(config_pattern_map_o[62]), .A1N(n183), .B0(n145), .Y(
        n177) );
  INVX1 U271 ( .A(n147), .Y(n17) );
  INVX1 U272 ( .A(n148), .Y(n16) );
  INVX1 U273 ( .A(n149), .Y(n15) );
  INVX1 U274 ( .A(n212), .Y(n199) );
  INVX1 U275 ( .A(n150), .Y(n14) );
  INVX1 U276 ( .A(n151), .Y(n13) );
  INVX1 U277 ( .A(n152), .Y(n12) );
  INVX1 U278 ( .A(n153), .Y(n11) );
  INVX1 U279 ( .A(n154), .Y(n10) );
  INVX1 U280 ( .A(n155), .Y(n9) );
  INVX1 U281 ( .A(n210), .Y(n198) );
  INVX1 U282 ( .A(n156), .Y(n8) );
  INVX1 U283 ( .A(n157), .Y(n7) );
  INVX1 U284 ( .A(n158), .Y(n6) );
  INVX1 U285 ( .A(n159), .Y(n5) );
  INVX1 U286 ( .A(n160), .Y(n4) );
  INVX1 U287 ( .A(n161), .Y(n3) );
  INVX1 U288 ( .A(n162), .Y(n2) );
  INVX1 U289 ( .A(n163), .Y(n1) );
  OAI22X1 U290 ( .A0(n81), .A1(n166), .B0(n103), .B1(n82), .Y(n180) );
  OAI22X1 U291 ( .A0(n164), .A1(n80), .B0(n83), .B1(n165), .Y(n178) );
  INVX1 U292 ( .A(sa_role_i), .Y(n83) );
  OAI21XL U293 ( .A0(n79), .A1(n182), .B0(n165), .Y(n179) );
  INVX1 U294 ( .A(n213), .Y(n196) );
  AND3X2 U295 ( .A(scan_index_reg[0]), .B(n102), .C(scan_index_reg[1]), .Y(N92) );
  INVX1 U296 ( .A(n213), .Y(n203) );
  OAI21XL U297 ( .A0(n82), .A1(n166), .B0(n143), .Y(n181) );
  INVX1 U298 ( .A(clk_i), .Y(n212) );
  INVX1 U299 ( .A(clk_i), .Y(n211) );
  NOR2X1 U300 ( .A(n164), .B(N92), .Y(n144) );
  INVX1 U301 ( .A(clk_i), .Y(n213) );
  INVX1 U302 ( .A(n104), .Y(n39) );
  BUFX3 U303 ( .A(n39), .Y(n193) );
  INVX1 U304 ( .A(n84), .Y(n57) );
  BUFX3 U305 ( .A(n57), .Y(n190) );
  INVX1 U306 ( .A(n124), .Y(n76) );
  BUFX3 U307 ( .A(n76), .Y(n187) );
  INVX1 U308 ( .A(clk_i), .Y(n210) );
  INVX1 U309 ( .A(n213), .Y(n197) );
  BUFX3 U310 ( .A(n77), .Y(n186) );
  BUFX3 U311 ( .A(n77), .Y(n185) );
  BUFX3 U312 ( .A(n58), .Y(n189) );
  BUFX3 U313 ( .A(n58), .Y(n188) );
  NAND2X1 U314 ( .A(n102), .B(n81), .Y(n103) );
  NAND2X1 U315 ( .A(n102), .B(n82), .Y(n143) );
  NAND2X1 U316 ( .A(n102), .B(n182), .Y(n145) );
  NAND2X1 U317 ( .A(n102), .B(n142), .Y(n124) );
  NAND2X1 U318 ( .A(n102), .B(n123), .Y(n104) );
  NAND2XL U319 ( .A(n102), .B(n101), .Y(n84) );
  NOR2BX1 U320 ( .AN(rst_ni), .B(n79), .Y(n102) );
  AOI22XL U321 ( .A0(n190), .A1(analyzer_candidate_valid_i[9]), .B0(
        config_pattern_map_o[19]), .B1(n188), .Y(n100) );
  AOI22XL U322 ( .A0(n190), .A1(analyzer_candidate_valid_i[8]), .B0(
        config_pattern_map_o[18]), .B1(n188), .Y(n99) );
  AOI22XL U323 ( .A0(n190), .A1(analyzer_candidate_valid_i[7]), .B0(
        config_pattern_map_o[17]), .B1(n188), .Y(n98) );
  AOI22XL U324 ( .A0(n190), .A1(analyzer_candidate_valid_i[6]), .B0(
        config_pattern_map_o[16]), .B1(n188), .Y(n97) );
  AOI22XL U325 ( .A0(n190), .A1(analyzer_candidate_valid_i[5]), .B0(
        config_pattern_map_o[15]), .B1(n188), .Y(n96) );
  AOI22XL U326 ( .A0(n190), .A1(analyzer_candidate_valid_i[4]), .B0(
        config_pattern_map_o[14]), .B1(n188), .Y(n95) );
  AOI22X1 U327 ( .A0(n190), .A1(analyzer_candidate_valid_i[3]), .B0(
        config_pattern_map_o[13]), .B1(n189), .Y(n94) );
  AOI22X1 U328 ( .A0(n190), .A1(analyzer_candidate_valid_i[2]), .B0(
        config_pattern_map_o[12]), .B1(n189), .Y(n93) );
  AOI22X1 U329 ( .A0(n57), .A1(analyzer_candidate_valid_i[1]), .B0(
        config_pattern_map_o[11]), .B1(n189), .Y(n92) );
  AOI22XL U330 ( .A0(n195), .A1(current_config_id_o[0]), .B0(
        config_pattern_map_o[61]), .B1(n183), .Y(n146) );
  AOI22XL U331 ( .A0(analyzer_candidate_valid_i[0]), .A1(n195), .B0(
        config_pattern_map_o[70]), .B1(n184), .Y(n154) );
  AOI22XL U332 ( .A0(analyzer_pattern_id_i[3]), .A1(n195), .B0(
        config_pattern_map_o[69]), .B1(n184), .Y(n153) );
  AOI22XL U333 ( .A0(analyzer_pattern_id_i[2]), .A1(n195), .B0(
        config_pattern_map_o[68]), .B1(n144), .Y(n152) );
  AOI22XL U334 ( .A0(analyzer_pattern_id_i[1]), .A1(n195), .B0(
        config_pattern_map_o[67]), .B1(n184), .Y(n151) );
  AOI22XL U335 ( .A0(analyzer_pattern_id_i[0]), .A1(n195), .B0(
        config_pattern_map_o[66]), .B1(n183), .Y(n150) );
  AOI22XL U336 ( .A0(analyzer_repairable_i), .A1(n195), .B0(
        config_pattern_map_o[65]), .B1(n183), .Y(n149) );
  AOI22X1 U337 ( .A0(analyzer_solution_valid_i), .A1(n195), .B0(
        config_pattern_map_o[64]), .B1(n183), .Y(n148) );
  AOI22X1 U338 ( .A0(current_config_id_o[2]), .A1(n195), .B0(
        config_pattern_map_o[63]), .B1(n183), .Y(n147) );
  AOI22XL U339 ( .A0(analyzer_candidate_valid_i[9]), .A1(n187), .B0(
        config_pattern_map_o[59]), .B1(n185), .Y(n141) );
  AOI22XL U340 ( .A0(analyzer_candidate_valid_i[8]), .A1(n187), .B0(
        config_pattern_map_o[58]), .B1(n185), .Y(n140) );
  AOI22XL U341 ( .A0(analyzer_candidate_valid_i[7]), .A1(n187), .B0(
        config_pattern_map_o[57]), .B1(n185), .Y(n139) );
  AOI22XL U342 ( .A0(analyzer_candidate_valid_i[6]), .A1(n187), .B0(
        config_pattern_map_o[56]), .B1(n185), .Y(n138) );
  AOI22XL U343 ( .A0(analyzer_candidate_valid_i[5]), .A1(n187), .B0(
        config_pattern_map_o[55]), .B1(n185), .Y(n137) );
  AOI22XL U344 ( .A0(analyzer_candidate_valid_i[4]), .A1(n187), .B0(
        config_pattern_map_o[54]), .B1(n185), .Y(n136) );
  AOI22XL U345 ( .A0(analyzer_candidate_valid_i[3]), .A1(n187), .B0(
        config_pattern_map_o[53]), .B1(n185), .Y(n135) );
  AOI22X1 U346 ( .A0(analyzer_candidate_valid_i[2]), .A1(n187), .B0(
        config_pattern_map_o[52]), .B1(n186), .Y(n134) );
  AOI22X1 U347 ( .A0(analyzer_candidate_valid_i[1]), .A1(n187), .B0(
        config_pattern_map_o[51]), .B1(n186), .Y(n133) );
  AOI22XL U348 ( .A0(analyzer_candidate_valid_i[9]), .A1(n194), .B0(
        config_pattern_map_o[79]), .B1(n184), .Y(n163) );
  AOI22XL U349 ( .A0(analyzer_candidate_valid_i[8]), .A1(n194), .B0(
        config_pattern_map_o[78]), .B1(n183), .Y(n162) );
  AOI22XL U350 ( .A0(analyzer_candidate_valid_i[7]), .A1(n194), .B0(
        config_pattern_map_o[77]), .B1(n144), .Y(n161) );
  AOI22XL U351 ( .A0(analyzer_candidate_valid_i[6]), .A1(n194), .B0(
        config_pattern_map_o[76]), .B1(n184), .Y(n160) );
  AOI22XL U352 ( .A0(analyzer_candidate_valid_i[5]), .A1(n194), .B0(
        config_pattern_map_o[75]), .B1(n144), .Y(n159) );
  AOI22XL U353 ( .A0(analyzer_candidate_valid_i[4]), .A1(n194), .B0(
        config_pattern_map_o[74]), .B1(n144), .Y(n158) );
  AOI22XL U354 ( .A0(analyzer_candidate_valid_i[3]), .A1(n194), .B0(
        config_pattern_map_o[73]), .B1(n184), .Y(n157) );
  AOI22X1 U355 ( .A0(analyzer_candidate_valid_i[2]), .A1(n194), .B0(
        config_pattern_map_o[72]), .B1(n144), .Y(n156) );
  AOI22X1 U356 ( .A0(analyzer_candidate_valid_i[1]), .A1(n194), .B0(
        config_pattern_map_o[71]), .B1(n144), .Y(n155) );
  AND2X1 U357 ( .A(config_pattern_map_o[21]), .B(n192), .Y(n172) );
  OAI2BB1X1 U358 ( .A0N(config_pattern_map_o[20]), .A1N(n192), .B0(n104), .Y(
        n171) );
  AOI22XL U359 ( .A0(analyzer_pattern_id_i[3]), .A1(n193), .B0(
        config_pattern_map_o[29]), .B1(n192), .Y(n112) );
  AOI22XL U360 ( .A0(analyzer_pattern_id_i[2]), .A1(n193), .B0(
        config_pattern_map_o[28]), .B1(n192), .Y(n111) );
  AOI22XL U361 ( .A0(analyzer_pattern_id_i[1]), .A1(n193), .B0(
        config_pattern_map_o[27]), .B1(n192), .Y(n110) );
  AOI22XL U362 ( .A0(analyzer_pattern_id_i[0]), .A1(n193), .B0(
        config_pattern_map_o[26]), .B1(n192), .Y(n109) );
  AOI22XL U363 ( .A0(analyzer_repairable_i), .A1(n193), .B0(
        config_pattern_map_o[25]), .B1(n192), .Y(n108) );
  AOI22XL U364 ( .A0(analyzer_solution_valid_i), .A1(n193), .B0(
        config_pattern_map_o[24]), .B1(n192), .Y(n107) );
  AOI22X1 U365 ( .A0(n193), .A1(current_config_id_o[2]), .B0(
        config_pattern_map_o[23]), .B1(n192), .Y(n106) );
  AOI22X1 U366 ( .A0(n193), .A1(current_config_id_o[1]), .B0(
        config_pattern_map_o[22]), .B1(n192), .Y(n105) );
  AND2X1 U367 ( .A(config_pattern_map_o[3]), .B(n188), .Y(n170) );
  AND2X1 U368 ( .A(config_pattern_map_o[2]), .B(n188), .Y(n169) );
  AND2X1 U369 ( .A(config_pattern_map_o[1]), .B(n188), .Y(n168) );
  OAI2BB1X1 U370 ( .A0N(config_pattern_map_o[0]), .A1N(n188), .B0(n84), .Y(
        n167) );
  AOI22XL U371 ( .A0(n190), .A1(analyzer_candidate_valid_i[0]), .B0(
        config_pattern_map_o[10]), .B1(n189), .Y(n91) );
  AOI22XL U372 ( .A0(n57), .A1(analyzer_pattern_id_i[3]), .B0(
        config_pattern_map_o[9]), .B1(n189), .Y(n90) );
  AOI22XL U373 ( .A0(n190), .A1(analyzer_pattern_id_i[2]), .B0(
        config_pattern_map_o[8]), .B1(n189), .Y(n89) );
  AOI22XL U374 ( .A0(n57), .A1(analyzer_pattern_id_i[1]), .B0(
        config_pattern_map_o[7]), .B1(n189), .Y(n88) );
  AOI22XL U375 ( .A0(n57), .A1(analyzer_pattern_id_i[0]), .B0(
        config_pattern_map_o[6]), .B1(n189), .Y(n87) );
  AOI22XL U376 ( .A0(n57), .A1(analyzer_repairable_i), .B0(
        config_pattern_map_o[5]), .B1(n189), .Y(n86) );
  AOI22X1 U377 ( .A0(n57), .A1(analyzer_solution_valid_i), .B0(
        config_pattern_map_o[4]), .B1(n189), .Y(n85) );
  AND2X1 U378 ( .A(config_pattern_map_o[42]), .B(n185), .Y(n175) );
  OAI2BB1X1 U379 ( .A0N(config_pattern_map_o[41]), .A1N(n185), .B0(n124), .Y(
        n174) );
  OAI2BB1X1 U380 ( .A0N(config_pattern_map_o[40]), .A1N(n185), .B0(n124), .Y(
        n173) );
  AOI22XL U381 ( .A0(analyzer_candidate_valid_i[0]), .A1(n187), .B0(
        config_pattern_map_o[50]), .B1(n186), .Y(n132) );
  AOI22XL U382 ( .A0(analyzer_pattern_id_i[3]), .A1(n76), .B0(
        config_pattern_map_o[49]), .B1(n186), .Y(n131) );
  AOI22XL U383 ( .A0(analyzer_pattern_id_i[2]), .A1(n76), .B0(
        config_pattern_map_o[48]), .B1(n186), .Y(n130) );
  AOI22XL U384 ( .A0(analyzer_pattern_id_i[1]), .A1(n76), .B0(
        config_pattern_map_o[47]), .B1(n186), .Y(n129) );
  AOI22XL U385 ( .A0(analyzer_pattern_id_i[0]), .A1(n76), .B0(
        config_pattern_map_o[46]), .B1(n186), .Y(n128) );
  AOI22XL U386 ( .A0(analyzer_repairable_i), .A1(n76), .B0(
        config_pattern_map_o[45]), .B1(n186), .Y(n127) );
  AOI22XL U387 ( .A0(analyzer_solution_valid_i), .A1(n76), .B0(
        config_pattern_map_o[44]), .B1(n186), .Y(n126) );
  AOI22X1 U388 ( .A0(current_config_id_o[2]), .A1(n76), .B0(
        config_pattern_map_o[43]), .B1(n186), .Y(n125) );
  AOI22XL U389 ( .A0(analyzer_candidate_valid_i[9]), .A1(n39), .B0(
        config_pattern_map_o[39]), .B1(n191), .Y(n122) );
  AOI22XL U390 ( .A0(analyzer_candidate_valid_i[8]), .A1(n193), .B0(
        config_pattern_map_o[38]), .B1(n191), .Y(n121) );
  AOI22XL U391 ( .A0(analyzer_candidate_valid_i[7]), .A1(n39), .B0(
        config_pattern_map_o[37]), .B1(n191), .Y(n120) );
  AOI22XL U392 ( .A0(analyzer_candidate_valid_i[6]), .A1(n39), .B0(
        config_pattern_map_o[36]), .B1(n191), .Y(n119) );
  AOI22XL U393 ( .A0(analyzer_candidate_valid_i[5]), .A1(n39), .B0(
        config_pattern_map_o[35]), .B1(n191), .Y(n118) );
  AOI22XL U394 ( .A0(analyzer_candidate_valid_i[4]), .A1(n39), .B0(
        config_pattern_map_o[34]), .B1(n191), .Y(n117) );
  AOI22XL U395 ( .A0(analyzer_candidate_valid_i[3]), .A1(n39), .B0(
        config_pattern_map_o[33]), .B1(n191), .Y(n116) );
  AOI22XL U396 ( .A0(analyzer_candidate_valid_i[2]), .A1(n39), .B0(
        config_pattern_map_o[32]), .B1(n191), .Y(n115) );
  AOI22X1 U397 ( .A0(analyzer_candidate_valid_i[1]), .A1(n39), .B0(
        config_pattern_map_o[31]), .B1(n191), .Y(n114) );
  AOI22XL U398 ( .A0(analyzer_candidate_valid_i[0]), .A1(n193), .B0(
        config_pattern_map_o[30]), .B1(n191), .Y(n113) );
  INVXL U399 ( .A(n144), .Y(n182) );
  INVXL U400 ( .A(n182), .Y(n183) );
  INVXL U401 ( .A(n182), .Y(n184) );
endmodule

