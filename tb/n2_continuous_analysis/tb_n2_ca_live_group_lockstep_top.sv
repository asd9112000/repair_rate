`default_nettype none

module tb_n2_ca_live_group_lockstep_top (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic canonical_start_i,
    input  logic live_state_update_i,
    input  logic [1:0] live_state_sa_i,
    input  logic live_test_done_valid_i,
    input  logic [1:0] live_test_done_sa_i,
    input  logic [63:0] candidate_image_lo_i,
    input  logic [15:0] candidate_image_hi_i,
    output logic canonical_done_o,
    output logic canonical_repairable_o,
    output logic [11:0] canonical_config_o,
    output logic [15:0] canonical_pattern_o,
    output logic [7:0] canonical_donor_o,
    output logic [3:0] canonical_borrow_o,
    output logic [3:0] canonical_release_o,
    output logic live_ready_o,
    output logic [3:0] live_frozen_o,
    output logic [79:0] live_candidate_store_o,
    output logic live_repairable_o,
    output logic [11:0] live_config_o,
    output logic [15:0] live_pattern_o,
    output logic [7:0] live_donor_o,
    output logic [3:0] live_borrow_o,
    output logic [3:0] live_release_o,
    output logic live_scan_active_o,
    output logic [1:0] live_active_sa_o,
    output logic [1:0] live_scan_slot_o
);
    logic [79:0] candidate_image;
    logic [1:0] canonical_sa;
    logic [1:0] canonical_slot;
    logic canonical_valid;
    logic [3:0] canonical_pattern;
    logic live_valid;
    logic [3:0] live_pattern;

    function automatic logic image_valid(
        input logic [79:0] image,
        input logic [1:0] sa,
        input logic [1:0] slot
    );
        logic [6:0] offset;
        begin
            offset = (({5'b0, sa} << 2) + {5'b0, slot}) * 5;
            image_valid = image[offset];
        end
    endfunction

    function automatic logic [3:0] image_pattern(
        input logic [79:0] image,
        input logic [1:0] sa,
        input logic [1:0] slot
    );
        logic [6:0] offset;
        begin
            offset = (({5'b0, sa} << 2) + {5'b0, slot}) * 5;
            image_pattern = image[offset + 1 +: 4];
        end
    endfunction

    assign candidate_image = {candidate_image_hi_i, candidate_image_lo_i};
    assign canonical_valid = image_valid(candidate_image, canonical_sa, canonical_slot);
    assign canonical_pattern = image_pattern(candidate_image, canonical_sa, canonical_slot);
    assign live_valid = image_valid(candidate_image, live_active_sa_o, live_scan_slot_o);
    assign live_pattern = image_pattern(candidate_image, live_active_sa_o, live_scan_slot_o);

    recam_dss_hyp02_static_global_core canonical (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(canonical_start_i),
        .candidate_valid_i(canonical_valid), .candidate_pattern_id_i(canonical_pattern),
        .collection_active_o(), .allocation_active_o(), .current_sa_o(canonical_sa),
        .current_slot_o(canonical_slot), .current_config_id_o(),
        .candidate_store_image_o(), .busy_o(), .done_o(canonical_done_o),
        .group_repairable_o(canonical_repairable_o), .sa_commit_valid_o(),
        .ledger_released_borrower_o(), .selected_config_flat_o(canonical_config_o),
        .selected_pattern_flat_o(canonical_pattern_o), .selected_donor_flat_o(canonical_donor_o),
        .borrow_flat_o(canonical_borrow_o), .release_flat_o(canonical_release_o),
        .failure_position_o()
    );

    recam_dss_hyp02_static_global_live_state_core live (
        .clk_i(clk_i), .rst_ni(rst_ni), .state_update_i(live_state_update_i),
        .state_sa_i(live_state_sa_i), .test_done_valid_i(live_test_done_valid_i),
        .test_done_sa_i(live_test_done_sa_i), .candidate_valid_i(live_valid),
        .candidate_pattern_id_i(live_pattern), .scan_active_o(live_scan_active_o),
        .active_sa_o(live_active_sa_o), .scan_slot_o(live_scan_slot_o),
        .scan_config_id_o(), .sa_result_frozen_o(live_frozen_o),
        .solution_ready_o(live_ready_o), .candidate_store_image_o(live_candidate_store_o),
        .group_repairable_o(live_repairable_o), .sa_commit_valid_o(),
        .ledger_released_borrower_o(), .selected_config_flat_o(live_config_o),
        .selected_pattern_flat_o(live_pattern_o), .selected_donor_flat_o(live_donor_o),
        .borrow_flat_o(live_borrow_o), .release_flat_o(live_release_o)
    );
endmodule

`default_nettype wire
