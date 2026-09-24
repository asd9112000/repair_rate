# recam_n2_preopt_parallel_config_bank specification

Seven fixed ConfigID lanes (0--6) evaluate the same active-SA snapshot in
parallel. The output flattening is `ConfigID*10 + PatternOffset`; no time mux
exists in the analyzer bank.

```json
{ "signal": [
  {"name":"clk","wave":""},
  {"name":"active SA snapshot","wave":"2..."},
  {"name":"lane 0..6 candidate vectors","wave":"x234"}
] }
```

The WaveDrom payload depicts one combinational evaluation interval. Physical
columns and Hybrid identities use 13 bits.
