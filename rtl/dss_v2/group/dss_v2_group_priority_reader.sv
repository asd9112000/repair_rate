`default_nettype none

// Pure read-side GROUP priority view.  It deliberately has no ledger input,
// physical-feasibility evaluation, selection state, or commit side effect.
module dss_v2_group_priority_reader (
    input  dss_v2_types_pkg::dss_v2_sa_id_t sa_id_i,
    input  logic [79:0] candidate_store_image_i,
    output logic [7:0] priority_slot_flat_o,
    output logic [3:0] priority_valid_o,
    output logic [15:0] priority_pattern_id_flat_o
);
    import dss_v2_params_pkg::*;

    localparam int unsigned BITS_PER_CANDIDATE = 1 + PATTERN_ID_W;
    localparam int unsigned BITS_PER_SA = LEGACY_CONFIG_SLOT_NUM * BITS_PER_CANDIDATE;
    logic [BITS_PER_SA-1:0] selected_sa_store;
    logic [1:0] slot_for_rank;
    integer rank;
    integer offset;

    always_comb begin
        selected_sa_store = candidate_store_image_i[sa_id_i * BITS_PER_SA +: BITS_PER_SA];
        priority_slot_flat_o = '0;
        priority_valid_o = '0;
        priority_pattern_id_flat_o = '0;
        for (rank = 0; rank < LEGACY_CONFIG_SLOT_NUM; rank = rank + 1) begin
            case (rank)
                0: slot_for_rank = 2'd1;
                1: slot_for_rank = 2'd0;
                2: slot_for_rank = 2'd3;
                default: slot_for_rank = 2'd2;
            endcase
            offset = slot_for_rank * BITS_PER_CANDIDATE;
            priority_slot_flat_o[rank*2 +: 2] = slot_for_rank;
            priority_valid_o[rank] = selected_sa_store[offset];
            priority_pattern_id_flat_o[rank*PATTERN_ID_W +: PATTERN_ID_W] =
                selected_sa_store[offset + 1 +: PATTERN_ID_W];
        end
    end
endmodule
`default_nettype wire
