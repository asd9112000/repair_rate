`default_nettype none

// 4 SA × 4 canonical slots × {valid, PatternID[5:0]} = 112 bits.
module dss_v2_rs3cs3m1_group_candidate_store (
    input logic clk_i, rst_ni, write_enable_i,
    input logic [1:0] write_sa_i, write_slot_i,
    input logic write_valid_i,
    input logic [5:0] write_pattern_id_i,
    input logic [1:0] read_sa_i, read_slot_i,
    output logic read_valid_o,
    output logic [5:0] read_pattern_id_o,
    output logic [111:0] image_o
);
    logic [111:0] store_q;
    logic [6:0] write_offset, read_offset;
    logic [6:0] write_sa_extended, write_slot_extended;
    logic [6:0] read_sa_extended, read_slot_extended;
    always_comb begin
        write_sa_extended = {5'b0, write_sa_i};
        write_slot_extended = {5'b0, write_slot_i};
        read_sa_extended = {5'b0, read_sa_i};
        read_slot_extended = {5'b0, read_slot_i};
        write_offset = ((write_sa_extended << 2) + write_slot_extended) * 7;
        read_offset = ((read_sa_extended << 2) + read_slot_extended) * 7;
        read_valid_o = store_q[read_offset];
        read_pattern_id_o = store_q[read_offset + 1 +: 6];
    end
    always_ff @(posedge clk_i) begin
        if (!rst_ni) store_q <= '0;
        else if (write_enable_i) begin
            if (write_valid_i) store_q[write_offset +: 7] <= {write_pattern_id_i, 1'b1};
            else store_q[write_offset +: 7] <= '0;
        end
    end
    assign image_o = store_q;
    initial if ($bits(store_q) != 112) $fatal(1, "RS3CS3M1 GROUP history must be 112 bits");
endmodule
`default_nettype wire
