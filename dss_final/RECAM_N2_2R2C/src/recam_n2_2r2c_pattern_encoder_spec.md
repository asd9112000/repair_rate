# recam_n2_2r2c_pattern_encoder specification

The combinational encoder preserves historical candidate order and emits the
first legal PatternID, or zero when none is legal. It has no state.

```json
{"signal":[{"name":"candidate_valid_i","wave":"x3"},{"name":"repairable_o","wave":"x3"},{"name":"pattern_id_o","wave":"x3"}]}
```
