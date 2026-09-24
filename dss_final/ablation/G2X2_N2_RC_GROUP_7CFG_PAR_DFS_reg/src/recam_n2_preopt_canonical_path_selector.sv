`default_nettype none

// Adapts the retained dense candidate maps to the frozen DATE2026 81-path
// selector. Each map slot supplies its first ascending valid PatternID.
module recam_n2_preopt_canonical_path_selector (
    input wire [159:0] candidate_valid_i,
    output wire group_repairable_o,
    output wire [3:0] selected_valid_o,
    output wire [7:0] selected_action_flat_o,
    output wire [11:0] selected_config_flat_o,
    output wire [15:0] selected_pattern_flat_o,
    output wire [7:0] selected_donor_flat_o,
    output wire [3:0] selected_release_o,
    output wire [3:0] selected_release_by_sa_o,
    output wire [3:0] selected_borrow_o,
    output wire [79:0] canonical_slot_image_o
);
    reg [79:0] canonical_slot_image;
    reg [3:0] first_pattern;
    reg found_pattern;
    integer sa_index;
    integer action_index;
    integer pattern_index;
    wire selector_valid;
    wire [1:0] selected_a_slot;
    wire [1:0] selected_b_slot;
    wire [1:0] selected_c_slot;
    wire [1:0] selected_d_slot;
    wire [2:0] selected_a_config;
    wire [2:0] selected_b_config;
    wire [2:0] selected_c_config;
    wire [2:0] selected_d_config;
    wire [3:0] selected_a_pattern;
    wire [3:0] selected_b_pattern;
    wire [3:0] selected_c_pattern;
    wire [3:0] selected_d_pattern;

    always @* begin
        canonical_slot_image = 80'd0;
        first_pattern = 4'd0;
        found_pattern = 1'b0;
        for (sa_index = 0; sa_index < 4; sa_index = sa_index + 1) begin
            for (action_index = 0; action_index < 4; action_index = action_index + 1) begin
                first_pattern = 4'd0;
                found_pattern = 1'b0;
                for (pattern_index = 0; pattern_index < 10; pattern_index = pattern_index + 1) begin
                    if (!found_pattern && candidate_valid_i[sa_index*40 + action_index*10 + pattern_index]) begin
                        first_pattern = pattern_index[3:0] + 4'd1;
                        found_pattern = 1'b1;
                    end
                end
                canonical_slot_image[sa_index*20 + action_index*5] = found_pattern;
                canonical_slot_image[sa_index*20 + action_index*5 + 1 +: 4] = first_pattern;
            end
        end
    end

    recam_dss_hyp02_static_selector canonical_selector (
        .candidate_store_image_i(canonical_slot_image), .selected_valid_o(selector_valid),
        .selected_a_slot_o(selected_a_slot), .selected_b_slot_o(selected_b_slot),
        .selected_c_slot_o(selected_c_slot), .selected_d_slot_o(selected_d_slot),
        .selected_a_config_id_o(selected_a_config), .selected_b_config_id_o(selected_b_config),
        .selected_c_config_id_o(selected_c_config), .selected_d_config_id_o(selected_d_config),
        .selected_a_pattern_id_o(selected_a_pattern), .selected_b_pattern_id_o(selected_b_pattern),
        .selected_c_pattern_id_o(selected_c_pattern), .selected_d_pattern_id_o(selected_d_pattern)
    );

    assign group_repairable_o = selector_valid;
    assign selected_valid_o = selector_valid ? 4'hf : 4'h0;
    assign selected_action_flat_o = selector_valid ? {selected_d_slot, selected_c_slot, selected_b_slot, selected_a_slot} : 8'd0;
    assign selected_config_flat_o = selector_valid ? {selected_d_config, selected_c_config, selected_b_config, selected_a_config} : 12'd0;
    assign selected_pattern_flat_o = selector_valid ? {selected_d_pattern, selected_c_pattern, selected_b_pattern, selected_a_pattern} : 16'd0;
    assign selected_donor_flat_o = selector_valid ? {2'd3, 2'd0, 2'd1, 2'd2} : 8'd0;
    assign selected_release_o = selector_valid ? {selected_c_slot[0], selected_b_slot[0], selected_d_slot[0], selected_a_slot[0]} : 4'd0;
    assign selected_release_by_sa_o = selector_valid ? {selected_d_slot[0], selected_c_slot[0], selected_b_slot[0], selected_a_slot[0]} : 4'd0;
    assign selected_borrow_o = selector_valid ? {selected_d_slot[1], selected_c_slot[1], selected_b_slot[1], selected_a_slot[1]} : 4'd0;
    assign canonical_slot_image_o = canonical_slot_image;
endmodule

`default_nettype wire
