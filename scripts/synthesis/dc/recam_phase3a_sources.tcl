# Ordered Phase 3A sources.  No Phase 2/DSS sources are part of this manifest.
set recam_phase3a_relative_sources [list \
    rtl/recam/recam_2r2c_matrix_builder.sv \
    rtl/recam/recam_2r2c_pattern_analyzer.sv \
    rtl/recam/recam_2r2c_pattern_encoder.sv \
    rtl/recam/recam_2r2c_analyzer.sv]

set recam_phase3a_sources [list]
foreach relative_source $recam_phase3a_relative_sources {
    lappend recam_phase3a_sources [file join $repo_root $relative_source]
}
