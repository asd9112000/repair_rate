# P3-4PT-SYNTH / SYN-C only: LINE1X4 single-hop streaming-early integrated
# boundary.  No GLOBAL policy source is part of this hierarchy.
set p3_4pt_synth_sources [list \
    [file join $repo_root rtl dss_1x4 producer recam_dss_line1x4_rs2_cs2_m1_normalized_shared_analyzer.v] \
    [file join $repo_root rtl dss_1x4 producer recam_dss_line1x4_rs2_cs2_m1_normalized_candidate_producer.v] \
    [file join $repo_root rtl dss_1x4 policy recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_core.v] \
    [file join $repo_root rtl dss_1x4 top recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_top.v]]
