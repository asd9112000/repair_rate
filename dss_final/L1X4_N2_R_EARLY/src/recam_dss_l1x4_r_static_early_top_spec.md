# `recam_dss_l1x4_r_static_early_top`

The top connects the 13-bit physical-column shared analyzer to the sequential
L1X4 EARLY controller. Successful immediate decisions capture only selected
row/column physical line addresses, with rows zero-extended from 9 to 13 bits.
No GROUP candidate store, 27-path GROUP selector, or full pivot warehouse is
instantiated.

```wavedrom
{ "signal": [
  {"name":"clk_i", "wave":"p...."},
  {"name":"start_i", "wave":"010.."},
  {"name":"commit", "wave":"0.10."},
  {"name":"line valid", "wave":"0.1.."}
] }
```
