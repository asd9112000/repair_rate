`default_nettype none

// Directional group ledger.  This wrapper gives the multi-config path a
// dedicated ownership boundary while retaining the already-validated 2x2
// physical lending rules in resource_tracker.
module physical_resource_ledger #(
    parameter integer RS = 2,
    parameter integer CS = 2,
    parameter integer SHARED_ROWS = 1,
    parameter integer SHARED_COLS = 1,
    parameter integer MAX_BORROWS = 3,
    parameter integer SPARE_WIDTH = 3
) (
    input  wire [4*SPARE_WIDTH-1:0] demand_rows_i,
    input  wire [4*SPARE_WIDTH-1:0] demand_cols_i,
    output wire                     feasible_o,
    output wire [SPARE_WIDTH-1:0]   borrowed_rows_o,
    output wire [SPARE_WIDTH-1:0]   borrowed_cols_o,
    output wire [4*SPARE_WIDTH-1:0] borrowed_rows_by_sa_o,
    output wire [4*SPARE_WIDTH-1:0] borrowed_cols_by_sa_o,
    output wire [7:0]               used_rows_o,
    output wire [7:0]               used_cols_o
);
    resource_tracker #(
        .RS(RS), .CS(CS),
        .SHARED_ROWS(SHARED_ROWS), .SHARED_COLS(SHARED_COLS),
        .MAX_BORROWS(MAX_BORROWS), .TOPOLOGY(1),
        .SPARE_WIDTH(SPARE_WIDTH)
    ) directional_rules (
        .demands_rows_i(demand_rows_i),
        .demands_cols_i(demand_cols_i),
        .allocation_success_o(feasible_o),
        .borrowed_rows_o(borrowed_rows_o),
        .borrowed_cols_o(borrowed_cols_o),
        .borrowed_rows_by_sa_o(borrowed_rows_by_sa_o),
        .borrowed_cols_by_sa_o(borrowed_cols_by_sa_o),
        .used_rows_o(used_rows_o),
        .used_cols_o(used_cols_o)
    );
endmodule

`default_nettype wire
