`default_nettype none

// Composite supporting ablation: registered maps -> SYN-B OPT1/DFS -> atomic
// commit, with the frozen 440-bit GROUP pivot-address retention block.
module recam_n2_preopt_group_top #(
    parameter integer ROW_ADDR_W = 9, parameter integer PHYS_COL_ADDR_W = 13,
    parameter integer WORD_COL_ADDR_W = 5, parameter integer HYBRID_LINE_ADDR_W = 13,
    parameter integer HYBRID_ENTRIES = 7
) (
    input wire clk_i, input wire rst_ni, input wire start_i,
    input wire [19:0] pivot_valid_flat_i,
    input wire [4*5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input wire [4*5*PHYS_COL_ADDR_W-1:0] pivot_cols_flat_i,
    input wire [19:0] row_gt1_flat_i, input wire [19:0] row_gt2_flat_i,
    input wire [19:0] row_gt3_flat_i, input wire [19:0] col_gt1_flat_i,
    input wire [19:0] col_gt2_flat_i, input wire [19:0] col_gt3_flat_i,
    input wire [4*HYBRID_ENTRIES-1:0] hybrid_valid_flat_i,
    input wire [4*HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input wire [4*HYBRID_ENTRIES-1:0] hybrid_descriptor_flat_i,
    input wire [4*HYBRID_ENTRIES*HYBRID_LINE_ADDR_W-1:0] hybrid_differing_flat_i,
    input wire [3:0] conventional_overflow_i,
    output wire busy_o, output wire done_o, output wire group_repairable_o,
    output wire commit_error_o, output wire [3:0] selected_valid_o,
    output wire [7:0] selected_action_flat_o, output wire [11:0] selected_config_flat_o,
    output wire [15:0] selected_pattern_flat_o, output wire [7:0] selected_donor_flat_o,
    output wire [3:0] selected_release_o, output wire [3:0] selected_borrow_o,
    output wire [3:0] resource_released_o, output wire [3:0] resource_borrowed_o,
    output wire [7:0] borrower_id_flat_o, output wire [159:0] candidate_valid_debug_o,
    output wire [159:0] candidate_release_debug_o, output wire [159:0] candidate_borrow_debug_o,
    output wire [56:0] historical_dfs_selection_debug_o,
    output wire [4*5*PHYS_COL_ADDR_W-1:0] final_repair_address_flat_o,
    output wire [19:0] final_repair_is_row_flat_o, output wire [19:0] final_repair_line_valid_flat_o
);
    localparam [2:0] IDLE=0, PSTART=1, PWAIT=2, SSTART=3, SWAIT=4, CSTART=5, CWAIT=6;
    reg [2:0] state_q; reg producer_start_q, search_start_q, commit_start_q, done_q, repairable_q, commit_error_q;
    reg [3:0] selected_valid_q, selected_release_q, selected_borrow_q;
    reg [7:0] selected_action_q, selected_donor_q; reg [11:0] selected_config_q; reg [15:0] selected_pattern_q;
    reg [19:0] pivot_valid_q, row_gt1_q, row_gt2_q, row_gt3_q, col_gt1_q, col_gt2_q, col_gt3_q;
    reg [4*5*ROW_ADDR_W-1:0] pivot_rows_q; reg [4*5*PHYS_COL_ADDR_W-1:0] pivot_cols_q;
    reg [4*HYBRID_ENTRIES-1:0] hybrid_valid_q, hybrid_descriptor_q;
    reg [4*HYBRID_ENTRIES*3-1:0] hybrid_pointer_q;
    reg [4*HYBRID_ENTRIES*HYBRID_LINE_ADDR_W-1:0] hybrid_differing_q; reg [3:0] overflow_q;
    wire producer_busy, producer_done, search_busy, search_done, search_repairable, commit_busy, commit_accepted, commit_error;
    wire [159:0] candidate_valid, candidate_release, candidate_borrow;
    wire [1:0] active_sa, active_action;
    /* verilator lint_off UNUSED */
    wire [2:0] active_config;
    /* verilator lint_on UNUSED */
    wire [3:0] search_valid, search_release, search_borrow, unused_released, unused_used;
    wire [7:0] search_action, search_donor; wire [11:0] search_config; wire [15:0] search_pattern;
    wire canonical_repairable;
    wire [3:0] canonical_valid, canonical_release, canonical_release_by_sa, canonical_borrow;
    wire [7:0] canonical_action, canonical_donor;
    wire [11:0] canonical_config;
    wire [15:0] canonical_pattern;
    wire [79:0] unused_canonical_slot_image;
    recam_n2_preopt_candidate_map_producer #(.ROW_ADDR_W(ROW_ADDR_W), .PHYS_COL_ADDR_W(PHYS_COL_ADDR_W), .WORD_COL_ADDR_W(WORD_COL_ADDR_W), .HYBRID_LINE_ADDR_W(HYBRID_LINE_ADDR_W), .HYBRID_ENTRIES(HYBRID_ENTRIES)) producer (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(producer_start_q), .pivot_valid_flat_i(pivot_valid_q), .pivot_rows_flat_i(pivot_rows_q), .pivot_cols_flat_i(pivot_cols_q), .row_gt1_flat_i(row_gt1_q), .row_gt2_flat_i(row_gt2_q), .row_gt3_flat_i(row_gt3_q), .col_gt1_flat_i(col_gt1_q), .col_gt2_flat_i(col_gt2_q), .col_gt3_flat_i(col_gt3_q), .hybrid_valid_flat_i(hybrid_valid_q), .hybrid_pointer_flat_i(hybrid_pointer_q), .hybrid_descriptor_flat_i(hybrid_descriptor_q), .hybrid_differing_flat_i(hybrid_differing_q), .conventional_overflow_i(overflow_q), .busy_o(producer_busy), .done_o(producer_done), .candidate_valid_o(candidate_valid), .candidate_release_o(candidate_release), .candidate_borrow_o(candidate_borrow), .active_sa_o(active_sa), .active_action_o(active_action), .active_config_id_o(active_config));
    recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_core search (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(search_start_q), .candidate_valid_i(candidate_valid), .candidate_release_i(candidate_release), .candidate_borrow_i(candidate_borrow), .busy_o(search_busy), .done_o(search_done), .group_repairable_o(search_repairable), .selected_valid_o(search_valid), .selected_action_flat_o(search_action), .selected_config_flat_o(search_config), .selected_pattern_flat_o(search_pattern), .selected_donor_flat_o(search_donor), .selected_release_o(search_release), .selected_borrow_o(search_borrow), .final_released_mask_o(unused_released), .final_used_mask_o(unused_used));
    recam_n2_preopt_canonical_path_selector canonical_path_selector (
        .candidate_valid_i(candidate_valid), .group_repairable_o(canonical_repairable),
        .selected_valid_o(canonical_valid), .selected_action_flat_o(canonical_action),
        .selected_config_flat_o(canonical_config), .selected_pattern_flat_o(canonical_pattern),
        .selected_donor_flat_o(canonical_donor), .selected_release_o(canonical_release),
        .selected_release_by_sa_o(canonical_release_by_sa), .selected_borrow_o(canonical_borrow), .canonical_slot_image_o(unused_canonical_slot_image)
    );
    recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_atomic_group_commit commit (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(commit_start_q), .selected_valid_i(canonical_valid), .selected_action_flat_i(canonical_action), .selected_release_i(canonical_release_by_sa), .selected_borrow_i(canonical_borrow), .selected_donor_flat_i(canonical_donor), .busy_o(commit_busy), .commit_accepted_o(commit_accepted), .commit_error_o(commit_error), .resource_released_o(resource_released_o), .resource_borrowed_o(resource_borrowed_o), .borrower_id_flat_o(borrower_id_flat_o));
    dss_group_pivot_address_regs #(.ROW_ADDR_W(ROW_ADDR_W), .PHYS_COL_ADDR_W(PHYS_COL_ADDR_W)) retention (
        .clk_i(clk_i), .rst_ni(rst_ni), .capture_enable_i(producer_busy && active_action == 2'd3), .capture_sa_i(active_sa), .pivot_rows_flat_i(pivot_rows_q[active_sa*(5*ROW_ADDR_W) +: 5*ROW_ADDR_W]), .pivot_cols_flat_i(pivot_cols_q[active_sa*(5*PHYS_COL_ADDR_W) +: 5*PHYS_COL_ADDR_W]), .group_commit_valid_i(selected_valid_q), .selected_config_flat_i(selected_config_q), .selected_pattern_flat_i(selected_pattern_q), .final_repair_address_flat_o(final_repair_address_flat_o), .final_repair_is_row_flat_o(final_repair_is_row_flat_o), .final_repair_line_valid_flat_o(final_repair_line_valid_flat_o));
    always @(posedge clk_i) begin
        producer_start_q<=0; search_start_q<=0; commit_start_q<=0; done_q<=0;
        if (!rst_ni) begin state_q<=IDLE; repairable_q<=0; commit_error_q<=0; selected_valid_q<=0; selected_action_q<=0; selected_config_q<=0; selected_pattern_q<=0; selected_donor_q<=0; selected_release_q<=0; selected_borrow_q<=0; pivot_valid_q<=0; pivot_rows_q<=0; pivot_cols_q<=0; row_gt1_q<=0; row_gt2_q<=0; row_gt3_q<=0; col_gt1_q<=0; col_gt2_q<=0; col_gt3_q<=0; hybrid_valid_q<=0; hybrid_pointer_q<=0; hybrid_descriptor_q<=0; hybrid_differing_q<=0; overflow_q<=0; end
        else case(state_q)
            IDLE: if(start_i && resource_released_o==0 && resource_borrowed_o==0) begin pivot_valid_q<=pivot_valid_flat_i; pivot_rows_q<=pivot_rows_flat_i; pivot_cols_q<=pivot_cols_flat_i; row_gt1_q<=row_gt1_flat_i; row_gt2_q<=row_gt2_flat_i; row_gt3_q<=row_gt3_flat_i; col_gt1_q<=col_gt1_flat_i; col_gt2_q<=col_gt2_flat_i; col_gt3_q<=col_gt3_flat_i; hybrid_valid_q<=hybrid_valid_flat_i; hybrid_pointer_q<=hybrid_pointer_flat_i; hybrid_descriptor_q<=hybrid_descriptor_flat_i; hybrid_differing_q<=hybrid_differing_flat_i; overflow_q<=conventional_overflow_i; repairable_q<=0; commit_error_q<=0; selected_valid_q<=0; state_q<=PSTART; end
            PSTART: begin producer_start_q<=1; state_q<=PWAIT; end
            PWAIT: if(producer_done) state_q<=SSTART;
            SSTART: begin search_start_q<=1; state_q<=CSTART; end
            SWAIT: if(search_done) if(canonical_repairable) state_q<=CSTART; else begin repairable_q<=0; done_q<=1; state_q<=IDLE; end
            CSTART: begin commit_start_q<=1; state_q<=CWAIT; end
            default: if(commit_accepted || commit_error) begin if(commit_error) commit_error_q<=1; selected_valid_q<=canonical_valid; selected_action_q<=canonical_action; selected_config_q<=canonical_config; selected_pattern_q<=canonical_pattern; selected_donor_q<=canonical_donor; selected_release_q<=canonical_release; selected_borrow_q<=canonical_borrow; repairable_q<=canonical_repairable; done_q<=1; state_q<=IDLE; end
        endcase
    end
    assign busy_o=(state_q!=IDLE)||producer_busy||search_busy||commit_busy; assign done_o=done_q; assign group_repairable_o=repairable_q; assign commit_error_o=commit_error_q; assign selected_valid_o=selected_valid_q; assign selected_action_flat_o=selected_action_q; assign selected_config_flat_o=selected_config_q; assign selected_pattern_flat_o=selected_pattern_q; assign selected_donor_flat_o=selected_donor_q; assign selected_release_o=selected_release_q; assign selected_borrow_o=selected_borrow_q; assign candidate_valid_debug_o=candidate_valid; assign candidate_release_debug_o=candidate_release; assign candidate_borrow_debug_o=candidate_borrow; assign historical_dfs_selection_debug_o={search_repairable, search_valid, search_action, search_config, search_pattern, search_donor, search_release, search_borrow};
endmodule
`default_nettype wire
