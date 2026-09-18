`default_nettype none

// Verification-only lockstep oracle: the unmodified historical collector and
// the A2G retained bank consume the same accepted faults.  It compares every
// retained semantic field, the frozen 232-bit boundary, and the nine-slot
// analyzer result after each transition.
module recam_dss_v2_retained_collector_lockstep_top (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic clear_i,
    input  logic fault_valid_i,
    input  logic [9:0] fault_row_i,
    input  logic [9:0] fault_col_i,
    input  logic [2:0] config_id_i,
    output logic packing_match_o,
    output logic must_match_o,
    output logic view_match_o,
    output logic analyzer_match_o,
    output logic [3:0] retained_fault_count_o,
    output logic [3:0] retained_hybrid_count_o,
    output logic [3:0] retained_reuse_count_o,
    output logic [3:0] projected_count_o,
    output logic [3:0] membership_count_o
);
    logic historical_ready;
    logic [3:0] historical_fault_count;
    logic [4:0] historical_pivot_valid;
    logic [49:0] historical_pivot_rows;
    logic [49:0] historical_pivot_cols;
    logic [34:0] unused_historical_pivot_cfg;
    logic [2:0] unused_historical_pivot_occupancy;
    logic [13:0] historical_hybrid_valid;
    logic [139:0] historical_hybrid_rows;
    logic [139:0] historical_hybrid_cols;
    logic [41:0] historical_hybrid_pointer;
    logic [13:0] historical_hybrid_descriptor;
    logic [97:0] historical_hybrid_cfg;
    logic [3:0] unused_historical_hybrid_occupancy;
    logic [11:0] historical_reuse_valid;
    logic [119:0] historical_reuse_rows;
    logic [119:0] historical_reuse_cols;
    logic [83:0] historical_reuse_cfg;
    logic [3:0] unused_historical_reuse_occupancy;
    logic historical_counter_overflow;
    logic historical_hybrid_overflow;
    logic unused_historical_reuse_overflow;
    logic [11:0] historical_row_counter_valid;
    logic [119:0] historical_row_counter_addrs;
    logic [47:0] historical_row_counter_counts;
    logic [11:0] historical_col_counter_valid;
    logic [119:0] historical_col_counter_addrs;
    logic [47:0] historical_col_counter_counts;

    logic bank_ready;
    logic [820:0] bank_state;
    logic [3:0] unused_bank_fault_generation;
    logic unused_bank_analysis_valid;
    logic [3:0] unused_bank_analysis_generation;
    logic [4:0] bank_pivot_valid;
    logic [49:0] bank_pivot_rows;
    logic [49:0] bank_pivot_cols;
    logic [13:0] bank_hybrid_valid;
    logic [139:0] bank_hybrid_rows;
    logic [139:0] bank_hybrid_cols;
    logic [41:0] bank_hybrid_pointer;
    logic [13:0] bank_hybrid_descriptor;
    logic [97:0] bank_hybrid_cfg;
    logic [11:0] bank_reuse_valid;
    logic [119:0] bank_reuse_rows;
    logic [119:0] bank_reuse_cols;
    logic [83:0] bank_reuse_cfg;
    logic [34:0] bank_row_must;
    logic [34:0] bank_col_must;
    logic [3:0] bank_fault_count;
    logic [231:0] bank_view;
    logic [34:0] historical_row_must;
    logic [34:0] historical_col_must;
    logic [125:0] historical_hybrid_differing;
    logic [8:0] projected_valid;
    logic [26:0] projected_pointer;
    logic [8:0] projected_descriptor;
    logic [80:0] projected_differing;
    logic [3:0] projected_count;
    logic projected_overflow;
    logic [231:0] historical_view;
    logic [8:0] bank_projected_valid;
    logic [26:0] bank_projected_pointer;
    logic [8:0] bank_projected_descriptor;
    logic [80:0] bank_projected_differing;
    logic [9:0] bank_candidates;
    logic [3:0] bank_pattern;
    logic bank_solution;
    logic bank_repairable;
    logic bank_dictionary_overflow;
    logic [9:0] historical_candidates;
    logic [3:0] historical_pattern;
    logic historical_solution;
    logic historical_repairable;
    logic historical_dictionary_overflow;
    integer cfg;
    integer pivot;
    integer entry;
    integer row_count;
    integer col_count;
    integer compact;
    integer membership;

    function automatic integer cfg_rows(input integer cfg_id);
        case (cfg_id)
            0, 1, 5: cfg_rows = 2;
            2, 3: cfg_rows = 3;
            4, 6: cfg_rows = 1;
            default: cfg_rows = 0;
        endcase
    endfunction
    function automatic integer cfg_cols(input integer cfg_id);
        case (cfg_id)
            0, 2, 4: cfg_cols = 2;
            1, 3: cfg_cols = 1;
            5, 6: cfg_cols = 3;
            default: cfg_cols = 0;
        endcase
    endfunction
    function automatic integer cfg_k(input integer cfg_id);
        cfg_k = cfg_rows(cfg_id) + cfg_cols(cfg_id);
    endfunction

    /* verilator lint_off PINCONNECTEMPTY */
    shared_fault_collector historical (
        .clk_i(clk_i), .rst_ni(rst_ni), .clear_i(clear_i), .fault_valid_i(fault_valid_i),
        .fault_row_i(fault_row_i), .fault_col_i(fault_col_i), .fault_ready_o(historical_ready),
        .fault_count_o(historical_fault_count), .pivot_valid_o(historical_pivot_valid),
        .pivot_rows_flat_o(historical_pivot_rows), .pivot_cols_flat_o(historical_pivot_cols),
        .cfg_pivot_valid_o(unused_historical_pivot_cfg), .pivot_occupancy_o(unused_historical_pivot_occupancy),
        .hybrid_valid_o(historical_hybrid_valid), .hybrid_rows_flat_o(historical_hybrid_rows),
        .hybrid_cols_flat_o(historical_hybrid_cols), .hybrid_ptrs_flat_o(historical_hybrid_pointer),
        .hybrid_descriptors_o(historical_hybrid_descriptor), .hybrid_cfg_valid_flat_o(historical_hybrid_cfg),
        .hybrid_occupancy_o(unused_historical_hybrid_occupancy), .cam_reuse_valid_o(historical_reuse_valid),
        .cam_reuse_rows_flat_o(historical_reuse_rows), .cam_reuse_cols_flat_o(historical_reuse_cols),
        .cam_reuse_cfg_valid_flat_o(historical_reuse_cfg), .cam_reuse_occupancy_o(unused_historical_reuse_occupancy),
        .counter_overflow_o(historical_counter_overflow), .hybrid_overflow_o(historical_hybrid_overflow),
        .cam_reuse_overflow_o(unused_historical_reuse_overflow),
        .row_counter_valid_o(historical_row_counter_valid), .row_counter_addrs_flat_o(historical_row_counter_addrs),
        .row_counter_counts_flat_o(historical_row_counter_counts), .col_counter_valid_o(historical_col_counter_valid),
        .col_counter_addrs_flat_o(historical_col_counter_addrs), .col_counter_counts_flat_o(historical_col_counter_counts)
    );

    recam_dss_v2_retained_collector_bank bank (
        .clk_i(clk_i), .rst_ni(rst_ni), .clear_i(clear_i), .fault_valid_i(fault_valid_i),
        .fault_row_i(fault_row_i), .fault_col_i(fault_col_i), .config_id_i(config_id_i),
        .analysis_complete_i(1'b0), .analysis_generation_i('0), .fault_ready_o(bank_ready),
        .retained_state_o(bank_state), .fault_generation_o(unused_bank_fault_generation),
        .analysis_valid_o(unused_bank_analysis_valid), .analysis_generation_o(unused_bank_analysis_generation),
        .pivot_valid_o(bank_pivot_valid), .pivot_rows_flat_o(bank_pivot_rows), .pivot_cols_flat_o(bank_pivot_cols),
        .hybrid_valid_o(bank_hybrid_valid), .hybrid_rows_flat_o(bank_hybrid_rows), .hybrid_cols_flat_o(bank_hybrid_cols),
        .hybrid_pointer_flat_o(bank_hybrid_pointer), .hybrid_descriptor_o(bank_hybrid_descriptor),
        .hybrid_cfg_valid_flat_o(bank_hybrid_cfg), .reuse_valid_o(bank_reuse_valid),
        .reuse_rows_flat_o(bank_reuse_rows), .reuse_cols_flat_o(bank_reuse_cols), .reuse_cfg_valid_flat_o(bank_reuse_cfg),
        .row_must_by_cfg_o(bank_row_must), .col_must_by_cfg_o(bank_col_must),
        .fault_count_o(bank_fault_count), .analyzer_view_o(bank_view)
    );
    /* verilator lint_on PINCONNECTEMPTY */

    always_comb begin
        historical_row_must = '0;
        historical_col_must = '0;
        historical_hybrid_differing = '0;
        for (entry = 0; entry < 14; entry = entry + 1) begin
            historical_hybrid_differing[entry * 9 +: 9] = historical_hybrid_descriptor[entry]
                ? historical_hybrid_rows[entry * 10 +: 9]
                : historical_hybrid_cols[entry * 10 +: 9];
        end
        for (cfg = 0; cfg < 7; cfg = cfg + 1) begin
            for (pivot = 0; pivot < 5; pivot = pivot + 1) begin
                row_count = 0;
                col_count = 0;
                for (entry = 0; entry < 12; entry = entry + 1) begin
                    if (historical_row_counter_valid[entry] &&
                        historical_row_counter_addrs[entry * 10 +: 10] == historical_pivot_rows[pivot * 10 +: 10])
                        row_count = int'(historical_row_counter_counts[entry * 4 +: 4]);
                    if (historical_col_counter_valid[entry] &&
                        historical_col_counter_addrs[entry * 10 +: 10] == historical_pivot_cols[pivot * 10 +: 10])
                        col_count = int'(historical_col_counter_counts[entry * 4 +: 4]);
                end
                historical_row_must[cfg * 5 + pivot] = pivot < unused_historical_pivot_occupancy && row_count > cfg_cols(cfg);
                historical_col_must[cfg * 5 + pivot] = pivot < unused_historical_pivot_occupancy && col_count > cfg_rows(cfg);
            end
        end
    end

    recam_post_must_hybrid_projector historical_projector (
        .config_id_i(config_id_i), .physical_hybrid_valid_i(historical_hybrid_valid),
        .physical_hybrid_pointer_flat_i(historical_hybrid_pointer), .physical_hybrid_descriptor_i(historical_hybrid_descriptor),
        .physical_hybrid_differing_flat_i(historical_hybrid_differing), .physical_hybrid_cfg_valid_flat_i(historical_hybrid_cfg),
        .row_must_by_cfg_i(historical_row_must), .col_must_by_cfg_i(historical_col_must),
        .projected_hybrid_valid_o(projected_valid), .projected_hybrid_pointer_flat_o(projected_pointer),
        .projected_hybrid_descriptor_o(projected_descriptor), .projected_hybrid_differing_flat_o(projected_differing),
        .projected_count_o(projected_count), .projection_overflow_o(projected_overflow)
    );

    always_comb begin
        historical_view = '0;
        for (pivot = 0; pivot < 5; pivot = pivot + 1) begin
            historical_view[pivot] = pivot < unused_historical_pivot_occupancy && pivot < cfg_k(int'(config_id_i));
            row_count = 0;
            col_count = 0;
            if (historical_pivot_valid[pivot]) begin
                historical_view[5 + pivot * 9 +: 9] = historical_pivot_rows[pivot * 10 +: 9];
                historical_view[50 + pivot * 5 +: 5] = historical_pivot_cols[pivot * 10 +: 5];
                for (entry = 0; entry < 12; entry = entry + 1) begin
                    if (historical_row_counter_valid[entry] && historical_row_counter_addrs[entry * 10 +: 10] == historical_pivot_rows[pivot * 10 +: 10])
                        row_count = int'(historical_row_counter_counts[entry * 4 +: 4]);
                    if (historical_col_counter_valid[entry] && historical_col_counter_addrs[entry * 10 +: 10] == historical_pivot_cols[pivot * 10 +: 10])
                        col_count = int'(historical_col_counter_counts[entry * 4 +: 4]);
                end
            end
            historical_view[75 + pivot] = row_count > 1;
            historical_view[80 + pivot] = row_count > 2;
            historical_view[85 + pivot] = row_count > 3;
            historical_view[90 + pivot] = col_count > 1;
            historical_view[95 + pivot] = col_count > 2;
            historical_view[100 + pivot] = col_count > 3;
        end
        historical_view[105] = historical_counter_overflow || historical_hybrid_overflow;
        for (compact = 0; compact < 9; compact = compact + 1) begin
            historical_view[106 + compact * 14] = projected_valid[compact];
            historical_view[107 + compact * 14 +: 3] = projected_pointer[compact * 3 +: 3];
            historical_view[110 + compact * 14] = projected_descriptor[compact];
            historical_view[111 + compact * 14 +: 9] = projected_differing[compact * 9 +: 9];
        end
    end

    always_comb begin
        bank_projected_valid = '0;
        bank_projected_pointer = '0;
        bank_projected_descriptor = '0;
        bank_projected_differing = '0;
        for (compact = 0; compact < 9; compact = compact + 1) begin
            bank_projected_valid[compact] = bank_view[106 + compact * 14];
            bank_projected_pointer[compact * 3 +: 3] = bank_view[107 + compact * 14 +: 3];
            bank_projected_descriptor[compact] = bank_view[110 + compact * 14];
            bank_projected_differing[compact * 9 +: 9] = bank_view[111 + compact * 14 +: 9];
        end
    end

    recam_shared_config_analyzer #(.HYBRID_ENTRIES(9)) bank_analyzer (
        .config_id_i(config_id_i), .pivot_valid_i(bank_view[4:0]), .pivot_rows_flat_i(bank_view[49:5]),
        .pivot_cols_flat_i(bank_view[74:50]), .row_gt1_i(bank_view[79:75]), .row_gt2_i(bank_view[84:80]),
        .row_gt3_i(bank_view[89:85]), .col_gt1_i(bank_view[94:90]), .col_gt2_i(bank_view[99:95]),
        .col_gt3_i(bank_view[104:100]), .hybrid_valid_i(bank_projected_valid),
        .hybrid_pointer_flat_i(bank_projected_pointer), .hybrid_descriptor_i(bank_projected_descriptor),
        .hybrid_differing_flat_i(bank_projected_differing), .conventional_overflow_i(bank_view[105]),
        .candidate_valid_o(bank_candidates), .pattern_id_o(bank_pattern), .solution_valid_o(bank_solution),
        .repairable_o(bank_repairable), .dictionary_overflow_o(bank_dictionary_overflow)
    );
    recam_shared_config_analyzer #(.HYBRID_ENTRIES(9)) historical_analyzer (
        .config_id_i(config_id_i), .pivot_valid_i(historical_view[4:0]), .pivot_rows_flat_i(historical_view[49:5]),
        .pivot_cols_flat_i(historical_view[74:50]), .row_gt1_i(historical_view[79:75]), .row_gt2_i(historical_view[84:80]),
        .row_gt3_i(historical_view[89:85]), .col_gt1_i(historical_view[94:90]), .col_gt2_i(historical_view[99:95]),
        .col_gt3_i(historical_view[104:100]), .hybrid_valid_i(projected_valid),
        .hybrid_pointer_flat_i(projected_pointer), .hybrid_descriptor_i(projected_descriptor),
        .hybrid_differing_flat_i(projected_differing), .conventional_overflow_i(historical_view[105]),
        .candidate_valid_o(historical_candidates), .pattern_id_o(historical_pattern), .solution_valid_o(historical_solution),
        .repairable_o(historical_repairable), .dictionary_overflow_o(historical_dictionary_overflow)
    );

    always_comb begin
        packing_match_o = historical_ready == bank_ready && historical_fault_count == bank_fault_count &&
            historical_pivot_valid == bank_pivot_valid && historical_hybrid_valid == bank_hybrid_valid &&
            historical_reuse_valid == bank_reuse_valid;
        for (pivot = 0; pivot < 5; pivot = pivot + 1) begin
            if (historical_pivot_valid[pivot]) begin
                packing_match_o = packing_match_o &&
                    historical_pivot_rows[pivot * 10 +: 10] == bank_pivot_rows[pivot * 10 +: 10] &&
                    historical_pivot_cols[pivot * 10 +: 10] == bank_pivot_cols[pivot * 10 +: 10];
            end
        end
        for (entry = 0; entry < 11; entry = entry + 1) begin
            if (historical_hybrid_valid[entry]) begin
                packing_match_o = packing_match_o &&
                    historical_hybrid_rows[entry * 10 +: 10] == bank_hybrid_rows[entry * 10 +: 10] &&
                    historical_hybrid_cols[entry * 10 +: 10] == bank_hybrid_cols[entry * 10 +: 10] &&
                    historical_hybrid_pointer[entry * 3 +: 3] == bank_hybrid_pointer[entry * 3 +: 3] &&
                    historical_hybrid_descriptor[entry] == bank_hybrid_descriptor[entry] &&
                    historical_hybrid_cfg[entry * 7 +: 7] == bank_hybrid_cfg[entry * 7 +: 7];
            end
            if (historical_reuse_valid[entry]) begin
                packing_match_o = packing_match_o &&
                    historical_reuse_rows[entry * 10 +: 10] == bank_reuse_rows[entry * 10 +: 10] &&
                    historical_reuse_cols[entry * 10 +: 10] == bank_reuse_cols[entry * 10 +: 10] &&
                    historical_reuse_cfg[entry * 7 +: 7] == bank_reuse_cfg[entry * 7 +: 7];
            end
        end
        for (entry = 0; entry < 12; entry = entry + 1) begin
            packing_match_o = packing_match_o &&
                historical_row_counter_counts[entry * 4 +: 4] == bank_state[485 + entry * 14 +: 4] &&
                historical_col_counter_counts[entry * 4 +: 4] == bank_state[653 + entry * 14 +: 4];
            if (historical_row_counter_valid[entry])
                packing_match_o = packing_match_o && historical_row_counter_addrs[entry * 10 +: 10] == bank_state[489 + entry * 14 +: 10];
            if (historical_col_counter_valid[entry])
                packing_match_o = packing_match_o && historical_col_counter_addrs[entry * 10 +: 10] == bank_state[657 + entry * 14 +: 10];
        end
        must_match_o = historical_row_must == bank_row_must && historical_col_must == bank_col_must;
        view_match_o = historical_view == bank_view && !projected_overflow && projected_count <= 9;
        analyzer_match_o = bank_candidates == historical_candidates && bank_pattern == historical_pattern &&
            bank_solution == historical_solution && bank_repairable == historical_repairable &&
            bank_dictionary_overflow == historical_dictionary_overflow;
        retained_fault_count_o = bank_fault_count;
        retained_hybrid_count_o = bank_state[103 +: 4];
        retained_reuse_count_o = bank_state[261 +: 4];
        projected_count_o = projected_count;
        membership = 0;
        for (entry = 0; entry < 14; entry = entry + 1) begin
            if (historical_hybrid_valid[entry] && historical_hybrid_cfg[entry * 7 + config_id_i])
                membership = membership + 1;
        end
        membership_count_o = membership[3:0];
    end
endmodule

`default_nettype wire
