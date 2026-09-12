`default_nettype none
module recam_phase3e_comparison_top (
    input wire sa_role_i,
    input wire [79:0] config_pattern_map_i,
    output wire early_solution_valid_o,
    output wire [1:0] early_selected_map_index_o,
    output wire [2:0] early_selected_config_id_o,
    output wire [3:0] early_selected_pattern_id_o,
    output wire [9:0] early_selected_candidate_valid_o,
    output wire group_solution_valid_o,
    output wire [1:0] group_selected_map_index_o,
    output wire [2:0] group_selected_config_id_o,
    output wire [3:0] group_selected_pattern_id_o,
    output wire [9:0] group_selected_candidate_valid_o,
    output wire group_selected_repairable_o,
    output wire [1:0] group_selected_score_o
);
    wire early_repairable_unused;
    recam_early_solution_selector early_selector (
        .config_pattern_map_i(config_pattern_map_i), .early_solution_valid_o(early_solution_valid_o),
        .selected_map_index_o(early_selected_map_index_o), .selected_config_id_o(early_selected_config_id_o),
        .selected_pattern_id_o(early_selected_pattern_id_o),
        .selected_candidate_valid_o(early_selected_candidate_valid_o),
        .selected_repairable_o(early_repairable_unused)
    );
    recam_group_compressed_solution_selector group_selector (
        .sa_role_i(sa_role_i), .config_pattern_map_i(config_pattern_map_i),
        .group_solution_valid_o(group_solution_valid_o),
        .selected_map_index_o(group_selected_map_index_o), .selected_config_id_o(group_selected_config_id_o),
        .selected_pattern_id_o(group_selected_pattern_id_o),
        .selected_candidate_valid_o(group_selected_candidate_valid_o),
        .selected_repairable_o(group_selected_repairable_o), .selected_score_o(group_selected_score_o)
    );
endmodule
