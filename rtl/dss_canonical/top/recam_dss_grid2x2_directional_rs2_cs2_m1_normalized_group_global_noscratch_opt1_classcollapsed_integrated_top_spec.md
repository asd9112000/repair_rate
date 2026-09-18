# OPT1 integrated top specification

`recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_integrated_top` is a separately elaborated SYN-B variant. It fixes `USE_OPT1=1`, reuses the same snapshot/producer/commit path as OPT0, and instantiates only the approved class-collapse search core.

There is no runtime OPT0/OPT1 datapath mux. The externally visible start, done, selected tuple, and committed ledger contracts are identical to the OPT0 integrated top.

```json
{"signal":[{"name":"start_i","wave":"010................"},{"name":"busy_o","wave":"0.1..............0"},{"name":"done_o","wave":"0................1"}]}
```
