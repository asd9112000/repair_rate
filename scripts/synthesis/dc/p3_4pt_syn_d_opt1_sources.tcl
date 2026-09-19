# P3-4PT-SYNTH / SYN-D only: LINE1X4 GLOBAL top whose elaborated parameter
# fixes ENABLE_OPT1_CLASS_COLLAPSE=1.  No OPT0 public top is analyzed here.
set p3_4pt_synth_sources [list \
    [file join $repo_root rtl dss_1x4 producer recam_dss_line1x4_rs2_cs2_m1_normalized_shared_analyzer.v] \
    [file join $repo_root rtl dss_1x4 producer recam_dss_line1x4_rs2_cs2_m1_normalized_candidate_producer.v] \
    [file join $repo_root rtl dss_1x4 policy recam_dss_line1x4_rs2_cs2_m1_normalized_global_core.v] \
    [file join $repo_root rtl dss_1x4 policy recam_dss_line1x4_rs2_cs2_m1_normalized_global_atomic_group_commit.v] \
    [file join $repo_root rtl dss_1x4 top recam_dss_line1x4_rs2_cs2_m1_normalized_global_top.v] \
    [file join $repo_root rtl dss_1x4 top recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_top.v]]
