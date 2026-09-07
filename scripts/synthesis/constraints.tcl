# Phase 1 uses Yosys generic-cell synthesis as a structural sanity check.
# CLK_PERIOD is recorded for provenance but no timing claim is made without
# a Liberty library. Phase 6 must replace this with the selected ASIC setup.
if {![info exists ::env(CLK_PERIOD)]} {
    set ::env(CLK_PERIOD) 10.0
}
set clock_period_ns $::env(CLK_PERIOD)
