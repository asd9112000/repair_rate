# Phase 3A is purely combinational.  The virtual clock models registers placed
# immediately before and after this analyzer, without adding sequential logic.
set clock_period_ns 20.0
set input_delay_ns 0.0
set output_delay_ns 0.0
if {[info exists ::env(CLOCK_PERIOD_NS)]} { set clock_period_ns $::env(CLOCK_PERIOD_NS) }
if {[info exists ::env(INPUT_DELAY_NS)]} { set input_delay_ns $::env(INPUT_DELAY_NS) }
if {[info exists ::env(OUTPUT_DELAY_NS)]} { set output_delay_ns $::env(OUTPUT_DELAY_NS) }

create_clock -name recam_boundary_clk -period $clock_period_ns
set_input_transition 0.5 [all_inputs]
set_input_delay -max $input_delay_ns -clock recam_boundary_clk [all_inputs]
set_input_delay -min 0.0 -clock recam_boundary_clk [all_inputs]
set_output_delay -max $output_delay_ns -clock recam_boundary_clk [all_outputs]
set_output_delay -min 0.0 -clock recam_boundary_clk [all_outputs]
set_load 0.05 [all_outputs]
set_max_transition 3.0 [current_design]
set_max_capacitance 0.15 [current_design]
set_max_fanout 10 [current_design]
