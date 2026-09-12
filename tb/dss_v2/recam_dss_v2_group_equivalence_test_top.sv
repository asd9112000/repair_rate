`default_nettype none

// Test-only bridge.  Input maps use the V2 canonical slots (A/D: 0,4,5,6;
// B/C: 0,1,2,3).  The legacy GROUP selector keeps its frozen B/C map layout
// (0,2,1,3), so this bridge performs only that representation conversion.
// Production V2 logic sees no ConfigPatternMap.
module recam_dss_v2_group_equivalence_test_top(
    input wire clk_i, rst_ni, start_i,
    input wire [79:0] map_a_i, map_b_i, map_c_i, map_d_i,
    output wire legacy_busy_o, legacy_done_o, legacy_group_repairable_o,
    output wire [3:0] legacy_sa_commit_valid_o,
    output wire [11:0] legacy_ledger_o, legacy_selected_config_flat_o,
    output wire [15:0] legacy_selected_pattern_flat_o,
    output wire [7:0] legacy_selected_donor_flat_o,
    output wire [3:0] legacy_borrow_flat_o, legacy_release_flat_o,
    output wire v2_busy_o, v2_done_o, v2_group_repairable_o,
    output wire [3:0] v2_sa_commit_valid_o,
    output wire [11:0] v2_ledger_o, v2_selected_config_flat_o,
    output wire [15:0] v2_selected_pattern_flat_o,
    output wire [7:0] v2_selected_donor_flat_o,
    output wire [3:0] v2_borrow_flat_o, v2_release_flat_o,
    output wire [1:0] v2_failure_position_o,
    output wire v2_collection_active_o, v2_allocation_active_o,
    output wire [1:0] v2_current_sa_o, v2_current_slot_o
);
    reg [79:0] legacy_map_b, legacy_map_c;
    reg [79:0] current_canonical_map;
    reg [19:0] collection_entry;
    wire v2_candidate_valid = collection_entry[0] && collection_entry[4] && collection_entry[5];
    wire [3:0] v2_candidate_pattern = collection_entry[9:6];

    always @* begin
        // A/D are already canonical.  Convert only B/C legacy map indices.
        legacy_map_b = '0;
        legacy_map_c = '0;
        legacy_map_b[19:0] = map_b_i[19:0];
        legacy_map_b[39:20] = map_b_i[59:40];
        legacy_map_b[59:40] = map_b_i[39:20];
        legacy_map_b[79:60] = map_b_i[79:60];
        legacy_map_c[19:0] = map_c_i[19:0];
        legacy_map_c[39:20] = map_c_i[59:40];
        legacy_map_c[59:40] = map_c_i[39:20];
        legacy_map_c[79:60] = map_c_i[79:60];

        case (v2_current_sa_o)
            2'd0: current_canonical_map = map_a_i;
            2'd1: current_canonical_map = map_b_i;
            2'd2: current_canonical_map = map_c_i;
            default: current_canonical_map = map_d_i;
        endcase
        collection_entry = current_canonical_map[v2_current_slot_o*20 +: 20];
    end

    recam_dss_group_allocator legacy(
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i), .policy_i(1'b1),
        .map_a_i(map_a_i), .map_b_i(legacy_map_b), .map_c_i(legacy_map_c), .map_d_i(map_d_i),
        .busy_o(legacy_busy_o), .done_o(legacy_done_o),
        .group_repairable_o(legacy_group_repairable_o),
        .sa_commit_valid_o(legacy_sa_commit_valid_o), .ledger_released_borrower_o(legacy_ledger_o),
        .selected_config_flat_o(legacy_selected_config_flat_o),
        .selected_pattern_flat_o(legacy_selected_pattern_flat_o), .selected_map_index_flat_o(),
        .selected_donor_flat_o(legacy_selected_donor_flat_o),
        .borrow_flat_o(legacy_borrow_flat_o), .release_flat_o(legacy_release_flat_o)
    );

    recam_dss_v2_group_core v2(
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(start_i),
        .candidate_valid_i(v2_candidate_valid), .candidate_pattern_id_i(v2_candidate_pattern),
        .collection_active_o(v2_collection_active_o), .allocation_active_o(v2_allocation_active_o),
        .current_sa_o(v2_current_sa_o), .current_slot_o(v2_current_slot_o), .current_config_id_o(),
        .candidate_store_image_o(), .busy_o(v2_busy_o), .done_o(v2_done_o),
        .group_repairable_o(v2_group_repairable_o), .sa_commit_valid_o(v2_sa_commit_valid_o),
        .ledger_released_borrower_o(v2_ledger_o), .selected_config_flat_o(v2_selected_config_flat_o),
        .selected_pattern_flat_o(v2_selected_pattern_flat_o), .selected_donor_flat_o(v2_selected_donor_flat_o),
        .borrow_flat_o(v2_borrow_flat_o), .release_flat_o(v2_release_flat_o),
        .failure_position_o(v2_failure_position_o)
    );
endmodule
`default_nettype wire
