# P2DOM RTL state accounting

`ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH`

Count RTL-visible stored information, not mapped sequential cells or area.
Transient combinational nets are not state. Report build-epoch peak and
search-epoch state separately whenever they differ.

| Variant | Raw history | Summary | DFS stack | Dominance cache | Control/result | Builder | Total logical state | Status |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| A baseline | 480 | 0 | 56 | 0 | 94 | 0 | 630 | retained RTL evidence |
| B class collapse | 480 | 0 | 56 | 0 | 94 | 0 | 630 preliminary | planned |
| C S1 summary | 0 | <=224 | 56 | 0 | 94 | 0 | <=374 preliminary | planned |
| D S1 dominance-1 | 0 | <=224 | 56 | 45 | 94 | 0 | <=419 preliminary | planned |
| C/D S2 build epoch | 480 | <=224 | 0 before DFS | 0 | NOT_FROZEN | NOT_FROZEN | >=704 plus control | planned peak |
| C/D S2 search epoch | 0 | <=224 | 56 | 0 / 45 | 94 | 0 | <=374 / <=419 preliminary | planned |

The baseline DFS stack is four 14-bit snapshots: released, used, and
release-obligation masks plus a two-bit borrow count, or 56 bits. Baseline
control/result is 24 cursor bits plus 70 state/depth/selection/final/done bits,
or 94 bits. The existing 630-bit accounting is therefore preserved.

For D, B/C/D are initially useful depths. One entry is 14 state bits plus one
valid bit, hence `3*(14+1)=45`. Recount if implementation changes useful depths
or requires replacement metadata.

```text
RAW_CANDIDATE_HISTORY_BITS: report by epoch
SUMMARY_STATE_BITS: <=224 for fixed eight-class summary only
DFS_STACK_BITS: 56 unless source proves a different encoding
DOMINANCE_CACHE_BITS: 45 preliminary for D/S1
CONTROL_RESULT_BITS: 94 baseline-derived; builder additions explicit
ADAPTER_BITS: 0 for S1; NOT_FROZEN for S2/S3
TOTAL_LOGICAL_STATE_BITS: never inferred from sequential-cell count
```

These are not PPA claims. Future rows must retain this baseline row and add
source-level evidence rather than overwrite preliminary figures.
