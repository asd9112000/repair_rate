# N3 RC-GROUP structural audit

## Structural compilation

The following local structural compilation completed with no errors:

```
verilator --lint-only --sv -Wall -Wno-fatal -Wno-PINCONNECTEMPTY -Wno-UNUSED \
  rtl/G2X2_N3_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/*.sv
```

Empty named output observations are intentional in the N2-inherited top/core
composition; they are excluded from this structural compile command.  No
syntax, connection-width, or elaboration error remains.

| Audit | Result | Evidence |
| --- | --- | --- |
| physical contract | PASS | top ports: 7x9 pivot rows, 7x13 pivot columns, 17x13 Hybrid differing |
| analyzer payload arithmetic | PASS | 7 + 63 + 91 + 56 + 17 + 51 + 17 + 221 + 1 = 524 |
| analyzer instances | 1 | `recam_n3_rc_fixed_mask_analyzer analyzer` in top |
| fixed masks | PASS | 80 table cases plus default; no runtime enumerator |
| PatternID | 6 bits | analyzer, candidate store, selector, and reconstruction all use 6 bits |
| GROUP store | 112 bits | 16 x 7-bit records, `(SA*4 + slot)*7` indexing |
| private pivot capture | 616 bits | `4 x 7 x (ROW_ADDR_W 9 + PHYS_COL_ADDR_W 13)` |
| reconstruction dependency | 656 bits | pivot 616 + commit 4 + config 12 + selected pattern 24 |
| final outputs | widened by count only | address 364, row identity 28, valid 28 |
| duplicated analyzer StateBank | ABSENT | analyzer consumes direct top inputs only |

```
STRUCTURAL_COMPILE: PASS
ANALYZER_PAYLOAD_W: 524
GROUP_STORE_W: 112
RECONSTRUCTION_WIDTH_AUDIT: PASS
PRIVATE_PIVOT_CAPTURE_WIDTH: 616
```
