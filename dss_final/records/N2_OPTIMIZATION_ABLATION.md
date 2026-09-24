# N2 GROUP pre-optimization supporting ablation

`CASE_ID: G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg`
`LIFECYCLE: POST-CLOSURE SUPPORTING_ABLATION`
`FUNCTIONAL_COMMIT: eed173cb6e8dacc047e557d67346f3f0c71cb59a`

This is supporting evidence for final directional RC GROUP case `G2X2_N2_RC_GROUP_reg`. It does not change the canonical N2 set, canonical PPA table, or architecture boundary.

## Implementation and semantics

The ablation is a provenance-backed composite: seven parallel current-SA ConfigID analyzers, three 160-bit registered maps (`valid`, `release`, and `borrow`; 480 bits total), OPT1 class collapse, GLOBAL DFS diagnostic logic, atomic group commit, and 440-bit corrected GROUP pivot retention. There is no Config analyzer time multiplexing; SA collection remains scheduled.

Dense-map storage follows historical SYN-B `L,R,B,RB` and its historical DFS priority is `R,L,RB,B`. The frozen DATE2026 selector is externally visible because seed 1 proved historical priority selects a different visible ConfigID/action. Role legality comes from `dss_v2_group_slot_decode.sv`: SA0/SA3 `{0,4,5,6}` and SA1/SA2 `{0,1,2,3}`. `HISTORICAL_DFS_PRIORITY_USED_FOR_FINAL_ABLATION: NO`.

## Functional evidence

| Gate | Result |
|---|---|
| Seed-1 canonical reproducer | PASS |
| Canonical lockstep | 10 smoke + 1000/1000 full vectors PASS; 0 mismatches |
| Exact tuple comparison | repairability, ConfigID, PatternID, action, donor, release/borrow, final line validity/type, and physical address: PASS |
| Physical address | `1 != 257` and `0,31,32,255,256,257,4095,8191`: PASS |
| Dense-map regression | seven lanes, 16 writes, boundaries `0/39/40/.../159`, release/borrow replication: PASS |
| Verilator | full top and lockstep wrapper, `-Wall -Werror-WIDTH`: PASS |

The archived source hash and frozen functional-to-synthesis source hash both pass. Canonical seven-case synthesizable RTL was not modified.

## Matched DC result

DC reports are archived at `dss_final/ablation/G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg/synthesis/reports/`.

| Item | Pre-optimization ablation |
|---|---:|
| Tool / library | DC W-2024.09-SP2 / TSMC018 `slow.db` |
| Corner / clock / I/O | slow / 20.0 ns / 0.0 ns input and output |
| Total cell area | 1,066,806.427911 um^2 |
| NAND2X1 equivalents (9.979200 um^2) | 106,903.00 GE |
| Combinational area | 915,521.753137 um^2 |
| Sequential area | 151,284.674774 um^2 |
| Leaf cells / combinational / sequential | 48,869 / 46,130 / 2,739 |
| WNS / TNS | 0.00 ns / 0.00 ns |
| Critical path | 19.61 ns, `producer/sa_q_reg[0] -> producer/candidate_borrow_q_reg[137]` |

The runner status is `PASS`; all required reports, generated netlist, and the post-run source-hash check are present. Timing has no violating paths. The report also lists 785 max-capacitance and 632 max-fanout violations (836 nets total; 1,334 individual constraint-report entries). This is a synthesized PPA measurement, not physical signoff; frozen RTL was intentionally not changed to improve the ablation.

## Hierarchy (cell area)

| Block | Area (um^2) | Note |
|---|---:|---|
| Candidate-map producer | 721,173.5052 | includes map registers and transition adapter |
| Seven-parallel-analyzer bank | 646,781.8949 | nested in producer |
| OPT1/GLOBAL DFS search | 176,651.8006 | includes canonical global core (60,813.2455) |
| GROUP pivot retention | 43,988.3148 | 440-bit raw pivot payload |
| Canonical path selector | 14,782.5217 | 81-path external semantic adapter |
| Atomic commit | 5,774.6305 | retained diagnostic hardware |
| Top local control | 42,231.9743 | local cell area reported at top |

DC does not isolate the 480 map-register bits as a separate hierarchy entry; they are contained in the candidate-map producer line.

## Delta against frozen final GROUP

The baseline is final `G2X2_N2_RC_GROUP_reg`: 166,140.376634 um^2, 16,648.67 GE, 133,448.516656 um^2 combinational area, 32,691.859978 um^2 sequential area, 7,620 leaf cells, 19.90 ns critical path, and +0.02 ns WNS.

| Metric | Delta / interpretation |
|---|---:|
| Area | pre-opt is +900,666.051277 um^2; 6.4211x final |
| Area framing | pre-opt is 542.111% larger; final reduces area 84.426% relative to pre-opt |
| GE | final reduces 90,254.33 GE (84.426%) |
| Combinational area | final reduces 782,073.236481 um^2 (85.424%) |
| Sequential area | final reduces 118,592.814796 um^2 (78.391%) |
| Leaf cells | final reduces 41,249 cells (84.407%) |
| Critical path | pre-opt is 0.29 ns shorter (19.61 vs 19.90 ns) |
| WNS | pre-opt 0.00 ns versus final +0.02 ns |

This supports a **combined implementation optimization** conclusion: the final hardware-friendly GROUP implementation preserves tested repair semantics at substantially lower cost than this provenance-backed composite. The result must not be attributed to parallel-analyzer removal alone or candidate-map compression alone, because several implementation choices differ together.

## Archive integrity and scope

The package is self-contained with no symlinks. It contains all 11 DC HDL units, verification sources, runner/TCL, reports, generated netlist, and archive-local source-hash manifest. The runner/TCL are included as experiment-related synthesis scripts. Other user changes under `scripts/` were not modified or staged. No N3 work was started.
