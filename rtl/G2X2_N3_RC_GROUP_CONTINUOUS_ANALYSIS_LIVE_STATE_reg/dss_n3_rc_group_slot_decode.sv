`default_nettype none

// Versioned RS=CS=3, SHARE_M=1 architecture-point table.  T0..T6 are local
// target-table IDs only; they do not reinterpret frozen legacy CFG0..CFG6.
module dss_n3_rc_group_slot_decode (
    input  logic [1:0] sa_i,
    input  logic [1:0] slot_i,
    output logic       valid_o,
    output logic [2:0] config_id_o,
    output logic [2:0] row_count_o,
    output logic [2:0] col_count_o,
    output logic [1:0] canonical_class_o,
    output logic       transpose_o,
    output logic [1:0] action_o,
    output logic       release_required_o,
    output logic       borrow_required_o
);
    localparam logic [1:0] LOCAL = 2'b00;
    localparam logic [1:0] RELEASE_ONLY = 2'b10;
    localparam logic [1:0] BORROW_ONLY = 2'b01;
    localparam logic [1:0] RELEASE_AND_BORROW = 2'b11;

    always_comb begin
        valid_o = 1'b1;
        config_id_o = 3'd0;
        row_count_o = 3'd3;
        col_count_o = 3'd3;
        canonical_class_o = 2'd0;
        transpose_o = 1'b0;
        action_o = LOCAL;
        if (sa_i == 2'd0 || sa_i == 2'd3) begin
            unique case (slot_i)
                2'd0: begin config_id_o = 3'd0; end // T0: 3R3C
                2'd1: begin config_id_o = 3'd1; row_count_o = 3'd2; canonical_class_o = 2'd1; transpose_o = 1'b1; action_o = RELEASE_ONLY; end
                2'd2: begin config_id_o = 3'd2; col_count_o = 3'd4; canonical_class_o = 2'd2; transpose_o = 1'b1; action_o = BORROW_ONLY; end
                2'd3: begin config_id_o = 3'd3; row_count_o = 3'd2; col_count_o = 3'd4; canonical_class_o = 2'd3; transpose_o = 1'b1; action_o = RELEASE_AND_BORROW; end
            endcase
        end else begin
            unique case (slot_i)
                2'd0: begin config_id_o = 3'd0; end // T0: 3R3C
                2'd1: begin config_id_o = 3'd4; col_count_o = 3'd2; canonical_class_o = 2'd1; action_o = RELEASE_ONLY; end
                2'd2: begin config_id_o = 3'd5; row_count_o = 3'd4; canonical_class_o = 2'd2; action_o = BORROW_ONLY; end
                2'd3: begin config_id_o = 3'd6; row_count_o = 3'd4; col_count_o = 3'd2; canonical_class_o = 2'd3; action_o = RELEASE_AND_BORROW; end
            endcase
        end
        release_required_o = action_o[1];
        borrow_required_o = action_o[0];
    end
endmodule
`default_nettype wire
