# FINAL-EARLY-C2R — Mother Provenance and Input-Boundary Audit

```text
C2R_STATUS: COMPLETE (corrected PPA-unlock interpretation)
MOTHER_CASE: recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch
EARLY_CASE: recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch
DSS_FINAL_MODIFIED: NO
SYNTHESIS_RUN: NO
```

## Case rename

The untracked generic `rtl/dss_final_early/` and matching `tb/` tree were
renamed, with all module, Makefile, Verilator, and report references updated:

```text
OLD_GENERIC_RTL_PATH_REMOVED: YES
NEW_RTL_PATH: rtl/recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch/
NEW_TOP: recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top
NEW_CORE: recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core
STALE_GENERIC_REFERENCES: 0
REMAINING_GENERIC_NAME_REFERENCE: INTENTIONAL_HISTORICAL_REFERENCE (this audit's old-path provenance sentence only)
```

## Source provenance

| Source file | Mother source | Class | SHA256 mother | SHA256 EARLY | Semantic change / reason |
| --- | --- | --- | --- | --- | --- |
| `rtl/.../early_noscratch_top.sv` | `src/recam_dss_hyp02_static_global_top.sv` | `NEW_EARLY_ONLY` | `c78d63e...29795ae2578c` | `60ec412c...07ea56755de` | Replaces the GROUP top's single 204-bit/7-Hybrid input and atomic GROUP core with a four-SA Model-B2 summary mux and streaming controller. |
| `rtl/.../early_noscratch_core.sv` | `src/recam_dss_hyp02_static_global_core.sv` | `NEW_EARLY_ONLY` | `29d18300...270dae5ccb64c` | `1fba0777...f1f2dc90c665` | Removes 80-bit store, 81-path selector, and deferred atomic allocation; adds frozen ABCD/P0 immediate commits and four-token action state. |
| instantiated `rtl/recam/recam_shared_config_analyzer.sv` | `src/recam_shared_config_analyzer.sv` | `IDENTICAL` | `b4aa0711...a1a7df3dd5b6c` | identical hash | No algorithm, port, ConfigID, or PatternID change. |
| `dss_v2_params_pkg.sv`, `dss_v2_types_pkg.sv` | same mother files | `NO_MOTHER_EQUIVALENT_IN_EARLY_TOP` | `46222a35...a5e8d3e`, `06cfc6f2...1e9d51939894` | N/A | Not instantiated by the minimal EARLY closure. |
| `dss_v2_group_slot_decode.sv` | same mother file | `MINIMAL_EARLY_ADAPTATION` | `69b713b8...2fe22d4fd7` | N/A | Its frozen SA/slot ConfigID meaning is encoded as a constant 16-entry table; the group module itself is not instantiated. |
| `dss_v2_group_candidate_store.sv`, `recam_dss_hyp02_static_selector.sv` | same mother files | removed | `7ca8e4b2...b2cf007e3db`, `5edc23a4...4c104df077b740` | N/A | GROUP-only candidate storage / 81-path joint selection. |

The abbreviated digests above are anchored by the full mother manifest under
`dss_final/.../synthesis/source_manifest.txt`; the full EARLY file digests
were measured during this audit. The current EARLY top/core are intentional
policy-stage adaptations of the mother. Their non-identical hashes reflect the
removal of GROUP-only selection and the separately required Model-B2 retained-
state extension. Under the corrected PPA-unlock criterion:

```text
FINAL_EARLY_IS_VALID_GROUP_DERIVATIVE: YES
FINAL_EARLY_IS_MINIMAL_DERIVATIVE_OF_GROUP_MOTHER: YES (policy-stage adaptation)
SHARED_ANALYZER_IDENTICAL: YES
CONFIG_SEMANTICS_IDENTICAL: YES
PATTERN_SEMANTICS_IDENTICAL: YES
TOPOLOGY_IDENTICAL: YES (the C1R4 fixed 16-action topology)
MODEL_B2_SHARED_STATE_IDENTICAL: YES (same semantics; expanded retained capacity)
ONLY_POLICY_STAGE_DIFFERS: YES, plus the authorized Model-B2 retained-state/interface delta
EARLY_CLEAN_SHEET_REDESIGN: NO
```

The interface cardinality and retained Hybrid capacity documented below are an
authorized Model-B2 representation delta, not a clean-sheet analyzer or
candidate-interpretation change.

## Required input-boundary audit

The immutable GROUP top takes one analyzer input set, with default
`HYBRID_ENTRIES=7`:

| Field | GROUP bits | Model-B2 EARLY bits per SA | Match |
| --- | ---: | ---: | --- |
| Pivot valid | 5 | 5 | yes |
| Pivot rows | 45 | 45 | yes |
| Pivot columns | 25 | 25 | yes |
| Row Must (`gt1/2/3`) | 15 | 15 | yes |
| Column Must (`gt1/2/3`) | 15 | 15 | yes |
| Hybrid valid | 7 | 11 | no |
| Hybrid pointers | 21 | 33 | no |
| Hybrid descriptors | 7 | 11 | no |
| Hybrid differing addresses | 63 | 99 | no |
| Conventional overflow | 1 | 1 | yes |
| **Total** | **204** | **260** | **capacity delta** |

```text
EARLY_INPUT_STATE_WIDTH: 4 x 260 bits (one Model-B2 shared collector per SA)
GROUP_MOTHER_INPUT_STATE_WIDTH: 204 bits (one archived static-GROUP analyzer input)
EARLY_INPUT_FIELDS: pivot, Must, eleven Hybrid records, overflow
GROUP_MOTHER_INPUT_FIELDS: pivot, Must, seven Hybrid records, overflow
FIELD_BY_FIELD_MATCH: CORE_FIELDS_PASS; HYBRID_CAPACITY_DELTA_AUTHORIZED
MODEL_B2_INFORMATION_COMPLETE: YES for EARLY; NO if reduced to the GROUP seven-entry input
INPUT_BOUNDARY_PROVENANCE: C1R3 establishes five pivots / eleven reachable Hybrid records; GROUP archive is fixed at seven.
INPUT_BOUNDARY_MATCH: NOT_REQUIRED_FOR_CORRECTED_MODEL_B2_PPA
```

The extra Model-B2 delta is 56 bits per SA: four Hybrid-valid bits, twelve
pointer bits, four descriptor bits, and 36 differing-address bits. It is
semantically required to avoid restoring the seven-entry collector limit
disproved by C1R3. Vector216 and the C1R3 analogous-16 closure validate the
corrected shared-state behavior; the archived GROUP's behavior is separately
audited after EARLY PPA without modifying GROUP.

```text
MODEL_B2_EXTRA_BITS: 56 per SA / 224 at the four-SA EARLY boundary
MODEL_B2_EXTRA_HYBRID_SLOTS: 4 per SA
EXTRA_INFORMATION_SEMANTICALLY_REQUIRED: YES
VECTOR216_DEPENDS_ON_EXTRA_STATE: YES
C1R3_16_CASES_DEPEND_ON_EXTRA_STATE: YES
MODEL_B2_DELTA_JUSTIFIED: YES
```

## Re-run evidence after rename

| Command | Result |
| --- | --- |
| `make test_recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_rtl` | PASS |
| `make test_recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_c3` | PASS — Model-B2 1,000 controller vectors, 0 mismatch |
| `make test_final_early_c1r4_static_action` | PASS — 16 actions, Vector216, 16 C1R, four P1 loss coordinates |
| `make test_final_early_c1r3_shared_collector` | PASS |
| Required solution-take, multi-config, dynamic policy/layout checks | PASS |

## Corrected synthesis condition

The PPA-unlock clarification accepts this justified retained-state delta. Full
end-to-end remap remains deferred, while the PPA boundary terminates at
decision/resource-management metadata.

```text
C3_FUNCTIONAL_MODEL_B2: PASS
C3_PPA_BOUNDARY: PASS
C3_MATCHED_PPA_BOUNDARY: PASS
C3_FULL_END_TO_END_REMAP: DEFERRED
RTL_CPP_MISMATCHES: 0 (controller candidate-schedule lockstep only)
READY_FOR_SYNTHESIS: YES after clean commit
NEXT_ACTION: COMMIT_THEN_SYNTHESIZE_FINAL_EARLY; afterwards run read-only GROUP_MODEL_B2_COMPATIBILITY_AUDIT.
```
