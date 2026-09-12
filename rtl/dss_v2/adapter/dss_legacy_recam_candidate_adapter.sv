`default_nettype none

// Converts the legacy scheduler's ConfigPatternMap bundle into four semantic,
// policy-neutral V2 candidate summaries.  No candidate selection is performed.
module dss_legacy_recam_candidate_adapter (
    input dss_v2_types_pkg::dss_v2_sa_id_t sa_id_i,
    input logic sa_valid_i,
    input logic [dss_v2_params_pkg::LEGACY_CONFIG_SLOT_NUM*20-1:0] legacy_config_pattern_map_i,
    input dss_v2_types_pkg::dss_v2_candidate_reconstruction_metadata_t legacy_reconstruction_metadata_i,
    output logic [dss_v2_params_pkg::LEGACY_CONFIG_SLOT_NUM*$bits(dss_v2_types_pkg::dss_v2_candidate_summary_t)-1:0] candidate_summary_flat_o,
    output dss_v2_types_pkg::dss_v2_candidate_reconstruction_metadata_t reconstruction_metadata_o
);
    import dss_v2_params_pkg::*;
    import dss_v2_types_pkg::*;

    localparam int unsigned SUMMARY_W = $bits(dss_v2_candidate_summary_t);
    logic [2:0] legacy_config_id [0:LEGACY_CONFIG_SLOT_NUM-1];
    dss_v2_config_descriptor_t descriptor [0:LEGACY_CONFIG_SLOT_NUM-1];
    logic descriptor_valid [0:LEGACY_CONFIG_SLOT_NUM-1];
    logic [2:0] unused_reverse_config_id [0:LEGACY_CONFIG_SLOT_NUM-1];
    logic unused_reverse_descriptor_valid [0:LEGACY_CONFIG_SLOT_NUM-1];
    logic [1:0] source_slot;
    logic [19:0] legacy_entry;
    dss_v2_candidate_summary_t summary;
    integer index;

    genvar slot;
    generate
        for (slot = 0; slot < LEGACY_CONFIG_SLOT_NUM; slot = slot + 1) begin : legacy_descriptor
            assign legacy_config_id[slot] = legacy_config_pattern_map_i[slot*20 + 3 -: 3];
            dss_legacy_config_adapter config_adapter (
                .legacy_config_id_i(legacy_config_id[slot]), .config_descriptor_o(descriptor[slot]),
                .legacy_config_valid_o(descriptor_valid[slot]), .config_descriptor_i('0),
                .legacy_config_id_o(unused_reverse_config_id[slot]),
                .descriptor_valid_o(unused_reverse_descriptor_valid[slot])
            );
        end
    endgenerate

    always_comb begin
        candidate_summary_flat_o = '0;
        reconstruction_metadata_o = legacy_reconstruction_metadata_i;
        for (index = 0; index < LEGACY_CONFIG_SLOT_NUM; index = index + 1) begin
            // Stable V2 enumeration: A/D use 0,4,5,6; B/C use ascending 0,1,2,3.
            if (sa_id_i == 2'd1 || sa_id_i == 2'd2) begin
                case (index)
                    0: source_slot = 0; // CFG0
                    1: source_slot = 2; // scheduler slot CFG1
                    2: source_slot = 1; // scheduler slot CFG2
                    default: source_slot = 3; // CFG3
                endcase
            end else begin
                source_slot = index[1:0];
            end
            legacy_entry = legacy_config_pattern_map_i[source_slot*20 +: 20];
            summary = '0;
            summary.local_candidate_valid = sa_valid_i && legacy_entry[0] && legacy_entry[4] &&
                                            legacy_entry[5] && descriptor_valid[source_slot];
            summary.config_descriptor = descriptor[source_slot];
            summary.pattern_id = legacy_entry[9:6];
            summary.pattern_valid_bitmap = legacy_entry[19:10];
            summary.canonical_index = index[CONFIG_INDEX_W-1:0];
            summary.legacy_config_id_debug = legacy_entry[3:1];
            candidate_summary_flat_o[index*SUMMARY_W +: SUMMARY_W] = summary;
        end
    end
endmodule
`default_nettype wire
