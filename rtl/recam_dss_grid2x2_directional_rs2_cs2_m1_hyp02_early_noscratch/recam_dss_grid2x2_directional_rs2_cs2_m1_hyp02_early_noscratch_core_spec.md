# `recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_core`

The controller accepts the current HYP02 analyzer candidate and produces one
immediate A→B→C→D EARLY result.  `start_i` clears an old result and starts at
A/slot 1.  Candidate validity is accepted only when the selected prefix has
at least one completion in the frozen HYP02 81-path universe.  The scan order
is `1,0,3,2`; failed slots advance the scan and failure terminates after the
fourth slot without rolling back already committed SAs.

`current_config_id_o` is fixed ConfigID decode for the current SA/slot;
`candidate_pattern_id_i` is written unchanged at a commit.  The four internal
prefix facts encode only the HYP02 implications, not physical resources.
