###################################################################

# Created by write_sdc on Thu Sep 10 03:32:17 2026

###################################################################
set sdc_version 2.1

set_units -time ns -resistance kOhm -capacitance pF -voltage V -current mA
set_operating_conditions slow -library slow
set_wire_load_mode top
set_wire_load_model -name tsmc18_wl10 -library slow
set_max_fanout 10 [current_design]
set_max_capacitance 0.15 [current_design]
set_max_transition 3 [current_design]
set_load -pin_load 0.05 [get_ports repairable_o]
set_load -pin_load 0.05 [get_ports {pattern_id_o[3]}]
set_load -pin_load 0.05 [get_ports {pattern_id_o[2]}]
set_load -pin_load 0.05 [get_ports {pattern_id_o[1]}]
set_load -pin_load 0.05 [get_ports {pattern_id_o[0]}]
create_clock -name recam_boundary_clk  -period 20  -waveform {0 10}
set_input_delay -clock recam_boundary_clk  0  [get_ports {candidate_valid_i[5]}]
set_input_delay -clock recam_boundary_clk  0  [get_ports {candidate_valid_i[4]}]
set_input_delay -clock recam_boundary_clk  0  [get_ports {candidate_valid_i[3]}]
set_input_delay -clock recam_boundary_clk  0  [get_ports {candidate_valid_i[2]}]
set_input_delay -clock recam_boundary_clk  0  [get_ports {candidate_valid_i[1]}]
set_input_delay -clock recam_boundary_clk  0  [get_ports {candidate_valid_i[0]}]
set_output_delay -clock recam_boundary_clk  0  [get_ports repairable_o]
set_output_delay -clock recam_boundary_clk  0  [get_ports {pattern_id_o[3]}]
set_output_delay -clock recam_boundary_clk  0  [get_ports {pattern_id_o[2]}]
set_output_delay -clock recam_boundary_clk  0  [get_ports {pattern_id_o[1]}]
set_output_delay -clock recam_boundary_clk  0  [get_ports {pattern_id_o[0]}]
set_input_transition -max 0.5  [get_ports {candidate_valid_i[5]}]
set_input_transition -min 0.5  [get_ports {candidate_valid_i[5]}]
set_input_transition -max 0.5  [get_ports {candidate_valid_i[4]}]
set_input_transition -min 0.5  [get_ports {candidate_valid_i[4]}]
set_input_transition -max 0.5  [get_ports {candidate_valid_i[3]}]
set_input_transition -min 0.5  [get_ports {candidate_valid_i[3]}]
set_input_transition -max 0.5  [get_ports {candidate_valid_i[2]}]
set_input_transition -min 0.5  [get_ports {candidate_valid_i[2]}]
set_input_transition -max 0.5  [get_ports {candidate_valid_i[1]}]
set_input_transition -min 0.5  [get_ports {candidate_valid_i[1]}]
set_input_transition -max 0.5  [get_ports {candidate_valid_i[0]}]
set_input_transition -min 0.5  [get_ports {candidate_valid_i[0]}]
