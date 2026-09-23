`default_nettype none

// HYP02 fixed-four-edge selector. All legal GLOBAL tuples are fixed at
// elaboration; this module has no ledger, donor search, score, or path state.
module recam_dss_hyp02_static_selector (
    input  logic [79:0] candidate_store_image_i,
    output logic selected_valid_o,
    output logic [1:0] selected_a_slot_o,
    output logic [1:0] selected_b_slot_o,
    output logic [1:0] selected_c_slot_o,
    output logic [1:0] selected_d_slot_o,
    output logic [2:0] selected_a_config_id_o,
    output logic [2:0] selected_b_config_id_o,
    output logic [2:0] selected_c_config_id_o,
    output logic [2:0] selected_d_config_id_o,
    output logic [3:0] selected_a_pattern_id_o,
    output logic [3:0] selected_b_pattern_id_o,
    output logic [3:0] selected_c_pattern_id_o,
    output logic [3:0] selected_d_pattern_id_o
);
    logic [3:0] a_valid;
    logic [3:0] b_valid;
    logic [3:0] c_valid;
    logic [3:0] d_valid;
    logic [3:0] a_pattern [0:3];
    logic [3:0] b_pattern [0:3];
    logic [3:0] c_pattern [0:3];
    logic [3:0] d_pattern [0:3];
    logic [80:0] path_valid;

    assign a_valid[0] = candidate_store_image_i[0];
    assign a_valid[1] = candidate_store_image_i[5];
    assign a_valid[2] = candidate_store_image_i[10];
    assign a_valid[3] = candidate_store_image_i[15];
    assign b_valid[0] = candidate_store_image_i[20];
    assign b_valid[1] = candidate_store_image_i[25];
    assign b_valid[2] = candidate_store_image_i[30];
    assign b_valid[3] = candidate_store_image_i[35];
    assign c_valid[0] = candidate_store_image_i[40];
    assign c_valid[1] = candidate_store_image_i[45];
    assign c_valid[2] = candidate_store_image_i[50];
    assign c_valid[3] = candidate_store_image_i[55];
    assign d_valid[0] = candidate_store_image_i[60];
    assign d_valid[1] = candidate_store_image_i[65];
    assign d_valid[2] = candidate_store_image_i[70];
    assign d_valid[3] = candidate_store_image_i[75];

    assign a_pattern[0] = candidate_store_image_i[4:1];
    assign a_pattern[1] = candidate_store_image_i[9:6];
    assign a_pattern[2] = candidate_store_image_i[14:11];
    assign a_pattern[3] = candidate_store_image_i[19:16];
    assign b_pattern[0] = candidate_store_image_i[24:21];
    assign b_pattern[1] = candidate_store_image_i[29:26];
    assign b_pattern[2] = candidate_store_image_i[34:31];
    assign b_pattern[3] = candidate_store_image_i[39:36];
    assign c_pattern[0] = candidate_store_image_i[44:41];
    assign c_pattern[1] = candidate_store_image_i[49:46];
    assign c_pattern[2] = candidate_store_image_i[54:51];
    assign c_pattern[3] = candidate_store_image_i[59:56];
    assign d_pattern[0] = candidate_store_image_i[64:61];
    assign d_pattern[1] = candidate_store_image_i[69:66];
    assign d_pattern[2] = candidate_store_image_i[74:71];
    assign d_pattern[3] = candidate_store_image_i[79:76];

    assign path_valid[0] = a_valid[0] & b_valid[0] & c_valid[0] & d_valid[0];
    assign path_valid[1] = a_valid[1] & b_valid[0] & c_valid[0] & d_valid[0];
    assign path_valid[2] = a_valid[0] & b_valid[1] & c_valid[0] & d_valid[0];
    assign path_valid[3] = a_valid[1] & b_valid[1] & c_valid[0] & d_valid[0];
    assign path_valid[4] = a_valid[2] & b_valid[1] & c_valid[0] & d_valid[0];
    assign path_valid[5] = a_valid[3] & b_valid[1] & c_valid[0] & d_valid[0];
    assign path_valid[6] = a_valid[0] & b_valid[0] & c_valid[1] & d_valid[0];
    assign path_valid[7] = a_valid[1] & b_valid[0] & c_valid[1] & d_valid[0];
    assign path_valid[8] = a_valid[0] & b_valid[1] & c_valid[1] & d_valid[0];
    assign path_valid[9] = a_valid[1] & b_valid[1] & c_valid[1] & d_valid[0];
    assign path_valid[10] = a_valid[2] & b_valid[1] & c_valid[1] & d_valid[0];
    assign path_valid[11] = a_valid[3] & b_valid[1] & c_valid[1] & d_valid[0];
    assign path_valid[12] = a_valid[1] & b_valid[0] & c_valid[2] & d_valid[0];
    assign path_valid[13] = a_valid[1] & b_valid[1] & c_valid[2] & d_valid[0];
    assign path_valid[14] = a_valid[3] & b_valid[1] & c_valid[2] & d_valid[0];
    assign path_valid[15] = a_valid[1] & b_valid[0] & c_valid[3] & d_valid[0];
    assign path_valid[16] = a_valid[1] & b_valid[1] & c_valid[3] & d_valid[0];
    assign path_valid[17] = a_valid[3] & b_valid[1] & c_valid[3] & d_valid[0];
    assign path_valid[18] = a_valid[0] & b_valid[0] & c_valid[0] & d_valid[1];
    assign path_valid[19] = a_valid[1] & b_valid[0] & c_valid[0] & d_valid[1];
    assign path_valid[20] = a_valid[0] & b_valid[1] & c_valid[0] & d_valid[1];
    assign path_valid[21] = a_valid[1] & b_valid[1] & c_valid[0] & d_valid[1];
    assign path_valid[22] = a_valid[2] & b_valid[1] & c_valid[0] & d_valid[1];
    assign path_valid[23] = a_valid[3] & b_valid[1] & c_valid[0] & d_valid[1];
    assign path_valid[24] = a_valid[0] & b_valid[2] & c_valid[0] & d_valid[1];
    assign path_valid[25] = a_valid[1] & b_valid[2] & c_valid[0] & d_valid[1];
    assign path_valid[26] = a_valid[0] & b_valid[3] & c_valid[0] & d_valid[1];
    assign path_valid[27] = a_valid[1] & b_valid[3] & c_valid[0] & d_valid[1];
    assign path_valid[28] = a_valid[2] & b_valid[3] & c_valid[0] & d_valid[1];
    assign path_valid[29] = a_valid[3] & b_valid[3] & c_valid[0] & d_valid[1];
    assign path_valid[30] = a_valid[0] & b_valid[0] & c_valid[1] & d_valid[1];
    assign path_valid[31] = a_valid[1] & b_valid[0] & c_valid[1] & d_valid[1];
    assign path_valid[32] = a_valid[0] & b_valid[1] & c_valid[1] & d_valid[1];
    assign path_valid[33] = a_valid[1] & b_valid[1] & c_valid[1] & d_valid[1];
    assign path_valid[34] = a_valid[2] & b_valid[1] & c_valid[1] & d_valid[1];
    assign path_valid[35] = a_valid[3] & b_valid[1] & c_valid[1] & d_valid[1];
    assign path_valid[36] = a_valid[0] & b_valid[2] & c_valid[1] & d_valid[1];
    assign path_valid[37] = a_valid[1] & b_valid[2] & c_valid[1] & d_valid[1];
    assign path_valid[38] = a_valid[0] & b_valid[3] & c_valid[1] & d_valid[1];
    assign path_valid[39] = a_valid[1] & b_valid[3] & c_valid[1] & d_valid[1];
    assign path_valid[40] = a_valid[2] & b_valid[3] & c_valid[1] & d_valid[1];
    assign path_valid[41] = a_valid[3] & b_valid[3] & c_valid[1] & d_valid[1];
    assign path_valid[42] = a_valid[1] & b_valid[0] & c_valid[2] & d_valid[1];
    assign path_valid[43] = a_valid[1] & b_valid[1] & c_valid[2] & d_valid[1];
    assign path_valid[44] = a_valid[3] & b_valid[1] & c_valid[2] & d_valid[1];
    assign path_valid[45] = a_valid[1] & b_valid[2] & c_valid[2] & d_valid[1];
    assign path_valid[46] = a_valid[1] & b_valid[3] & c_valid[2] & d_valid[1];
    assign path_valid[47] = a_valid[3] & b_valid[3] & c_valid[2] & d_valid[1];
    assign path_valid[48] = a_valid[1] & b_valid[0] & c_valid[3] & d_valid[1];
    assign path_valid[49] = a_valid[1] & b_valid[1] & c_valid[3] & d_valid[1];
    assign path_valid[50] = a_valid[3] & b_valid[1] & c_valid[3] & d_valid[1];
    assign path_valid[51] = a_valid[1] & b_valid[2] & c_valid[3] & d_valid[1];
    assign path_valid[52] = a_valid[1] & b_valid[3] & c_valid[3] & d_valid[1];
    assign path_valid[53] = a_valid[3] & b_valid[3] & c_valid[3] & d_valid[1];
    assign path_valid[54] = a_valid[0] & b_valid[0] & c_valid[1] & d_valid[2];
    assign path_valid[55] = a_valid[1] & b_valid[0] & c_valid[1] & d_valid[2];
    assign path_valid[56] = a_valid[0] & b_valid[1] & c_valid[1] & d_valid[2];
    assign path_valid[57] = a_valid[1] & b_valid[1] & c_valid[1] & d_valid[2];
    assign path_valid[58] = a_valid[2] & b_valid[1] & c_valid[1] & d_valid[2];
    assign path_valid[59] = a_valid[3] & b_valid[1] & c_valid[1] & d_valid[2];
    assign path_valid[60] = a_valid[1] & b_valid[0] & c_valid[3] & d_valid[2];
    assign path_valid[61] = a_valid[1] & b_valid[1] & c_valid[3] & d_valid[2];
    assign path_valid[62] = a_valid[3] & b_valid[1] & c_valid[3] & d_valid[2];
    assign path_valid[63] = a_valid[0] & b_valid[0] & c_valid[1] & d_valid[3];
    assign path_valid[64] = a_valid[1] & b_valid[0] & c_valid[1] & d_valid[3];
    assign path_valid[65] = a_valid[0] & b_valid[1] & c_valid[1] & d_valid[3];
    assign path_valid[66] = a_valid[1] & b_valid[1] & c_valid[1] & d_valid[3];
    assign path_valid[67] = a_valid[2] & b_valid[1] & c_valid[1] & d_valid[3];
    assign path_valid[68] = a_valid[3] & b_valid[1] & c_valid[1] & d_valid[3];
    assign path_valid[69] = a_valid[0] & b_valid[2] & c_valid[1] & d_valid[3];
    assign path_valid[70] = a_valid[1] & b_valid[2] & c_valid[1] & d_valid[3];
    assign path_valid[71] = a_valid[0] & b_valid[3] & c_valid[1] & d_valid[3];
    assign path_valid[72] = a_valid[1] & b_valid[3] & c_valid[1] & d_valid[3];
    assign path_valid[73] = a_valid[2] & b_valid[3] & c_valid[1] & d_valid[3];
    assign path_valid[74] = a_valid[3] & b_valid[3] & c_valid[1] & d_valid[3];
    assign path_valid[75] = a_valid[1] & b_valid[0] & c_valid[3] & d_valid[3];
    assign path_valid[76] = a_valid[1] & b_valid[1] & c_valid[3] & d_valid[3];
    assign path_valid[77] = a_valid[3] & b_valid[1] & c_valid[3] & d_valid[3];
    assign path_valid[78] = a_valid[1] & b_valid[2] & c_valid[3] & d_valid[3];
    assign path_valid[79] = a_valid[1] & b_valid[3] & c_valid[3] & d_valid[3];
    assign path_valid[80] = a_valid[3] & b_valid[3] & c_valid[3] & d_valid[3];

    always_comb begin
        selected_valid_o = 1'b0;
        selected_a_slot_o = 2'd0;
        selected_b_slot_o = 2'd0;
        selected_c_slot_o = 2'd0;
        selected_d_slot_o = 2'd0;
        priority case (1'b1)
            path_valid[0]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd0;
            end
            path_valid[1]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd0;
            end
            path_valid[2]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd0;
            end
            path_valid[3]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd0;
            end
            path_valid[4]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd2;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd0;
            end
            path_valid[5]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd0;
            end
            path_valid[6]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd0;
            end
            path_valid[7]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd0;
            end
            path_valid[8]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd0;
            end
            path_valid[9]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd0;
            end
            path_valid[10]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd2;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd0;
            end
            path_valid[11]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd0;
            end
            path_valid[12]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd2;
                selected_d_slot_o = 2'd0;
            end
            path_valid[13]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd2;
                selected_d_slot_o = 2'd0;
            end
            path_valid[14]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd2;
                selected_d_slot_o = 2'd0;
            end
            path_valid[15]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd0;
            end
            path_valid[16]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd0;
            end
            path_valid[17]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd0;
            end
            path_valid[18]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd1;
            end
            path_valid[19]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd1;
            end
            path_valid[20]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd1;
            end
            path_valid[21]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd1;
            end
            path_valid[22]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd2;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd1;
            end
            path_valid[23]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd1;
            end
            path_valid[24]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd2;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd1;
            end
            path_valid[25]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd2;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd1;
            end
            path_valid[26]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd1;
            end
            path_valid[27]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd1;
            end
            path_valid[28]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd2;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd1;
            end
            path_valid[29]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd0;
                selected_d_slot_o = 2'd1;
            end
            path_valid[30]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd1;
            end
            path_valid[31]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd1;
            end
            path_valid[32]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd1;
            end
            path_valid[33]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd1;
            end
            path_valid[34]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd2;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd1;
            end
            path_valid[35]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd1;
            end
            path_valid[36]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd2;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd1;
            end
            path_valid[37]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd2;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd1;
            end
            path_valid[38]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd1;
            end
            path_valid[39]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd1;
            end
            path_valid[40]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd2;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd1;
            end
            path_valid[41]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd1;
            end
            path_valid[42]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd2;
                selected_d_slot_o = 2'd1;
            end
            path_valid[43]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd2;
                selected_d_slot_o = 2'd1;
            end
            path_valid[44]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd2;
                selected_d_slot_o = 2'd1;
            end
            path_valid[45]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd2;
                selected_c_slot_o = 2'd2;
                selected_d_slot_o = 2'd1;
            end
            path_valid[46]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd2;
                selected_d_slot_o = 2'd1;
            end
            path_valid[47]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd2;
                selected_d_slot_o = 2'd1;
            end
            path_valid[48]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd1;
            end
            path_valid[49]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd1;
            end
            path_valid[50]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd1;
            end
            path_valid[51]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd2;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd1;
            end
            path_valid[52]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd1;
            end
            path_valid[53]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd1;
            end
            path_valid[54]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd2;
            end
            path_valid[55]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd2;
            end
            path_valid[56]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd2;
            end
            path_valid[57]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd2;
            end
            path_valid[58]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd2;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd2;
            end
            path_valid[59]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd2;
            end
            path_valid[60]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd2;
            end
            path_valid[61]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd2;
            end
            path_valid[62]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd2;
            end
            path_valid[63]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd3;
            end
            path_valid[64]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd3;
            end
            path_valid[65]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd3;
            end
            path_valid[66]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd3;
            end
            path_valid[67]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd2;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd3;
            end
            path_valid[68]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd3;
            end
            path_valid[69]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd2;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd3;
            end
            path_valid[70]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd2;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd3;
            end
            path_valid[71]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd0;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd3;
            end
            path_valid[72]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd3;
            end
            path_valid[73]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd2;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd3;
            end
            path_valid[74]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd1;
                selected_d_slot_o = 2'd3;
            end
            path_valid[75]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd0;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd3;
            end
            path_valid[76]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd3;
            end
            path_valid[77]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd1;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd3;
            end
            path_valid[78]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd2;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd3;
            end
            path_valid[79]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd1;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd3;
            end
            path_valid[80]: begin
                selected_valid_o = 1'b1;
                selected_a_slot_o = 2'd3;
                selected_b_slot_o = 2'd3;
                selected_c_slot_o = 2'd3;
                selected_d_slot_o = 2'd3;
            end
            default: begin end
        endcase
    end

    always_comb begin
        selected_a_config_id_o = 3'd0;
        selected_b_config_id_o = 3'd0;
        selected_c_config_id_o = 3'd0;
        selected_d_config_id_o = 3'd0;
        case (selected_a_slot_o)
            2'd0: selected_a_config_id_o = 3'd0;
            2'd1: selected_a_config_id_o = 3'd4;
            2'd2: selected_a_config_id_o = 3'd5;
            default: selected_a_config_id_o = 3'd6;
        endcase
        case (selected_b_slot_o)
            2'd0: selected_b_config_id_o = 3'd0;
            2'd1: selected_b_config_id_o = 3'd1;
            2'd2: selected_b_config_id_o = 3'd2;
            default: selected_b_config_id_o = 3'd3;
        endcase
        case (selected_c_slot_o)
            2'd0: selected_c_config_id_o = 3'd0;
            2'd1: selected_c_config_id_o = 3'd1;
            2'd2: selected_c_config_id_o = 3'd2;
            default: selected_c_config_id_o = 3'd3;
        endcase
        case (selected_d_slot_o)
            2'd0: selected_d_config_id_o = 3'd0;
            2'd1: selected_d_config_id_o = 3'd4;
            2'd2: selected_d_config_id_o = 3'd5;
            default: selected_d_config_id_o = 3'd6;
        endcase
    end

    always_comb begin
        selected_a_pattern_id_o = 4'd0;
        selected_b_pattern_id_o = 4'd0;
        selected_c_pattern_id_o = 4'd0;
        selected_d_pattern_id_o = 4'd0;
        case (selected_a_slot_o)
            2'd0: selected_a_pattern_id_o = a_pattern[0];
            2'd1: selected_a_pattern_id_o = a_pattern[1];
            2'd2: selected_a_pattern_id_o = a_pattern[2];
            default: selected_a_pattern_id_o = a_pattern[3];
        endcase
        case (selected_b_slot_o)
            2'd0: selected_b_pattern_id_o = b_pattern[0];
            2'd1: selected_b_pattern_id_o = b_pattern[1];
            2'd2: selected_b_pattern_id_o = b_pattern[2];
            default: selected_b_pattern_id_o = b_pattern[3];
        endcase
        case (selected_c_slot_o)
            2'd0: selected_c_pattern_id_o = c_pattern[0];
            2'd1: selected_c_pattern_id_o = c_pattern[1];
            2'd2: selected_c_pattern_id_o = c_pattern[2];
            default: selected_c_pattern_id_o = c_pattern[3];
        endcase
        case (selected_d_slot_o)
            2'd0: selected_d_pattern_id_o = d_pattern[0];
            2'd1: selected_d_pattern_id_o = d_pattern[1];
            2'd2: selected_d_pattern_id_o = d_pattern[2];
            default: selected_d_pattern_id_o = d_pattern[3];
        endcase
    end
endmodule
`default_nettype wire
