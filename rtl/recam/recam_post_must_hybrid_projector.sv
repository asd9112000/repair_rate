`default_nettype none

// Stable per-Config compaction of the historical physical Hybrid store.  The
// projector is combinational: it does not retain collector state or alter
// physical-store occupancy.
module recam_post_must_hybrid_projector #(
    parameter integer ROW_ADDR_W = 9,
    parameter integer COL_ADDR_W = 5,
    parameter integer DIFF_ADDR_W = 9,
    parameter integer MAX_K = 5,
    parameter integer NUM_CONFIGS = 7,
    parameter integer PHYSICAL_HYBRID_ENTRIES = 14,
    parameter integer POST_MUST_VIEW_ENTRIES = 9
) (
    input  wire [2:0] config_id_i,
    input  wire [PHYSICAL_HYBRID_ENTRIES-1:0] physical_hybrid_valid_i,
    input  wire [PHYSICAL_HYBRID_ENTRIES*3-1:0] physical_hybrid_pointer_flat_i,
    input  wire [PHYSICAL_HYBRID_ENTRIES-1:0] physical_hybrid_descriptor_i,
    input  wire [PHYSICAL_HYBRID_ENTRIES*DIFF_ADDR_W-1:0] physical_hybrid_differing_flat_i,
    input  wire [PHYSICAL_HYBRID_ENTRIES*NUM_CONFIGS-1:0] physical_hybrid_cfg_valid_flat_i,
    input  wire [NUM_CONFIGS*MAX_K-1:0] row_must_by_cfg_i,
    input  wire [NUM_CONFIGS*MAX_K-1:0] col_must_by_cfg_i,
    output reg  [POST_MUST_VIEW_ENTRIES-1:0] projected_hybrid_valid_o,
    output reg  [POST_MUST_VIEW_ENTRIES*3-1:0] projected_hybrid_pointer_flat_o,
    output reg  [POST_MUST_VIEW_ENTRIES-1:0] projected_hybrid_descriptor_o,
    output reg  [POST_MUST_VIEW_ENTRIES*DIFF_ADDR_W-1:0] projected_hybrid_differing_flat_o,
    output reg  [3:0] projected_count_o,
    output reg        projection_overflow_o
);
    integer physical_index;
    integer config_index;
    integer pointer_index;
    reg [2:0] physical_pointer;
    reg physical_config_valid;
    reg physical_pointer_legal;
    reg final_must_retired;
    reg physical_survives;

    always @* begin
        projected_hybrid_valid_o = {POST_MUST_VIEW_ENTRIES{1'b0}};
        projected_hybrid_pointer_flat_o = {POST_MUST_VIEW_ENTRIES*3{1'b0}};
        projected_hybrid_descriptor_o = {POST_MUST_VIEW_ENTRIES{1'b0}};
        projected_hybrid_differing_flat_o = {POST_MUST_VIEW_ENTRIES*DIFF_ADDR_W{1'b0}};
        projected_count_o = 4'd0;
        projection_overflow_o = 1'b0;
        config_index = {29'd0, config_id_i};

        for (physical_index = 0; physical_index < PHYSICAL_HYBRID_ENTRIES;
             physical_index = physical_index + 1) begin
            physical_pointer = physical_hybrid_pointer_flat_i[physical_index*3 +: 3];
            pointer_index = {29'd0, physical_pointer};
            physical_config_valid = (config_index < NUM_CONFIGS) &&
                physical_hybrid_cfg_valid_flat_i[physical_index*NUM_CONFIGS + config_index];

            // This is the exact historical consumer check.  Config-local
            // membership is represented by the stored cfg_valid bit above.
            physical_pointer_legal = pointer_index < MAX_K;
            final_must_retired = 1'b0;
            if (physical_pointer_legal && (config_index < NUM_CONFIGS)) begin
                if (physical_hybrid_descriptor_i[physical_index])
                    final_must_retired = col_must_by_cfg_i[config_index*MAX_K + pointer_index];
                else
                    final_must_retired = row_must_by_cfg_i[config_index*MAX_K + pointer_index];
            end

            physical_survives = physical_hybrid_valid_i[physical_index] &&
                physical_config_valid && physical_pointer_legal && !final_must_retired;
            if (physical_survives) begin
                if (projected_count_o < 4'd9) begin
                    projected_hybrid_valid_o[projected_count_o] = 1'b1;
                    projected_hybrid_pointer_flat_o[projected_count_o*3 +: 3] = physical_pointer;
                    projected_hybrid_descriptor_o[projected_count_o] =
                        physical_hybrid_descriptor_i[physical_index];
                    projected_hybrid_differing_flat_o[projected_count_o*DIFF_ADDR_W +: DIFF_ADDR_W] =
                        physical_hybrid_differing_flat_i[physical_index*DIFF_ADDR_W +: DIFF_ADDR_W];
                end else begin
                    // An explicit failure indicator prevents an illegal input
                    // state from being silently treated as a nine-entry view.
                    projection_overflow_o = 1'b1;
                end
                projected_count_o = projected_count_o + 1'b1;
            end
        end
    end

`ifndef SYNTHESIS
    always @* begin
        if (projection_overflow_o)
            $error("post-Must analyzer view exceeds POST_MUST_VIEW_ENTRIES");
    end
`endif
endmodule

`default_nettype wire
