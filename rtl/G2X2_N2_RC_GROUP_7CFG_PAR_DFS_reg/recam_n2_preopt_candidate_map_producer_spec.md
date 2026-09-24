# recam_n2_preopt_candidate_map_producer specification

On `start_i`, the producer clears three 160-bit registers. It makes exactly 16
writes in SA-major, `L,R,B,RB` storage order. Each write selects one of the
seven simultaneously evaluated Config lanes using the documented canonical
role map; its ten candidate bits and gated effects occupy `SA*40+action*10`.

```json
{ "signal": [
  {"name":"clk","wave":"p................"},
  {"name":"start","wave":"010.............."},
  {"name":"SA/action map write","wave":"x23456789ABCDEFG"},
  {"name":"done","wave":"0................1"}
] }
```

The only map registers are valid[159:0], release[159:0], and borrow[159:0]
(480 candidate-effect bits total).
