package types_pkg;
    typedef enum logic [1:0] {
        REPAIR_NONE   = 2'b00,
        REPAIR_ROW    = 2'b01,
        REPAIR_COLUMN = 2'b10,
        REPAIR_REUSE  = 2'b11
    } repair_kind_t;

    typedef enum logic [1:0] {
        TOPOLOGY_NONE        = 2'd0,
        TOPOLOGY_DIRECTIONAL = 2'd1,
        TOPOLOGY_EDGE        = 2'd2
    } dss_topology_t;
endpackage
