set design dss_analyzer_top
if {[info exists ::env(DESIGN)]} {
    set design $::env(DESIGN)
}
set repo_root $::env(REPO_ROOT)
set report_dir $::env(REPORT_DIR)
set work_dir [file join $report_dir work]
set netlist_dir [file join $report_dir netlist]
set library_root /cad/std_libraries/CBDK_TSMC018_Arm_f1.0/CIC/SynopsysDC
set library_db_dir [file join $library_root db]
set slow_library slow.db

file mkdir $report_dir
file mkdir $work_dir
file mkdir $netlist_dir
set_svf [file join $report_dir default.svf]

set_app_var search_path [list . $library_db_dir \
    /usr/cad/synopsys/synthesis/cur/libraries/syn \
    /usr/cad/synopsys/synthesis/cur/dw]
set_app_var target_library [list $slow_library]
set_app_var synthetic_library [list dw_foundation.sldb]
set_app_var link_library [concat * $target_library $synthetic_library]

define_design_lib WORK -path $work_dir
set_app_var alib_library_analysis_path $work_dir
source [file join $repo_root scripts synthesis dc dss_analyzer_sources.tcl]

analyze -format sverilog $dss_analyzer_sources
if {[info exists ::env(ELAB_PARAMS)] && $::env(ELAB_PARAMS) ne ""} {
    elaborate $design -parameters $::env(ELAB_PARAMS)
} elseif {$design eq "dss_analyzer_top" && [info exists ::env(PARALLEL_ORIENTATION)]} {
    elaborate $design -parameters "PARALLEL_ORIENTATION=$::env(PARALLEL_ORIENTATION)"
} else {
    elaborate $design
}
set elaborated_design [get_object_name [current_design]]
if {![link]} {
    error "Link failed for $elaborated_design"
}
uniquify

set_operating_conditions -library slow slow
set_wire_load_mode top
set_wire_load_model -library slow -name tsmc18_wl10
set defer_constraints 0
if {[info exists ::env(COMPILE_MODE)] && $::env(COMPILE_MODE) eq "structural_report"} {
    set defer_constraints 1
} else {
    source [file join $repo_root scripts synthesis dc dss_analyzer_constraints.tcl]
}

redirect -file [file join $report_dir check_design_before_compile.rpt] {check_design}
if {!$defer_constraints} {
    redirect -file [file join $report_dir check_timing_before_compile.rpt] {check_timing}
}
set_fix_multiple_port_nets -all -buffer_constants [get_designs *]

if {[info exists ::env(COMPILE_MODE)] && $::env(COMPILE_MODE) eq "structural_report"} {
    compile -exact_map -no_design_rule -map_effort low -area_effort none
} elseif {[info exists ::env(COMPILE_MODE)] && $::env(COMPILE_MODE) eq "mapped_no_drc"} {
    compile -exact_map -no_design_rule -map_effort low -area_effort none
} elseif {[info exists ::env(COMPILE_MODE)] && $::env(COMPILE_MODE) eq "mapped_only"} {
    compile -exact_map
} elseif {[info exists ::env(COMPILE_MODE)] && $::env(COMPILE_MODE) eq "quick"} {
    compile -map_effort low
} else {
    compile_ultra
}

if {$defer_constraints} {
    source [file join $repo_root scripts synthesis dc dss_analyzer_constraints.tcl]
    redirect -file [file join $report_dir check_timing_after_mapping.rpt] {check_timing}
}

redirect -file [file join $report_dir design.rpt] {report_design}
redirect -file [file join $report_dir resources.rpt] {report_resources -hierarchy}
redirect -file [file join $report_dir references.rpt] {report_reference -hierarchy}
redirect -file [file join $report_dir area.rpt] {report_area -hierarchy}
redirect -file [file join $report_dir timing.rpt] {report_timing -delay_type max -max_paths 10}
redirect -file [file join $report_dir constraints.rpt] {report_constraint -all_violators}
redirect -file [file join $report_dir power.rpt] {report_power}
redirect -file [file join $report_dir qos.rpt] {report_qor}

change_names -hierarchy -rules verilog
write_file -format verilog -hierarchy -output [file join $netlist_dir ${design}_SYN.v]
write_file -format ddc -hierarchy -output [file join $netlist_dir ${design}_SYN.ddc]
write_sdc [file join $netlist_dir ${design}_SYN.sdc]
write_sdf -version 3.0 [file join $netlist_dir ${design}_SYN.sdf]

quit
