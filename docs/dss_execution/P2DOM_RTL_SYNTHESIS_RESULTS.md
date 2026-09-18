# P2DOM RTL synthesis results and methodology

`ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH`

Comparable variants retain the policy/search-core boundary:

```text
completed candidate maps -> GLOBAL DFS -> selected speculative tuple
```

Frozen methodology: Synopsys DC W-2024.09-SP2; TSMC018 Arm CBDK `slow.db`,
slow corner; 20.0 ns clock; zero IO delay; `compile -map_effort low`; and
`slow/NAND2X1 = 9.979200` for GE. The retained manifest is
`scripts/synthesis/dc/recam_canonical_rs2_global_noscratch_sources.tcl`; the
retained runner is `scripts/synthesis/run_recam_canonical_rs2_global_noscratch_dc.sh`.
Each future architecture requires a separate fully named runner and result
directory. `metadata.txt` must end `STATUS=PASS` before reporting PPA.

| Exp | Source / manifest | Total area | Comb. | Seq. | GE | Seq. cells | WNS | Critical path | Status |
|---|---|---:|---:|---:|---:|---:|---:|---|---|
| A | retained RTL SHA `40244...dd362` | 61,751.290432 | 26,917.228756 | 34,834.061676 | 6,188.00 | 616 | +0.02 ns | depth register to selected-release register; 41 levels | retained mapped PASS |
| B | planned explicit manifest | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | PLANNED |
| C | planned explicit manifest | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | PLANNED |
| D/E | planned explicit manifests | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | PLANNED |

The A row is retained-ledger provenance, not comparison with integrated
Streaming EARLY. Future rows must add TNS when available, leaf-cell counts,
exact startpoint/endpoint, logic depth, report paths, and classification.
