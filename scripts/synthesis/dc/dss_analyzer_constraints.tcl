# Phase 2 analyzer block-level constraints.  The analyzer is an internal block,
# so its default boundary model assumes adjacent upstream/downstream registers.
set clock_name clk
set clock_port clk_i
set clock_period_ns 20.0
set input_delay_ns 0.0
set output_delay_ns 0.0
if {[info exists ::env(CLOCK_PERIOD_NS)]} {
    set clock_period_ns $::env(CLOCK_PERIOD_NS)
}
if {[info exists ::env(INPUT_DELAY_NS)]} {
    set input_delay_ns $::env(INPUT_DELAY_NS)
}
if {[info exists ::env(OUTPUT_DELAY_NS)]} {
    set output_delay_ns $::env(OUTPUT_DELAY_NS)
}

create_clock -name $clock_name -period $clock_period_ns [get_ports $clock_port]
set_dont_touch_network [get_clocks $clock_name]
set_fix_hold [get_clocks $clock_name]
set_clock_uncertainty 0.1 [get_clocks $clock_name]
set_clock_transition 0.1 [get_clocks $clock_name]

set data_inputs [remove_from_collection [all_inputs] [get_ports {clk_i rst_ni}]]
set_input_transition 0.5 $data_inputs
set_input_delay -max $input_delay_ns -clock $clock_name $data_inputs
set_input_delay -min 0.0 -clock $clock_name $data_inputs
set_output_delay -max $output_delay_ns -clock $clock_name [all_outputs]
set_output_delay -min 0.0 -clock $clock_name [all_outputs]
set_false_path -from [get_ports rst_ni]
set_load 0.05 [all_outputs]

set_max_transition 3.0 [current_design]
set_max_capacitance 0.15 [current_design]
set_max_fanout 10 [current_design]
