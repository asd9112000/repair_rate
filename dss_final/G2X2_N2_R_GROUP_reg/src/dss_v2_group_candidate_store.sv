`default_nettype none

// Phase 4F-2 GROUP-NoScratch persistent candidate state.  The packed image is
// four SA banks of three dense analyzer configurations, with
// {PatternID[3:0], valid} per entry.  Action-to-config aliasing is selector logic.
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
    output logic [59:0] candidate_store_image_o
);
    import dss_v2_params_pkg::*;
    import dss_v2_types_pkg::*;

    localparam int unsigned BITS_PER_CANDIDATE = 1 + PATTERN_ID_W;
    localparam int unsigned CANDIDATES_PER_SA = 3;
    localparam int unsigned STORE_BITS = SA_NUM * CANDIDATES_PER_SA * BITS_PER_CANDIDATE;
    logic [STORE_BITS-1:0] store_q;
    logic [5:0] write_offset;
    logic [5:0] read_offset;

    function automatic logic [5:0] candidate_offset(
        input dss_v2_sa_id_t sa,
        input dss_v2_config_index_t slot
    );
        logic [5:0] sa_extended;
        logic [5:0] slot_extended;
        begin
            sa_extended = {4'b0, sa};
            slot_extended = {4'b0, slot};
            // (SA * 3 + dense config) * 5, with a range of 0 through 55.
            candidate_offset = (sa_extended << 3) + (sa_extended << 2) +
                               (sa_extended << 1) + sa_extended +
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
        if (STORE_BITS != 60)
            $fatal(1, "Phase 4F-2 GROUP-NoScratch store must contain exactly 60 bits");
    end
endmodule
`default_nettype wire
