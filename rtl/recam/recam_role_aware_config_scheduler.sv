`default_nettype none

// Phase 3C wrapper around the frozen Phase 3B combinational analyzer.
// sa_role_i=0 selects the A/D physical role; sa_role_i=1 selects B/C.
// The logical analyzer input state is owned upstream and must remain stable
// from the start-accept edge through the fourth result-capture edge.
module recam_role_aware_config_scheduler #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 7
) (
    input  wire clk_i,
    input  wire rst_ni,
    input  wire start_i,
    input  wire sa_role_i,
    input  wire [4:0] pivot_valid_i,
    input  wire [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input  wire [5*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input  wire [4:0] row_gt1_i, row_gt2_i, row_gt3_i,
    input  wire [4:0] col_gt1_i, col_gt2_i, col_gt3_i,
    input  wire [HYBRID_ENTRIES-1:0] hybrid_valid_i,
    input  wire [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input  wire [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,
    input  wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,
    input  wire conventional_overflow_i,
    output reg  busy_o,
    output reg  done_o,
    output wire [2:0] current_config_id_o,
    // Four 20-bit entries, least-significant entry first:
    // {candidate_valid[9:0], pattern_id[3:0], repairable, solution_valid,
    //  config_id[2:0], valid}.
    output reg  [4*20-1:0] config_pattern_map_o,
    // Combinational Phase 3B result for the currently presented ConfigID.
    output wire [9:0] current_candidate_valid_o,
    output wire [3:0] current_pattern_id_o,
    output wire current_solution_valid_o,
    output wire current_repairable_o
);
    reg sa_role_reg;
    reg [1:0] scan_index_reg;
    reg [2:0] active_config_id;
    wire dictionary_overflow_unused;

    always @* begin
        case ({sa_role_reg, scan_index_reg})
            // A/D: CFG0, CFG4, CFG5, CFG6.
            3'b0_00: active_config_id = 3'd0;
            3'b0_01: active_config_id = 3'd4;
            3'b0_10: active_config_id = 3'd5;
            3'b0_11: active_config_id = 3'd6;
            // B/C: CFG0, CFG2, CFG1, CFG3.
            3'b1_00: active_config_id = 3'd0;
            3'b1_01: active_config_id = 3'd2;
            3'b1_10: active_config_id = 3'd1;
            default: active_config_id = 3'd3;
        endcase
    end

    assign current_config_id_o = active_config_id;

    recam_shared_config_analyzer #(
        .ROW_ADDR_W(ROW_ADDR_W), .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W), .HYBRID_ENTRIES(HYBRID_ENTRIES)
    ) shared_analyzer (
        .config_id_i(active_config_id), .pivot_valid_i(pivot_valid_i),
        .pivot_rows_flat_i(pivot_rows_flat_i), .pivot_cols_flat_i(pivot_cols_flat_i),
        .row_gt1_i(row_gt1_i), .row_gt2_i(row_gt2_i), .row_gt3_i(row_gt3_i),
        .col_gt1_i(col_gt1_i), .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i),
        .hybrid_valid_i(hybrid_valid_i), .hybrid_pointer_flat_i(hybrid_pointer_flat_i),
        .hybrid_descriptor_i(hybrid_descriptor_i),
        .hybrid_differing_flat_i(hybrid_differing_flat_i),
        .conventional_overflow_i(conventional_overflow_i),
        .candidate_valid_o(current_candidate_valid_o), .pattern_id_o(current_pattern_id_o),
        .solution_valid_o(current_solution_valid_o), .repairable_o(current_repairable_o),
        .dictionary_overflow_o(dictionary_overflow_unused)
    );

    always @(posedge clk_i) begin
        done_o <= 1'b0;
        if (!rst_ni) begin
            busy_o <= 1'b0;
            sa_role_reg <= 1'b0;
            scan_index_reg <= 2'd0;
            config_pattern_map_o <= 80'd0;
        end else if (start_i && !busy_o) begin
            // No CAM snapshot is taken: the upstream state-stability contract
            // applies for the four following analyzer evaluation intervals.
            busy_o <= 1'b1;
            sa_role_reg <= sa_role_i;
            scan_index_reg <= 2'd0;
            config_pattern_map_o <= 80'd0;
        end else if (busy_o) begin
            config_pattern_map_o[scan_index_reg*20 +: 20] <= {
                current_candidate_valid_o, current_pattern_id_o,
                current_repairable_o, current_solution_valid_o,
                active_config_id, 1'b1
            };
            if (scan_index_reg == 2'd3) begin
                busy_o <= 1'b0;
                done_o <= 1'b1;
            end else begin
                scan_index_reg <= scan_index_reg + 2'd1;
            end
        end
    end
endmodule
