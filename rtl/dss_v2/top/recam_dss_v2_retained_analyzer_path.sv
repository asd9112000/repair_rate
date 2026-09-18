`default_nettype none

// Isolated S1G-A2G retained collector -> frozen 232-bit view -> nine-slot
// analyzer path.  It intentionally owns no ledger or A-to-D scheduler state.
module recam_dss_v2_retained_analyzer_path #(
    parameter int unsigned GENERATION_W = 4
) (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic clear_i,
    input  logic fault_valid_i,
    input  logic [9:0] fault_row_i,
    input  logic [9:0] fault_col_i,
    input  logic [2:0] config_id_i,
    input  logic analysis_complete_i,
    input  logic [GENERATION_W-1:0] analysis_generation_i,
    output logic fault_ready_o,
    output logic [820:0] retained_state_o,
    output logic [GENERATION_W-1:0] fault_generation_o,
    output logic analysis_valid_o,
    output logic [GENERATION_W-1:0] analysis_generation_o,
    output logic [231:0] analyzer_view_o,
    output logic [9:0] candidate_valid_o,
    output logic [3:0] pattern_id_o,
    output logic solution_valid_o,
    output logic repairable_o,
    output logic dictionary_overflow_o
);
    logic [4:0] unused_pivot_valid;
    logic [49:0] unused_pivot_rows;
    logic [49:0] unused_pivot_cols;
    logic [13:0] unused_hybrid_valid;
    logic [139:0] unused_hybrid_rows;
    logic [139:0] unused_hybrid_cols;
    logic [41:0] unused_hybrid_pointer;
    logic [13:0] unused_hybrid_descriptor;
    logic [97:0] unused_hybrid_cfg;
    logic [11:0] unused_reuse_valid;
    logic [119:0] unused_reuse_rows;
    logic [119:0] unused_reuse_cols;
    logic [83:0] unused_reuse_cfg;
    logic [34:0] unused_row_must;
    logic [34:0] unused_col_must;
    logic [3:0] unused_fault_count;
    logic [8:0] projected_hybrid_valid;
    logic [26:0] projected_hybrid_pointer;
    logic [8:0] projected_hybrid_descriptor;
    logic [80:0] projected_hybrid_differing;
    integer projected_index;

    always_comb begin
        projected_hybrid_valid = '0;
        projected_hybrid_pointer = '0;
        projected_hybrid_descriptor = '0;
        projected_hybrid_differing = '0;
        for (projected_index = 0; projected_index < 9; projected_index = projected_index + 1) begin
            projected_hybrid_valid[projected_index] = analyzer_view_o[106 + projected_index * 14];
            projected_hybrid_pointer[projected_index * 3 +: 3] = analyzer_view_o[107 + projected_index * 14 +: 3];
            projected_hybrid_descriptor[projected_index] = analyzer_view_o[110 + projected_index * 14];
            projected_hybrid_differing[projected_index * 9 +: 9] = analyzer_view_o[111 + projected_index * 14 +: 9];
        end
    end

    recam_dss_v2_retained_collector_bank #(.GENERATION_W(GENERATION_W)) collector (
        .clk_i(clk_i), .rst_ni(rst_ni), .clear_i(clear_i),
        .fault_valid_i(fault_valid_i), .fault_row_i(fault_row_i),
        .fault_col_i(fault_col_i), .config_id_i(config_id_i),
        .analysis_complete_i(analysis_complete_i),
        .analysis_generation_i(analysis_generation_i), .fault_ready_o(fault_ready_o),
        .retained_state_o(retained_state_o), .fault_generation_o(fault_generation_o),
        .analysis_valid_o(analysis_valid_o), .analysis_generation_o(analysis_generation_o),
        .pivot_valid_o(unused_pivot_valid), .pivot_rows_flat_o(unused_pivot_rows),
        .pivot_cols_flat_o(unused_pivot_cols), .hybrid_valid_o(unused_hybrid_valid),
        .hybrid_rows_flat_o(unused_hybrid_rows), .hybrid_cols_flat_o(unused_hybrid_cols),
        .hybrid_pointer_flat_o(unused_hybrid_pointer), .hybrid_descriptor_o(unused_hybrid_descriptor),
        .hybrid_cfg_valid_flat_o(unused_hybrid_cfg), .reuse_valid_o(unused_reuse_valid),
        .reuse_rows_flat_o(unused_reuse_rows), .reuse_cols_flat_o(unused_reuse_cols),
        .reuse_cfg_valid_flat_o(unused_reuse_cfg), .row_must_by_cfg_o(unused_row_must),
        .col_must_by_cfg_o(unused_col_must), .fault_count_o(unused_fault_count),
        .analyzer_view_o(analyzer_view_o)
    );

    recam_shared_config_analyzer #(.HYBRID_ENTRIES(9)) analyzer (
        .config_id_i(config_id_i), .pivot_valid_i(analyzer_view_o[4:0]),
        .pivot_rows_flat_i(analyzer_view_o[49:5]), .pivot_cols_flat_i(analyzer_view_o[74:50]),
        .row_gt1_i(analyzer_view_o[79:75]), .row_gt2_i(analyzer_view_o[84:80]),
        .row_gt3_i(analyzer_view_o[89:85]), .col_gt1_i(analyzer_view_o[94:90]),
        .col_gt2_i(analyzer_view_o[99:95]), .col_gt3_i(analyzer_view_o[104:100]),
        .hybrid_valid_i(projected_hybrid_valid),
        .hybrid_pointer_flat_i(projected_hybrid_pointer),
        .hybrid_descriptor_i(projected_hybrid_descriptor),
        .hybrid_differing_flat_i(projected_hybrid_differing),
        .conventional_overflow_i(analyzer_view_o[105]), .candidate_valid_o(candidate_valid_o),
        .pattern_id_o(pattern_id_o), .solution_valid_o(solution_valid_o),
        .repairable_o(repairable_o), .dictionary_overflow_o(dictionary_overflow_o)
    );
endmodule

`default_nettype wire
