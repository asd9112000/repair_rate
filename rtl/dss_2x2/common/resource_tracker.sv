`default_nettype none

module resource_tracker #(
    parameter integer RS = 2,
    parameter integer CS = 2,
    parameter integer SHARED_ROWS = 1,
    parameter integer SHARED_COLS = 1,
    parameter integer MAX_BORROWS = 3,
    parameter integer TOPOLOGY = 1,
    parameter integer SPARE_WIDTH = 3
) (
    input  wire [4*SPARE_WIDTH-1:0] demands_rows_i,
    input  wire [4*SPARE_WIDTH-1:0] demands_cols_i,
    output reg                      allocation_success_o,
    output reg  [SPARE_WIDTH-1:0]  borrowed_rows_o,
    output reg  [SPARE_WIDTH-1:0]  borrowed_cols_o,
    output reg  [4*SPARE_WIDTH-1:0] borrowed_rows_by_sa_o,
    output reg  [4*SPARE_WIDTH-1:0] borrowed_cols_by_sa_o,
    output reg  [7:0]              used_rows_o,
    output reg  [7:0]              used_cols_o
);
    integer rows [0:3];
    integer cols [0:3];
    integer row_borrow [0:3];
    integer col_borrow [0:3];
    integer row_total_borrow;
    integer col_total_borrow;
    integer row_total;
    integer col_total;
    integer sa;
    reg feasible;

    always @* begin
        feasible = 1'b1;
        row_total_borrow = 0;
        col_total_borrow = 0;
        row_total = 0;
        col_total = 0;
        borrowed_rows_by_sa_o = {(4*SPARE_WIDTH){1'b0}};
        borrowed_cols_by_sa_o = {(4*SPARE_WIDTH){1'b0}};
        for (sa = 0; sa < 4; sa = sa + 1) begin
            /* verilator lint_off WIDTH */
            rows[sa] = demands_rows_i[sa*SPARE_WIDTH +: SPARE_WIDTH];
            cols[sa] = demands_cols_i[sa*SPARE_WIDTH +: SPARE_WIDTH];
            /* verilator lint_on WIDTH */
            row_borrow[sa] = rows[sa] > RS ? rows[sa] - RS : 0;
            col_borrow[sa] = cols[sa] > CS ? cols[sa] - CS : 0;
            row_total_borrow = row_total_borrow + row_borrow[sa];
            col_total_borrow = col_total_borrow + col_borrow[sa];
            row_total = row_total + rows[sa];
            col_total = col_total + cols[sa];
            borrowed_rows_by_sa_o[sa*SPARE_WIDTH +: SPARE_WIDTH] =
                row_borrow[sa][SPARE_WIDTH-1:0];
            borrowed_cols_by_sa_o[sa*SPARE_WIDTH +: SPARE_WIDTH] =
                col_borrow[sa][SPARE_WIDTH-1:0];
        end

        if (TOPOLOGY == 0) begin
            for (sa = 0; sa < 4; sa = sa + 1)
                if (rows[sa] > RS || cols[sa] > CS)
                    feasible = 1'b0;
        end else if (TOPOLOGY == 1) begin
            // Directional ownership: A-row -> C, D-row -> B,
            // B-column -> A, C-column -> D.
            if (row_borrow[0] != 0 || col_borrow[1] != 0 ||
                col_borrow[2] != 0 || row_borrow[3] != 0)
                feasible = 1'b0;
            if (row_borrow[1] > SHARED_ROWS ||
                row_borrow[2] > SHARED_ROWS ||
                col_borrow[0] > SHARED_COLS ||
                col_borrow[3] > SHARED_COLS)
                feasible = 1'b0;
            if (rows[3] + row_borrow[1] > RS ||
                rows[0] + row_borrow[2] > RS ||
                cols[1] + col_borrow[0] > CS ||
                cols[2] + col_borrow[3] > CS)
                feasible = 1'b0;
        end else if (TOPOLOGY == 2) begin
            // Bidirectional 2x2 edge pairs: vertical for rows, horizontal
            // for columns. No diagonal or multi-hop transfer is allowed.
            for (sa = 0; sa < 4; sa = sa + 1) begin
                if (row_borrow[sa] > SHARED_ROWS ||
                    col_borrow[sa] > SHARED_COLS)
                    feasible = 1'b0;
            end
            if (rows[0] + rows[2] > 2*RS ||
                rows[1] + rows[3] > 2*RS ||
                cols[0] + cols[1] > 2*CS ||
                cols[2] + cols[3] > 2*CS)
                feasible = 1'b0;
        end else begin
            feasible = 1'b0;
        end

        if (row_total_borrow + col_total_borrow > MAX_BORROWS)
            feasible = 1'b0;
        allocation_success_o = feasible;
        borrowed_rows_o = row_total_borrow[SPARE_WIDTH-1:0];
        borrowed_cols_o = col_total_borrow[SPARE_WIDTH-1:0];
        used_rows_o = row_total[7:0];
        used_cols_o = col_total[7:0];
    end
endmodule

`default_nettype wire
