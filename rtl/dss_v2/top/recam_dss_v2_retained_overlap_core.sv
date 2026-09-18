`default_nettype none

// Isolated S1G-A2G overlap controller.  Unlike the earlier 204-bit-summary
// experiment, its only collector input is an accepted historical fault.  Each
// SA owns one coherent 821-bit retained collector bank; this controller owns
// the one shared analyzer schedule and the final ledger commit boundary.
module recam_dss_v2_retained_overlap_core #(
    parameter int unsigned GENERATION_W = 4
) (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic start_i,
    input  logic fault_valid_i,
    input  logic [1:0] fault_sa_i,
    input  logic [9:0] fault_row_i,
    input  logic [9:0] fault_col_i,
    input  logic test_done_valid_i,
    input  logic [1:0] test_done_sa_i,

    output logic fault_ready_o,
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

    // Verification observability; none of these signals affects a decision.
    output logic [1:0] analyzer_owner_sa_o,
    output logic [3:0] analysis_valid_o,
    output logic [4*GENERATION_W-1:0] fault_generation_flat_o,
    output logic [4*GENERATION_W-1:0] analysis_generation_flat_o,
    output logic [3:0] retained_update_event_o,
    output logic [3:0] latest_candidate_ready_o,
    output logic [3:0] test_done_event_o,
    output logic final_ledger_decision_ready_o,
    output logic [4*821-1:0] retained_state_flat_o
);
    import dss_v2_types_pkg::*;

    localparam int unsigned SA_NUM = 4;
    localparam int unsigned RETAINED_COLLECTOR_BITS_PER_SA = 821;
    localparam int unsigned RETAINED_COLLECTOR_BITS_4SA = 3284;

    typedef enum logic [1:0] {
        STATE_IDLE,
        STATE_SWEEP,
        STATE_WAIT_TEST_DONE,
        STATE_FINAL_SELECT
    } state_t;

    state_t state_q;
    logic [1:0] owner_q;
    logic [1:0] slot_q;
    logic [3:0] test_done_q;
    logic [3:0] candidate_valid_q;
    logic [15:0] candidate_pattern_q;
    logic [3:0] bank_fault_ready;
    logic [3:0] bank_analysis_valid;
    logic [GENERATION_W-1:0] bank_fault_generation [0:SA_NUM-1];
    logic [GENERATION_W-1:0] bank_analysis_generation [0:SA_NUM-1];
    logic [820:0] bank_retained_state [0:SA_NUM-1];
    logic [9:0] bank_candidate_valid [0:SA_NUM-1];
    logic [3:0] bank_pattern_id [0:SA_NUM-1];
    logic bank_solution_valid [0:SA_NUM-1];
    logic bank_repairable [0:SA_NUM-1];
    logic bank_dictionary_overflow [0:SA_NUM-1];
    logic [2:0] current_config;
    logic owner_fault_update;
    logic analyzer_complete;
    logic selected_solution_valid;
    logic selected_repairable;
    logic [9:0] selected_candidate_bitmap;
    logic [3:0] selected_pattern_id;
    logic selected_dictionary_overflow;

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
    logic [3:0] canonical_released;
    logic [3:0] canonical_borrowed;
    logic [7:0] canonical_borrower_id_flat;
    logic commit_now;

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

    assign current_config = role_config_id(owner_q, slot_q);
    assign owner_fault_update = fault_valid_i && busy_o && fault_sa_i == owner_q &&
                                bank_fault_ready[owner_q];
    // Completion is sampled by the selected bank only after its fourth role
    // has been evaluated.  An accepted fault to that SA suppresses it, so the
    // bank's update-wins rule and the controller's restart are identical.
    assign analyzer_complete = state_q == STATE_SWEEP && slot_q == 2'd3 &&
                               !owner_fault_update;
    assign selected_solution_valid = bank_solution_valid[owner_q];
    assign selected_repairable = bank_repairable[owner_q];
    assign selected_candidate_bitmap = bank_candidate_valid[owner_q];
    assign selected_pattern_id = bank_pattern_id[owner_q];
    assign selected_dictionary_overflow = bank_dictionary_overflow[owner_q];
    assign fault_ready_o = busy_o && bank_fault_ready[fault_sa_i];
    assign analyzer_owner_sa_o = owner_q;
    assign analysis_valid_o = bank_analysis_valid;

    /* verilator lint_off PINCONNECTEMPTY */
    genvar sa;
    generate
        for (sa = 0; sa < SA_NUM; sa = sa + 1) begin : gen_sa_bank
            recam_dss_v2_retained_analyzer_path #(.GENERATION_W(GENERATION_W)) path (
                .clk_i(clk_i), .rst_ni(rst_ni),
                .clear_i(start_i && state_q == STATE_IDLE),
                .fault_valid_i(fault_valid_i && busy_o && fault_sa_i == sa),
                .fault_row_i(fault_row_i), .fault_col_i(fault_col_i),
                .config_id_i(current_config),
                .analysis_complete_i(analyzer_complete && owner_q == sa),
                .analysis_generation_i(bank_fault_generation[sa]),
                .fault_ready_o(bank_fault_ready[sa]),
                .retained_state_o(bank_retained_state[sa]),
                .fault_generation_o(bank_fault_generation[sa]),
                .analysis_valid_o(bank_analysis_valid[sa]),
                .analysis_generation_o(bank_analysis_generation[sa]),
                .analyzer_view_o(), .candidate_valid_o(bank_candidate_valid[sa]),
                .pattern_id_o(bank_pattern_id[sa]), .solution_valid_o(bank_solution_valid[sa]),
                .repairable_o(bank_repairable[sa]),
                .dictionary_overflow_o(bank_dictionary_overflow[sa])
            );
            assign fault_generation_flat_o[sa * GENERATION_W +: GENERATION_W] =
                bank_fault_generation[sa];
            assign analysis_generation_flat_o[sa * GENERATION_W +: GENERATION_W] =
                bank_analysis_generation[sa];
            assign retained_state_flat_o[sa * RETAINED_COLLECTOR_BITS_PER_SA +:
                                         RETAINED_COLLECTOR_BITS_PER_SA] = bank_retained_state[sa];
        end
    endgenerate

    dss_legacy_config_adapter descriptor_adapter (
        .legacy_config_id_i(current_config), .config_descriptor_o(current_descriptor),
        .legacy_config_valid_o(descriptor_valid), .config_descriptor_i('0),
        .legacy_config_id_o(), .descriptor_valid_o()
    );

    dss_v2_resource_feasibility feasibility (
        .sa_id_i(owner_q), .sa_valid_i(1'b1),
        .config_descriptor_i(current_descriptor), .resource_released_i(canonical_released),
        .resource_borrowed_i(canonical_borrowed), .resource_state_valid_o(resource_state_valid),
        .descriptor_legal_o(), .physical_feasible_o(resource_feasible),
        .borrow_required_o(borrow_required), .selected_donor_valid_o(donor_valid),
        .selected_donor_resource_o(donor_resource), .release_required_o(release_required),
        .release_resource_valid_o(release_resource_valid), .release_resource_o(release_resource)
    );

    assign commit_now = state_q == STATE_FINAL_SELECT && !owner_fault_update &&
                        test_done_q[owner_q] && bank_analysis_valid[owner_q] &&
                        bank_analysis_generation[owner_q] == bank_fault_generation[owner_q] &&
                        candidate_valid_q[slot_q] && descriptor_valid &&
                        resource_state_valid && resource_feasible;

    dss_v2_resource_ledger ledger (
        .clk_i(clk_i), .rst_ni(rst_ni), .commit_i(commit_now),
        .transaction_valid_i(commit_now), .requester_sa_i(owner_q),
        .release_required_i(release_required), .release_resource_valid_i(release_resource_valid),
        .release_resource_id_i(release_resource), .borrow_required_i(borrow_required),
        .selected_donor_valid_i(donor_valid), .selected_donor_resource_id_i(donor_resource),
        .resource_released_o(canonical_released), .resource_borrowed_o(canonical_borrowed),
        .borrower_valid_o(), .borrower_id_flat_o(canonical_borrower_id_flat),
        .commit_accepted_o(), .commit_error_o()
    );

    dss_v2_legacy_ledger_diagnostic_adapter ledger_adapter (
        .resource_released_i(canonical_released), .borrower_valid_i(canonical_borrowed),
        .borrower_id_flat_i(canonical_borrower_id_flat),
        .legacy_ledger_o(ledger_released_borrower_o), .canonical_state_valid_o()
    );
    /* verilator lint_on PINCONNECTEMPTY */

    always_ff @(posedge clk_i) begin
        done_o <= 1'b0;
        retained_update_event_o <= '0;
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
            test_done_q <= '0;
            candidate_valid_q <= '0;
            candidate_pattern_q <= '0;
        end else if (start_i && state_q == STATE_IDLE) begin
            state_q <= STATE_SWEEP;
            owner_q <= '0;
            slot_q <= '0;
            busy_o <= 1'b1;
            group_repairable_o <= 1'b0;
            sa_commit_valid_o <= '0;
            selected_config_flat_o <= '0;
            selected_pattern_flat_o <= '0;
            selected_donor_flat_o <= '0;
            borrow_flat_o <= '0;
            release_flat_o <= '0;
            failure_position_o <= 2'd3;
            test_done_q <= '0;
            candidate_valid_q <= '0;
            candidate_pattern_q <= '0;
        end else begin
            if (fault_valid_i && busy_o && bank_fault_ready[fault_sa_i]) begin
                retained_update_event_o[fault_sa_i] <= 1'b1;
                if (fault_sa_i == owner_q) begin
                    candidate_valid_q <= '0;
                    candidate_pattern_q <= '0;
                end
            end
            if (test_done_valid_i) begin
                test_done_q[test_done_sa_i] <= 1'b1;
                test_done_event_o[test_done_sa_i] <= 1'b1;
            end
            case (state_q)
                STATE_IDLE: busy_o <= 1'b0;
                STATE_SWEEP: begin
                    if (owner_fault_update) begin
                        slot_q <= '0;
                    end else begin
                        candidate_valid_q[slot_q] <= selected_solution_valid &&
                                                     selected_repairable &&
                                                     (|selected_candidate_bitmap) &&
                                                     !selected_dictionary_overflow;
                        candidate_pattern_q[slot_q * 4 +: 4] <= selected_pattern_id;
                        if (slot_q == 2'd3) begin
                            latest_candidate_ready_o[owner_q] <= 1'b1;
                            slot_q <= '0;
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
                    if (owner_fault_update) begin
                        slot_q <= '0;
                        state_q <= STATE_SWEEP;
                    end else if (test_done_q[owner_q] && bank_analysis_valid[owner_q] &&
                                 bank_analysis_generation[owner_q] == bank_fault_generation[owner_q]) begin
                        slot_q <= '0;
                        state_q <= STATE_FINAL_SELECT;
                    end
                end
                STATE_FINAL_SELECT: begin
                    if (owner_fault_update) begin
                        slot_q <= '0;
                        state_q <= STATE_SWEEP;
                    end else if (commit_now) begin
                        final_ledger_decision_ready_o <= 1'b1;
                        sa_commit_valid_o[owner_q] <= 1'b1;
                        selected_config_flat_o[owner_q * 3 +: 3] <= current_config;
                        selected_pattern_flat_o[owner_q * 4 +: 4] <= candidate_pattern_q[slot_q * 4 +: 4];
                        selected_donor_flat_o[owner_q * 2 +: 2] <= donor_resource;
                        borrow_flat_o[owner_q] <= borrow_required;
                        release_flat_o[owner_q] <= release_required;
                        if (owner_q == 2'd3) begin
                            state_q <= STATE_IDLE;
                            busy_o <= 1'b0;
                            done_o <= 1'b1;
                            group_repairable_o <= 1'b1;
                        end else begin
                            owner_q <= owner_q + 1'b1;
                            slot_q <= '0;
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
        if (RETAINED_COLLECTOR_BITS_PER_SA != 821 ||
            RETAINED_COLLECTOR_BITS_4SA != 3284 ||
            $bits(retained_state_flat_o) != RETAINED_COLLECTOR_BITS_4SA)
            $fatal(1, "S1G-A2G retained overlap state-width drift");
    end
endmodule

`default_nettype wire
