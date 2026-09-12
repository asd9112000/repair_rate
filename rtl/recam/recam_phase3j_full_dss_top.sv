`default_nettype none

// Phase 3J measurement top.  One shared Phase 3B analyzer/scheduler builds
// four ConfigPatternMaps in A->B->C->D order, then the verified Phase 3H/3I
// allocator consumes them with a compile-time EARLY/GROUP policy selection.
module recam_phase3j_full_dss_top #(
    parameter integer GROUP_POLICY = 0,
    parameter integer ROW_ADDR_W = 9, COL_ADDR_W = 5, DIFF_ADDR_W = 9,
    parameter integer HYBRID_ENTRIES = 7
) (
    input wire clk_i, rst_ni, start_i,
    input wire [4:0] pivot_valid_i,
    input wire [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,
    input wire [5*COL_ADDR_W-1:0] pivot_cols_flat_i,
    input wire [4:0] row_gt1_i, row_gt2_i, row_gt3_i,
    input wire [4:0] col_gt1_i, col_gt2_i, col_gt3_i,
    input wire [HYBRID_ENTRIES-1:0] hybrid_valid_i,
    input wire [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,
    input wire [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,
    input wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,
    input wire conventional_overflow_i,
    output reg busy_o, done_o, output wire group_repairable_o,
    output wire [11:0] ledger_released_borrower_o
);
    localparam COLLECT_START = 3'd1, COLLECT_WAIT = 3'd2, ALLOC_START = 3'd3, ALLOC_WAIT = 3'd4;
    reg [2:0] state;
    reg [1:0] collect_sa;
    reg [79:0] map_a, map_b, map_c, map_d;
    wire scheduler_start = state == COLLECT_START;
    wire scheduler_role = (collect_sa == 2'd1 || collect_sa == 2'd2);
    wire scheduler_busy, scheduler_done;
    wire [79:0] scheduler_map;
    wire allocator_busy, allocator_done;

    recam_role_aware_config_scheduler #(.ROW_ADDR_W(ROW_ADDR_W), .COL_ADDR_W(COL_ADDR_W),
        .DIFF_ADDR_W(DIFF_ADDR_W), .HYBRID_ENTRIES(HYBRID_ENTRIES)) scheduler (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(scheduler_start), .sa_role_i(scheduler_role),
        .pivot_valid_i(pivot_valid_i), .pivot_rows_flat_i(pivot_rows_flat_i), .pivot_cols_flat_i(pivot_cols_flat_i),
        .row_gt1_i(row_gt1_i), .row_gt2_i(row_gt2_i), .row_gt3_i(row_gt3_i), .col_gt1_i(col_gt1_i),
        .col_gt2_i(col_gt2_i), .col_gt3_i(col_gt3_i), .hybrid_valid_i(hybrid_valid_i),
        .hybrid_pointer_flat_i(hybrid_pointer_flat_i), .hybrid_descriptor_i(hybrid_descriptor_i),
        .hybrid_differing_flat_i(hybrid_differing_flat_i), .conventional_overflow_i(conventional_overflow_i),
        .busy_o(scheduler_busy), .done_o(scheduler_done), .current_config_id_o(), .config_pattern_map_o(scheduler_map),
        .current_candidate_valid_o(), .current_pattern_id_o(), .current_solution_valid_o(), .current_repairable_o());

    recam_dss_group_allocator allocator (
        .clk_i(clk_i), .rst_ni(rst_ni), .start_i(state == ALLOC_START), .policy_i(GROUP_POLICY != 0),
        .map_a_i(map_a), .map_b_i(map_b), .map_c_i(map_c), .map_d_i(map_d), .busy_o(allocator_busy),
        .done_o(allocator_done), .group_repairable_o(group_repairable_o), .sa_commit_valid_o(),
        .ledger_released_borrower_o(ledger_released_borrower_o), .selected_config_flat_o(),
        .selected_pattern_flat_o(), .selected_map_index_flat_o(), .selected_donor_flat_o(), .borrow_flat_o(), .release_flat_o());

    always @(posedge clk_i) begin
        done_o <= 1'b0;
        if (!rst_ni) begin
            state <= 0; collect_sa <= 0; busy_o <= 0; done_o <= 0;
            map_a <= 0; map_b <= 0; map_c <= 0; map_d <= 0;
        end else begin
            case (state)
                0: if (start_i) begin busy_o <= 1; collect_sa <= 0; state <= COLLECT_START; end
                COLLECT_START: state <= COLLECT_WAIT;
                COLLECT_WAIT: if (scheduler_done) begin
                    case (collect_sa) 0:map_a<=scheduler_map; 1:map_b<=scheduler_map; 2:map_c<=scheduler_map; default:map_d<=scheduler_map; endcase
                    if (collect_sa == 3) state <= ALLOC_START;
                    else begin collect_sa <= collect_sa + 1'b1; state <= COLLECT_START; end
                end
                ALLOC_START: state <= ALLOC_WAIT;
                ALLOC_WAIT: if (allocator_done) begin state <= 0; busy_o <= 0; done_o <= 1; end
                default: state <= 0;
            endcase
        end
    end
endmodule

module recam_phase3j_full_early_top #(parameter integer ROW_ADDR_W=9, COL_ADDR_W=5, DIFF_ADDR_W=9, HYBRID_ENTRIES=7) (
    input wire clk_i,rst_ni,start_i,input wire [4:0] pivot_valid_i,input wire [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,input wire [5*COL_ADDR_W-1:0] pivot_cols_flat_i,input wire [4:0] row_gt1_i,row_gt2_i,row_gt3_i,col_gt1_i,col_gt2_i,col_gt3_i,input wire [HYBRID_ENTRIES-1:0] hybrid_valid_i,input wire [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,input wire [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,input wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,input wire conventional_overflow_i,output wire busy_o,done_o,group_repairable_o,output wire [11:0] ledger_released_borrower_o);
 recam_phase3j_full_dss_top #(.GROUP_POLICY(0),.ROW_ADDR_W(ROW_ADDR_W),.COL_ADDR_W(COL_ADDR_W),.DIFF_ADDR_W(DIFF_ADDR_W),.HYBRID_ENTRIES(HYBRID_ENTRIES)) d(.*);
endmodule
module recam_phase3j_full_group_top #(parameter integer ROW_ADDR_W=9, COL_ADDR_W=5, DIFF_ADDR_W=9, HYBRID_ENTRIES=7) (
    input wire clk_i,rst_ni,start_i,input wire [4:0] pivot_valid_i,input wire [5*ROW_ADDR_W-1:0] pivot_rows_flat_i,input wire [5*COL_ADDR_W-1:0] pivot_cols_flat_i,input wire [4:0] row_gt1_i,row_gt2_i,row_gt3_i,col_gt1_i,col_gt2_i,col_gt3_i,input wire [HYBRID_ENTRIES-1:0] hybrid_valid_i,input wire [HYBRID_ENTRIES*3-1:0] hybrid_pointer_flat_i,input wire [HYBRID_ENTRIES-1:0] hybrid_descriptor_i,input wire [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_flat_i,input wire conventional_overflow_i,output wire busy_o,done_o,group_repairable_o,output wire [11:0] ledger_released_borrower_o);
 recam_phase3j_full_dss_top #(.GROUP_POLICY(1),.ROW_ADDR_W(ROW_ADDR_W),.COL_ADDR_W(COL_ADDR_W),.DIFF_ADDR_W(DIFF_ADDR_W),.HYBRID_ENTRIES(HYBRID_ENTRIES)) d(.*);
endmodule
