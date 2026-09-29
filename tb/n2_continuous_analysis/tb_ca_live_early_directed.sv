`default_nettype none
module tb_ca_live_early_directed(
    input logic clk_i, rst_ni, state_update_i, input logic [1:0] state_sa_i,
    input logic test_done_valid_i, input logic [1:0] test_done_sa_i,
    input logic candidate_valid_i, output logic scan_active_o,
    output logic [1:0] active_sa_o, scan_slot_o, output logic [2:0] scan_config_o,
    output logic solution_commit_o, done_o, group_repairable_o, output logic [1:0] failure_position_o
);
    `CA_EARLY_CORE dut (
        .clk_i(clk_i), .rst_ni(rst_ni), .state_update_i(state_update_i), .state_sa_i(state_sa_i),
        .test_done_valid_i(test_done_valid_i), .test_done_sa_i(test_done_sa_i),
        .candidate_valid_i(candidate_valid_i), .scan_active_o(scan_active_o), .active_sa_o(active_sa_o),
        .scan_slot_o(scan_slot_o), .scan_config_id_o(scan_config_o), .solution_commit_o(solution_commit_o),
        .done_o(done_o), .group_repairable_o(group_repairable_o), .failure_position_o(failure_position_o)
    );
endmodule
`default_nettype wire
