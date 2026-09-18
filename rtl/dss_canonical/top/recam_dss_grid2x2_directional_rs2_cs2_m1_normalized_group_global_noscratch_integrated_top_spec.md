# OPT0 integrated top specification

`recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_integrated_top` is the OPT0 external SYN-B boundary. It fixes `USE_OPT1=0` and includes four snapshot capture, one shared analyzer producer, canonical GLOBAL DFS, atomic group commit, and persistent directional ledger output.

All snapshot fields are flattened by SA in A/B/C/D order. The debug candidate-map outputs are observability signals for functional closure; the 480-bit producer-to-search boundary remains raw and uncompressed.

```json
{"signal":[{"name":"start_i","wave":"010................"},{"name":"candidate_valid_debug_o","wave":"x..=...............","data":["complete map"]},{"name":"done_o","wave":"0................1"}]}
```
