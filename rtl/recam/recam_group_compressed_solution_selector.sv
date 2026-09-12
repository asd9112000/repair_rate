`default_nettype none

// Phase 3E local GROUP_COMPRESSED policy. This ranks only the four completed
// role-aware map entries; resource ownership and feasibility are outside it.
module recam_group_compressed_solution_selector (
    input wire sa_role_i,
    input wire [79:0] config_pattern_map_i,
    output reg group_solution_valid_o,
    output reg [1:0] selected_map_index_o,
    output reg [2:0] selected_config_id_o,
    output reg [3:0] selected_pattern_id_o,
    output reg [9:0] selected_candidate_valid_o,
    output reg selected_repairable_o,
    output reg [1:0] selected_score_o
);
    integer index;
    reg [19:0] entry;
    reg [1:0] entry_score;

    always @* begin
        group_solution_valid_o = 1'b0;
        selected_map_index_o = 2'd0;
        selected_config_id_o = 3'd0;
        selected_pattern_id_o = 4'd0;
        selected_candidate_valid_o = 10'd0;
        selected_repairable_o = 1'b0;
        selected_score_o = 2'd0;

        // Scores are fixed by the frozen Phase 3C role-aware map layout.
        // A/D map[0..3] scores: 1,0,3,2; B/C: 1,3,0,2.
        // Strictly lower replacement preserves map order on a score tie.
        for (index = 0; index < 4; index = index + 1) begin
            entry = config_pattern_map_i[index*20 +: 20];
            if (!sa_role_i) begin
                case (index)
                    0: entry_score = 2'd1;
                    1: entry_score = 2'd0;
                    2: entry_score = 2'd3;
                    default: entry_score = 2'd2;
                endcase
            end else begin
                case (index)
                    0: entry_score = 2'd1;
                    1: entry_score = 2'd3;
                    2: entry_score = 2'd0;
                    default: entry_score = 2'd2;
                endcase
            end
            if (entry[0] && entry[4] && entry[5] &&
                (!group_solution_valid_o || entry_score < selected_score_o)) begin
                group_solution_valid_o = 1'b1;
                selected_map_index_o = index[1:0];
                selected_config_id_o = entry[3:1];
                selected_pattern_id_o = entry[9:6];
                selected_candidate_valid_o = entry[19:10];
                selected_repairable_o = entry[5];
                selected_score_o = entry_score;
            end
        end
    end
endmodule
