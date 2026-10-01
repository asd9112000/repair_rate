`default_nettype none

// Passive fixed-slot warehouse for GROUP-delayed physical repair addresses.
module dss_n3_rc_group_pivot_address_regs #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer PHYS_COL_ADDR_W = 13,
    parameter integer PIVOT_SLOTS = 7
) (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic capture_enable_i,
    input  logic [1:0] capture_sa_i,
    input  logic [PIVOT_SLOTS*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input  logic [PIVOT_SLOTS*PHYS_COL_ADDR_W-1:0] pivot_cols_flat_i,
    input  logic [3:0] group_commit_valid_i,
    input  logic [11:0] selected_config_flat_i,
    input  logic [23:0] selected_pattern_flat_i,
    output logic [4*PIVOT_SLOTS*PHYS_COL_ADDR_W-1:0] final_repair_address_flat_o,
    output logic [4*PIVOT_SLOTS-1:0] final_repair_is_row_flat_o,
    output logic [4*PIVOT_SLOTS-1:0] final_repair_line_valid_flat_o
);
    logic [ROW_ADDR_W-1:0] pivot_row_q [0:3][0:PIVOT_SLOTS-1];
    logic [PHYS_COL_ADDR_W-1:0] pivot_col_q [0:3][0:PIVOT_SLOTS-1];
    integer sa;
    integer slot;
    logic [2:0] selected_config;
    logic [5:0] selected_pattern_id;
    logic [6:0] selected_mask;
    logic selected_row;
    logic [2:0] active_slots;

    function automatic logic [6:0] fixed_mask_by_counts(
        input logic [2:0] rows, input logic [2:0] cols, input logic [5:0] pattern_id
    );
        begin
            fixed_mask_by_counts = 7'b0;
            unique case ({rows, cols, pattern_id})
                {3'd3,3'd3,6'd1}: fixed_mask_by_counts = 7'b0111000;
                {3'd3,3'd3,6'd2}: fixed_mask_by_counts = 7'b0110100;
                {3'd3,3'd3,6'd3}: fixed_mask_by_counts = 7'b0110010;
                {3'd3,3'd3,6'd4}: fixed_mask_by_counts = 7'b0110001;
                {3'd3,3'd3,6'd5}: fixed_mask_by_counts = 7'b0101100;
                {3'd3,3'd3,6'd6}: fixed_mask_by_counts = 7'b0101010;
                {3'd3,3'd3,6'd7}: fixed_mask_by_counts = 7'b0101001;
                {3'd3,3'd3,6'd8}: fixed_mask_by_counts = 7'b0100110;
                {3'd3,3'd3,6'd9}: fixed_mask_by_counts = 7'b0100101;
                {3'd3,3'd3,6'd10}: fixed_mask_by_counts = 7'b0100011;
                {3'd3,3'd3,6'd11}: fixed_mask_by_counts = 7'b0011100;
                {3'd3,3'd3,6'd12}: fixed_mask_by_counts = 7'b0011010;
                {3'd3,3'd3,6'd13}: fixed_mask_by_counts = 7'b0011001;
                {3'd3,3'd3,6'd14}: fixed_mask_by_counts = 7'b0010110;
                {3'd3,3'd3,6'd15}: fixed_mask_by_counts = 7'b0010101;
                {3'd3,3'd3,6'd16}: fixed_mask_by_counts = 7'b0010011;
                {3'd3,3'd3,6'd17}: fixed_mask_by_counts = 7'b0001110;
                {3'd3,3'd3,6'd18}: fixed_mask_by_counts = 7'b0001101;
                {3'd3,3'd3,6'd19}: fixed_mask_by_counts = 7'b0001011;
                {3'd3,3'd3,6'd20}: fixed_mask_by_counts = 7'b0000111;
                {3'd3,3'd2,6'd1}: fixed_mask_by_counts = 7'b0011100;
                {3'd3,3'd2,6'd2}: fixed_mask_by_counts = 7'b0011010;
                {3'd3,3'd2,6'd3}: fixed_mask_by_counts = 7'b0011001;
                {3'd3,3'd2,6'd4}: fixed_mask_by_counts = 7'b0010110;
                {3'd3,3'd2,6'd5}: fixed_mask_by_counts = 7'b0010101;
                {3'd3,3'd2,6'd6}: fixed_mask_by_counts = 7'b0010011;
                {3'd3,3'd2,6'd7}: fixed_mask_by_counts = 7'b0001110;
                {3'd3,3'd2,6'd8}: fixed_mask_by_counts = 7'b0001101;
                {3'd3,3'd2,6'd9}: fixed_mask_by_counts = 7'b0001011;
                {3'd3,3'd2,6'd10}: fixed_mask_by_counts = 7'b0000111;
                {3'd4,3'd3,6'd1}: fixed_mask_by_counts = 7'b1111000;
                {3'd4,3'd3,6'd2}: fixed_mask_by_counts = 7'b1110100;
                {3'd4,3'd3,6'd3}: fixed_mask_by_counts = 7'b1110010;
                {3'd4,3'd3,6'd4}: fixed_mask_by_counts = 7'b1110001;
                {3'd4,3'd3,6'd5}: fixed_mask_by_counts = 7'b1101100;
                {3'd4,3'd3,6'd6}: fixed_mask_by_counts = 7'b1101010;
                {3'd4,3'd3,6'd7}: fixed_mask_by_counts = 7'b1101001;
                {3'd4,3'd3,6'd8}: fixed_mask_by_counts = 7'b1100110;
                {3'd4,3'd3,6'd9}: fixed_mask_by_counts = 7'b1100101;
                {3'd4,3'd3,6'd10}: fixed_mask_by_counts = 7'b1100011;
                {3'd4,3'd3,6'd11}: fixed_mask_by_counts = 7'b1011100;
                {3'd4,3'd3,6'd12}: fixed_mask_by_counts = 7'b1011010;
                {3'd4,3'd3,6'd13}: fixed_mask_by_counts = 7'b1011001;
                {3'd4,3'd3,6'd14}: fixed_mask_by_counts = 7'b1010110;
                {3'd4,3'd3,6'd15}: fixed_mask_by_counts = 7'b1010101;
                {3'd4,3'd3,6'd16}: fixed_mask_by_counts = 7'b1010011;
                {3'd4,3'd3,6'd17}: fixed_mask_by_counts = 7'b1001110;
                {3'd4,3'd3,6'd18}: fixed_mask_by_counts = 7'b1001101;
                {3'd4,3'd3,6'd19}: fixed_mask_by_counts = 7'b1001011;
                {3'd4,3'd3,6'd20}: fixed_mask_by_counts = 7'b1000111;
                {3'd4,3'd3,6'd21}: fixed_mask_by_counts = 7'b0111100;
                {3'd4,3'd3,6'd22}: fixed_mask_by_counts = 7'b0111010;
                {3'd4,3'd3,6'd23}: fixed_mask_by_counts = 7'b0111001;
                {3'd4,3'd3,6'd24}: fixed_mask_by_counts = 7'b0110110;
                {3'd4,3'd3,6'd25}: fixed_mask_by_counts = 7'b0110101;
                {3'd4,3'd3,6'd26}: fixed_mask_by_counts = 7'b0110011;
                {3'd4,3'd3,6'd27}: fixed_mask_by_counts = 7'b0101110;
                {3'd4,3'd3,6'd28}: fixed_mask_by_counts = 7'b0101101;
                {3'd4,3'd3,6'd29}: fixed_mask_by_counts = 7'b0101011;
                {3'd4,3'd3,6'd30}: fixed_mask_by_counts = 7'b0100111;
                {3'd4,3'd3,6'd31}: fixed_mask_by_counts = 7'b0011110;
                {3'd4,3'd3,6'd32}: fixed_mask_by_counts = 7'b0011101;
                {3'd4,3'd3,6'd33}: fixed_mask_by_counts = 7'b0011011;
                {3'd4,3'd3,6'd34}: fixed_mask_by_counts = 7'b0010111;
                {3'd4,3'd3,6'd35}: fixed_mask_by_counts = 7'b0001111;
                {3'd4,3'd2,6'd1}: fixed_mask_by_counts = 7'b0111100;
                {3'd4,3'd2,6'd2}: fixed_mask_by_counts = 7'b0111010;
                {3'd4,3'd2,6'd3}: fixed_mask_by_counts = 7'b0111001;
                {3'd4,3'd2,6'd4}: fixed_mask_by_counts = 7'b0110110;
                {3'd4,3'd2,6'd5}: fixed_mask_by_counts = 7'b0110101;
                {3'd4,3'd2,6'd6}: fixed_mask_by_counts = 7'b0110011;
                {3'd4,3'd2,6'd7}: fixed_mask_by_counts = 7'b0101110;
                {3'd4,3'd2,6'd8}: fixed_mask_by_counts = 7'b0101101;
                {3'd4,3'd2,6'd9}: fixed_mask_by_counts = 7'b0101011;
                {3'd4,3'd2,6'd10}: fixed_mask_by_counts = 7'b0100111;
                {3'd4,3'd2,6'd11}: fixed_mask_by_counts = 7'b0011110;
                {3'd4,3'd2,6'd12}: fixed_mask_by_counts = 7'b0011101;
                {3'd4,3'd2,6'd13}: fixed_mask_by_counts = 7'b0011011;
                {3'd4,3'd2,6'd14}: fixed_mask_by_counts = 7'b0010111;
                {3'd4,3'd2,6'd15}: fixed_mask_by_counts = 7'b0001111;
                default: fixed_mask_by_counts = 7'b0;
            endcase
        end
    endfunction

    function automatic logic [6:0] candidate_mask(
        input logic [2:0] config_id, input logic [5:0] pattern_id
    );
        begin
            candidate_mask = 7'b0;
            case (config_id)
                3'd0: candidate_mask = fixed_mask_by_counts(3'd3, 3'd3, pattern_id);
                3'd1, 3'd4: candidate_mask = fixed_mask_by_counts(3'd3, 3'd2, pattern_id);
                3'd2, 3'd5: candidate_mask = fixed_mask_by_counts(3'd4, 3'd3, pattern_id);
                3'd3, 3'd6: candidate_mask = fixed_mask_by_counts(3'd4, 3'd2, pattern_id);
                default: candidate_mask = 7'b0;
            endcase
        end
    endfunction

    function automatic logic [2:0] config_slots(input logic [2:0] config_id);
        begin
            case (config_id)
                3'd0: config_slots = 3'd6;
                3'd1, 3'd4: config_slots = 3'd5;
                3'd2, 3'd5: config_slots = 3'd7;
                3'd3, 3'd6: config_slots = 3'd6;
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
            selected_pattern_id = selected_pattern_flat_i[sa*6 +: 6];
            selected_mask = candidate_mask(selected_config, selected_pattern_id);
            active_slots = config_slots(selected_config);
            for (slot = 0; slot < PIVOT_SLOTS; slot = slot + 1) begin
                selected_row = selected_mask[slot];
                if (selected_config != 3'd0 && selected_config <= 3'd3)
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
