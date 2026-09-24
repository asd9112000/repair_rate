set design $::env(DESIGN)
set source_dir $::env(RTL_CASE_DIR)
set report_dir $::env(REPORT_DIR)
set work_dir [file join $report_dir work]
set netlist_dir [file join $report_dir netlist]
set library_root /cad/std_libraries/CBDK_TSMC018_Arm_f1.0/CIC/SynopsysDC
set library_db_dir [file join $library_root db]

file mkdir $report_dir
file mkdir $work_dir
file mkdir $netlist_dir
set_svf [file join $report_dir default.svf]
set_app_var search_path [list . $library_db_dir /usr/cad/synopsys/synthesis/cur/libraries/syn /usr/cad/synopsys/synthesis/cur/dw]
set_app_var target_library [list slow.db]
set_app_var synthetic_library [list dw_foundation.sldb]
set_app_var link_library [concat * $target_library $synthetic_library]
define_design_lib WORK -path $work_dir
set_app_var alib_library_analysis_path $work_dir

set sources [list \
    [file join $source_dir recam_n2_2r2c_matrix_builder.sv] \
    [file join $source_dir recam_n2_2r2c_pattern_analyzer.sv] \
    [file join $source_dir recam_n2_2r2c_pattern_encoder.sv] \
    [file join $source_dir recam_n2_2r2c_analyzer.sv]]
analyze -format sverilog $sources
elaborate $design
if {![link]} { error "Link failed for $design" }
uniquify
set_operating_conditions -library slow slow
set_wire_load_mode top
set_wire_load_model -library slow -name tsmc18_wl10
redirect -file [file join $report_dir check_design_before_compile.rpt] {check_design}

# Same 20 ns virtual register-boundary constraints as historical RECAM and
# the accepted current DSS methodology; this remains a combinational baseline.
create_clock -name recam_boundary_clk -period 20.0
set_input_transition 0.5 [all_inputs]
set_input_delay -max 0.0 -clock recam_boundary_clk [all_inputs]
set_input_delay -min 0.0 -clock recam_boundary_clk [all_inputs]
set_output_delay -max 0.0 -clock recam_boundary_clk [all_outputs]
set_output_delay -min 0.0 -clock recam_boundary_clk [all_outputs]
set_load 0.05 [all_outputs]
set_max_transition 3.0 [current_design]
set_max_capacitance 0.15 [current_design]
set_max_fanout 10 [current_design]
compile -map_effort low

redirect -file [file join $report_dir check_design_after_compile.rpt] {check_design}
redirect -file [file join $report_dir design.rpt] {report_design}
redirect -file [file join $report_dir area.rpt] {report_area -hierarchy}
redirect -file [file join $report_dir hierarchy_area.rpt] {report_area -hierarchy}
redirect -file [file join $report_dir references.rpt] {report_reference -hierarchy}
set nand2_cells [get_lib_cells */NAND2X1]
redirect -file [file join $report_dir nand2_reference.rpt] {
    foreach_in_collection nand2_cell $nand2_cells {
        puts "NAND2_REFERENCE_CELL=[get_object_name $nand2_cell]"
        puts "NAND2_REFERENCE_AREA=[get_attribute $nand2_cell area]"
    }
}
redirect -file [file join $report_dir timing.rpt] {report_timing -delay_type max -max_paths 10 -input_pins}
redirect -file [file join $report_dir constraints.rpt] {report_constraint -all_violators}
redirect -file [file join $report_dir qos.rpt] {report_qor}
redirect -file [file join $report_dir cells.rpt] {report_cell}
write_file -format verilog -hierarchy -output [file join $netlist_dir ${design}_SYN.v]
write_file -format ddc -hierarchy -output [file join $netlist_dir ${design}_SYN.ddc]
write_sdc [file join $netlist_dir ${design}_SYN.sdc]
quit
