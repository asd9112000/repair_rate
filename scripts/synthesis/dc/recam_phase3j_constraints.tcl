# Phase 3J is sequential: unlike the Phase 3A combinational boundary, bind
# the frozen 20 ns clock to the actual controller/ledger clock port.
set clock_period_ns 20.0
if {[info exists ::env(CLOCK_PERIOD_NS)]} { set clock_period_ns $::env(CLOCK_PERIOD_NS) }
create_clock -name recam_clk -period $clock_period_ns [get_ports clk_i]
set_input_transition 0.5 [remove_from_collection [all_inputs] [get_ports clk_i]]
set_input_delay -max 0.0 -clock recam_clk [remove_from_collection [all_inputs] [get_ports clk_i]]
set_input_delay -min 0.0 -clock recam_clk [remove_from_collection [all_inputs] [get_ports clk_i]]
set_output_delay -max 0.0 -clock recam_clk [all_outputs]
set_output_delay -min 0.0 -clock recam_clk [all_outputs]
set_load 0.05 [all_outputs]
set_max_transition 3.0 [current_design]
set_max_capacitance 0.15 [current_design]
set_max_fanout 10 [current_design]
