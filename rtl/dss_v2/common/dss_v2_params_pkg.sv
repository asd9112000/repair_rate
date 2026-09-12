`default_nettype none

// Phase 4A V2 defaults.  These describe the verified DATE point only; their
// presence does not claim that arbitrary parameter values are functionally
// supported by later V2 implementations.
package dss_v2_params_pkg;
    parameter int unsigned SA_NUM = 4;
    parameter int unsigned RS = 2;
    parameter int unsigned CS = 2;
    parameter int unsigned SHARE_M = 1;
    parameter int unsigned ROW_ADDR_W = 9;
    parameter int unsigned COL_ADDR_W = 5;

    // Explicit default: it equals RS+CS+SHARE_M only at the DATE point.
    parameter int unsigned MAX_K = 5;
    parameter int unsigned RESOURCE_NUM = 4;
    parameter int unsigned PATTERN_ID_W = 4;

    // Explicit Phase 4D legacy-adapter interface bounds.  These describe the
    // frozen DATE analyzer bundle; they are not claims of generic RECAM size.
    parameter int unsigned LEGACY_CONFIG_SLOT_NUM = 4;
    parameter int unsigned LEGACY_CANDIDATE_VALID_W = 10;
    parameter int unsigned LEGACY_PIVOT_NUM = 5;
    parameter int unsigned LEGACY_HYBRID_ENTRIES = 7;
    parameter int unsigned DIFF_ADDR_W = 9;

    localparam int unsigned SA_ID_W = (SA_NUM > 1) ? $clog2(SA_NUM) : 1;
    localparam int unsigned RESOURCE_ID_W = (RESOURCE_NUM > 1) ? $clog2(RESOURCE_NUM) : 1;
    localparam int unsigned CONFIG_INDEX_W = 2; // Four legacy role-map entries.
    localparam int unsigned CONFIG_COUNT_W = (MAX_K > 1) ? $clog2(MAX_K + 1) : 1;
endpackage
