`default_nettype none

module sync_fifo #(
    parameter integer DEPTH = 4,
    parameter integer DATA_WIDTH = 16,
    parameter integer PTR_WIDTH = (DEPTH <= 1) ? 1 : $clog2(DEPTH),
    parameter integer COUNT_WIDTH = (DEPTH <= 1) ? 1 : $clog2(DEPTH + 1)
) (
    input  wire                    clk_i,
    input  wire                    rst_ni,
    input  wire                    clear_i,
    input  wire                    push_valid_i,
    output wire                    push_ready_o,
    input  wire [DATA_WIDTH-1:0]   push_data_i,
    output wire                    pop_valid_o,
    input  wire                    pop_ready_i,
    output wire [DATA_WIDTH-1:0]   pop_data_o,
    output wire                    full_o,
    output wire                    empty_o,
    output reg  [COUNT_WIDTH-1:0]  count_o
);
    reg [DATA_WIDTH-1:0] data_mem [0:DEPTH-1];
    reg [PTR_WIDTH-1:0] read_ptr;
    reg [PTR_WIDTH-1:0] write_ptr;
    // DEPTH is range-checked below; these constants intentionally use the
    // minimum counter/pointer widths selected by the parameters.
    /* verilator lint_off WIDTH */
    localparam [COUNT_WIDTH-1:0] DEPTH_COUNT = DEPTH;
    localparam [PTR_WIDTH-1:0] LAST_PTR = DEPTH - 1;
    /* verilator lint_on WIDTH */
    wire do_pop;
    wire do_push;

    assign empty_o = count_o == 0;
    assign full_o = count_o == DEPTH_COUNT;
    assign pop_valid_o = !empty_o;
    assign pop_data_o = data_mem[read_ptr];
    assign do_pop = pop_valid_o && pop_ready_i;
    assign push_ready_o = !full_o || do_pop;
    assign do_push = push_valid_i && push_ready_o;

    function [PTR_WIDTH-1:0] increment_ptr;
        input [PTR_WIDTH-1:0] pointer;
        begin
            if (pointer == LAST_PTR)
                increment_ptr = {PTR_WIDTH{1'b0}};
            else
                increment_ptr = pointer + 1'b1;
        end
    endfunction

    always @(posedge clk_i) begin
        if (!rst_ni || clear_i) begin
            read_ptr <= {PTR_WIDTH{1'b0}};
            write_ptr <= {PTR_WIDTH{1'b0}};
            count_o <= {COUNT_WIDTH{1'b0}};
        end else begin
            if (do_push) begin
                data_mem[write_ptr] <= push_data_i;
                write_ptr <= increment_ptr(write_ptr);
            end
            if (do_pop)
                read_ptr <= increment_ptr(read_ptr);
            case ({do_push, do_pop})
                2'b10: count_o <= count_o + 1'b1;
                2'b01: count_o <= count_o - 1'b1;
                default: count_o <= count_o;
            endcase
        end
    end

    initial begin
        if (DEPTH <= 0 || DATA_WIDTH <= 0)
            $error("sync_fifo parameters must be positive");
    end
endmodule

`default_nettype wire
