# `recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top`

This top instantiates exactly one `recam_shared_config_analyzer` with default
`HYBRID_ENTRIES=7`.  The analyzer's `solution_valid && repairable` and its
4-bit PatternID feed the EARLY core directly.  Input field widths and ConfigID
semantics are the final HYP02 GROUP H=7 boundary.  It intentionally has no
candidate store, GROUP selector, H=11 summary expansion, or physical-demand
resource interface.
