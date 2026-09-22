# DATE2026 FINAL-EARLY-D — Model-B2 synthesis closure

`recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch` has a completed,
source-provenanced synthesis characterization.  This is the corrected Model-B2
EARLY hardware boundary: decision and resource-management metadata, including
the four 260-bit per-SA analyzer summaries.  It is not a full physical remap
implementation.

## Result

| Field | Value |
| --- | --- |
| EARLY_CASE | `recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch` |
| RTL commit | `7fdaf0ad86700d40588f31735d4eb2513b77478f` |
| Tool / library | DC W-2024.09-SP2 / TSMC018 `slow.db` |
| Clock / I/O delay | 20.0 ns / 0.0 ns input and output |
| Requested / effective map effort | `low` / `medium` (`OPT-1303`) |
| Total cell area | 200824.749004 um2 |
| NAND2X1 GE | 20124.33 (`9.979200` um2 reference) |
| Combinational / sequential area | 198107.080162 / 2717.668842 um2 |
| Instantiated cells / mapped leaf cells | 9566 / 9564 |
| Combinational / sequential leaf cells | 9515 / 49 |
| WNS / TNS | 0.00 / 0.00 ns |
| Critical path delay | 19.72 ns |
| Critical path | `core/sa_q_reg[0]` to `core/selected_config_flat_o_reg[0]` |
| Critical-path class | registered FSM state through analyzer to registered selected decision metadata |
| Timing met | YES |

The analyzer accounts for 168362.4111 um2 (83.8%) of cell area and the EARLY
core for 6256.9585 um2.  Net interconnect area is reported separately by DC
and is not included in the total-cell-area/GE comparison.

## Evidence and limitations

The run finished with exit status zero and `STATUS=PASS`.  Its frozen source
hash is `37e37dcb1bc97d52a21dfe0c081f2d19083e0093b2d04fc88451b36c6760bb2b`;
the source-list hash is
`90510cce416717bf53976c0c00428452de36e91d0fef3b637b2f46970b81f25e`.
Raw reports and netlists are under
`results/final_early/dc_tsmc018_slow/recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_20ns/`.

The pre-compile lint includes analyzer-generated unused-combinational-cell
warnings and two intentionally unconsumed `current_slot` top nets.  DC
reported no violated constraints, zero setup/hold violating paths, and zero
max-transition/max-capacitance violating nets after mapping.  Existing signed
to unsigned warnings are in the inherited shared analyzer.  These diagnostics
are recorded, not waived by this document.

This closure is functionally gated by the committed C2/C3 evidence: directed
EARLY tests pass and the deterministic 1000-vector corrected Model-B2 RTL/C++
lockstep reports zero mismatches.  Full end-to-end remap remains deferred by
scope; it was not used as a synthesis blocker.
