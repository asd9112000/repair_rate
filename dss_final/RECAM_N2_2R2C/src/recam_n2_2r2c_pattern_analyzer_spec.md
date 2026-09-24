# recam_n2_2r2c_pattern_analyzer specification

The combinational analyzer evaluates the unchanged six C(4,2) candidates. Its
diagonal rule remains `pattern[row] && !pattern[col]`; it has no state.

```json
{"signal":[{"name":"matrix_flat_i","wave":"x3"},{"name":"candidate_valid_o","wave":"x3"}]}
```
