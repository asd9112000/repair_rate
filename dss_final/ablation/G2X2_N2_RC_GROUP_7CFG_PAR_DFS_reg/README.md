# G2X2 N2 RC GROUP 7CFG parallel dense-DFS ablation archive

`CASE_ID: G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg`
`LIFECYCLE: SUPPORTING_ABLATION`
`FUNCTIONAL_COMMIT: eed173cb6e8dacc047e557d67346f3f0c71cb59a`

This is a self-contained provenance-backed composite pre-optimization representation, not an eighth canonical N2 case and not a literal SYN-B replay. It retains seven physically parallel current-SA Config analyzers, three registered 160-bit effect maps (480 bits), OPT1, GLOBAL DFS diagnostic logic, atomic commit, and the canonical 440-bit GROUP pivot-retention implementation. Its visible tuple uses the frozen canonical 81-path priority so PPA compares equal repair semantics rather than different policy order.

Historical map storage is `L,R,B,RB`; historical DFS priority is `R,L,RB,B`. That priority is retained only as diagnostic hardware: `HISTORICAL_DFS_PRIORITY_USED_FOR_FINAL_ABLATION: NO`.

## Package contents

- `src/`: all 11 HDL compilation units, module specs, provenance/mapping records, and the machine-readable canonical 81-path table.
- `verification/`: lockstep and map-producer test sources plus the frozen verification manifest.
- `synthesis/`: matched DC runner/TCL, reports, generated netlist, and logs.

No file in this package is a symlink to mutable `rtl/` content. From this directory, verify the archived HDL with `sha256sum -c source_sha256.txt`.

The completed synthesis uses DC W-2024.09-SP2, TSMC018 `slow.db`, a 20.0 ns clock, zero I/O delay, and `compile -map_effort low`, matching the canonical N2 runner command. Full PPA comparison and constraints caveats are in `../../records/N2_OPTIMIZATION_ABLATION.md`.
