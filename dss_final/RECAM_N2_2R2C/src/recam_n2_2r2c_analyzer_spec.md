# recam_n2_2r2c_analyzer specification

The top-level fixed 2R2C RECAM analyzer is combinational. It uses 9-bit rows
and 13-bit physical repair columns; the independent five-bit BIST wordColumn
interface is intentionally not part of this repair-line analyzer.

```json
{"signal":[{"name":"pivot_valid_i","wave":"x3"},{"name":"hybrid_cols_flat_i","wave":"x3"},{"name":"repairable_o","wave":"x3"},{"name":"pattern_id_o","wave":"x3"}]}
```
