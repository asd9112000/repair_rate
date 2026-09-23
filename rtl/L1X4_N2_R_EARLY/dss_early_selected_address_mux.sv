`default_nettype none

// Combinational selected-line formatter for one immediate EARLY commit.
module dss_early_selected_address_mux #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer PHYS_COL_ADDR_W = 13,
    parameter integer PIVOT_SLOTS = 5
) (
    input  logic [2:0] selected_config_i,
    input  logic [3:0] selected_pattern_id_i,
    input  logic [PIVOT_SLOTS*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input  logic [PIVOT_SLOTS*PHYS_COL_ADDR_W-1:0] pivot_cols_flat_i,
    output logic [PIVOT_SLOTS*PHYS_COL_ADDR_W-1:0] solution_line_address_flat_o,
    output logic [PIVOT_SLOTS-1:0] solution_line_is_row_o,
    output logic [PIVOT_SLOTS-1:0] solution_line_valid_o
);
    logic [4:0] selected_mask;
    logic [2:0] active_slots;
    logic transpose_config;
    integer slot;

    function automatic logic [4:0] candidate_mask(
        input logic [2:0] config_id,
        input logic [3:0] pattern_id
    );
        begin
            candidate_mask = 5'b00000;
            case (config_id)
                3'd0: case (pattern_id)
                    4'd1: candidate_mask = 5'b01100;
                    4'd2: candidate_mask = 5'b01010;
                    4'd3: candidate_mask = 5'b00110;
                    4'd4: candidate_mask = 5'b01001;
                    4'd5: candidate_mask = 5'b00101;
                    4'd6: candidate_mask = 5'b00011;
                    default: candidate_mask = 5'b00000;
                endcase
                3'd1, 3'd4: case (pattern_id)
                    4'd1: candidate_mask = 5'b00100;
                    4'd2: candidate_mask = 5'b00010;
                    4'd3: candidate_mask = 5'b00001;
                    default: candidate_mask = 5'b00000;
                endcase
                3'd2, 3'd5: case (pattern_id)
                    4'd1: candidate_mask = 5'b11000;
                    4'd2: candidate_mask = 5'b10100;
                    4'd3: candidate_mask = 5'b01100;
                    4'd4: candidate_mask = 5'b10010;
                    4'd5: candidate_mask = 5'b01010;
                    4'd6: candidate_mask = 5'b00110;
                    4'd7: candidate_mask = 5'b10001;
                    4'd8: candidate_mask = 5'b01001;
                    4'd9: candidate_mask = 5'b00101;
                    4'd10: candidate_mask = 5'b00011;
                    default: candidate_mask = 5'b00000;
                endcase
                3'd3, 3'd6: case (pattern_id)
                    4'd1: candidate_mask = 5'b01000;
                    4'd2: candidate_mask = 5'b00100;
                    4'd3: candidate_mask = 5'b00010;
                    4'd4: candidate_mask = 5'b00001;
                    default: candidate_mask = 5'b00000;
                endcase
                default: candidate_mask = 5'b00000;
            endcase
        end
    endfunction

    always_comb begin
        selected_mask = candidate_mask(selected_config_i, selected_pattern_id_i);
        transpose_config = (selected_config_i == 3'd4) ||
                           (selected_config_i == 3'd5) ||
                           (selected_config_i == 3'd6);
        case (selected_config_i)
            3'd0: active_slots = 3'd4;
            3'd1, 3'd4: active_slots = 3'd3;
            3'd2, 3'd5: active_slots = 3'd5;
            3'd3, 3'd6: active_slots = 3'd4;
            default: active_slots = 3'd0;
        endcase

        solution_line_address_flat_o = '0;
        solution_line_is_row_o = '0;
        solution_line_valid_o = '0;
        for (slot = 0; slot < PIVOT_SLOTS; slot = slot + 1) begin
            if (slot < active_slots) begin
                solution_line_is_row_o[slot] = selected_mask[slot] ^ transpose_config;
                solution_line_valid_o[slot] = 1'b1;
                if (selected_mask[slot] ^ transpose_config)
                    solution_line_address_flat_o[slot*PHYS_COL_ADDR_W +: PHYS_COL_ADDR_W] =
                        {{(PHYS_COL_ADDR_W-ROW_ADDR_W){1'b0}}, pivot_rows_flat_i[slot*ROW_ADDR_W +: ROW_ADDR_W]};
                else
                    solution_line_address_flat_o[slot*PHYS_COL_ADDR_W +: PHYS_COL_ADDR_W] =
                        pivot_cols_flat_i[slot*PHYS_COL_ADDR_W +: PHYS_COL_ADDR_W];
            end
        end
    end
endmodule

`default_nettype wire
