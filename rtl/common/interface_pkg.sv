package interface_pkg;
    // Common analyzer transaction protocol. The concrete top-level keeps
    // signals scalar/packed so the same RTL is accepted by both Verilator
    // and the intentionally conservative Yosys sanity flow.
    localparam int ANALYZER_PROTOCOL_VERSION = 1;
    localparam int SUBARRAY_COUNT = 4;
endpackage
