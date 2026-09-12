`default_nettype none

// Stateless DATE 2x2 directional topology interpretation.  This module
// contains only role legality, resource action shape, and donor priority.
module dss_topology_2x2_directional (
    input  dss_v2_types_pkg::dss_v2_sa_id_t sa_id_i,
    input  logic sa_valid_i,
    input  dss_v2_types_pkg::dss_v2_config_descriptor_t config_descriptor_i,
    output logic descriptor_legal_o,
    output logic borrow_required_o,
    output logic release_required_o,
    output logic release_resource_valid_o,
    output dss_v2_types_pkg::dss_v2_resource_id_t release_resource_o,
    output dss_v2_types_pkg::dss_v2_resource_id_t donor_primary_o,
    output dss_v2_types_pkg::dss_v2_resource_id_t donor_secondary_o
);
    import dss_v2_params_pkg::*;
    import dss_v2_types_pkg::*;

    logic is_row_role;
    logic supported_descriptor;
    localparam logic [CONFIG_COUNT_W-1:0] RS_COUNT = CONFIG_COUNT_W'(RS);
    localparam logic [CONFIG_COUNT_W-1:0] CS_COUNT = CONFIG_COUNT_W'(CS);
    localparam logic [CONFIG_COUNT_W-1:0] SHARE_M_COUNT = CONFIG_COUNT_W'(SHARE_M);

    always_comb begin
        descriptor_legal_o = 1'b0;
        borrow_required_o = 1'b0;
        release_required_o = 1'b0;
        release_resource_valid_o = 1'b0;
        release_resource_o = DSS_RESOURCE_A_ROW;
        donor_primary_o = DSS_RESOURCE_A_ROW;
        donor_secondary_o = DSS_RESOURCE_D_ROW;
        is_row_role = 1'b0;
        supported_descriptor = 1'b0;

        case (sa_id_i)
            2'd0: begin // A: releases A_ROW; B_COL then C_COL.
                is_row_role = 1'b1;
                release_resource_o = DSS_RESOURCE_A_ROW;
                donor_primary_o = DSS_RESOURCE_B_COL;
                donor_secondary_o = DSS_RESOURCE_C_COL;
            end
            2'd3: begin // D: releases D_ROW; C_COL then B_COL.
                is_row_role = 1'b1;
                release_resource_o = DSS_RESOURCE_D_ROW;
                donor_primary_o = DSS_RESOURCE_C_COL;
                donor_secondary_o = DSS_RESOURCE_B_COL;
            end
            2'd1: begin // B: releases B_COL; A_ROW then D_ROW.
                release_resource_o = DSS_RESOURCE_B_COL;
                donor_primary_o = DSS_RESOURCE_A_ROW;
                donor_secondary_o = DSS_RESOURCE_D_ROW;
            end
            2'd2: begin // C: releases C_COL; D_ROW then A_ROW.
                release_resource_o = DSS_RESOURCE_C_COL;
                donor_primary_o = DSS_RESOURCE_D_ROW;
                donor_secondary_o = DSS_RESOURCE_A_ROW;
            end
            default: begin end
        endcase

        if (sa_valid_i) begin
            if (is_row_role) begin
                // DATE A/D legacy envelopes: (2,2), (1,2), (2,3), (1,3).
                supported_descriptor =
                    ((config_descriptor_i.row_count == RS_COUNT) ||
                     (config_descriptor_i.row_count + 1'b1 == RS_COUNT)) &&
                    ((config_descriptor_i.col_count == CS_COUNT) ||
                     (config_descriptor_i.col_count == CS_COUNT + SHARE_M_COUNT));
                if (supported_descriptor) begin
                    descriptor_legal_o = 1'b1;
                    release_required_o = (config_descriptor_i.row_count < RS_COUNT);
                    borrow_required_o = (config_descriptor_i.col_count > CS_COUNT);
                end
            end else begin
                // DATE B/C legacy envelopes: (2,2), (2,1), (3,2), (3,1).
                supported_descriptor =
                    ((config_descriptor_i.row_count == RS_COUNT) ||
                     (config_descriptor_i.row_count == RS_COUNT + SHARE_M_COUNT)) &&
                    ((config_descriptor_i.col_count == CS_COUNT) ||
                     (config_descriptor_i.col_count + 1'b1 == CS_COUNT));
                if (supported_descriptor) begin
                    descriptor_legal_o = 1'b1;
                    release_required_o = (config_descriptor_i.col_count < CS_COUNT);
                    borrow_required_o = (config_descriptor_i.row_count > RS_COUNT);
                end
            end
            release_resource_valid_o = descriptor_legal_o && release_required_o;
        end
    end

    initial begin
        if (SA_NUM != 4 || RS == 0 || CS == 0 || SHARE_M != 1 || RESOURCE_NUM != 4)
            $fatal(1, "Phase 4B supports only the frozen DATE 2x2 directional point");
    end
endmodule
`default_nettype wire
