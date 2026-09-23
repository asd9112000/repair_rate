`default_nettype none

// Passive fixed-slot warehouse for GROUP-delayed physical repair addresses.
module dss_group_pivot_address_regs #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer PHYS_COL_ADDR_W = 13,
    parameter integer PIVOT_SLOTS = 5
) (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic capture_enable_i,
    input  logic [1:0] capture_sa_i,
    input  logic [PIVOT_SLOTS*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input  logic [PIVOT_SLOTS*PHYS_COL_ADDR_W-1:0] pivot_cols_flat_i,
    input  logic [3:0] group_commit_valid_i,
    input  logic [11:0] selected_config_flat_i,
    input  logic [15:0] selected_pattern_flat_i,
    output logic [4*PIVOT_SLOTS*PHYS_COL_ADDR_W-1:0] final_repair_address_flat_o,
    output logic [4*PIVOT_SLOTS-1:0] final_repair_is_row_flat_o,
    output logic [4*PIVOT_SLOTS-1:0] final_repair_line_valid_flat_o
);
    logic [ROW_ADDR_W-1:0] pivot_row_q [0:3][0:PIVOT_SLOTS-1];
    logic [PHYS_COL_ADDR_W-1:0] pivot_col_q [0:3][0:PIVOT_SLOTS-1];
    integer sa;
    integer slot;
    logic [2:0] selected_config;
    logic [3:0] selected_pattern_id;
    logic [4:0] selected_mask;
    logic selected_row;
    logic [2:0] active_slots;

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

    function automatic logic [2:0] config_slots(input logic [2:0] config_id);
        begin
            case (config_id)
                3'd0: config_slots = 3'd4;
                3'd1, 3'd4: config_slots = 3'd3;
                3'd2, 3'd5: config_slots = 3'd5;
                3'd3, 3'd6: config_slots = 3'd4;
                default: config_slots = 3'd0;
            endcase
        end
    endfunction

    always_ff @(posedge clk_i) begin
        if (!rst_ni) begin
            for (sa = 0; sa < 4; sa = sa + 1)
                for (slot = 0; slot < PIVOT_SLOTS; slot = slot + 1) begin
                    pivot_row_q[sa][slot] <= '0;
                    pivot_col_q[sa][slot] <= '0;
                end
        end else if (capture_enable_i) begin
            case (capture_sa_i)
                2'd0: for (slot = 0; slot < PIVOT_SLOTS; slot = slot + 1) begin
                    pivot_row_q[0][slot] <= pivot_rows_flat_i[slot*ROW_ADDR_W +: ROW_ADDR_W];
                    pivot_col_q[0][slot] <= pivot_cols_flat_i[slot*PHYS_COL_ADDR_W +: PHYS_COL_ADDR_W];
                end
                2'd1: for (slot = 0; slot < PIVOT_SLOTS; slot = slot + 1) begin
                    pivot_row_q[1][slot] <= pivot_rows_flat_i[slot*ROW_ADDR_W +: ROW_ADDR_W];
                    pivot_col_q[1][slot] <= pivot_cols_flat_i[slot*PHYS_COL_ADDR_W +: PHYS_COL_ADDR_W];
                end
                2'd2: for (slot = 0; slot < PIVOT_SLOTS; slot = slot + 1) begin
                    pivot_row_q[2][slot] <= pivot_rows_flat_i[slot*ROW_ADDR_W +: ROW_ADDR_W];
                    pivot_col_q[2][slot] <= pivot_cols_flat_i[slot*PHYS_COL_ADDR_W +: PHYS_COL_ADDR_W];
                end
                default: for (slot = 0; slot < PIVOT_SLOTS; slot = slot + 1) begin
                    pivot_row_q[3][slot] <= pivot_rows_flat_i[slot*ROW_ADDR_W +: ROW_ADDR_W];
                    pivot_col_q[3][slot] <= pivot_cols_flat_i[slot*PHYS_COL_ADDR_W +: PHYS_COL_ADDR_W];
                end
            endcase
        end
    end

    always_comb begin
        final_repair_address_flat_o = '0;
        final_repair_is_row_flat_o = '0;
        final_repair_line_valid_flat_o = '0;
        for (sa = 0; sa < 4; sa = sa + 1) begin
            selected_config = selected_config_flat_i[sa*3 +: 3];
            selected_pattern_id = selected_pattern_flat_i[sa*4 +: 4];
            selected_mask = candidate_mask(selected_config, selected_pattern_id);
            active_slots = config_slots(selected_config);
            for (slot = 0; slot < PIVOT_SLOTS; slot = slot + 1) begin
                selected_row = selected_mask[slot];
                if (selected_config == 3'd4 || selected_config == 3'd5 || selected_config == 3'd6)
                    selected_row = !selected_row;
                if (group_commit_valid_i[sa] && slot < active_slots) begin
                    final_repair_is_row_flat_o[sa*PIVOT_SLOTS + slot] = selected_row;
                    final_repair_line_valid_flat_o[sa*PIVOT_SLOTS + slot] = 1'b1;
                    if (selected_row)
                        final_repair_address_flat_o[(sa*PIVOT_SLOTS + slot)*PHYS_COL_ADDR_W +: PHYS_COL_ADDR_W] = {{(PHYS_COL_ADDR_W-ROW_ADDR_W){1'b0}}, pivot_row_q[sa][slot]};
                    else
                        final_repair_address_flat_o[(sa*PIVOT_SLOTS + slot)*PHYS_COL_ADDR_W +: PHYS_COL_ADDR_W] = pivot_col_q[sa][slot];
                end
            end
        end
    end
endmodule

`default_nettype wire
