# 1x4 RS2/CS2/m1 shared analyzer

`recam_dss_line1x4_rs2_cs2_m1_normalized_shared_analyzer` is combinational.
It evaluates one capacity attempt (`2R2C`, `3R2C`, or `4R2C`) from one
six-entry collector snapshot and returns PatternID-valid bits plus the actual
row/column demand decoded from populated dictionaries.

```wavedrom
{signal:[{name:'attempt_i',wave:'3'},{name:'snapshot',wave:'3'},{name:'candidate_valid_o',wave:'3'},{name:'candidate demand',wave:'3'}]}
```
