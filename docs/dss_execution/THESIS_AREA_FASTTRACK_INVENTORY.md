# DATE2026 — Thesis Area Fast-Track Inventory

```text
INVENTORY_SCOPE: READ_ONLY
RTL_MODIFIED: NO
SIMULATOR_MODIFIED: NO
DSS_FINAL_MODIFIED: NO
SYNTHESIS_RUN: NO
FINAL_EARLY_AREA_CLOSURE: BLOCKED_PENDING_C2_C3_AND_NEW_SYNTHESIS
```

This is an architecture-separated inventory, not a new characterization. It
does not merge legacy/group/global results with the final 2x2 directional EARLY
Model-B2 target. The compact machine-readable copy is
`tmp/date2026/thesis_area_inventory/summary.csv` and is deliberately temporary.

## Frozen interpretation and proposed thesis presentation

| Record | Architecture / top | Total cell area (um²) | GE | Timing at 20 ns | Evidence state | Thesis status | Comparison rule |
| --- | --- | ---: | ---: | --- | --- | --- | --- |
| RECAM baseline | `recam_2r2c_analyzer` | 34,105.579462 | 3,417.67 | WNS 0.00, TNS 0.00 | phase3a reports; old provenance | usable with caveat | Baseline component only; not an integrated DSS comparison. |
| Canonical EARLY | `recam_dss_canonical_rs2_streaming_early_top` | 83,831.933613 | 8,400.67 | critical 19.79 ns, WNS 0.01, TNS 0.00 | report + metadata | historical only | `PRE_MODEL_B2`; do not label as final EARLY or compare as such. |
| Canonical GLOBAL core | `recam_dss_canonical_rs2_global_noscratch_top` | 61,751.290432 | 6,188.00 | WNS 0.02, TNS 0.00 | report + metadata | historical only | Core-only boundary; never compare with integrated EARLY. |
| V2 specialized EARLY | `recam_dss_v2_early_top` | 80,871.437539 | 8,104.00 | 20 ns reported | phase4e archive | historical only | No current source hash; predates Model-B2. |
| V2 specialized GROUP | `recam_dss_v2_group_noscratch_top` | 105,393.658552 | 10,561.33 | 20 ns reported | phase4f archive | historical only | Separate group policy, not EARLY. |
| Final GROUP archive | `recam_dss_hyp02_static_global_top` | 106,341.682630 | 10,656.33 | critical 19.82 ns, WNS/TNS 0.00/0.00 | immutable final archive | usable with caveat | Compare only with the same fixed-four-edge static GLOBAL boundary. |
| P3 SYN-A | canonical 2x2 EARLY | 83,831.933613 | 8,400.67 | 19.79 ns, WNS/TNS 0.00/0.00 | documented closure; raw directory absent | historical only | Same pre-Model-B2 result as canonical EARLY. |
| P3 SYN-B OPT1 | 2x2 GROUP/GLOBAL integrated | 395,369.256316 | 39,619.33 | 19.88 ns, WNS/TNS 0.00/0.00 | documented closure; raw directory absent | historical only | Architecture family differs. |
| P3 SYN-C | 1x4 EARLY | 1,080,926.994646 | 108,318.00 | 19.81 ns, WNS/TNS 0.00/0.00 | documented closure; raw directory absent | historical only | Topology differs from final 2x2 EARLY. |
| P3 SYN-D OPT1 | 1x4 GLOBAL | 1,267,025.771037 | 126,966.67 | 19.72 ns, WNS/TNS 0.00/0.00 | documented closure; raw directory absent | historical only | Topology and policy differ. |
| RS3 H5 EARLY | generic RS3/CS3/M1 EARLY | 12,190,913.30 | 1,221,632.33 | critical 62.52 ns; WNS -42.65 ns | documentation only; raw directory absent | provisional/historical | Failed timing; not a final normalized result. |
| RS3 H5 GROUP | generic RS3/CS3/M1 GROUP | 12,053,273.51 | 1,207,839.66 | critical 64.14 ns; WNS -44.24 ns | documentation only; raw directory absent | provisional/historical | Failed timing and a distinct family. |

The only current final-style static archive is the GROUP entry. Its metadata
declares `FIXED_FOUR_EDGE_STATIC_GLOBAL`, `GRID2X2_DIRECTIONAL_FIXED_FOUR_EDGE`,
151 architectural-state bits, 80 candidate bits, 81 legal static paths, a
20.0 ns clock, zero IO delays, `slow.db`, and DC W-2024.09-SP2. It is a
credible archival characterization, but it is not a substitute for a fresh
final EARLY top.

## Methodology/provenance and comparability notes

All area figures above use mapped total cell area divided by the recorded
NAND2X1 reference area of 9.979200 for GE. Where the report is present, the
corner is TSMC018 `slow.db`, 20.0 ns clock, and zero input/output delay. The
P3 closure requested `compile -map_effort low`; DC W-2024.09-SP2 emitted
`OPT-1303` and effectively used medium mapping effort uniformly. Any new
final EARLY run must preserve and report both the requested command and that
effective-tool behavior.

`results/p3_4pt_synth/` and `results/dss_v2_rs3cs3m1/h5_hardware_retry2/` are
not present in this checkout although their summarized values are documented.
Their numbers are retained above as documentation-only evidence, not as
reusable raw reports. The final GROUP archive has a source manifest and RTL
tree hashes; the pre-Model-B2 EARLY archive points to git `3304c86...` rather
than the current C1R3/C1R4 implementation. No old synthesis artifact can
therefore be reused for a final EARLY thesis result.

## Final EARLY area block and next step

```text
AREA_BLOCKED_BY_THIS_PHASE:
  The final directional EARLY RTL top does not yet exist. C1R4 establishes
  the static resource-action contract in the Model-B2 simulator only; it does
  not create RTL or a current source-provenanced synthesis closure.

AREA_NEXT_REQUIRED_STEP:
  Obtain human approval for C2, implement the separate final EARLY RTL and
  C3 regressions, then run a new isolated DC characterization of that exact
  top under the frozen 20 ns / zero-IO / slow.db method.

EXISTING_SYNTHESIS_REUSABLE: NO
NEW_SYNTHESIS_REQUIRED: YES
```

## C2 source/provenance plan only — no implementation

The immutable archive under
`dss_final/recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch/`
is a source reference, not a writable C2 destination. C2 must create a
separate EARLY source closure under `rtl/` and a new manifest which records
every copied/replaced file and SHA-256.

| Group-mother archive file | Classification for C2 | Reason / intended treatment |
| --- | --- | --- |
| `dss_v2_params_pkg.sv` | `IDENTICAL_TO_GROUP_MOTHER` | Shared constants/types only; copy only if the standalone final-EARLY closure needs local package provenance. |
| `dss_v2_types_pkg.sv` | `IDENTICAL_TO_GROUP_MOTHER` | Shared type definitions; same treatment as parameter package. |
| `recam_shared_config_analyzer.sv` | `IDENTICAL_TO_GROUP_MOTHER` | Preserve the C1R2/C1R3 shared analyzer contract; no EARLY policy logic belongs inside it. |
| `dss_v2_group_slot_decode.sv` | `IDENTICAL_TO_GROUP_MOTHER` or omit | Reuse unchanged only if a structural slot/config decode module is needed; otherwise fold a fixed, audited decode into the new EARLY core. |
| `dss_v2_group_candidate_store.sv` | omit | Candidate-store sequencing belongs to the GROUP mother and is not implied by the EARLY static-action theorem. |
| `recam_dss_hyp02_static_selector.sv` | omit | The 81-path static GLOBAL selector is a group/global policy artifact. |
| `recam_dss_hyp02_static_global_core.sv` | `EARLY_SPECIFIC_REPLACEMENT` | Replace with fixed `ABCD` / `R,L,RB,B` EARLY control, four-token availability state, and Model-B2 analyzer interface. |
| `recam_dss_hyp02_static_global_top.sv` | `EARLY_SPECIFIC_REPLACEMENT` | Replace with an EARLY top that exposes only the required final contract and drives the replacement core. |

The proposed core uses `available_common[3:0]` initialized to `4'b1111` and
updated only by `available_common <= available_common & ~claim_mask`; a token
bit can only transition 1 to 0 within a transaction. The fixed decode is:

| Current SA | R | L | RB | B |
| --- | --- | --- | --- | --- |
| A | `0000` | `0001` A-row | `0100` B-column | `0101` A-row+B-column |
| B | `0000` | `0100` B-column | `0010` D-row | `0110` B-column+D-row |
| C | `0000` | `1000` C-column | `0001` A-row | `1001` C-column+A-row |
| D | `0000` | `0010` D-row | `1000` C-column | `1010` D-row+C-column |

This gives `[3:0]={C-column,B-column,D-row,A-row}`. The current SA FSM state
and accepted slot decode the physical action; no `usedRows`, `usedColumns`, or
generic resource ledger is part of the final action decision. A fixed
borrowed-slot indicator still enforces `m=1`; it is derived from the accepted
slot, not candidate demand.

Policy order is a compile-time/fixed control fact: `SA=A,B,C,D` and slots
`R,L,RB,B`. The current SA requires FSM state. The accepted slot (or its
equivalent fixed config decode) must remain stable from acceptance through the
analyzer-result/remap handoff; token availability is committed on the
acceptance edge and need not be carried as an output payload. Candidate and
remap data already required by that handoff retain their normal lifetime; the
static claim mask itself is not a demand payload.

Minimum C3 acceptance tests, to be frozen before synthesis, are: complete
16-action decode truth table; reset/restart `4'b1111`; monotonic token
consumption; `ABCD` and `R,L,RB,B` priority; Vector216 (`C:R`, local L not
reached); the 16 C1R coordinates; the four P1-only losses under the
development replay; and a per-acceptance assertion that the physical action
equals the fixed SA/slot table. Synthesis may begin only after those tests
pass and the C2 source manifest is frozen.

```text
AREA_BLOCKED_BY_THIS_PHASE: YES
AREA_NEXT_REQUIRED_STEP: HUMAN_APPROVAL_FOR_C2_THEN_C2_C3
EXISTING_SYNTHESIS_REUSABLE: NO
NEW_SYNTHESIS_REQUIRED: YES
```
