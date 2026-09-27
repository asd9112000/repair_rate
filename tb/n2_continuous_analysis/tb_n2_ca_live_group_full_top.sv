`default_nettype none

module tb_n2_ca_live_group_state_source (
    input logic [7:0] seed_i,
    output logic [4:0] pivot_valid_o, output logic [44:0] pivot_rows_o,
    output logic [64:0] pivot_cols_o, output logic [4:0] row_gt1_o,
    output logic [4:0] row_gt2_o, output logic [4:0] row_gt3_o,
    output logic [4:0] col_gt1_o, output logic [4:0] col_gt2_o,
    output logic [4:0] col_gt3_o, output logic [6:0] hybrid_valid_o,
    output logic [20:0] hybrid_pointer_o, output logic [6:0] hybrid_descriptor_o,
    output logic [90:0] hybrid_differing_o, output logic conventional_overflow_o
);
    always_comb begin
        pivot_valid_o = seed_i[4:0]; pivot_rows_o = {5{{1'b0, seed_i}}};
        pivot_cols_o = {5{{5'b0, seed_i}}}; row_gt1_o = {seed_i[4:1], seed_i[0]};
        row_gt2_o = {seed_i[3:0], seed_i[4]}; row_gt3_o = {seed_i[2:0], seed_i[4:3]};
        col_gt1_o = {seed_i[0], seed_i[4:1]}; col_gt2_o = {seed_i[1:0], seed_i[4:2]};
        col_gt3_o = {seed_i[2:0], seed_i[4:3]}; hybrid_valid_o = seed_i[6:0];
        hybrid_pointer_o = '0; hybrid_descriptor_o = seed_i[6:0]; hybrid_differing_o = '0;
        conventional_overflow_o = seed_i[7];
    end
endmodule

module tb_n2_ca_live_group_full_top (
    input logic clk_i, input logic rst_ni, input logic canonical_start_i,
    input logic live_state_update_i, input logic [1:0] live_state_sa_i,
    input logic live_test_done_valid_i, input logic [1:0] live_test_done_sa_i,
    input logic [31:0] state_seed_flat_i, output logic canonical_done_o,
    output logic canonical_repairable_o, output logic [3:0] canonical_commit_o,
    output logic [11:0] canonical_config_o, output logic [15:0] canonical_pattern_o,
    output logic [7:0] canonical_donor_o, output logic [3:0] canonical_borrow_o,
    output logic [3:0] canonical_release_o, output logic [259:0] canonical_address_o,
    output logic [19:0] canonical_is_row_o, output logic [19:0] canonical_line_valid_o,
    output logic live_ready_o, output logic [3:0] live_frozen_o,
    output logic live_repairable_o, output logic [3:0] live_commit_o,
    output logic [11:0] live_config_o, output logic [15:0] live_pattern_o,
    output logic [7:0] live_donor_o, output logic [3:0] live_borrow_o,
    output logic [3:0] live_release_o, output logic [259:0] live_address_o,
    output logic [19:0] live_is_row_o, output logic [19:0] live_line_valid_o,
    output logic live_scan_active_o, output logic [1:0] live_active_sa_o
);
    logic [1:0] canonical_sa; logic [7:0] canonical_seed, live_seed;
    logic [4:0] cpv, lpv, cr1, cr2, cr3, cc1, cc2, cc3, lr1, lr2, lr3, lc1, lc2, lc3;
    logic [44:0] cpr, lpr; logic [64:0] cpc, lpc; logic [6:0] chv, chd, lhv, lhd;
    logic [20:0] chp, lhp; logic [90:0] chdf, lhdf; logic cov, lov;
    assign canonical_sa = canonical.core.collect_sa_q;
    assign canonical_seed = state_seed_flat_i[canonical_sa*8 +: 8];
    assign live_seed = state_seed_flat_i[live_active_sa_o*8 +: 8];
    tb_n2_ca_live_group_state_source canonical_source(.seed_i(canonical_seed),.pivot_valid_o(cpv),.pivot_rows_o(cpr),.pivot_cols_o(cpc),.row_gt1_o(cr1),.row_gt2_o(cr2),.row_gt3_o(cr3),.col_gt1_o(cc1),.col_gt2_o(cc2),.col_gt3_o(cc3),.hybrid_valid_o(chv),.hybrid_pointer_o(chp),.hybrid_descriptor_o(chd),.hybrid_differing_o(chdf),.conventional_overflow_o(cov));
    tb_n2_ca_live_group_state_source live_source(.seed_i(live_seed),.pivot_valid_o(lpv),.pivot_rows_o(lpr),.pivot_cols_o(lpc),.row_gt1_o(lr1),.row_gt2_o(lr2),.row_gt3_o(lr3),.col_gt1_o(lc1),.col_gt2_o(lc2),.col_gt3_o(lc3),.hybrid_valid_o(lhv),.hybrid_pointer_o(lhp),.hybrid_descriptor_o(lhd),.hybrid_differing_o(lhdf),.conventional_overflow_o(lov));
    recam_dss_hyp02_static_global_top canonical(.clk_i(clk_i),.rst_ni(rst_ni),.start_i(canonical_start_i),.pivot_valid_i(cpv),.pivot_rows_flat_i(cpr),.pivot_cols_flat_i(cpc),.row_gt1_i(cr1),.row_gt2_i(cr2),.row_gt3_i(cr3),.col_gt1_i(cc1),.col_gt2_i(cc2),.col_gt3_i(cc3),.hybrid_valid_i(chv),.hybrid_pointer_flat_i(chp),.hybrid_descriptor_i(chd),.hybrid_differing_flat_i(chdf),.conventional_overflow_i(cov),.busy_o(),.done_o(canonical_done_o),.group_repairable_o(canonical_repairable_o),.sa_commit_valid_o(canonical_commit_o),.ledger_released_borrower_o(),.selected_config_flat_o(canonical_config_o),.selected_pattern_flat_o(canonical_pattern_o),.selected_donor_flat_o(canonical_donor_o),.borrow_flat_o(canonical_borrow_o),.release_flat_o(canonical_release_o),.failure_position_o(),.final_repair_address_flat_o(canonical_address_o),.final_repair_is_row_flat_o(canonical_is_row_o),.final_repair_line_valid_flat_o(canonical_line_valid_o));
    recam_dss_hyp02_static_global_live_state_top live(.clk_i(clk_i),.rst_ni(rst_ni),.state_update_i(live_state_update_i),.state_sa_i(live_state_sa_i),.test_done_valid_i(live_test_done_valid_i),.test_done_sa_i(live_test_done_sa_i),.pivot_valid_i(lpv),.pivot_rows_flat_i(lpr),.pivot_cols_flat_i(lpc),.row_gt1_i(lr1),.row_gt2_i(lr2),.row_gt3_i(lr3),.col_gt1_i(lc1),.col_gt2_i(lc2),.col_gt3_i(lc3),.hybrid_valid_i(lhv),.hybrid_pointer_flat_i(lhp),.hybrid_descriptor_i(lhd),.hybrid_differing_flat_i(lhdf),.conventional_overflow_i(lov),.scan_active_o(live_scan_active_o),.active_sa_o(live_active_sa_o),.scan_slot_o(),.scan_config_o(),.sa_result_frozen_o(live_frozen_o),.solution_ready_o(live_ready_o),.candidate_store_image_o(),.group_repairable_o(live_repairable_o),.sa_commit_valid_o(live_commit_o),.ledger_released_borrower_o(),.selected_config_flat_o(live_config_o),.selected_pattern_flat_o(live_pattern_o),.selected_donor_flat_o(live_donor_o),.borrow_flat_o(live_borrow_o),.release_flat_o(live_release_o),.final_repair_address_flat_o(live_address_o),.final_repair_is_row_flat_o(live_is_row_o),.final_repair_line_valid_flat_o(live_line_valid_o));
endmodule

`default_nettype wire
