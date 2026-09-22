# FINAL-EARLY-C3 — Equivalence and Synthesis Gate

```text
FINAL_EARLY_C3_STATUS: PASS_FOR_MATCHED_PPA_BOUNDARY
SYNTHESIS_AUTHORIZATION_CONSUMED: NO
RTL_MODIFIED: YES (C2 only)
SIMULATOR_SEMANTICS_MODIFIED: NO
DSS_FINAL_MODIFIED: NO
SYNTHESIS_RUN: NO
```

## Passed controller and Model-B2 gates

| Required evidence | Result |
| --- | --- |
| C2 directed SA/slot table, immediate commit, first failure | PASS — `make test_recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_rtl` |
| C1R4 16-action static theorem, Vector216, 16 C1R and four P1 losses | PASS — `make test_final_early_c1r4_static_action` |
| C1R3 shared collector, Vector216 PatternID 2 and analogous 16 | PASS — `make test_final_early_c1r3_shared_collector` |
| Corrected Model-B2 vs Verilator controller lockstep | PASS — exactly 1,000 vectors, 0 mismatches |
| Existing policy and layout regression set | PASS — solution-take, multi-config analyzer, dynamic policy and layout |
| Final top/analyzer static compile | PASS — Verilator lint-only with Model-B2 `HYBRID_ENTRIES=11` |

The 1,000 lockstep vectors are not a statistical result. The generator creates
four deterministic 250-group partitions at F_GROUP 16/20/24/28 from the
corrected Model-B2 `DirectionalV2Early` simulator. For each group it records
the complete production-created 4-SA x 4-slot validity/pattern schedule and
the production selected ConfigID/PatternID/failure result. The Verilator
controller consumes that same schedule. Its repairability, failure position,
prefix commit bitmap, and every committed ConfigID/PatternID match:

```text
MODEL_B2: PASS
STATIC_ACTION: PASS
VECTOR216: PASS
1000_VECTOR_MISMATCHES: 0
NO_GENERIC_LEDGER: PASS
COMMON_SPARE_STATE_BITS: 4
NO_ACTUAL_DEMAND_PORT: PASS
NO_USEDROWS_PORT: PASS
NO_USEDCOLUMNS_PORT: PASS
NO_GROUP_CANDIDATE_STORE: PASS
NO_GROUP_81_PATH_SELECTOR: PASS
NO_DYNAMIC_RESOURCE_INDEXING: PASS
ANALYZER_ALGORITHM_UNCHANGED: PASS
MODEL_B2_SHARED_STATE: PASS (260-bit/SA, eleven Hybrid entries)
```

`borrow_flat_o` is a committed repair-result field, not a generic resource
index or resource-owner ledger. The only availability state is
`common_spare_available_o[3:0]` and each update is an AND with the complement
of the fixed SA/slot claim mask.

## Deferred full-remap scope

The full item “final repair-address decode coverage” cannot be passed from
the current C2 interface. `recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top` exports ConfigID,
PatternID, borrow classification, and completion metadata but does not export
a final physical repair-line/remap address payload. The immutable GROUP mother
has the same kind of final result metadata interface, but its top likewise
does not establish a physical remap-address output contract.

The Model-B2 260-bit source summary contains enough retained collector
information for a future address reconstruction stage, but C2 deliberately
does not invent its output format, update timing, ownership, or lifetime.
Adding a new address output now would be an architecture/interface expansion.
Per C2R clarification, full end-to-end remap is deferred and therefore does
not block the matched PPA comparison.

```text
FINAL_REPAIR_ADDRESS_DECODE_COVERAGE: DEFERRED
```

## Synthesis decision

The C2R/PPA-unlock clarification permits DC with full remap deferred and
accepts the justified 204/7 to 260/11 Model-B2 retained-state delta. A
separate environmental issue previously prevented the requested C2 staging/commit:
the `git add` escalation was rejected by the execution service for account
usage limit, not by a source-control or RTL error. No workaround was used.

```text
C3_FUNCTIONAL_MODEL_B2: PASS
C3_MATCHED_PPA_BOUNDARY: PASS (see FINAL_EARLY_C2R_MOTHER_PROVENANCE_AND_INPUT_BOUNDARY_AUDIT.md)
C3_FULL_END_TO_END_REMAP: DEFERRED
PRE_SYNTH_STRUCTURAL_GATE: PASS
READY_FOR_SYNTHESIS: YES after clean commit
NEXT_ACTION: COMMIT_AND_SYNTHESIZE_FINAL_EARLY
```
