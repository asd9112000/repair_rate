`default_nettype none

module solution_store #(
    parameter integer SPARE_WIDTH = 3,
    parameter integer CANDIDATE_ID_WIDTH = 5,
    parameter integer MAX_K = 5
) (
    input  wire                                   clk_i,
    input  wire                                   rst_ni,
    input  wire                                   clear_i,
    input  wire                                   capture_i,
    input  wire                                   repairable_i,
    input  wire [3:0]                             selected_attempts_i,
    input  wire [4*CANDIDATE_ID_WIDTH-1:0]        selected_candidates_i,
    input  wire [4*SPARE_WIDTH-1:0]               selected_rows_i,
    input  wire [4*SPARE_WIDTH-1:0]               selected_cols_i,
    input  wire [4*MAX_K-1:0]                     selected_masks_i,
    input  wire [SPARE_WIDTH-1:0]                 borrowed_rows_i,
    input  wire [SPARE_WIDTH-1:0]                 borrowed_cols_i,
    input  wire [31:0]                            analysis_cycles_i,
    output reg                                    solution_valid_o,
    output reg                                    repairable_o,
    output reg  [3:0]                             selected_attempts_o,
    output reg  [4*CANDIDATE_ID_WIDTH-1:0]        selected_candidates_o,
    output reg  [4*SPARE_WIDTH-1:0]               selected_rows_o,
    output reg  [4*SPARE_WIDTH-1:0]               selected_cols_o,
    output reg  [4*MAX_K-1:0]                     selected_masks_o,
    output reg  [SPARE_WIDTH-1:0]                 borrowed_rows_o,
    output reg  [SPARE_WIDTH-1:0]                 borrowed_cols_o,
    output reg  [31:0]                            analysis_cycles_o
);
    always @(posedge clk_i) begin
        if (!rst_ni || clear_i) begin
            solution_valid_o <= 1'b0;
            repairable_o <= 1'b0;
            selected_attempts_o <= 4'b0000;
            selected_candidates_o <= {(4*CANDIDATE_ID_WIDTH){1'b0}};
            selected_rows_o <= {(4*SPARE_WIDTH){1'b0}};
            selected_cols_o <= {(4*SPARE_WIDTH){1'b0}};
            selected_masks_o <= {(4*MAX_K){1'b0}};
            borrowed_rows_o <= {SPARE_WIDTH{1'b0}};
            borrowed_cols_o <= {SPARE_WIDTH{1'b0}};
            analysis_cycles_o <= 32'd0;
        end else begin
            solution_valid_o <= capture_i;
            if (capture_i) begin
                repairable_o <= repairable_i;
                selected_attempts_o <= selected_attempts_i;
                selected_candidates_o <= selected_candidates_i;
                selected_rows_o <= selected_rows_i;
                selected_cols_o <= selected_cols_i;
                selected_masks_o <= selected_masks_i;
                borrowed_rows_o <= borrowed_rows_i;
                borrowed_cols_o <= borrowed_cols_i;
                analysis_cycles_o <= analysis_cycles_i;
            end
        end
    end
endmodule

`default_nettype wire
