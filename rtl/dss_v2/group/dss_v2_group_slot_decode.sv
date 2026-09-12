`default_nettype none

// Canonical GROUP slot identity is static at the frozen DATE 2x2 point.
// ConfigID and descriptor are combinationally derived, never stored.
module dss_v2_group_slot_decode (
    input  dss_v2_types_pkg::dss_v2_sa_id_t sa_id_i,
    input  dss_v2_types_pkg::dss_v2_config_index_t canonical_slot_i,
    output logic [2:0] legacy_config_id_o,
    output dss_v2_types_pkg::dss_v2_config_descriptor_t config_descriptor_o
);
    import dss_v2_types_pkg::*;

    always_comb begin
        legacy_config_id_o = 3'd0;
        if (sa_id_i == 2'd1 || sa_id_i == 2'd2) begin
            case (canonical_slot_i)
                2'd0: legacy_config_id_o = 3'd0;
                2'd1: legacy_config_id_o = 3'd1;
                2'd2: legacy_config_id_o = 3'd2;
                default: legacy_config_id_o = 3'd3;
            endcase
        end else begin
            case (canonical_slot_i)
                2'd0: legacy_config_id_o = 3'd0;
                2'd1: legacy_config_id_o = 3'd4;
                2'd2: legacy_config_id_o = 3'd5;
                default: legacy_config_id_o = 3'd6;
            endcase
        end
        config_descriptor_o = '0;
        case (legacy_config_id_o)
            3'd0: begin config_descriptor_o.row_count = 2; config_descriptor_o.col_count = 2; end
            3'd1: begin config_descriptor_o.row_count = 2; config_descriptor_o.col_count = 1; end
            3'd2: begin config_descriptor_o.row_count = 3; config_descriptor_o.col_count = 2; end
            3'd3: begin config_descriptor_o.row_count = 3; config_descriptor_o.col_count = 1; end
            3'd4: begin config_descriptor_o.row_count = 1; config_descriptor_o.col_count = 2; end
            3'd5: begin config_descriptor_o.row_count = 2; config_descriptor_o.col_count = 3; end
            default: begin config_descriptor_o.row_count = 1; config_descriptor_o.col_count = 3; end
        endcase
    end
endmodule
`default_nettype wire
