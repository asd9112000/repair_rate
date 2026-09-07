`default_nettype none

module pway_search #(
    parameter integer ENTRY_NUM = 8,
    parameter integer ENTRY_WIDTH = 8,
    parameter integer P = 2,
    parameter integer INDEX_WIDTH = (ENTRY_NUM <= 1) ? 1 : $clog2(ENTRY_NUM),
    parameter integer CYCLE_WIDTH = (ENTRY_NUM <= 1) ? 1 : $clog2(ENTRY_NUM + 1)
) (
    input  wire                             clk_i,
    input  wire                             rst_ni,
    input  wire                             start_i,
    input  wire [ENTRY_WIDTH-1:0]           search_key_i,
    input  wire [ENTRY_NUM-1:0]             valid_vector_i,
    input  wire [ENTRY_NUM*ENTRY_WIDTH-1:0] entries_flat_i,
    output reg                              busy_o,
    output reg                              done_o,
    output reg                              match_o,
    output reg  [INDEX_WIDTH-1:0]           match_index_o,
    output reg  [CYCLE_WIDTH-1:0]           search_cycles_o
);
    reg [ENTRY_WIDTH-1:0] latched_key;
    reg [INDEX_WIDTH:0] base_index;
    reg [CYCLE_WIDTH-1:0] elapsed_cycles;
    // Constants intentionally narrow to the bounded search-index domain.
    /* verilator lint_off WIDTH */
    localparam [INDEX_WIDTH:0] ENTRY_LIMIT = ENTRY_NUM;
    localparam [INDEX_WIDTH:0] P_STEP = P;
    /* verilator lint_on WIDTH */
    reg chunk_match;
    reg [INDEX_WIDTH-1:0] chunk_match_index;
    integer lane;
    integer absolute_index;

    always @* begin
        chunk_match = 1'b0;
        chunk_match_index = {INDEX_WIDTH{1'b0}};
        for (lane = 0; lane < P; lane = lane + 1) begin
            /* verilator lint_off WIDTH */
            absolute_index = base_index;
            /* verilator lint_on WIDTH */
            absolute_index = absolute_index + lane;
            if (!chunk_match && absolute_index < ENTRY_NUM &&
                valid_vector_i[absolute_index] &&
                entries_flat_i[absolute_index*ENTRY_WIDTH +: ENTRY_WIDTH] == latched_key) begin
                chunk_match = 1'b1;
                chunk_match_index = absolute_index[INDEX_WIDTH-1:0];
            end
        end
    end

    always @(posedge clk_i) begin
        if (!rst_ni) begin
            busy_o <= 1'b0;
            done_o <= 1'b0;
            match_o <= 1'b0;
            match_index_o <= {INDEX_WIDTH{1'b0}};
            search_cycles_o <= {CYCLE_WIDTH{1'b0}};
            latched_key <= {ENTRY_WIDTH{1'b0}};
            base_index <= {(INDEX_WIDTH+1){1'b0}};
            elapsed_cycles <= {CYCLE_WIDTH{1'b0}};
        end else begin
            done_o <= 1'b0;
            if (start_i && !busy_o) begin
                busy_o <= 1'b1;
                match_o <= 1'b0;
                match_index_o <= {INDEX_WIDTH{1'b0}};
                search_cycles_o <= {CYCLE_WIDTH{1'b0}};
                latched_key <= search_key_i;
                base_index <= {(INDEX_WIDTH+1){1'b0}};
                elapsed_cycles <= {CYCLE_WIDTH{1'b0}};
            end else if (busy_o) begin
                elapsed_cycles <= elapsed_cycles + 1'b1;
                if (chunk_match) begin
                    busy_o <= 1'b0;
                    done_o <= 1'b1;
                    match_o <= 1'b1;
                    match_index_o <= chunk_match_index;
                    search_cycles_o <= elapsed_cycles + 1'b1;
                end else if (base_index + P_STEP >= ENTRY_LIMIT) begin
                    busy_o <= 1'b0;
                    done_o <= 1'b1;
                    match_o <= 1'b0;
                    search_cycles_o <= elapsed_cycles + 1'b1;
                end else begin
                    base_index <= base_index + P_STEP;
                end
            end
        end
    end

    initial begin
        if (ENTRY_NUM <= 0 || ENTRY_WIDTH <= 0 || P <= 0 || P > ENTRY_NUM)
            $error("pway_search requires positive parameters and P <= ENTRY_NUM");
    end
endmodule

`default_nettype wire
