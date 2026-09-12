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
    localparam integer TOTAL_WIDTH = SPARE_WIDTH + 2;
    localparam [SPARE_WIDTH-1:0] RS_VALUE = RS;
    localparam [SPARE_WIDTH-1:0] CS_VALUE = CS;
    localparam [SPARE_WIDTH-1:0] SHARED_ROWS_VALUE = SHARED_ROWS;
    localparam [SPARE_WIDTH-1:0] SHARED_COLS_VALUE = SHARED_COLS;
    localparam [SPARE_WIDTH:0] TWO_RS_VALUE = 2*RS;
    localparam [SPARE_WIDTH:0] TWO_CS_VALUE = 2*CS;
    localparam [TOTAL_WIDTH:0] MAX_BORROWS_VALUE = MAX_BORROWS;
    reg [SPARE_WIDTH-1:0] rows [0:3];
    reg [SPARE_WIDTH-1:0] cols [0:3];
    reg [SPARE_WIDTH-1:0] row_borrow [0:3];
    reg [SPARE_WIDTH-1:0] col_borrow [0:3];
    reg [TOTAL_WIDTH-1:0] row_total_borrow;
    reg [TOTAL_WIDTH-1:0] col_total_borrow;
    reg [TOTAL_WIDTH-1:0] row_total;
    reg [TOTAL_WIDTH-1:0] col_total;
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
            rows[sa] = demands_rows_i[sa*SPARE_WIDTH +: SPARE_WIDTH];
            cols[sa] = demands_cols_i[sa*SPARE_WIDTH +: SPARE_WIDTH];
            row_borrow[sa] = rows[sa] > RS_VALUE ? rows[sa] - RS_VALUE : '0;
            col_borrow[sa] = cols[sa] > CS_VALUE ? cols[sa] - CS_VALUE : '0;
            row_total_borrow = row_total_borrow +
                {{(TOTAL_WIDTH-SPARE_WIDTH){1'b0}}, row_borrow[sa]};
            col_total_borrow = col_total_borrow +
                {{(TOTAL_WIDTH-SPARE_WIDTH){1'b0}}, col_borrow[sa]};
            row_total = row_total +
                {{(TOTAL_WIDTH-SPARE_WIDTH){1'b0}}, rows[sa]};
            col_total = col_total +
                {{(TOTAL_WIDTH-SPARE_WIDTH){1'b0}}, cols[sa]};
            borrowed_rows_by_sa_o[sa*SPARE_WIDTH +: SPARE_WIDTH] =
                row_borrow[sa][SPARE_WIDTH-1:0];
            borrowed_cols_by_sa_o[sa*SPARE_WIDTH +: SPARE_WIDTH] =
                col_borrow[sa][SPARE_WIDTH-1:0];
        end

        if (TOPOLOGY == 0) begin
            for (sa = 0; sa < 4; sa = sa + 1)
                if (rows[sa] > RS_VALUE || cols[sa] > CS_VALUE)
                    feasible = 1'b0;
        end else if (TOPOLOGY == 1) begin
            // Directional ownership: A-row -> C, D-row -> B,
            // B-column -> A, C-column -> D.
            if (row_borrow[0] != 0 || col_borrow[1] != 0 ||
                col_borrow[2] != 0 || row_borrow[3] != 0)
                feasible = 1'b0;
            if (row_borrow[1] > SHARED_ROWS_VALUE ||
                row_borrow[2] > SHARED_ROWS_VALUE ||
                col_borrow[0] > SHARED_COLS_VALUE ||
                col_borrow[3] > SHARED_COLS_VALUE)
                feasible = 1'b0;
            if ({1'b0, rows[3]} + {1'b0, row_borrow[1]} > {1'b0, RS_VALUE} ||
                {1'b0, rows[0]} + {1'b0, row_borrow[2]} > {1'b0, RS_VALUE} ||
                {1'b0, cols[1]} + {1'b0, col_borrow[0]} > {1'b0, CS_VALUE} ||
                {1'b0, cols[2]} + {1'b0, col_borrow[3]} > {1'b0, CS_VALUE})
                feasible = 1'b0;
        end else if (TOPOLOGY == 2) begin
            // Bidirectional 2x2 edge pairs: vertical for rows, horizontal
            // for columns. No diagonal or multi-hop transfer is allowed.
            for (sa = 0; sa < 4; sa = sa + 1) begin
                if (row_borrow[sa] > SHARED_ROWS_VALUE ||
                    col_borrow[sa] > SHARED_COLS_VALUE)
                    feasible = 1'b0;
            end
            if ({1'b0, rows[0]} + {1'b0, rows[2]} > TWO_RS_VALUE ||
                {1'b0, rows[1]} + {1'b0, rows[3]} > TWO_RS_VALUE ||
                {1'b0, cols[0]} + {1'b0, cols[1]} > TWO_CS_VALUE ||
                {1'b0, cols[2]} + {1'b0, cols[3]} > TWO_CS_VALUE)
                feasible = 1'b0;
        end else begin
            feasible = 1'b0;
        end

        if ({1'b0, row_total_borrow} + {1'b0, col_total_borrow} >
            MAX_BORROWS_VALUE)
            feasible = 1'b0;
        allocation_success_o = feasible;
        borrowed_rows_o = row_total_borrow[SPARE_WIDTH-1:0];
        borrowed_cols_o = col_total_borrow[SPARE_WIDTH-1:0];
        used_rows_o = row_total;
        used_cols_o = col_total;
    end
endmodule

`default_nettype wire
