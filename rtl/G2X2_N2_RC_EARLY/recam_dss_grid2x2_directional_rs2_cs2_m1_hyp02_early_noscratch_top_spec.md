# `recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top`

This top presents one 9-bit-row/13-bit-physical-column pivot set to the shared
analyzer and immediately commits frozen HYP02 decisions. The controller's
ConfigID and PatternID select the physical row or column half at commit. Rows
are explicitly zero-extended to the 13-bit physical-address output width.

No full four-SA pivot-bank retention, candidate store, or GROUP selector is
instantiated; only committed selected lines persist for the output interface.

```wavedrom
{ "signal": [
  {"name":"clk_i", "wave":"p...."},
  {"name":"start_i", "wave":"010.."},
  {"name":"selected commit", "wave":"0.10."},
  {"name":"final address valid", "wave":"0.1.."}
] }
```
