`default_nettype none

// Phase 3C control-only measurement boundary.  Analyzer result inputs are
// captured exactly as the integrated scheduler captures its one Phase 3B
// instance's combinational outputs.
module recam_role_aware_config_control (
    input wire clk_i, input wire rst_ni, input wire start_i, input wire sa_role_i,
    input wire [9:0] analyzer_candidate_valid_i,
    input wire [3:0] analyzer_pattern_id_i,
    input wire analyzer_solution_valid_i, input wire analyzer_repairable_i,
    output reg busy_o, output reg done_o, output wire [2:0] current_config_id_o,
    output reg [79:0] config_pattern_map_o
);
    reg sa_role_reg;
    reg [1:0] scan_index_reg;
    reg [2:0] active_config_id;

    always @* begin
        case ({sa_role_reg, scan_index_reg})
            3'b0_00: active_config_id = 3'd0;
            3'b0_01: active_config_id = 3'd4;
            3'b0_10: active_config_id = 3'd5;
            3'b0_11: active_config_id = 3'd6;
            3'b1_00: active_config_id = 3'd0;
            3'b1_01: active_config_id = 3'd2;
            3'b1_10: active_config_id = 3'd1;
            default: active_config_id = 3'd3;
        endcase
    end
    assign current_config_id_o = active_config_id;

    always @(posedge clk_i) begin
        done_o <= 1'b0;
        if (!rst_ni) begin
            busy_o <= 1'b0;
            sa_role_reg <= 1'b0;
            scan_index_reg <= 2'd0;
            config_pattern_map_o <= 80'd0;
        end else if (start_i && !busy_o) begin
            busy_o <= 1'b1;
            sa_role_reg <= sa_role_i;
            scan_index_reg <= 2'd0;
            config_pattern_map_o <= 80'd0;
        end else if (busy_o) begin
            config_pattern_map_o[scan_index_reg*20 +: 20] <= {
                analyzer_candidate_valid_i, analyzer_pattern_id_i,
                analyzer_repairable_i, analyzer_solution_valid_i,
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
