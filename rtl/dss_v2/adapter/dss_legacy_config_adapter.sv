`default_nettype none

// Phase 4A compatibility boundary.  It translates only the frozen DATE
// ConfigID capacity meaning; it contains no policy, topology, or donor logic.
module dss_legacy_config_adapter (
    input wire [2:0] legacy_config_id_i,
    output dss_v2_types_pkg::dss_v2_config_descriptor_t config_descriptor_o,
    output logic legacy_config_valid_o,
    input dss_v2_types_pkg::dss_v2_config_descriptor_t config_descriptor_i,
    output logic [2:0] legacy_config_id_o,
    output logic descriptor_valid_o
);
    import dss_v2_params_pkg::*;
    import dss_v2_types_pkg::*;

    always_comb begin
        config_descriptor_o = '0;
        legacy_config_valid_o = 1'b1;
        case (legacy_config_id_i)
            3'd0: begin config_descriptor_o.row_count = 2; config_descriptor_o.col_count = 2; end
            3'd1: begin config_descriptor_o.row_count = 2; config_descriptor_o.col_count = 1; end
            3'd2: begin config_descriptor_o.row_count = 3; config_descriptor_o.col_count = 2; end
            3'd3: begin config_descriptor_o.row_count = 3; config_descriptor_o.col_count = 1; end
            3'd4: begin config_descriptor_o.row_count = 1; config_descriptor_o.col_count = 2; end
            3'd5: begin config_descriptor_o.row_count = 2; config_descriptor_o.col_count = 3; end
            3'd6: begin config_descriptor_o.row_count = 1; config_descriptor_o.col_count = 3; end
            default: legacy_config_valid_o = 1'b0; // CFG7 is explicitly invalid.
        endcase

        legacy_config_id_o = 3'd7;
        descriptor_valid_o = 1'b1;
        unique case ({config_descriptor_i.row_count, config_descriptor_i.col_count})
            {3'd2, 3'd2}: legacy_config_id_o = 3'd0;
            {3'd2, 3'd1}: legacy_config_id_o = 3'd1;
            {3'd3, 3'd2}: legacy_config_id_o = 3'd2;
            {3'd3, 3'd1}: legacy_config_id_o = 3'd3;
            {3'd1, 3'd2}: legacy_config_id_o = 3'd4;
            {3'd2, 3'd3}: legacy_config_id_o = 3'd5;
            {3'd1, 3'd3}: legacy_config_id_o = 3'd6;
            default: descriptor_valid_o = 1'b0;
        endcase
    end

    initial begin
        if (SA_NUM == 0 || ROW_ADDR_W == 0 || COL_ADDR_W == 0 || MAX_K == 0 ||
            PATTERN_ID_W == 0 || CONFIG_COUNT_W == 0)
            $fatal(1, "dss_v2 parameter package has an invalid zero-width/default setting");
    end
endmodule
