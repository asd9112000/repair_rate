# L1X4_R_GROUP_V1 — Final Archive

| Field | Value |
| --- | --- |
| Canonical case | `L1X4_R_GROUP` |
| Implementation / lifecycle | `L1X4_R_GROUP_V1` / **FINAL** |
| Top | `recam_dss_l1x4_r_static_global_top` |
| Top SHA-256 | `33b901864987fc3839abe54ef6985431df2cdbfd02333c33793f3c8da2b20123` |
| Functional closure commit | `e15072d0955126ae235802fbc92c1fc6469969ab` |
| Synthesis-result commit | `ed911478477b6e6efc594aae5719e24cf9b89d13` |

## Architecture

Directed row-only topology: `A -> B -> C -> D`; `Rs=2`, `Cs=2`, `m=1`.
It is GROUP/static-global with 27 fixed paths, dense configurations 1R2C,
2R2C, and 3R2C, and 12 collected dense results. The shared analyzer is
unchanged from the frozen G2X2_R analyzer family.

## Functional closure

C++ policy: `SolutionTakePolicy::Line1x4RowStaticGlobal`.

- Static paths: 27
- Selector qualification: 4096 / 4096 PASS
- Directed verification: PASS
- C++ oracle: 1000 / 1000 PASS
- RTL/C++ lockstep: 1000 / 1000 PASS; 0 mismatches
- Analyzer changed from G2X2_R: NO

## Final matched synthesis

| Field | Value |
| --- | ---: |
| DC / library / corner | W-2024.09-SP2 / TSMC018 `slow.db` / slow |
| Clock / I/O delay | 20 ns / 0 |
| Total cell area / GE | 96898.032853 um² / 9710.00 |
| Combinational / sequential area | 90418.205571 / 6479.827282 um² |
| WNS / TNS | 0.00 / 0.00 ns |
| Critical path | 19.90 ns; `core/collect_slot_q_reg[1]` to `core/candidate_store/store_q_reg[21]` |
| Critical-path class | candidate/result collection |

Matched G2X2_R_GROUP is 97140.859997 um² / 9734.33 GE, with a 2943.8640 um²
selector. L1X4 has a 1470.2688 um² 27-path selector. Timing-driven whole-design
mapping increased analyzer and candidate-store hierarchy areas, so total area
decreases by only 242.827144 um² (about 0.25%); it is not attributed solely to
path count.

## Provenance

`synthesis/source_manifest_original.txt` and
`synthesis/authoritative_source_manifest_original.tcl` preserve the working-tree
paths actually used by historical DC. `synthesis/authoritative_source_manifest.tcl`
is the active FINAL archive manifest and resolves only `src/` files. Hashes prove
these source bytes are identical.

`READABLE_VERILOG_STRICT_GATE: NOT_RUN_ENVIRONMENT_DEPENDENCY` because host
Python is 3.8 while the tool requires Python >= 3.9. `RTL_FINDING_FROM_STRICT_GATE: NONE`.
