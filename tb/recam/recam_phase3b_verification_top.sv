`default_nettype none

// Verification-only wrapper: presents one logical-state vector to the Phase
// 3B CFG0 analyzer and reconstructs the equivalent full Hybrid tuples expected
// by the Phase 3A 2R2C reference RTL.
module recam_phase3b_verification_top #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 7
) (
    input wire [2:0] config_id_i,
    input wire [4:0] pivot_valid_i,
    input wire [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input wire [5*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input wire [4:0] row_gt1_i,
    input wire [4:0] row_gt2_i,
    input wire [4:0] row_gt3_i,
    input wire [4:0] col_gt1_i,
    input wire [4:0] col_gt2_i,
    input wire [4:0] col_gt3_i,
    input wire [HYBRID_ENTRIES-1:0] hybrid_valid_i,
    input wire [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input wire [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,
    input wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,
    input wire conventional_overflow_i,
    output wire [9:0] shared_candidate_valid_o,
    output wire [3:0] shared_pattern_id_o,
    output wire shared_solution_valid_o,
    output wire shared_repairable_o,
    output wire shared_dictionary_overflow_o,
    output wire [5:0] phase3a_candidate_valid_o,
    output wire [3:0] phase3a_pattern_id_o,
    output wire phase3a_repairable_o
);
    reg [HYBRID_ENTRIES*ROW_ADDR_W-1:0] phase3a_hybrid_rows;
    reg [HYBRID_ENTRIES*COL_ADDR_W-1:0] phase3a_hybrid_cols;
    reg [2:0] pointer;
    wire [15:0] phase3a_matrix_unused;
    integer hybrid;

    always @* begin
        phase3a_hybrid_rows = {HYBRID_ENTRIES*ROW_ADDR_W{1'b0}};
        phase3a_hybrid_cols = {HYBRID_ENTRIES*COL_ADDR_W{1'b0}};
        pointer = 3'd0;
        for (hybrid = 0; hybrid < HYBRID_ENTRIES; hybrid = hybrid + 1) begin
            pointer = hybrid_pointer_flat_i[hybrid*3 +: 3];
            if (pointer < 3'd4) begin
                if (hybrid_descriptor_i[hybrid]) begin
                    phase3a_hybrid_rows[hybrid*ROW_ADDR_W +: ROW_ADDR_W] =
                        hybrid_differing_flat_i[hybrid*DIFF_ADDR_W +: ROW_ADDR_W];
                    phase3a_hybrid_cols[hybrid*COL_ADDR_W +: COL_ADDR_W] =
                        pivot_cols_flat_i[pointer*COL_ADDR_W +: COL_ADDR_W];
                end else begin
                    phase3a_hybrid_rows[hybrid*ROW_ADDR_W +: ROW_ADDR_W] =
                        pivot_rows_flat_i[pointer*ROW_ADDR_W +: ROW_ADDR_W];
                    phase3a_hybrid_cols[hybrid*COL_ADDR_W +: COL_ADDR_W] =
                        hybrid_differing_flat_i[hybrid*DIFF_ADDR_W +: COL_ADDR_W];
                end
            end
        end
    end

    recam_shared_config_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W), .COL_ADDR_W(COL_ADDR_W), .DIFF_ADDR_W(DIFF_ADDR_W),
        .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) shared (
        .config_id_i(config_id_i), .pivot_valid_i(pivot_valid_i),
        .pivot_rows_flat_i(pivot_rows_flat_i), .pivot_cols_flat_i(pivot_cols_flat_i),
        .row_gt1_i(row_gt1_i), .row_gt2_i(row_gt2_i), .row_gt3_i(row_gt3_i),
        .col_gt1_i(col_gt1_i), .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i),
        .hybrid_valid_i(hybrid_valid_i), .hybrid_pointer_flat_i(hybrid_pointer_flat_i),
        .hybrid_descriptor_i(hybrid_descriptor_i),
        .hybrid_differing_flat_i(hybrid_differing_flat_i),
        .conventional_overflow_i(conventional_overflow_i),
        .candidate_valid_o(shared_candidate_valid_o), .pattern_id_o(shared_pattern_id_o),
        .solution_valid_o(shared_solution_valid_o), .repairable_o(shared_repairable_o),
        .dictionary_overflow_o(shared_dictionary_overflow_o)
    );

    recam_2r2c_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W), .COL_ADDR_W(COL_ADDR_W),
        .HYBRID_ENTRY_NUM(HYBRID_ENTRIES)
    ) phase3a (
        .pivot_valid_i(pivot_valid_i[3:0]),
        .pivot_rows_flat_i(pivot_rows_flat_i[4*ROW_ADDR_W-1:0]),
        .pivot_cols_flat_i(pivot_cols_flat_i[4*COL_ADDR_W-1:0]),
        .row_must_i(row_gt2_i[3:0]), .col_must_i(col_gt2_i[3:0]),
        .hybrid_valid_i(hybrid_valid_i),
        .hybrid_rows_flat_i(phase3a_hybrid_rows),
        .hybrid_cols_flat_i(phase3a_hybrid_cols),
        .cam_overflow_i(conventional_overflow_i), .matrix_flat_o(phase3a_matrix_unused),
        .candidate_valid_o(phase3a_candidate_valid_o),
        .repairable_o(phase3a_repairable_o), .pattern_id_o(phase3a_pattern_id_o)
    );
endmodule

`default_nettype wire
