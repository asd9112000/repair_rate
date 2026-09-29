`default_nettype none

module tb_g2x2_r_early_live_state_source (
    input logic [7:0] seed_i,
    output logic [4:0] pivot_valid_o,
    output logic [44:0] pivot_rows_flat_o,
    output logic [64:0] pivot_cols_flat_o,
    output logic [4:0] row_gt1_o, row_gt2_o, row_gt3_o,
    output logic [4:0] col_gt1_o, col_gt2_o, col_gt3_o,
    output logic [6:0] hybrid_valid_o,
    output logic [20:0] hybrid_pointer_flat_o,
    output logic [6:0] hybrid_descriptor_o,
    output logic [90:0] hybrid_differing_flat_o,
    output logic conventional_overflow_o
);
    always_comb begin
        pivot_valid_o = seed_i[4:0];
        pivot_rows_flat_o = {5{{1'b0, seed_i}}};
        pivot_cols_flat_o = {5{{5'b0, seed_i}}};
        row_gt1_o = {seed_i[4:1], seed_i[0]};
        row_gt2_o = {seed_i[3:0], seed_i[4]};
        row_gt3_o = {seed_i[2:0], seed_i[4:3]};
        col_gt1_o = {seed_i[0], seed_i[4:1]};
        col_gt2_o = {seed_i[1:0], seed_i[4:2]};
        col_gt3_o = {seed_i[2:0], seed_i[4:3]};
        hybrid_valid_o = seed_i[6:0];
        hybrid_pointer_flat_o = '0;
        hybrid_descriptor_o = seed_i[6:0];
        hybrid_differing_flat_o = '0;
        conventional_overflow_o = seed_i[7];
    end
endmodule

module tb_g2x2_r_early_live_full_top (
    input logic clk_i, rst_ni, canonical_start_i,
    input logic state_update_i, input logic [1:0] state_sa_i,
    input logic test_done_valid_i, input logic [1:0] test_done_sa_i,
    input logic [31:0] seeds_i,
    output logic canonical_done_o, canonical_repairable_o,
    output logic canonical_commit_o, output logic [1:0] canonical_sa_o,
    output logic [2:0] canonical_config_o, output logic [3:0] canonical_pattern_o,
    output logic [64:0] canonical_address_o, output logic [4:0] canonical_is_row_o,
    output logic [4:0] canonical_line_valid_o,
    output logic live_done_o, live_repairable_o, live_commit_o,
    output logic [1:0] live_sa_o, output logic [2:0] live_config_o,
    output logic [3:0] live_pattern_o, output logic [64:0] live_address_o,
    output logic [4:0] live_is_row_o, output logic [4:0] live_line_valid_o,
    output logic live_scan_active_o, output logic [1:0] live_active_sa_o,
    output logic [1:0] live_scan_slot_o
);
    logic [1:0] canonical_sa_q;
    logic [7:0] canonical_seed, live_seed;
    logic [4:0] cpv, lpv, cr1, cr2, cr3, cc1, cc2, cc3, lr1, lr2, lr3, lc1, lc2, lc3;
    logic [44:0] cpr, lpr;
    logic [64:0] cpc, lpc;
    logic [6:0] chv, chd, lhv, lhd;
    logic [20:0] chp, lhp;
    logic [90:0] chdf, lhdf;
    logic cov, lov;

    assign canonical_sa_q = canonical.core.sa_q;
    assign canonical_seed = seeds_i[canonical_sa_q * 8 +: 8];
    assign live_seed = seeds_i[live_active_sa_o * 8 +: 8];

    tb_g2x2_r_early_live_state_source canonical_source (
        .seed_i(canonical_seed), .pivot_valid_o(cpv), .pivot_rows_flat_o(cpr),
        .pivot_cols_flat_o(cpc), .row_gt1_o(cr1), .row_gt2_o(cr2), .row_gt3_o(cr3),
        .col_gt1_o(cc1), .col_gt2_o(cc2), .col_gt3_o(cc3), .hybrid_valid_o(chv),
        .hybrid_pointer_flat_o(chp), .hybrid_descriptor_o(chd),
        .hybrid_differing_flat_o(chdf), .conventional_overflow_o(cov)
    );
    tb_g2x2_r_early_live_state_source live_source (
        .seed_i(live_seed), .pivot_valid_o(lpv), .pivot_rows_flat_o(lpr),
        .pivot_cols_flat_o(lpc), .row_gt1_o(lr1), .row_gt2_o(lr2), .row_gt3_o(lr3),
        .col_gt1_o(lc1), .col_gt2_o(lc2), .col_gt3_o(lc3), .hybrid_valid_o(lhv),
        .hybrid_pointer_flat_o(lhp), .hybrid_descriptor_o(lhd),
        .hybrid_differing_flat_o(lhdf), .conventional_overflow_o(lov)
    );

    recam_dss_g2x2_r_static_early_top canonical (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(canonical_start_i),
        .pivot_valid_i(cpv), .pivot_rows_flat_i(cpr), .pivot_cols_flat_i(cpc),
        .row_gt1_i(cr1), .row_gt2_i(cr2), .row_gt3_i(cr3), .col_gt1_i(cc1),
        .col_gt2_i(cc2), .col_gt3_i(cc3), .hybrid_valid_i(chv),
        .hybrid_pointer_flat_i(chp), .hybrid_descriptor_i(chd),
        .hybrid_differing_flat_i(chdf), .conventional_overflow_i(cov), .busy_o(),
        .done_o(canonical_done_o), .group_repairable_o(canonical_repairable_o),
        .failure_position_o(), .solution_commit_valid_o(canonical_commit_o),
        .solution_sa_o(canonical_sa_o), .solution_config_o(canonical_config_o),
        .solution_pattern_o(canonical_pattern_o), .solution_line_address_flat_o(canonical_address_o),
        .solution_line_is_row_o(canonical_is_row_o), .solution_line_valid_o(canonical_line_valid_o)
    );
    recam_dss_g2x2_r_static_early_live_state_top live (
        .clk_i(clk_i), .rst_ni(rst_ni), .state_update_i(state_update_i),
        .state_sa_i(state_sa_i), .test_done_valid_i(test_done_valid_i),
        .test_done_sa_i(test_done_sa_i), .pivot_valid_i(lpv), .pivot_rows_flat_i(lpr),
        .pivot_cols_flat_i(lpc), .row_gt1_i(lr1), .row_gt2_i(lr2), .row_gt3_i(lr3),
        .col_gt1_i(lc1), .col_gt2_i(lc2), .col_gt3_i(lc3), .hybrid_valid_i(lhv),
        .hybrid_pointer_flat_i(lhp), .hybrid_descriptor_i(lhd),
        .hybrid_differing_flat_i(lhdf), .conventional_overflow_i(lov),
        .scan_active_o(live_scan_active_o), .active_sa_o(live_active_sa_o),
        .scan_slot_o(live_scan_slot_o), .scan_config_o(), .solution_commit_valid_o(live_commit_o),
        .solution_sa_o(live_sa_o), .solution_config_o(live_config_o),
        .solution_pattern_o(live_pattern_o), .solution_line_address_flat_o(live_address_o),
        .solution_line_is_row_o(live_is_row_o), .solution_line_valid_o(live_line_valid_o),
        .done_o(live_done_o), .group_repairable_o(live_repairable_o), .failure_position_o()
    );
endmodule

`default_nettype wire
