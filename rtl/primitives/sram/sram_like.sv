`default_nettype none

module sram_like #(
    parameter integer ENTRY_NUM = 8,
    parameter integer DATA_WIDTH = 16,
    parameter integer ADDR_WIDTH = (ENTRY_NUM <= 1) ? 1 : $clog2(ENTRY_NUM)
) (
    input  wire                   clk_i,
    input  wire                   rst_ni,
    input  wire                   clear_i,
    input  wire                   write_valid_i,
    input  wire [ADDR_WIDTH-1:0]  write_addr_i,
    input  wire [DATA_WIDTH-1:0]  write_data_i,
    input  wire                   write_entry_valid_i,
    input  wire                   read_valid_i,
    input  wire [ADDR_WIDTH-1:0]  read_addr_i,
    output reg                    read_data_valid_o,
    output reg  [DATA_WIDTH-1:0]  read_data_o,
    output reg                    read_entry_valid_o
);
    reg [DATA_WIDTH-1:0] data_mem [0:ENTRY_NUM-1];
    reg [ENTRY_NUM-1:0] valid_mem;
    // Intentional constant narrowing to the address domain plus one guard bit.
    /* verilator lint_off WIDTH */
    localparam [ADDR_WIDTH:0] ENTRY_LIMIT = ENTRY_NUM;
    /* verilator lint_on WIDTH */

    always @(posedge clk_i) begin
        if (!rst_ni || clear_i) begin
            valid_mem <= {ENTRY_NUM{1'b0}};
            read_data_valid_o <= 1'b0;
            read_data_o <= {DATA_WIDTH{1'b0}};
            read_entry_valid_o <= 1'b0;
        end else begin
            read_data_valid_o <= read_valid_i &&
                {1'b0, read_addr_i} < ENTRY_LIMIT;
            if (read_valid_i && {1'b0, read_addr_i} < ENTRY_LIMIT) begin
                read_data_o <= data_mem[read_addr_i];
                read_entry_valid_o <= valid_mem[read_addr_i];
            end else begin
                read_entry_valid_o <= 1'b0;
            end
            if (write_valid_i && {1'b0, write_addr_i} < ENTRY_LIMIT) begin
                data_mem[write_addr_i] <= write_data_i;
                valid_mem[write_addr_i] <= write_entry_valid_i;
            end
        end
    end

    initial begin
        if (ENTRY_NUM <= 0 || DATA_WIDTH <= 0)
            $error("sram_like parameters must be positive");
    end
endmodule

`default_nettype wire
