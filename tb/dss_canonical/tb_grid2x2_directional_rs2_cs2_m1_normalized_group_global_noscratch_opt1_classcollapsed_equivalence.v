`default_nettype none

module tb_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_equivalence (
    input  wire         clk_i,
    input  wire         rst_ni,
    input  wire         start_i,
    input  wire [159:0] candidate_valid_i,
    input  wire [159:0] candidate_release_i,
    input  wire [159:0] candidate_borrow_i,
    output wire         baseline_done_o,
    output wire         baseline_repairable_o,
    output wire [7:0]   baseline_action_o,
    output wire [11:0]  baseline_config_o,
    output wire [15:0]  baseline_pattern_o,
    output wire [7:0]   baseline_donor_o,
    output wire [3:0]   baseline_release_o,
    output wire [3:0]   baseline_borrow_o,
    output wire [3:0]   baseline_released_o,
    output wire [3:0]   baseline_used_o,
    output wire         opt1_done_o,
    output wire         opt1_repairable_o,
    output wire [7:0]   opt1_action_o,
    output wire [11:0]  opt1_config_o,
    output wire [15:0]  opt1_pattern_o,
    output wire [7:0]   opt1_donor_o,
    output wire [3:0]   opt1_release_o,
    output wire [3:0]   opt1_borrow_o,
    output wire [3:0]   opt1_released_o,
    output wire [3:0]   opt1_used_o
);
    wire unused_baseline_busy;
    wire [3:0] unused_baseline_valid;
    wire unused_opt1_busy;
    wire [3:0] unused_opt1_valid;

    recam_dss_canonical_global_noscratch_core baseline_core (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i),
        .candidate_valid_i(candidate_valid_i),
        .candidate_release_i(candidate_release_i),
        .candidate_borrow_i(candidate_borrow_i),
        .busy_o(unused_baseline_busy), .done_o(baseline_done_o),
        .group_repairable_o(baseline_repairable_o),
        .selected_valid_o(unused_baseline_valid),
        .selected_action_flat_o(baseline_action_o),
        .selected_config_flat_o(baseline_config_o),
        .selected_pattern_flat_o(baseline_pattern_o),
        .selected_donor_flat_o(baseline_donor_o),
        .selected_release_o(baseline_release_o),
        .selected_borrow_o(baseline_borrow_o),
        .final_released_mask_o(baseline_released_o),
        .final_used_mask_o(baseline_used_o)
    );

    recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_core opt1_core (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i),
        .candidate_valid_i(candidate_valid_i),
        .candidate_release_i(candidate_release_i),
        .candidate_borrow_i(candidate_borrow_i),
        .busy_o(unused_opt1_busy), .done_o(opt1_done_o),
        .group_repairable_o(opt1_repairable_o),
        .selected_valid_o(unused_opt1_valid),
        .selected_action_flat_o(opt1_action_o),
        .selected_config_flat_o(opt1_config_o),
        .selected_pattern_flat_o(opt1_pattern_o),
        .selected_donor_flat_o(opt1_donor_o),
        .selected_release_o(opt1_release_o),
        .selected_borrow_o(opt1_borrow_o),
        .final_released_mask_o(opt1_released_o),
        .final_used_mask_o(opt1_used_o)
    );
endmodule

`default_nettype wire
