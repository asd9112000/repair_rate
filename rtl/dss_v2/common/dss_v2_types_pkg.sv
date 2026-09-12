`default_nettype none

package dss_v2_types_pkg;
    import dss_v2_params_pkg::*;

    typedef enum logic {DSS_LAYOUT_GRID_2X2, DSS_LAYOUT_LINE_1X4} dss_v2_layout_e;
    typedef enum logic [2:0] {DSS_SHARE_NONE, DSS_SHARE_EDGE, DSS_SHARE_NEIGHBOR,
                              DSS_SHARE_DIRECTIONAL, DSS_SHARE_GLOBAL} dss_v2_sharing_policy_e;
    typedef enum logic {DSS_TAKE_EARLY, DSS_TAKE_GROUP} dss_v2_solution_take_policy_e;
    typedef enum logic {DSS_CONFIG_CHECK_SERIAL, DSS_CONFIG_CHECK_PARALLEL} dss_v2_config_check_arch_e;
    typedef enum logic {DSS_NO_SCRATCH, DSS_EXTERNAL_SCRATCH} dss_v2_scratch_mode_e;
    typedef enum logic {DSS_MEMORY_CAM, DSS_MEMORY_SRAM_CHUNKED} dss_v2_memory_impl_e;

    typedef logic [SA_ID_W-1:0] dss_v2_sa_id_t;
    typedef logic [RESOURCE_ID_W-1:0] dss_v2_resource_id_t;
    typedef logic [CONFIG_INDEX_W-1:0] dss_v2_config_index_t;
    typedef logic [PATTERN_ID_W-1:0] dss_v2_pattern_id_t;

    // Kept numerically identical to the Phase 3J ledger so the adapter
    // boundary does not reinterpret physical resource ownership.
    typedef enum logic [RESOURCE_ID_W-1:0] {
        DSS_RESOURCE_A_ROW = 2'd0,
        DSS_RESOURCE_D_ROW = 2'd1,
        DSS_RESOURCE_B_COL = 2'd2,
        DSS_RESOURCE_C_COL = 2'd3
    } dss_v2_resource_id_e;

    // Capacity semantics only: no legacy ConfigID numbering is encoded here.
    typedef struct packed {
        logic [CONFIG_COUNT_W-1:0] row_count;
        logic [CONFIG_COUNT_W-1:0] col_count;
    } dss_v2_config_descriptor_t;

    // A locally valid ConfigDescriptor candidate. legacy_config_id_debug is
    // trace-only metadata and is not a V2 policy input.
    typedef struct packed {
        logic local_candidate_valid;
        dss_v2_config_descriptor_t config_descriptor;
        dss_v2_pattern_id_t pattern_id;
        logic [LEGACY_CANDIDATE_VALID_W-1:0] pattern_valid_bitmap;
        dss_v2_config_index_t canonical_index;
        logic [2:0] legacy_config_id_debug;
    } dss_v2_candidate_summary_t;

    // Preserved legacy analyzer context.  Phase 4D neither stores nor
    // interprets it as a policy input; later reconstruction must retain it.
    typedef struct packed {
        logic [LEGACY_PIVOT_NUM-1:0] pivot_valid;
        logic [LEGACY_PIVOT_NUM*ROW_ADDR_W-1:0] pivot_rows_flat;
        logic [LEGACY_PIVOT_NUM*COL_ADDR_W-1:0] pivot_cols_flat;
        logic [LEGACY_HYBRID_ENTRIES-1:0] hybrid_valid;
        logic [LEGACY_HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat;
        logic [LEGACY_HYBRID_ENTRIES-1:0] hybrid_descriptor;
        logic [LEGACY_HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat;
        logic conventional_overflow;
    } dss_v2_candidate_reconstruction_metadata_t;
endpackage
