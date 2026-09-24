# recam_n2_2r2c_matrix_builder specification

The combinational matrix builder accepts four fixed 2R2C pivot slots, 9-bit
rows, and 13-bit physical repair columns. It preserves historical matrix,
hybrid, and pivot ordering behavior; only physical column comparison width is
corrected. No clock, reset, or retained state exists.

```json
{"signal":[{"name":"pivot_cols_flat_i","wave":"x3"},{"name":"hybrid_cols_flat_i","wave":"x3"},{"name":"matrix_flat_o","wave":"x3"}]}
```
