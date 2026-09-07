source scripts/synthesis/constraints.tcl

foreach required {TOP SOURCES REPORT_DIR} {
    if {![info exists ::env($required)]} {
        error "missing required environment variable $required"
    }
}

set top $::env(TOP)
set sources [split $::env(SOURCES) ":"]
set report_dir $::env(REPORT_DIR)
file mkdir $report_dir

foreach source_file $sources {
    yosys read_verilog -sv $source_file
}

if {[info exists ::env(PARAMS)] && $::env(PARAMS) ne ""} {
    foreach assignment [split $::env(PARAMS) ","] {
        lassign [split $assignment "="] parameter value
        if {$parameter eq "" || $value eq ""} {
            error "invalid parameter assignment: $assignment"
        }
        yosys chparam -set $parameter $value $top
    }
}

yosys hierarchy -check -top $top
yosys proc
yosys opt
yosys memory
yosys opt
yosys flatten
yosys techmap
yosys opt
yosys abc
yosys clean
yosys tee -o "$report_dir/$top.stat.txt" stat -top $top
yosys write_json "$report_dir/$top.json"
