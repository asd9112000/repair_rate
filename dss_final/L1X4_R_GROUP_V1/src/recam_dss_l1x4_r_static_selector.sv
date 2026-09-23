`default_nettype none

// Frozen L1X4_R GROUP selector: directed row tokens A->B->C->D.
module recam_dss_l1x4_r_static_selector (
    input logic [59:0] candidate_store_image_i,
    output logic selected_valid_o,
    output logic [1:0] selected_a_slot_o, selected_b_slot_o,
    output logic [1:0] selected_c_slot_o, selected_d_slot_o,
    output logic [2:0] selected_a_config_id_o, selected_b_config_id_o,
    output logic [2:0] selected_c_config_id_o, selected_d_config_id_o,
    output logic [3:0] selected_a_pattern_id_o, selected_b_pattern_id_o,
    output logic [3:0] selected_c_pattern_id_o, selected_d_pattern_id_o
);
    logic [3:0] valid [0:3];
    logic [3:0] pattern [0:3][0:2];
    integer a, b, c, d;
    function automatic [2:0] config_id(input logic [1:0] slot);
        case (slot) 2'd1: config_id = 3'd4; 2'd2: config_id = 3'd2; default: config_id = 3'd0; endcase
    endfunction
    function automatic logic slot_valid(input integer sa, input logic [1:0] slot);
        if (slot == 2'd3) slot_valid = valid[sa][0];
        else slot_valid = valid[sa][slot];
    endfunction
    function automatic [3:0] slot_pattern(input integer sa, input logic [1:0] slot);
        if (slot == 2'd3) slot_pattern = pattern[sa][0];
        else slot_pattern = pattern[sa][slot];
    endfunction
    always_comb begin
        for (a = 0; a < 4; a = a + 1) begin
            valid[a][0] = candidate_store_image_i[a*15];
            valid[a][1] = candidate_store_image_i[a*15+5];
            valid[a][2] = candidate_store_image_i[a*15+10];
            valid[a][3] = valid[a][0];
            pattern[a][0] = candidate_store_image_i[a*15+4 -: 4];
            pattern[a][1] = candidate_store_image_i[a*15+9 -: 4];
            pattern[a][2] = candidate_store_image_i[a*15+14 -: 4];
        end
        selected_valid_o = 1'b0; selected_a_slot_o = 0; selected_b_slot_o = 0;
        selected_c_slot_o = 0; selected_d_slot_o = 0;
        for (d = 0; d < 4; d = d + 1) for (c = 0; c < 4; c = c + 1)
        for (b = 0; b < 4; b = b + 1) for (a = 0; a < 4; a = a + 1)
            if (!selected_valid_o && a < 2 && d != 1 &&
                (!b[1] || a[0]) && (!c[1] || b[0]) && (!d[1] || c[0]) &&
                slot_valid(0,a[1:0]) && slot_valid(1,b[1:0]) && slot_valid(2,c[1:0]) && slot_valid(3,d[1:0])) begin
                selected_valid_o = 1'b1; selected_a_slot_o = a[1:0]; selected_b_slot_o = b[1:0];
                selected_c_slot_o = c[1:0]; selected_d_slot_o = d[1:0];
            end
        selected_a_config_id_o = config_id(selected_a_slot_o); selected_b_config_id_o = config_id(selected_b_slot_o);
        selected_c_config_id_o = config_id(selected_c_slot_o); selected_d_config_id_o = config_id(selected_d_slot_o);
        selected_a_pattern_id_o = slot_pattern(0,selected_a_slot_o); selected_b_pattern_id_o = slot_pattern(1,selected_b_slot_o);
        selected_c_pattern_id_o = slot_pattern(2,selected_c_slot_o); selected_d_pattern_id_o = slot_pattern(3,selected_d_slot_o);
    end
endmodule
`default_nettype wire
