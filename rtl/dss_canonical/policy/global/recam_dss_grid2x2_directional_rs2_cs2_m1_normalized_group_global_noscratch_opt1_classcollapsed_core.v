`default_nettype none

// OPT1 wrapper for the frozen GRID_2X2 directional RS2/CS2/m1 GLOBAL core.
// It retains only the first R,L,RB,B/PatternID candidate for every safe
// {explicit_release, actual_release, actual_borrow} class at each subarray.
module recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_core (
    input  wire         clk_i,
    input  wire         rst_ni,
    input  wire         start_i,
    input  wire [159:0] candidate_valid_i,
    input  wire [159:0] candidate_release_i,
    input  wire [159:0] candidate_borrow_i,
    output wire         busy_o,
    output wire         done_o,
    output wire         group_repairable_o,
    output wire [3:0]   selected_valid_o,
    output wire [7:0]   selected_action_flat_o,
    output wire [11:0]  selected_config_flat_o,
    output wire [15:0]  selected_pattern_flat_o,
    output wire [7:0]   selected_donor_flat_o,
    output wire [3:0]   selected_release_o,
    output wire [3:0]   selected_borrow_o,
    output wire [3:0]   final_released_mask_o,
    output wire [3:0]   final_used_mask_o
);
    reg [159:0] first_class_valid;
    reg current_explicit_release;
    reg prior_explicit_release;
    reg duplicate_class;
    integer subarray_index;
    integer canonical_rank;
    integer prior_rank;
    reg [7:0] current_slot;
    reg [7:0] prior_slot;

    // Canonical rank is R/P1..P10, L/P1..P10, RB/P1..P10, B/P1..P10.
    // The stored map regions are L, R, B, RB, so translate rank to storage.
    function [7:0] stored_slot_from_rank;
        input [1:0] subarray;
        input [5:0] rank;
        reg [7:0] subarray_base;
        begin
            subarray_base = {6'd0, subarray} * 8'd40;
            if (rank < 10)
                stored_slot_from_rank = subarray_base + 8'd10 + {2'd0, rank};
            else if (rank < 20)
                stored_slot_from_rank = subarray_base + {2'd0, rank} - 8'd10;
            else if (rank < 30)
                stored_slot_from_rank = subarray_base + 8'd30 + {2'd0, rank} - 8'd20;
            else
                stored_slot_from_rank = subarray_base + 8'd20 + {2'd0, rank} - 8'd30;
        end
    endfunction

    always @* begin
        first_class_valid = 160'd0;
        current_explicit_release = 1'b0;
        prior_explicit_release = 1'b0;
        duplicate_class = 1'b0;
        current_slot = 0;
        prior_slot = 0;
        for (subarray_index = 0; subarray_index < 4;
             subarray_index = subarray_index + 1) begin
            for (canonical_rank = 0; canonical_rank < 40;
                 canonical_rank = canonical_rank + 1) begin
                current_slot = stored_slot_from_rank(subarray_index[1:0], canonical_rank[5:0]);
                current_explicit_release = (canonical_rank < 10) ||
                    ((canonical_rank >= 20) && (canonical_rank < 30));
                duplicate_class = 1'b0;
                if (candidate_valid_i[current_slot]) begin
                    for (prior_rank = 0; prior_rank < canonical_rank;
                         prior_rank = prior_rank + 1) begin
                        prior_slot = stored_slot_from_rank(subarray_index[1:0], prior_rank[5:0]);
                        prior_explicit_release = (prior_rank < 10) ||
                            ((prior_rank >= 20) && (prior_rank < 30));
                        if (candidate_valid_i[prior_slot] &&
                            (prior_explicit_release == current_explicit_release) &&
                            (candidate_release_i[prior_slot] == candidate_release_i[current_slot]) &&
                            (candidate_borrow_i[prior_slot] == candidate_borrow_i[current_slot]))
                            duplicate_class = 1'b1;
                    end
                    if (!duplicate_class)
                        first_class_valid[current_slot] = 1'b1;
                end
            end
        end
    end

    recam_dss_canonical_global_noscratch_core canonical_global_core (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .start_i(start_i),
        .candidate_valid_i(first_class_valid),
        .candidate_release_i(candidate_release_i),
        .candidate_borrow_i(candidate_borrow_i),
        .busy_o(busy_o),
        .done_o(done_o),
        .group_repairable_o(group_repairable_o),
        .selected_valid_o(selected_valid_o),
        .selected_action_flat_o(selected_action_flat_o),
        .selected_config_flat_o(selected_config_flat_o),
        .selected_pattern_flat_o(selected_pattern_flat_o),
        .selected_donor_flat_o(selected_donor_flat_o),
        .selected_release_o(selected_release_o),
        .selected_borrow_o(selected_borrow_o),
        .final_released_mask_o(final_released_mask_o),
        .final_used_mask_o(final_used_mask_o)
    );
endmodule

`default_nettype wire
