`default_nettype none

// Phase 3G: evaluates a frozen read-only ledger snapshot; it never updates it.
module recam_resource_aware_fallback_selector (
    input wire [1:0] sa_id_i,
    input wire policy_i, // 0=EARLY map order, 1=GROUP_COMPRESSED order
    input wire [79:0] config_pattern_map_i,
    input wire [3:0] resource_released_i,
    input wire [7:0] resource_borrower_flat_i,
    output wire ledger_state_valid_o,
    output reg [3:0] physical_feasible_bitmap_o,
    output reg physical_solution_valid_o,
    output reg [1:0] selected_map_index_o,
    output reg [2:0] selected_config_id_o,
    output reg [3:0] selected_pattern_id_o,
    output reg [9:0] selected_candidate_valid_o,
    output reg selected_repairable_o,
    output reg borrow_required_o,
    output reg selected_donor_valid_o,
    output reg [1:0] selected_donor_resource_o,
    output reg release_required_o,
    output reg [1:0] release_resource_o,
    output reg [1:0] selected_policy_rank_o
);
    wire [3:0] available;
    wire [3:0] legal, borrow, release_action;
    wire [7:0] release_resource, donor_primary, donor_secondary;
    wire [2:0] cfg0 = config_pattern_map_i[3:1];
    wire [2:0] cfg1 = config_pattern_map_i[23:21];
    wire [2:0] cfg2 = config_pattern_map_i[43:41];
    wire [2:0] cfg3 = config_pattern_map_i[63:61];
    recam_physical_resource_ledger_checker ledger (
        .resource_released_i(resource_released_i), .resource_borrower_flat_i(resource_borrower_flat_i),
        .ledger_state_valid_o(ledger_state_valid_o), .resource_available_o(available));
    recam_config_resource_action_decoder d0(.sa_id_i(sa_id_i),.config_id_i(cfg0),.config_legal_for_sa_o(legal[0]),.borrow_required_o(borrow[0]),.release_required_o(release_action[0]),.borrow_resource_type_o(),.release_resource_type_o(),.release_resource_o(release_resource[1:0]),.donor_primary_o(donor_primary[1:0]),.donor_secondary_o(donor_secondary[1:0]));
    recam_config_resource_action_decoder d1(.sa_id_i(sa_id_i),.config_id_i(cfg1),.config_legal_for_sa_o(legal[1]),.borrow_required_o(borrow[1]),.release_required_o(release_action[1]),.borrow_resource_type_o(),.release_resource_type_o(),.release_resource_o(release_resource[3:2]),.donor_primary_o(donor_primary[3:2]),.donor_secondary_o(donor_secondary[3:2]));
    recam_config_resource_action_decoder d2(.sa_id_i(sa_id_i),.config_id_i(cfg2),.config_legal_for_sa_o(legal[2]),.borrow_required_o(borrow[2]),.release_required_o(release_action[2]),.borrow_resource_type_o(),.release_resource_type_o(),.release_resource_o(release_resource[5:4]),.donor_primary_o(donor_primary[5:4]),.donor_secondary_o(donor_secondary[5:4]));
    recam_config_resource_action_decoder d3(.sa_id_i(sa_id_i),.config_id_i(cfg3),.config_legal_for_sa_o(legal[3]),.borrow_required_o(borrow[3]),.release_required_o(release_action[3]),.borrow_resource_type_o(),.release_resource_type_o(),.release_resource_o(release_resource[7:6]),.donor_primary_o(donor_primary[7:6]),.donor_secondary_o(donor_secondary[7:6]));

    integer index, rank;
    reg [19:0] entry;
    reg [1:0] map_index;
    reg candidate_feasible;
    reg [1:0] chosen_donor;
    always @* begin
        physical_feasible_bitmap_o = 4'd0;
        physical_solution_valid_o = 1'b0;
        selected_map_index_o = 2'd0; selected_config_id_o = 3'd0; selected_pattern_id_o = 4'd0;
        selected_candidate_valid_o = 10'd0; selected_repairable_o = 1'b0;
        borrow_required_o = 1'b0; selected_donor_valid_o = 1'b0; selected_donor_resource_o = 2'd0;
        release_required_o = 1'b0; release_resource_o = 2'd0; selected_policy_rank_o = 2'd0;
        if (ledger_state_valid_o) begin
            for (index=0; index<4; index=index+1) begin
                entry = config_pattern_map_i[index*20 +: 20];
                candidate_feasible = entry[0] && entry[4] && entry[5] && legal[index] &&
                    (!borrow[index] || available[donor_primary[index*2 +: 2]] || available[donor_secondary[index*2 +: 2]]);
                physical_feasible_bitmap_o[index] = candidate_feasible;
            end
            for (rank=0; rank<4; rank=rank+1) begin
                if (!policy_i) map_index = rank[1:0];
                // A/D use the row-role order; B/C use the column-role order.
                else if (sa_id_i == 2'd0 || sa_id_i == 2'd3) begin
                    case (rank) 0:map_index=2'd1; 1:map_index=2'd0; 2:map_index=2'd3; default:map_index=2'd2; endcase
                end else begin
                    case (rank) 0:map_index=2'd2; 1:map_index=2'd0; 2:map_index=2'd3; default:map_index=2'd1; endcase
                end
                entry = config_pattern_map_i[map_index*20 +: 20];
                if (!physical_solution_valid_o && physical_feasible_bitmap_o[map_index]) begin
                    physical_solution_valid_o = 1'b1;
                    selected_map_index_o = map_index; selected_config_id_o = entry[3:1];
                    selected_pattern_id_o = entry[9:6]; selected_candidate_valid_o = entry[19:10];
                    selected_repairable_o = entry[5]; selected_policy_rank_o = rank[1:0];
                    borrow_required_o = borrow[map_index]; release_required_o = release_action[map_index];
                    release_resource_o = release_resource[map_index*2 +: 2];
                    if (borrow[map_index]) begin
                        chosen_donor = donor_primary[map_index*2 +: 2];
                        if (!available[chosen_donor]) chosen_donor = donor_secondary[map_index*2 +: 2];
                        selected_donor_valid_o = 1'b1; selected_donor_resource_o = chosen_donor;
                    end
                end
            end
        end
    end
endmodule
