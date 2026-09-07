`default_nettype none

module solution_selector #(
    parameter integer RS = 2,
    parameter integer CS = 2,
    parameter integer SHARED_ROWS = 1,
    parameter integer SHARED_COLS = 1,
    parameter integer MAX_BORROWS = 3,
    parameter integer TOPOLOGY = 1,
    parameter integer MAX_RS = RS + SHARED_ROWS,
    parameter integer MAX_CS = CS + SHARED_COLS,
    parameter integer MAX_K = RS + CS + ((SHARED_ROWS > SHARED_COLS) ? SHARED_ROWS : SHARED_COLS),
    parameter integer MASK_SPACE = (1 << MAX_K),
    parameter integer SPARE_WIDTH = (MAX_K <= 1) ? 1 : $clog2(MAX_K + 1),
    parameter integer CANDIDATE_ID_WIDTH = (MASK_SPACE <= 1) ? 1 : $clog2(MASK_SPACE),
    parameter integer DEMAND_NUM = (MAX_RS + 1) * (MAX_CS + 1),
    parameter integer DEMAND_WIDTH = (DEMAND_NUM <= 1) ? 1 : $clog2(DEMAND_NUM)
) (
    input  wire                                      clk_i,
    input  wire                                      rst_ni,
    input  wire                                      start_i,
    input  wire [3:0]                                base_repairable_i,
    input  wire [4*MASK_SPACE-1:0]                   base_valid_candidates_i,
    input  wire [4*MASK_SPACE*SPARE_WIDTH-1:0]       base_used_rows_i,
    input  wire [4*MASK_SPACE*SPARE_WIDTH-1:0]       base_used_cols_i,
    input  wire [4*MASK_SPACE*MAX_K-1:0]             base_candidate_masks_i,
    input  wire [4*32-1:0]                           base_attempt_cycles_i,
    input  wire [3:0]                                extended_repairable_i,
    input  wire [4*MASK_SPACE-1:0]                   extended_valid_candidates_i,
    input  wire [4*MASK_SPACE*SPARE_WIDTH-1:0]       extended_used_rows_i,
    input  wire [4*MASK_SPACE*SPARE_WIDTH-1:0]       extended_used_cols_i,
    input  wire [4*MASK_SPACE*MAX_K-1:0]             extended_candidate_masks_i,
    input  wire [4*32-1:0]                           extended_attempt_cycles_i,
    output reg                                       busy_o,
    output reg                                       done_o,
    output reg                                       repairable_o,
    output reg  [3:0]                                selected_attempts_o,
    output reg  [4*CANDIDATE_ID_WIDTH-1:0]           selected_candidates_o,
    output reg  [4*SPARE_WIDTH-1:0]                  selected_rows_o,
    output reg  [4*SPARE_WIDTH-1:0]                  selected_cols_o,
    output reg  [4*MAX_K-1:0]                        selected_masks_o,
    output reg  [SPARE_WIDTH-1:0]                    borrowed_rows_o,
    output reg  [SPARE_WIDTH-1:0]                    borrowed_cols_o,
    output reg  [31:0]                               analysis_cycles_o
);
    reg [DEMAND_WIDTH-1:0] demand_index [0:3];
    reg [4*SPARE_WIDTH-1:0] current_rows;
    reg [4*SPARE_WIDTH-1:0] current_cols;
    reg [3:0] plan_valid;
    reg [3:0] plan_attempt;
    reg [4*CANDIDATE_ID_WIDTH-1:0] plan_candidate;
    reg [4*MAX_K-1:0] plan_mask;
    reg [31:0] selected_cycle_sum;
    reg [31:0] current_rank_cycles;
    reg [31:0] best_rank_cycles;
    reg [SPARE_WIDTH:0] current_borrow_total;
    reg [SPARE_WIDTH:0] best_borrow_total;
    reg best_found;
    reg better;
    reg final_combination;
    wire allocation_feasible;
    wire [SPARE_WIDTH-1:0] allocation_borrowed_rows;
    wire [SPARE_WIDTH-1:0] allocation_borrowed_cols;
    wire [7:0] unused_rows_total;
    wire [7:0] unused_cols_total;
    integer demand_row;
    integer demand_col;
    integer best_cycle_for_plan;
    integer best_candidate_for_plan;
    integer best_attempt_for_plan;
    integer candidate_rows;
    integer candidate_cols;
    integer candidate_cycle;
    integer sa;
    integer candidate;
    integer compare_sa;
    /* verilator lint_off WIDTH */
    localparam [DEMAND_WIDTH-1:0] DEMAND_LAST = DEMAND_NUM - 1;
    /* verilator lint_on WIDTH */

    sharing_controller #(
        .RS(RS),
        .CS(CS),
        .SHARED_ROWS(SHARED_ROWS),
        .SHARED_COLS(SHARED_COLS),
        .MAX_BORROWS(MAX_BORROWS),
        .TOPOLOGY(TOPOLOGY),
        .SPARE_WIDTH(SPARE_WIDTH)
    ) allocation (
        .demands_rows_i(current_rows),
        .demands_cols_i(current_cols),
        .feasible_o(allocation_feasible),
        .borrowed_rows_o(allocation_borrowed_rows),
        .borrowed_cols_o(allocation_borrowed_cols),
        .used_rows_o(unused_rows_total),
        .used_cols_o(unused_cols_total)
    );

    always @* begin
        current_rows = {(4*SPARE_WIDTH){1'b0}};
        current_cols = {(4*SPARE_WIDTH){1'b0}};
        for (sa = 0; sa < 4; sa = sa + 1) begin
            /* verilator lint_off WIDTH */
            demand_row = demand_index[sa] / (MAX_CS + 1);
            demand_col = demand_index[sa] % (MAX_CS + 1);
            /* verilator lint_on WIDTH */
            current_rows[sa*SPARE_WIDTH +: SPARE_WIDTH] =
                demand_row[SPARE_WIDTH-1:0];
            current_cols[sa*SPARE_WIDTH +: SPARE_WIDTH] =
                demand_col[SPARE_WIDTH-1:0];
        end
    end

    always @* begin
        plan_valid = 4'b0000;
        plan_attempt = 4'b0000;
        plan_candidate = {(4*CANDIDATE_ID_WIDTH){1'b0}};
        plan_mask = {(4*MAX_K){1'b0}};
        selected_cycle_sum = 32'd0;

        for (sa = 0; sa < 4; sa = sa + 1) begin
            /* verilator lint_off WIDTH */
            demand_row = current_rows[sa*SPARE_WIDTH +: SPARE_WIDTH];
            demand_col = current_cols[sa*SPARE_WIDTH +: SPARE_WIDTH];
            /* verilator lint_on WIDTH */
            best_cycle_for_plan = 32'h7fffffff;
            best_candidate_for_plan = 0;
            best_attempt_for_plan = 0;

            for (candidate = 0; candidate < MASK_SPACE; candidate = candidate + 1) begin
                /* verilator lint_off WIDTH */
                candidate_rows = base_used_rows_i[
                    (sa*MASK_SPACE + candidate)*SPARE_WIDTH +: SPARE_WIDTH];
                candidate_cols = base_used_cols_i[
                    (sa*MASK_SPACE + candidate)*SPARE_WIDTH +: SPARE_WIDTH];
                candidate_cycle = base_attempt_cycles_i[sa*32 +: 32];
                /* verilator lint_on WIDTH */
                if (base_repairable_i[sa] &&
                    base_valid_candidates_i[sa*MASK_SPACE + candidate] &&
                    candidate_rows == demand_row && candidate_cols == demand_col &&
                    (!plan_valid[sa] || candidate_cycle < best_cycle_for_plan ||
                     (candidate_cycle == best_cycle_for_plan &&
                      candidate < best_candidate_for_plan))) begin
                    plan_valid[sa] = 1'b1;
                    best_cycle_for_plan = candidate_cycle;
                    best_candidate_for_plan = candidate;
                    best_attempt_for_plan = 0;
                    plan_mask[sa*MAX_K +: MAX_K] =
                        base_candidate_masks_i[(sa*MASK_SPACE + candidate)*MAX_K +: MAX_K];
                end

                /* verilator lint_off WIDTH */
                candidate_rows = extended_used_rows_i[
                    (sa*MASK_SPACE + candidate)*SPARE_WIDTH +: SPARE_WIDTH];
                candidate_cols = extended_used_cols_i[
                    (sa*MASK_SPACE + candidate)*SPARE_WIDTH +: SPARE_WIDTH];
                candidate_cycle = extended_attempt_cycles_i[sa*32 +: 32];
                /* verilator lint_on WIDTH */
                if (!base_repairable_i[sa] && extended_repairable_i[sa] &&
                    extended_valid_candidates_i[sa*MASK_SPACE + candidate] &&
                    candidate_rows == demand_row && candidate_cols == demand_col &&
                    (!plan_valid[sa] || candidate_cycle < best_cycle_for_plan ||
                     (candidate_cycle == best_cycle_for_plan &&
                      (candidate < best_candidate_for_plan ||
                       (candidate == best_candidate_for_plan &&
                        1 < best_attempt_for_plan))))) begin
                    plan_valid[sa] = 1'b1;
                    best_cycle_for_plan = candidate_cycle;
                    best_candidate_for_plan = candidate;
                    best_attempt_for_plan = 1;
                    plan_mask[sa*MAX_K +: MAX_K] =
                        extended_candidate_masks_i[(sa*MASK_SPACE + candidate)*MAX_K +: MAX_K];
                end
            end
            plan_attempt[sa] = best_attempt_for_plan != 0;
            plan_candidate[sa*CANDIDATE_ID_WIDTH +: CANDIDATE_ID_WIDTH] =
                best_candidate_for_plan[CANDIDATE_ID_WIDTH-1:0];
            if (plan_valid[sa])
                selected_cycle_sum = selected_cycle_sum + best_cycle_for_plan;
        end

        current_borrow_total = allocation_borrowed_rows + allocation_borrowed_cols;
        /* verilator lint_off WIDTH */
        current_rank_cycles = selected_cycle_sum + current_borrow_total;
        /* verilator lint_on WIDTH */
        better = (&plan_valid) && allocation_feasible && !best_found;
        if ((&plan_valid) && allocation_feasible && best_found) begin
            if (current_borrow_total < best_borrow_total)
                better = 1'b1;
            else if (current_borrow_total == best_borrow_total &&
                     current_rank_cycles < best_rank_cycles)
                better = 1'b1;
            else if (current_borrow_total == best_borrow_total &&
                     current_rank_cycles == best_rank_cycles) begin
                better = 1'b0;
                for (compare_sa = 3; compare_sa >= 0; compare_sa = compare_sa - 1) begin
                    if (plan_candidate[compare_sa*CANDIDATE_ID_WIDTH +: CANDIDATE_ID_WIDTH] <
                        selected_candidates_o[compare_sa*CANDIDATE_ID_WIDTH +: CANDIDATE_ID_WIDTH])
                        better = 1'b1;
                    else if (plan_candidate[compare_sa*CANDIDATE_ID_WIDTH +: CANDIDATE_ID_WIDTH] >
                             selected_candidates_o[compare_sa*CANDIDATE_ID_WIDTH +: CANDIDATE_ID_WIDTH])
                        better = 1'b0;
                end
                if (plan_candidate == selected_candidates_o) begin
                    for (compare_sa = 3; compare_sa >= 0; compare_sa = compare_sa - 1) begin
                        if (plan_attempt[compare_sa] < selected_attempts_o[compare_sa])
                            better = 1'b1;
                        else if (plan_attempt[compare_sa] > selected_attempts_o[compare_sa])
                            better = 1'b0;
                    end
                end
            end
        end
        final_combination =
            demand_index[0] == DEMAND_LAST &&
            demand_index[1] == DEMAND_LAST &&
            demand_index[2] == DEMAND_LAST &&
            demand_index[3] == DEMAND_LAST;
    end

    always @(posedge clk_i) begin
        if (!rst_ni) begin
            busy_o <= 1'b0;
            done_o <= 1'b0;
            repairable_o <= 1'b0;
            selected_attempts_o <= 4'b0000;
            selected_candidates_o <= {(4*CANDIDATE_ID_WIDTH){1'b0}};
            selected_rows_o <= {(4*SPARE_WIDTH){1'b0}};
            selected_cols_o <= {(4*SPARE_WIDTH){1'b0}};
            selected_masks_o <= {(4*MAX_K){1'b0}};
            borrowed_rows_o <= {SPARE_WIDTH{1'b0}};
            borrowed_cols_o <= {SPARE_WIDTH{1'b0}};
            analysis_cycles_o <= 32'd0;
            best_rank_cycles <= 32'd0;
            best_borrow_total <= {(SPARE_WIDTH+1){1'b0}};
            best_found <= 1'b0;
            demand_index[0] <= {DEMAND_WIDTH{1'b0}};
            demand_index[1] <= {DEMAND_WIDTH{1'b0}};
            demand_index[2] <= {DEMAND_WIDTH{1'b0}};
            demand_index[3] <= {DEMAND_WIDTH{1'b0}};
        end else begin
            done_o <= 1'b0;
            if (start_i && !busy_o) begin
                busy_o <= 1'b1;
                repairable_o <= 1'b0;
                selected_attempts_o <= 4'b0000;
                selected_candidates_o <= {(4*CANDIDATE_ID_WIDTH){1'b0}};
                selected_rows_o <= {(4*SPARE_WIDTH){1'b0}};
                selected_cols_o <= {(4*SPARE_WIDTH){1'b0}};
                selected_masks_o <= {(4*MAX_K){1'b0}};
                borrowed_rows_o <= {SPARE_WIDTH{1'b0}};
                borrowed_cols_o <= {SPARE_WIDTH{1'b0}};
                analysis_cycles_o <= 32'd0;
                best_rank_cycles <= 32'd0;
                best_borrow_total <= {(SPARE_WIDTH+1){1'b0}};
                best_found <= 1'b0;
                demand_index[0] <= {DEMAND_WIDTH{1'b0}};
                demand_index[1] <= {DEMAND_WIDTH{1'b0}};
                demand_index[2] <= {DEMAND_WIDTH{1'b0}};
                demand_index[3] <= {DEMAND_WIDTH{1'b0}};
            end else if (busy_o) begin
                analysis_cycles_o <= analysis_cycles_o + 1'b1;
                if (better) begin
                    repairable_o <= 1'b1;
                    best_found <= 1'b1;
                    best_rank_cycles <= current_rank_cycles;
                    best_borrow_total <= current_borrow_total;
                    selected_attempts_o <= plan_attempt;
                    selected_candidates_o <= plan_candidate;
                    selected_rows_o <= current_rows;
                    selected_cols_o <= current_cols;
                    selected_masks_o <= plan_mask;
                    borrowed_rows_o <= allocation_borrowed_rows;
                    borrowed_cols_o <= allocation_borrowed_cols;
                end
                if (final_combination) begin
                    busy_o <= 1'b0;
                    done_o <= 1'b1;
                end else if (demand_index[3] != DEMAND_LAST) begin
                    demand_index[3] <= demand_index[3] + 1'b1;
                end else begin
                    demand_index[3] <= {DEMAND_WIDTH{1'b0}};
                    if (demand_index[2] != DEMAND_LAST)
                        demand_index[2] <= demand_index[2] + 1'b1;
                    else begin
                        demand_index[2] <= {DEMAND_WIDTH{1'b0}};
                        if (demand_index[1] != DEMAND_LAST)
                            demand_index[1] <= demand_index[1] + 1'b1;
                        else begin
                            demand_index[1] <= {DEMAND_WIDTH{1'b0}};
                            demand_index[0] <= demand_index[0] + 1'b1;
                        end
                    end
                end
            end
        end
    end
endmodule

`default_nettype wire
