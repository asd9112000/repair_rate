# N2 FINAL archive pre-reorganization snapshot

## Historical-snapshot warning

This document preserves the non-CA archive state before reorganization. Its
seven packages remain `VALID_NON_CA_CANONICAL_REFERENCE`; they are not the
primary final Continuous-Analysis implementation. For current CA selection,
use [CURRENT_N2_CANONICAL.md](CURRENT_N2_CANONICAL.md) and
`dss_final/continuous_analysis_live/`.

## Purpose

This checkpoint preserves the exact N2 FINAL state before archive and
documentation reorganization. It is a documentation boundary, not a new RTL
or PPA result.

## Base authoritative commit

`6351a82479ad6e95ed3e02cd441c9fe96cc6f462`

`archive: finalize corrected N2 RECAM and DSS hardware set`

## Historical non-CA state

- Archived cases: **7 / 7** non-CA references.
- Corrected RECAM: `RECAM_N2_2R2C`.
- Three EARLY streaming implementations and three GROUP retained-state
  implementations.
- The contemporaneous PPA snapshot is preserved in the historical archive.

## Common address contract

```text
ROW_ADDR_W         = 9
PHYS_COL_ADDR_W    = 13
WORD_COL_ADDR_W    = 5
HYBRID_LINE_ADDR_W = 13
```

The historical 5-bit RECAM source remains preserved under `rtl/recam/`; its
matched non-CA replacement is `RECAM_N2_2R2C`.
