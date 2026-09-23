`default_nettype none

// Phase 4F-2 GROUP-NoScratch persistent candidate state.  The packed image is
// four SA banks of four canonical slots, with {PatternID[3:0], valid} per slot.
module dss_v2_group_candidate_store (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic write_enable_i,
    input  dss_v2_types_pkg::dss_v2_sa_id_t write_sa_i,
    input  dss_v2_types_pkg::dss_v2_config_index_t write_slot_i,
    input  logic write_candidate_valid_i,
    input  dss_v2_types_pkg::dss_v2_pattern_id_t write_pattern_id_i,
    input  dss_v2_types_pkg::dss_v2_sa_id_t read_sa_i,
    input  dss_v2_types_pkg::dss_v2_config_index_t read_slot_i,
    output logic read_candidate_valid_o,
    output dss_v2_types_pkg::dss_v2_pattern_id_t read_pattern_id_o,
    output logic [79:0] candidate_store_image_o
);
    import dss_v2_params_pkg::*;
    import dss_v2_types_pkg::*;

    localparam int unsigned BITS_PER_CANDIDATE = 1 + PATTERN_ID_W;
    localparam int unsigned CANDIDATES_PER_SA = LEGACY_CONFIG_SLOT_NUM;
    localparam int unsigned STORE_BITS = SA_NUM * CANDIDATES_PER_SA * BITS_PER_CANDIDATE;
    logic [STORE_BITS-1:0] store_q;
    logic [6:0] write_offset;
    logic [6:0] read_offset;

    function automatic logic [6:0] candidate_offset(
        input dss_v2_sa_id_t sa,
        input dss_v2_config_index_t slot
    );
        logic [6:0] sa_extended;
        logic [6:0] slot_extended;
        begin
            sa_extended = {5'b0, sa};
            slot_extended = {5'b0, slot};
            // (SA * 4 + slot) * 5, with a range of 0 through 75.
            candidate_offset = (sa_extended << 4) + (sa_extended << 2) +
                               (slot_extended << 2) + slot_extended;
        end
    endfunction

    always_comb begin
        write_offset = candidate_offset(write_sa_i, write_slot_i);
        read_offset = candidate_offset(read_sa_i, read_slot_i);
        read_candidate_valid_o = store_q[read_offset];
        read_pattern_id_o = store_q[read_offset + 1 +: PATTERN_ID_W];
    end

    always_ff @(posedge clk_i) begin
        if (!rst_ni) begin
            store_q <= '0;
        end else if (write_enable_i) begin
            // Clearing invalid payloads makes reset/invalid images deterministic.
            if (write_candidate_valid_i)
                store_q[write_offset +: BITS_PER_CANDIDATE] <= {write_pattern_id_i, 1'b1};
            else
                store_q[write_offset +: BITS_PER_CANDIDATE] <= '0;
        end
    end

    assign candidate_store_image_o = store_q;

    initial begin
        if (STORE_BITS != 80)
            $fatal(1, "Phase 4F-2 GROUP-NoScratch store must contain exactly 80 bits");
    end
endmodule
`default_nettype wire
