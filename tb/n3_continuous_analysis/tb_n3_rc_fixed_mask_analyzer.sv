`default_nettype none

module tb_n3_rc_fixed_mask_analyzer;
    logic [2:0] row_count_i;
    logic [2:0] col_count_i;
    logic transpose_i;
    logic [6:0] pivot_valid_i;
    logic [62:0] pivot_rows_flat_i;
    logic [90:0] pivot_cols_flat_i;
    logic [6:0] row_gt1_i, row_gt2_i, row_gt3_i, row_gt4_i;
    logic [6:0] col_gt1_i, col_gt2_i, col_gt3_i, col_gt4_i;
    logic [16:0] hybrid_valid_i;
    logic [50:0] hybrid_pointer_flat_i;
    logic [16:0] hybrid_descriptor_i;
    logic [220:0] hybrid_differing_flat_i;
    logic conventional_overflow_i;
    logic [34:0] candidate_valid_o;
    logic [5:0] pattern_id_o;
    logic solution_valid_o, repairable_o, dictionary_overflow_o;
    integer case_count;

    recam_n3_rc_fixed_mask_analyzer dut (.*);

    task automatic clear_state;
        begin
            pivot_valid_i = '0;
            pivot_rows_flat_i = '0;
            pivot_cols_flat_i = '0;
            row_gt1_i = '0; row_gt2_i = '0; row_gt3_i = '0; row_gt4_i = '0;
            col_gt1_i = '0; col_gt2_i = '0; col_gt3_i = '0; col_gt4_i = '0;
            hybrid_valid_i = '0;
            hybrid_pointer_flat_i = '0;
            hybrid_descriptor_i = '0;
            hybrid_differing_flat_i = '0;
            conventional_overflow_i = 1'b0;
        end
    endtask

    task automatic expect_first_pattern(
        input logic [2:0] row_count,
        input logic [2:0] col_count,
        input logic transpose
    );
        integer pivot;
        begin
            clear_state();
            row_count_i = row_count;
            col_count_i = col_count;
            transpose_i = transpose;
            for (pivot = 0; pivot < row_count + col_count; pivot = pivot + 1)
                pivot_valid_i[pivot] = 1'b1;
            #1;
            if (!repairable_o || !solution_valid_o || pattern_id_o != 6'd1)
                $fatal(1, "first fixed mask failed rows=%0d cols=%0d transpose=%0d id=%0d",
                       row_count, col_count, transpose, pattern_id_o);
            case_count = case_count + 1;
        end
    endtask

    initial begin
        case_count = 0;
        expect_first_pattern(3, 3, 0); // 3R3C
        expect_first_pattern(3, 2, 0); // 3R2C
        expect_first_pattern(2, 3, 1); // 2R3C -> canonical 3R2C
        expect_first_pattern(4, 3, 0); // 4R3C
        expect_first_pattern(3, 4, 1); // 3R4C -> canonical 4R3C
        expect_first_pattern(4, 2, 0); // 4R2C
        expect_first_pattern(2, 4, 1); // 2R4C -> canonical 4R2C

        clear_state();
        row_count_i = 3; col_count_i = 3; transpose_i = 0;
        pivot_valid_i = 7'b0111111;
        conventional_overflow_i = 1'b1;
        #1;
        if (repairable_o || solution_valid_o)
            $fatal(1, "overflow did not suppress all candidates");
        case_count = case_count + 1;

        // Full address 257 must match pivot slot four, not physical column one.
        clear_state();
        row_count_i = 3; col_count_i = 3; transpose_i = 0;
        pivot_valid_i = 7'b0111111;
        pivot_cols_flat_i[0 +: 13] = 13'd1;
        pivot_cols_flat_i[4*13 +: 13] = 13'd257;
        hybrid_valid_i[0] = 1'b1;
        hybrid_pointer_flat_i[0 +: 3] = 3'd3;
        hybrid_descriptor_i[0] = 1'b0;
        hybrid_differing_flat_i[0 +: 13] = 13'd257;
        #1;
        if (!repairable_o || pattern_id_o != 6'd1)
            $fatal(1, "physical-column 257 lost distinction id=%0d", pattern_id_o);
        hybrid_differing_flat_i[0 +: 13] = 13'd1;
        #1;
        if (!repairable_o || pattern_id_o == 6'd1)
            $fatal(1, "physical-column alias did not alter full-width decision id=%0d", pattern_id_o);
        case_count = case_count + 1;

        $display("N3_RC_FIXED_MASK_ANALYZER_PASS cases=%0d alias=PASS", case_count);
        $finish;
    end
endmodule

`default_nettype wire
