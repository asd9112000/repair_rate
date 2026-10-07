`default_nettype none

// Test-only composition of the frozen CA-LIVE lockstep wrappers.  EARLY and
// GROUP receive identical candidate-state images and state-update events.
module tb_n2_ca_live_state_relative_top (
    input logic clk_i,
    input logic rst_ni,
    input logic canonical_start_i,
    input logic live_state_update_i,
    input logic [1:0] live_state_sa_i,
    input logic live_test_done_valid_i,
    input logic [1:0] live_test_done_sa_i,
    input logic [63:0] candidate_image_lo_i,
    input logic [15:0] candidate_image_hi_i,

    output logic early_canonical_done_o,
    output logic early_canonical_repairable_o,
    output logic [3:0] early_canonical_commit_mask_o,
    output logic [11:0] early_canonical_config_flat_o,
    output logic [15:0] early_canonical_pattern_flat_o,
    output logic early_live_done_o,
    output logic early_live_repairable_o,
    output logic [3:0] early_live_commit_mask_o,
    output logic [11:0] early_live_config_flat_o,
    output logic [15:0] early_live_pattern_flat_o,
    output logic early_live_scan_active_o,
    output logic [1:0] early_live_active_sa_o,
    output logic [1:0] early_live_scan_slot_o,

    output logic group_canonical_done_o,
    output logic group_canonical_repairable_o,
    output logic [11:0] group_canonical_config_o,
    output logic [15:0] group_canonical_pattern_o,
    output logic [7:0] group_canonical_donor_o,
    output logic [3:0] group_canonical_borrow_o,
    output logic [3:0] group_canonical_release_o,
    output logic group_live_ready_o,
    output logic [3:0] group_live_frozen_o,
    output logic group_live_repairable_o,
    output logic [11:0] group_live_config_o,
    output logic [15:0] group_live_pattern_o,
    output logic [7:0] group_live_donor_o,
    output logic [3:0] group_live_borrow_o,
    output logic [3:0] group_live_release_o
);
    logic [79:0] group_live_candidate_store_unused;
    logic group_live_scan_active_unused;
    logic [1:0] group_live_active_sa_unused;
    logic [1:0] group_live_scan_slot_unused;
    logic [2:0] early_live_scan_config_unused;
    logic early_live_commit_unused;

    tb_n2_ca_live_early_lockstep_top early_lockstep (
        .clk_i(clk_i), .rst_ni(rst_ni), .canonical_start_i(canonical_start_i),
        .live_state_update_i(live_state_update_i), .live_state_sa_i(live_state_sa_i),
        .live_test_done_valid_i(live_test_done_valid_i),
        .live_test_done_sa_i(live_test_done_sa_i),
        .candidate_image_lo_i(candidate_image_lo_i),
        .candidate_image_hi_i(candidate_image_hi_i),
        .canonical_done_o(early_canonical_done_o),
        .canonical_repairable_o(early_canonical_repairable_o),
        .canonical_commit_mask_o(early_canonical_commit_mask_o),
        .canonical_config_flat_o(early_canonical_config_flat_o),
        .canonical_pattern_flat_o(early_canonical_pattern_flat_o),
        .live_done_o(early_live_done_o), .live_repairable_o(early_live_repairable_o),
        .live_commit_mask_o(early_live_commit_mask_o),
        .live_config_flat_o(early_live_config_flat_o),
        .live_pattern_flat_o(early_live_pattern_flat_o),
        .live_scan_active_o(early_live_scan_active_o),
        .live_active_sa_o(early_live_active_sa_o),
        .live_scan_slot_o(early_live_scan_slot_o),
        .live_scan_config_o(early_live_scan_config_unused),
        .live_commit_o(early_live_commit_unused)
    );

    tb_n2_ca_live_group_lockstep_top group_lockstep (
        .clk_i(clk_i), .rst_ni(rst_ni), .canonical_start_i(canonical_start_i),
        .live_state_update_i(live_state_update_i), .live_state_sa_i(live_state_sa_i),
        .live_test_done_valid_i(live_test_done_valid_i),
        .live_test_done_sa_i(live_test_done_sa_i),
        .candidate_image_lo_i(candidate_image_lo_i),
        .candidate_image_hi_i(candidate_image_hi_i),
        .canonical_done_o(group_canonical_done_o),
        .canonical_repairable_o(group_canonical_repairable_o),
        .canonical_config_o(group_canonical_config_o),
        .canonical_pattern_o(group_canonical_pattern_o),
        .canonical_donor_o(group_canonical_donor_o),
        .canonical_borrow_o(group_canonical_borrow_o),
        .canonical_release_o(group_canonical_release_o),
        .live_ready_o(group_live_ready_o), .live_frozen_o(group_live_frozen_o),
        .live_candidate_store_o(group_live_candidate_store_unused),
        .live_repairable_o(group_live_repairable_o), .live_config_o(group_live_config_o),
        .live_pattern_o(group_live_pattern_o), .live_donor_o(group_live_donor_o),
        .live_borrow_o(group_live_borrow_o), .live_release_o(group_live_release_o),
        .live_scan_active_o(group_live_scan_active_unused),
        .live_active_sa_o(group_live_active_sa_unused),
        .live_scan_slot_o(group_live_scan_slot_unused)
    );
endmodule

`default_nettype wire
