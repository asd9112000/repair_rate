# `recam_shared_config_analyzer`

The combinational shared analyzer accepts five pivot rows and five 13-bit
physical pivot columns. It evaluates the requested ConfigID and returns the
lowest valid PatternID. Hybrid differing-line inputs are 13 bits; word-column
width is retained only for the explicit word-address boundary, never for a
physical repair-column comparison.

```wavedrom
{ "signal": [
  {"name":"config_id_i", "wave":"2....", "data":["cfg"]},
  {"name":"pivot_cols_flat_i", "wave":"3....", "data":["13-bit fields"]},
  {"name":"candidate_valid_o", "wave":"0.1.."}
] }
```
