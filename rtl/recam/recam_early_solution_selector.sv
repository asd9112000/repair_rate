`default_nettype none

// Phase 3D local EARLY policy: map order is the frozen role-aware priority.
module recam_early_solution_selector (
    input wire [79:0] config_pattern_map_i,
    output reg early_solution_valid_o,
    output reg [1:0] selected_map_index_o,
    output reg [2:0] selected_config_id_o,
    output reg [3:0] selected_pattern_id_o,
    output reg [9:0] selected_candidate_valid_o,
    output reg selected_repairable_o
);
    integer index;
    reg [19:0] entry;
    always @* begin
        early_solution_valid_o = 1'b0;
        selected_map_index_o = 2'd0;
        selected_config_id_o = 3'd0;
        selected_pattern_id_o = 4'd0;
        selected_candidate_valid_o = 10'd0;
        selected_repairable_o = 1'b0;
        for (index = 0; index < 4; index = index + 1) begin
            entry = config_pattern_map_i[index*20 +: 20];
            if (!early_solution_valid_o && entry[0] && entry[4] && entry[5]) begin
                early_solution_valid_o = 1'b1;
                selected_map_index_o = index[1:0];
                selected_config_id_o = entry[3:1];
                selected_pattern_id_o = entry[9:6];
                selected_candidate_valid_o = entry[19:10];
                selected_repairable_o = entry[5];
            end
        end
    end
endmodule
