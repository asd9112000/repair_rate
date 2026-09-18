`default_nettype none

module recam_dss_v2_early_overlap_core #(
    parameter int unsigned ROW_ADDR_W = 9,
    parameter int unsigned COL_ADDR_W = 5,
    parameter int unsigned DIFF_ADDR_W = 9,
    parameter int unsigned HYBRID_ENTRIES = 7,
    parameter int unsigned GENERATION_W = 4
) (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic start_i,

    // S1G-A verification-facing summary producer boundary.  It has no raw-fault
    // interface; one update atomically replaces one SA's retained summary.
    input  logic summary_update_valid_i,
    input  logic [1:0] summary_update_sa_i,
    input  logic [203:0] summary_payload_i,
    input  logic test_done_valid_i,
    input  logic [1:0] test_done_sa_i,

    output logic busy_o,
    output logic done_o,
    output logic group_repairable_o,
    output logic [3:0] sa_commit_valid_o,
    output logic [11:0] ledger_released_borrower_o,
    output logic [11:0] selected_config_flat_o,
    output logic [15:0] selected_pattern_flat_o,
    output logic [7:0] selected_donor_flat_o,
    output logic [3:0] borrow_flat_o,
    output logic [3:0] release_flat_o,
    output logic [1:0] failure_position_o,

    // Verification/debug events.  They do not alter repair semantics.
    output logic [3:0] summary_update_event_o,
    output logic [3:0] latest_candidate_ready_o,
    output logic [3:0] test_done_event_o,
    output logic final_ledger_decision_ready_o,
    output logic [1:0] analyzer_owner_sa_o,
    output logic [3:0] analysis_valid_o,
    output logic [4*GENERATION_W-1:0] fault_generation_flat_o,
    output logic [4*GENERATION_W-1:0] analysis_generation_flat_o,
    output logic [3:0] canonical_released_o,
    output logic [3:0] canonical_borrowed_o,
    output logic [7:0] canonical_borrower_id_flat_o
);
    import dss_v2_types_pkg::*;

    localparam int unsigned SA_NUM = 4;
    localparam int unsigned SUMMARY_W =
        5 + 5 * ROW_ADDR_W + 5 * COL_ADDR_W + 6 * 5 +
        HYBRID_ENTRIES + HYBRID_ENTRIES * 3 + HYBRID_ENTRIES +
        HYBRID_ENTRIES * DIFF_ADDR_W + 1;

    localparam int unsigned PIVOT_VALID_LSB = 0;
    localparam int unsigned PIVOT_ROWS_LSB = PIVOT_VALID_LSB + 5;
    localparam int unsigned PIVOT_COLS_LSB = PIVOT_ROWS_LSB + 5 * ROW_ADDR_W;
    localparam int unsigned ROW_GT_LSB = PIVOT_COLS_LSB + 5 * COL_ADDR_W;
    localparam int unsigned COL_GT_LSB = ROW_GT_LSB + 3 * 5;
    localparam int unsigned HYBRID_VALID_LSB = COL_GT_LSB + 3 * 5;
    localparam int unsigned HYBRID_POINTER_LSB =
        HYBRID_VALID_LSB + HYBRID_ENTRIES;
    localparam int unsigned HYBRID_DESCRIPTOR_LSB =
        HYBRID_POINTER_LSB + HYBRID_ENTRIES * 3;
    localparam int unsigned HYBRID_DIFFERING_LSB =
        HYBRID_DESCRIPTOR_LSB + HYBRID_ENTRIES;
    localparam int unsigned OVERFLOW_LSB =
        HYBRID_DIFFERING_LSB + HYBRID_ENTRIES * DIFF_ADDR_W;

    typedef enum logic [2:0] {
        STATE_IDLE,
        STATE_SWEEP,
        STATE_WAIT_TEST_DONE,
        STATE_FINAL_SELECT
    } state_t;

    state_t state_q;
    logic [1:0] owner_q;
    logic [1:0] slot_q;
    logic [SUMMARY_W-1:0] summary_q [0:SA_NUM-1];
    logic [GENERATION_W-1:0] fault_generation_q [0:SA_NUM-1];
    logic [GENERATION_W-1:0] analysis_generation_q [0:SA_NUM-1];
    logic [3:0] analysis_valid_q;
    logic [3:0] test_done_q;
    logic [3:0] candidate_valid_q;
    logic [15:0] candidate_pattern_q;

    logic [SUMMARY_W-1:0] owner_summary;
    logic [2:0] current_config;
    logic [9:0] analyzer_candidate_bitmap;
    logic [3:0] analyzer_pattern_id;
    logic analyzer_solution_valid;
    logic analyzer_repairable;
    logic analyzer_dictionary_overflow;

    dss_v2_config_descriptor_t current_descriptor;
    logic descriptor_valid;
    logic resource_state_valid;
    logic resource_feasible;
    logic borrow_required;
    logic donor_valid;
    dss_v2_resource_id_t donor_resource;
    logic release_required;
    logic release_resource_valid;
    dss_v2_resource_id_t release_resource;
    logic commit_now;
    logic owner_summary_update;

    function automatic logic [2:0] role_config_id(
        input logic [1:0] sa_id,
        input logic [1:0] slot_id
    );
        begin
            if (sa_id == 2'd0 || sa_id == 2'd3) begin
                case (slot_id)
                    2'd0: role_config_id = 3'd0;
                    2'd1: role_config_id = 3'd4;
                    2'd2: role_config_id = 3'd5;
                    default: role_config_id = 3'd6;
                endcase
            end else begin
                case (slot_id)
                    2'd0: role_config_id = 3'd0;
                    2'd1: role_config_id = 3'd2;
                    2'd2: role_config_id = 3'd1;
                    default: role_config_id = 3'd3;
                endcase
            end
        end
    endfunction

    assign owner_summary = summary_q[owner_q];
    assign current_config = role_config_id(owner_q, slot_q);
    assign owner_summary_update =
        summary_update_valid_i && summary_update_sa_i == owner_q;

    /* verilator lint_off PINCONNECTEMPTY */
    recam_shared_config_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W),
        .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) analyzer (
        .config_id_i(current_config),
        .pivot_valid_i(owner_summary[PIVOT_VALID_LSB +: 5]),
        .pivot_rows_flat_i(owner_summary[PIVOT_ROWS_LSB +: 5 * ROW_ADDR_W]),
        .pivot_cols_flat_i(owner_summary[PIVOT_COLS_LSB +: 5 * COL_ADDR_W]),
        .row_gt1_i(owner_summary[ROW_GT_LSB +: 5]),
        .row_gt2_i(owner_summary[ROW_GT_LSB + 5 +: 5]),
        .row_gt3_i(owner_summary[ROW_GT_LSB + 10 +: 5]),
        .col_gt1_i(owner_summary[COL_GT_LSB +: 5]),
        .col_gt2_i(owner_summary[COL_GT_LSB + 5 +: 5]),
        .col_gt3_i(owner_summary[COL_GT_LSB + 10 +: 5]),
        .hybrid_valid_i(
            owner_summary[HYBRID_VALID_LSB +: HYBRID_ENTRIES]),
        .hybrid_pointer_flat_i(
            owner_summary[HYBRID_POINTER_LSB +: HYBRID_ENTRIES * 3]),
        .hybrid_descriptor_i(
            owner_summary[HYBRID_DESCRIPTOR_LSB +: HYBRID_ENTRIES]),
        .hybrid_differing_flat_i(
            owner_summary[HYBRID_DIFFERING_LSB +:
                          HYBRID_ENTRIES * DIFF_ADDR_W]),
        .conventional_overflow_i(owner_summary[OVERFLOW_LSB]),
        .candidate_valid_o(analyzer_candidate_bitmap),
        .pattern_id_o(analyzer_pattern_id),
        .solution_valid_o(analyzer_solution_valid),
        .repairable_o(analyzer_repairable),
        .dictionary_overflow_o(analyzer_dictionary_overflow)
    );

    dss_legacy_config_adapter descriptor_adapter (
        .legacy_config_id_i(current_config),
        .config_descriptor_o(current_descriptor),
        .legacy_config_valid_o(descriptor_valid),
        .config_descriptor_i('0),
        .legacy_config_id_o(),
        .descriptor_valid_o()
    );

    dss_v2_resource_feasibility feasibility (
        .sa_id_i(owner_q),
        .sa_valid_i(1'b1),
        .config_descriptor_i(current_descriptor),
        .resource_released_i(canonical_released_o),
        .resource_borrowed_i(canonical_borrowed_o),
        .resource_state_valid_o(resource_state_valid),
        .descriptor_legal_o(),
        .physical_feasible_o(resource_feasible),
        .borrow_required_o(borrow_required),
        .selected_donor_valid_o(donor_valid),
        .selected_donor_resource_o(donor_resource),
        .release_required_o(release_required),
        .release_resource_valid_o(release_resource_valid),
        .release_resource_o(release_resource)
    );

    assign commit_now =
        state_q == STATE_FINAL_SELECT &&
        !owner_summary_update &&
        test_done_q[owner_q] &&
        analysis_valid_q[owner_q] &&
        analysis_generation_q[owner_q] == fault_generation_q[owner_q] &&
        candidate_valid_q[slot_q] &&
        descriptor_valid &&
        resource_state_valid &&
        resource_feasible;

    dss_v2_resource_ledger ledger (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .commit_i(commit_now),
        .transaction_valid_i(commit_now),
        .requester_sa_i(owner_q),
        .release_required_i(release_required),
        .release_resource_valid_i(release_resource_valid),
        .release_resource_id_i(release_resource),
        .borrow_required_i(borrow_required),
        .selected_donor_valid_i(donor_valid),
        .selected_donor_resource_id_i(donor_resource),
        .resource_released_o(canonical_released_o),
        .resource_borrowed_o(canonical_borrowed_o),
        .borrower_valid_o(),
        .borrower_id_flat_o(canonical_borrower_id_flat_o),
        .commit_accepted_o(),
        .commit_error_o()
    );

    dss_v2_legacy_ledger_diagnostic_adapter ledger_adapter (
        .resource_released_i(canonical_released_o),
        .borrower_valid_i(canonical_borrowed_o),
        .borrower_id_flat_i(canonical_borrower_id_flat_o),
        .legacy_ledger_o(ledger_released_borrower_o),
        .canonical_state_valid_o()
    );
    /* verilator lint_on PINCONNECTEMPTY */

    genvar state_sa;
    generate
        for (state_sa = 0; state_sa < SA_NUM; state_sa = state_sa + 1) begin : gen_debug_flatten
            assign fault_generation_flat_o[state_sa * GENERATION_W +:
                                            GENERATION_W] =
                fault_generation_q[state_sa];
            assign analysis_generation_flat_o[state_sa * GENERATION_W +:
                                               GENERATION_W] =
                analysis_generation_q[state_sa];
        end
    endgenerate
    assign analysis_valid_o = analysis_valid_q;
    assign analyzer_owner_sa_o = owner_q;

    integer sa_index;
    always_ff @(posedge clk_i) begin
        done_o <= 1'b0;
        summary_update_event_o <= '0;
        latest_candidate_ready_o <= '0;
        test_done_event_o <= '0;
        final_ledger_decision_ready_o <= 1'b0;

        if (!rst_ni) begin
            state_q <= STATE_IDLE;
            owner_q <= '0;
            slot_q <= '0;
            busy_o <= 1'b0;
            group_repairable_o <= 1'b0;
            sa_commit_valid_o <= '0;
            selected_config_flat_o <= '0;
            selected_pattern_flat_o <= '0;
            selected_donor_flat_o <= '0;
            borrow_flat_o <= '0;
            release_flat_o <= '0;
            failure_position_o <= 2'd3;
            analysis_valid_q <= '0;
            test_done_q <= '0;
            candidate_valid_q <= '0;
            candidate_pattern_q <= '0;
            for (sa_index = 0; sa_index < SA_NUM; sa_index = sa_index + 1) begin
                summary_q[sa_index] <= '0;
                fault_generation_q[sa_index] <= '0;
                analysis_generation_q[sa_index] <= '0;
            end
        end else if (start_i && state_q == STATE_IDLE) begin
            state_q <= STATE_SWEEP;
            owner_q <= 2'd0;
            slot_q <= 2'd0;
            busy_o <= 1'b1;
            group_repairable_o <= 1'b0;
            sa_commit_valid_o <= '0;
            selected_config_flat_o <= '0;
            selected_pattern_flat_o <= '0;
            selected_donor_flat_o <= '0;
            borrow_flat_o <= '0;
            release_flat_o <= '0;
            failure_position_o <= 2'd3;
            analysis_valid_q <= '0;
            test_done_q <= '0;
            candidate_valid_q <= '0;
            candidate_pattern_q <= '0;
            for (sa_index = 0; sa_index < SA_NUM; sa_index = sa_index + 1) begin
                summary_q[sa_index] <= '0;
                fault_generation_q[sa_index] <= '0;
                analysis_generation_q[sa_index] <= '0;
            end
        end else begin
            if (summary_update_valid_i) begin
                summary_q[summary_update_sa_i] <= summary_payload_i;
                fault_generation_q[summary_update_sa_i] <=
                    fault_generation_q[summary_update_sa_i] + 1'b1;
                analysis_valid_q[summary_update_sa_i] <= 1'b0;
                summary_update_event_o[summary_update_sa_i] <= 1'b1;
            end
            if (test_done_valid_i) begin
                test_done_q[test_done_sa_i] <= 1'b1;
                test_done_event_o[test_done_sa_i] <= 1'b1;
            end

            case (state_q)
                STATE_IDLE: begin
                    busy_o <= 1'b0;
                end
                STATE_SWEEP: begin
                    if (owner_summary_update) begin
                        slot_q <= 2'd0;
                        candidate_valid_q <= '0;
                        candidate_pattern_q <= '0;
                    end else begin
                        candidate_valid_q[slot_q] <=
                            analyzer_solution_valid && analyzer_repairable &&
                            (|analyzer_candidate_bitmap) &&
                            !analyzer_dictionary_overflow;
                        candidate_pattern_q[slot_q * 4 +: 4] <=
                            analyzer_pattern_id;
                        if (slot_q == 2'd3) begin
                            analysis_valid_q[owner_q] <= 1'b1;
                            analysis_generation_q[owner_q] <=
                                fault_generation_q[owner_q];
                            latest_candidate_ready_o[owner_q] <= 1'b1;
                            slot_q <= 2'd0;
                            if (test_done_q[owner_q])
                                state_q <= STATE_FINAL_SELECT;
                            else
                                state_q <= STATE_WAIT_TEST_DONE;
                        end else begin
                            slot_q <= slot_q + 1'b1;
                        end
                    end
                end
                STATE_WAIT_TEST_DONE: begin
                    if (owner_summary_update) begin
                        state_q <= STATE_SWEEP;
                        slot_q <= 2'd0;
                        candidate_valid_q <= '0;
                        candidate_pattern_q <= '0;
                    end else if (test_done_q[owner_q] &&
                                 analysis_valid_q[owner_q] &&
                                 analysis_generation_q[owner_q] ==
                                     fault_generation_q[owner_q]) begin
                        slot_q <= 2'd0;
                        state_q <= STATE_FINAL_SELECT;
                    end
                end
                STATE_FINAL_SELECT: begin
                    if (owner_summary_update) begin
                        state_q <= STATE_SWEEP;
                        slot_q <= 2'd0;
                        candidate_valid_q <= '0;
                        candidate_pattern_q <= '0;
                    end else if (commit_now) begin
                        final_ledger_decision_ready_o <= 1'b1;
                        sa_commit_valid_o[owner_q] <= 1'b1;
                        selected_config_flat_o[owner_q * 3 +: 3] <=
                            current_config;
                        selected_pattern_flat_o[owner_q * 4 +: 4] <=
                            candidate_pattern_q[slot_q * 4 +: 4];
                        selected_donor_flat_o[owner_q * 2 +: 2] <=
                            donor_resource;
                        borrow_flat_o[owner_q] <= borrow_required;
                        release_flat_o[owner_q] <= release_required;
                        if (owner_q == 2'd3) begin
                            state_q <= STATE_IDLE;
                            busy_o <= 1'b0;
                            done_o <= 1'b1;
                            group_repairable_o <= 1'b1;
                        end else begin
                            owner_q <= owner_q + 1'b1;
                            slot_q <= 2'd0;
                            candidate_valid_q <= '0;
                            candidate_pattern_q <= '0;
                            state_q <= STATE_SWEEP;
                        end
                    end else if (slot_q == 2'd3) begin
                        state_q <= STATE_IDLE;
                        busy_o <= 1'b0;
                        done_o <= 1'b1;
                        group_repairable_o <= 1'b0;
                        failure_position_o <= owner_q;
                    end else begin
                        slot_q <= slot_q + 1'b1;
                    end
                end
                default: begin
                    state_q <= STATE_IDLE;
                    busy_o <= 1'b0;
                end
            endcase
        end
    end

    initial begin
        if (SUMMARY_W != 204)
            $fatal(1, "S1G-A summary packing must equal 204 bits");
    end
endmodule

`default_nettype wire
