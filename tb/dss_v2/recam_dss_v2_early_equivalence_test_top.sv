`default_nettype none

// Test-only bridge.  The legacy maps use the frozen scheduler enumeration
// (A/D: 0,4,5,6; B/C: 0,2,1,3).  The bridge presents the matching current
// entry to the V2 streaming core without adding a ConfigPatternMap to V2 RTL.
module recam_dss_v2_early_equivalence_test_top(
    input wire clk_i, rst_ni, start_i,
    input wire [79:0] map_a_i, map_b_i, map_c_i, map_d_i,
    output wire legacy_busy_o, legacy_done_o, legacy_group_repairable_o,
    output wire [3:0] legacy_sa_commit_valid_o,
    output wire [11:0] legacy_ledger_o, legacy_selected_config_flat_o,
    output wire [15:0] legacy_selected_pattern_flat_o,
    output wire [7:0] legacy_selected_map_index_flat_o, legacy_selected_donor_flat_o,
    output wire [3:0] legacy_borrow_flat_o, legacy_release_flat_o,
    output wire v2_busy_o, v2_done_o, v2_group_repairable_o,
    output wire [3:0] v2_sa_commit_valid_o,
    output wire [11:0] v2_ledger_o, v2_selected_config_flat_o,
    output wire [15:0] v2_selected_pattern_flat_o,
    output wire [7:0] v2_selected_donor_flat_o,
    output wire [3:0] v2_borrow_flat_o, v2_release_flat_o,
    output wire [1:0] v2_failure_position_o, v2_current_sa_o,
    output wire [3:0] v2_canonical_released_o, v2_canonical_borrowed_o,
    output wire [7:0] v2_canonical_borrower_id_flat_o
);
    wire [2:0] v2_current_config;
    reg [79:0] current_map;
    reg [1:0] source_slot;
    reg [19:0] current_entry;
    wire v2_solution = current_entry[0] && current_entry[4];
    wire v2_repairable = current_entry[5];
    wire [3:0] v2_pattern = current_entry[9:6];

    always @* begin
        case (v2_current_sa_o)
            2'd0: current_map = map_a_i;
            2'd1: current_map = map_b_i;
            2'd2: current_map = map_c_i;
            default: current_map = map_d_i;
        endcase
        source_slot = 2'd0;
        if (v2_current_sa_o == 2'd0 || v2_current_sa_o == 2'd3) begin
            case (v2_current_config)
                3'd0: source_slot = 2'd0;
                3'd4: source_slot = 2'd1;
                3'd5: source_slot = 2'd2;
                default: source_slot = 2'd3; // CFG6
            endcase
        end else begin
            case (v2_current_config)
                3'd0: source_slot = 2'd0;
                3'd2: source_slot = 2'd1;
                3'd1: source_slot = 2'd2;
                default: source_slot = 2'd3; // CFG3
            endcase
        end
        current_entry = current_map[source_slot*20 +: 20];
    end

    recam_dss_group_allocator legacy (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i), .policy_i(1'b0),
        .map_a_i(map_a_i), .map_b_i(map_b_i), .map_c_i(map_c_i), .map_d_i(map_d_i),
        .busy_o(legacy_busy_o), .done_o(legacy_done_o),
        .group_repairable_o(legacy_group_repairable_o),
        .sa_commit_valid_o(legacy_sa_commit_valid_o),
        .ledger_released_borrower_o(legacy_ledger_o),
        .selected_config_flat_o(legacy_selected_config_flat_o),
        .selected_pattern_flat_o(legacy_selected_pattern_flat_o),
        .selected_map_index_flat_o(legacy_selected_map_index_flat_o),
        .selected_donor_flat_o(legacy_selected_donor_flat_o),
        .borrow_flat_o(legacy_borrow_flat_o), .release_flat_o(legacy_release_flat_o)
    );

    recam_dss_v2_early_core v2 (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i),
        .candidate_solution_valid_i(v2_solution),
        .candidate_repairable_i(v2_repairable), .candidate_pattern_id_i(v2_pattern),
        .current_config_id_o(v2_current_config), .current_sa_o(v2_current_sa_o),
        .busy_o(v2_busy_o), .done_o(v2_done_o), .group_repairable_o(v2_group_repairable_o),
        .sa_commit_valid_o(v2_sa_commit_valid_o), .ledger_released_borrower_o(v2_ledger_o),
        .selected_config_flat_o(v2_selected_config_flat_o),
        .selected_pattern_flat_o(v2_selected_pattern_flat_o),
        .selected_donor_flat_o(v2_selected_donor_flat_o),
        .borrow_flat_o(v2_borrow_flat_o), .release_flat_o(v2_release_flat_o),
        .failure_position_o(v2_failure_position_o),
        .canonical_released_o(v2_canonical_released_o),
        .canonical_borrowed_o(v2_canonical_borrowed_o),
        .canonical_borrower_id_flat_o(v2_canonical_borrower_id_flat_o)
    );
endmodule
`default_nettype wire
