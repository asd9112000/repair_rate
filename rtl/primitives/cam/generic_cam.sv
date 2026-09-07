`default_nettype none

module generic_cam #(
    parameter integer ENTRY_NUM = 4,
    parameter integer KEY_WIDTH = 8,
    parameter integer DATA_WIDTH = 8,
    parameter integer INDEX_WIDTH = (ENTRY_NUM <= 1) ? 1 : $clog2(ENTRY_NUM)
) (
    input  wire                         clk_i,
    input  wire                         rst_ni,
    input  wire                         clear_i,

    input  wire                         write_valid_i,
    input  wire [INDEX_WIDTH-1:0]       write_index_i,
    input  wire [KEY_WIDTH-1:0]         write_key_i,
    input  wire [DATA_WIDTH-1:0]        write_data_i,

    input  wire                         invalidate_valid_i,
    input  wire [INDEX_WIDTH-1:0]       invalidate_index_i,

    input  wire                         read_valid_i,
    input  wire [INDEX_WIDTH-1:0]       read_index_i,
    output reg                          read_entry_valid_o,
    output reg  [KEY_WIDTH-1:0]         read_key_o,
    output reg  [DATA_WIDTH-1:0]        read_data_o,

    input  wire                         search_valid_i,
    input  wire [KEY_WIDTH-1:0]         search_key_i,
    output reg                          search_match_o,
    output reg  [INDEX_WIDTH-1:0]       search_index_o,
    output reg  [DATA_WIDTH-1:0]        search_data_o,
    output reg  [ENTRY_NUM-1:0]         match_vector_o,

    output wire [ENTRY_NUM-1:0]         valid_vector_o,
    output wire [ENTRY_NUM*KEY_WIDTH-1:0] keys_flat_o,
    output wire [ENTRY_NUM*DATA_WIDTH-1:0] data_flat_o
);
    reg [KEY_WIDTH-1:0] key_mem [0:ENTRY_NUM-1];
    reg [DATA_WIDTH-1:0] data_mem [0:ENTRY_NUM-1];
    reg [ENTRY_NUM-1:0] valid_mem;
    // Intentional constant narrowing to the index domain plus one guard bit.
    /* verilator lint_off WIDTH */
    localparam [INDEX_WIDTH:0] ENTRY_LIMIT = ENTRY_NUM;
    /* verilator lint_on WIDTH */
    integer i;

    assign valid_vector_o = valid_mem;

    genvar entry;
    generate
        for (entry = 0; entry < ENTRY_NUM; entry = entry + 1) begin : gen_flatten
            assign keys_flat_o[entry*KEY_WIDTH +: KEY_WIDTH] = key_mem[entry];
            assign data_flat_o[entry*DATA_WIDTH +: DATA_WIDTH] = data_mem[entry];
        end
    endgenerate

    always @* begin
        read_entry_valid_o = 1'b0;
        read_key_o = {KEY_WIDTH{1'b0}};
        read_data_o = {DATA_WIDTH{1'b0}};
        if (read_valid_i && {1'b0, read_index_i} < ENTRY_LIMIT) begin
            read_entry_valid_o = valid_mem[read_index_i];
            read_key_o = key_mem[read_index_i];
            read_data_o = data_mem[read_index_i];
        end
    end

    always @* begin
        search_match_o = 1'b0;
        search_index_o = {INDEX_WIDTH{1'b0}};
        search_data_o = {DATA_WIDTH{1'b0}};
        match_vector_o = {ENTRY_NUM{1'b0}};
        if (search_valid_i) begin
            for (i = 0; i < ENTRY_NUM; i = i + 1) begin
                if (valid_mem[i] && key_mem[i] == search_key_i) begin
                    match_vector_o[i] = 1'b1;
                    if (!search_match_o) begin
                        search_match_o = 1'b1;
                        search_index_o = i[INDEX_WIDTH-1:0];
                        search_data_o = data_mem[i];
                    end
                end
            end
        end
    end

    always @(posedge clk_i) begin
        if (!rst_ni || clear_i) begin
            valid_mem <= {ENTRY_NUM{1'b0}};
        end else begin
            if (invalidate_valid_i &&
                {1'b0, invalidate_index_i} < ENTRY_LIMIT)
                valid_mem[invalidate_index_i] <= 1'b0;
            if (write_valid_i && {1'b0, write_index_i} < ENTRY_LIMIT) begin
                key_mem[write_index_i] <= write_key_i;
                data_mem[write_index_i] <= write_data_i;
                valid_mem[write_index_i] <= 1'b1;
            end
        end
    end

    initial begin
        if (ENTRY_NUM <= 0 || KEY_WIDTH <= 0 || DATA_WIDTH <= 0)
            $error("generic_cam parameters must be positive");
    end
endmodule

`default_nettype wire
