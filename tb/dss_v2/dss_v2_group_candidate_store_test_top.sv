`default_nettype none
module dss_v2_group_candidate_store_test_top (
    input logic clk_i, rst_ni, write_enable_i,
    input logic [1:0] write_sa_i, write_slot_i,
    input logic write_candidate_valid_i,
    input logic [3:0] write_pattern_id_i,
    input logic [1:0] read_sa_i, read_slot_i,
    output logic read_candidate_valid_o,
    output logic [3:0] read_pattern_id_o,
    output logic [79:0] candidate_store_image_o,
    input logic [1:0] decode_sa_i, decode_slot_i,
    output logic [2:0] decoded_config_id_o,
    output dss_v2_types_pkg::dss_v2_config_descriptor_t decoded_descriptor_o,
    input logic [1:0] priority_sa_i,
    output logic [7:0] priority_slot_flat_o,
    output logic [3:0] priority_valid_o,
    output logic [15:0] priority_pattern_id_flat_o
);
    dss_v2_group_candidate_store store (.*);
    dss_v2_group_slot_decode decode (
        .sa_id_i(decode_sa_i), .canonical_slot_i(decode_slot_i),
        .legacy_config_id_o(decoded_config_id_o), .config_descriptor_o(decoded_descriptor_o)
    );
    dss_v2_group_priority_reader priority_reader (
        .sa_id_i(priority_sa_i), .candidate_store_image_i(candidate_store_image_o),
        .priority_slot_flat_o(priority_slot_flat_o), .priority_valid_o(priority_valid_o),
        .priority_pattern_id_flat_o(priority_pattern_id_flat_o)
    );
endmodule
`default_nettype wire
