package dss_pkg;
    localparam int NUM_SA = 4;
    localparam int NUM_CFG = 7;
    localparam int NUM_CANONICAL = 4;
    localparam int MAX_K = 5;
    localparam int CONFIG_ID_W = 3;
    localparam int PATTERN_ID_W = 4;
    localparam int PIVOT_PTR_W = 3;

    typedef enum logic [CONFIG_ID_W-1:0] {
        CFG_2R2C = 3'd0,
        CFG_2R1C = 3'd1,
        CFG_3R2C = 3'd2,
        CFG_3R1C = 3'd3,
        CFG_1R2C = 3'd4,
        CFG_2R3C = 3'd5,
        CFG_1R3C = 3'd6
    } config_id_t;

    typedef enum logic [1:0] {
        CAN_2R2C = 2'd0,
        CAN_2R1C = 2'd1,
        CAN_3R2C = 2'd2,
        CAN_3R1C = 2'd3
    } canonical_id_t;

    function automatic int config_rows(input logic [CONFIG_ID_W-1:0] cfg);
        case (cfg)
            CFG_2R2C, CFG_2R1C, CFG_2R3C: config_rows = 2;
            CFG_3R2C, CFG_3R1C: config_rows = 3;
            CFG_1R2C, CFG_1R3C: config_rows = 1;
            default: config_rows = 0;
        endcase
    endfunction

    function automatic int config_cols(input logic [CONFIG_ID_W-1:0] cfg);
        case (cfg)
            CFG_2R2C, CFG_3R2C, CFG_1R2C: config_cols = 2;
            CFG_2R1C, CFG_3R1C: config_cols = 1;
            CFG_2R3C, CFG_1R3C: config_cols = 3;
            default: config_cols = 0;
        endcase
    endfunction
endpackage
