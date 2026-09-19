# P3 four-point GLOBAL optimization fairness audit

## Decision

```text
PHASE: P3-4PT-GLOBAL-OPT-AUDIT
P3_4PT_GLOBAL_OPT_AUDIT_STATUS: COMPLETE
COMMON_OPT_TAXONOMY:
  OPT0 = baseline exhaustive search
  OPT1 = semantics-preserving pre-DFS candidate-equivalence collapse only
  OPT2 = DFS state dominance/pruning
  OPT3 = summary/state compression

SYN_B_OPT0_VERIFIED: YES
SYN_B_HISTORICAL_OPT1_KEY: (explicit_release, actual_release, actual_borrow)
SYN_B_OPT1_SAFE: YES
SYN_B_OPT1_TAXONOMY: COMMON_OPT1

SYN_D_OPT0_VERIFIED: YES
SYN_D_PROPOSED_OPT1_KEY: (usedRows, usedColumns)
SYN_D_OPT1_THEOREM_FROZEN: YES
SYN_D_OPT1_SAFE: YES
SYN_D_CAN_USE_SYN_B_OPT1_KEY: NO
SYN_D_OPT1_IMPLEMENTED: YES

FINAL_COMMON_GLOBAL_OPT_LEVEL: OPT1
FINAL_SYN_B_GLOBAL_ELABORATION: recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_integrated_top
FINAL_SYN_D_GLOBAL_ELABORATION: recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_top
RUNTIME_OPT_SELECTION_MUX: NO
FOUR_POINT_SYNTHESIS_BOUNDARY_AUDITED: YES
VERILATOR_LINT: PASS
STRICT_READABLE_VERILOG_GATE: BLOCKED_BY_LOCAL_PYTHON_3_8_SKILL_RUNTIME
DC_SYNTHESIS_STARTED: NO
OPT2_STARTED: NO
OPT3_STARTED: NO
```

The common result is OPT1 because both GLOBAL designs now use only an
independently proved, pre-DFS candidate-equivalence collapse. Neither uses
DFS-state dominance, failure caching, summary compression, or an optimization
selection input at this level. The keys differ because the downstream resource
models differ; equivalence class, rather than identical key encoding, is the
fairness criterion.

## Common-optimum decision table

| Architecture | OPT0 | OPT1 theorem | OPT1 implemented | OPT1 oracle-equivalent | Eligible for final synthesis |
|---|---|---|---|---|---|
| SYN-B 2×2 GLOBAL | Yes | Yes | Yes: existing static wrapper/top | Yes: directed plus 1,000 maps | Yes: OPT1 integrated top |
| SYN-D 1×4 GLOBAL | Yes | Yes | Yes: static core/top wrappers added by this audit | Yes: directed plus 1,000 shared C++ cases | Yes: OPT1 integrated top |

## SYN-B reconstruction

```text
OPT1_CLASS_KEY:
  (explicit_release, actual_release, actual_borrow)

OPT1_REPRESENTATIVE_RULE:
  retain the first valid slot in canonical R, L, RB, B / PatternID order.

OPT1_DISCARD_RULE:
  discard a later valid slot only when an earlier valid slot at the same SA
  has exactly the same three key fields.

OPT1_CANONICAL_ORDER_PRESERVATION:
  the wrapper translates canonical rank to the stored L, R, B, RB map layout
  and passes the surviving bit at its original stored slot. The child core
  therefore reconstructs the same action, ConfigID, PatternID, and donor
  tuple as OPT0.

OPT1_PROOF_SCOPE:
  only candidate validity is filtered before the frozen DFS. No DFS state,
  cursor, resource map, objective, or commit behavior is changed.

OPT1_TEST_EVIDENCE:
  directed group172 and effect-only counterexample plus 1,000 seeded maps;
  all compared selected fields and final state match.
```

At a fixed depth the 2×2 transition reads only the key fields plus the prefix
state: actual release updates the depth's release resource; actual borrow
updates the fixed donor resource, borrow count, used mask, and possibly future
release obligation; `explicit_release` determines whether a release clears
that obligation. A same-key candidate therefore has an identical legality
predicate and successor `(released, used, obligation, borrow_count)` state.
PatternID and ConfigID are reconstruction metadata, not transition fields;
mapping identity is not an RTL candidate input. They cannot be arbitrarily
discarded: retaining the first canonical slot preserves their exact selected
representation and the frozen search order.

The authoritative implementation is
`rtl/dss_canonical/policy/global/recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_core.v`;
the downstream transition is
`rtl/dss_canonical/policy/global/recam_dss_canonical_global_noscratch_core.v`.

## SYN-D reconstruction and implementation gate

SYN-D does not encode the SYN-B three-effect key. Its candidate input is a
row/column demand. Adjacent donor identity, individual physical row owner,
borrow count, and lendable/non-lendable status are produced by applying that
demand to the private prefix ledger. The exact safe candidate relation is
equal `(usedRows, usedColumns)` demand, retaining the representative with
lower `(PatternID, attemptIndex)` under the frozen GLOBAL objective.

The C++ `globalPlansForSubarray` implementation already uses that exact map
key and representative rule. The RTL proof and counterexamples are recorded
in [the SYN-D theorem](P3_4PT_SYN_D_OPT1_THEOREM.md) and
[counterexample record](P3_4PT_SYN_D_OPT1_COUNTEREXAMPLES.md).

The audit reached the implementation gate. The former hidden canonicalization
is now an explicit compile-time choice in
`recam_dss_line1x4_rs2_cs2_m1_normalized_global_core`:

- OPT0: `recam_dss_line1x4_rs2_cs2_m1_normalized_global_top` fixes
  `ENABLE_OPT1_CLASS_COLLAPSE=0` and visits every valid candidate.

- OPT1: the distinct
  `recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_core`
  and `..._top` wrappers fix it to `1` during elaboration.

The setting is a Verilog parameter, not a port. There is no runtime mux. The
private ledger, atomic commit, producer, external ports, and selected result
interface are shared unchanged between the two static elaborations.

## Functional evidence

```text
SYN_D_OPT1_CPP_ORACLE_CASES: 1000
SYN_D_OPT1_CPP_ORACLE_SEED: 20260921
SYN_D_OPT1_REPAIRABILITY_MISMATCHES: 0
SYN_D_OPT1_SELECTED_TUPLE_MISMATCHES: 0
SYN_D_OPT1_OBJECTIVE_MISMATCHES: 0
SYN_D_OPT1_FINAL_OWNER_MISMATCHES: 0
SYN_D_OPT1_ATOMIC_COMMIT_MISMATCHES: 0
SYN_D_OPT1_GLOBAL_ADJACENCY_MISMATCHES: 0
SYN_D_OPT1_BORROWED_LINE_FORWARDING_VIOLATIONS: 0
SYN_D_OPT1_CANONICAL_TIE_MISMATCHES: 0
SYN_D_EARLY_GLOBAL_DISTINCTION_REGRESSION: PASS
SYN_D_D_TO_A_BOUNDARY_REGRESSION: PASS
SYN_D_NO_FORWARDING_REGRESSION: PASS
SYN_D_OPT1_INTEGRATED_END_TO_END_MISMATCHES: 0
SYN_D_OPT1_PARTIAL_COMMIT_VISIBILITY_ERRORS: 0
SYN_D_OPT0_OPT1_DIRECTED_MISMATCHES: 0 (3 directed cases)
SYN_D_OPT0_OPT1_SHARED_CORPUS_MISMATCHES: 0 (1,000 cases, seed 20260921)
```

OPT0 independently produced the same zero C++-oracle and directed-test
mismatch counts. A parallel-DUT wrapper also directly compares both static
elaborations across the 1,000 shared maps and three directed cases. The
comparison includes repairability, complete selected tuple, donor list, final
row assignment, borrow count, and used-row objective; it is not limited to a
success flag.

Candidate and visit distributions are in
[P3_4PT_GLOBAL_OPT_STATISTICS.md](P3_4PT_GLOBAL_OPT_STATISTICS.md).

## Synthesis boundary audit

| Point | Event/input boundary | Policy elaboration for final synthesis | End boundary |
|---|---|---|---|
| SYN-A | `recam_dss_canonical_rs2_streaming_early_top`: integrated snapshot/analyzer inputs | closed STREAMING_EARLY | selected repair and persistent resource result |
| SYN-B | integrated four-snapshot/analyzer boundary | `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_integrated_top` | selected group tuple and atomic committed ledger |
| SYN-C | `recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_top`: integrated four-snapshot/analyzer inputs | closed STREAMING_EARLY | selected repair and persistent resource result |
| SYN-D | integrated four-snapshot/producer boundary | `recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_top` | selected group tuple and atomic committed ledger |

The OPT0 comparison tops remain available for the two GLOBAL points:
`...group_global_noscratch_integrated_top` for SYN-B and
`recam_dss_line1x4_rs2_cs2_m1_normalized_global_top` for SYN-D. The final
four-point selection uses the OPT1 pair only. Candidate interface size,
physical ownership state, pivot count, and internal latency remain
architecture-specific costs; they were not forced equal by this audit.

No DC, area, timing, or cycle-performance conclusion is made here.
The readable-Verilog skill's strict generated-deliverable gate could not start
RTL analysis locally because Python 3.8 rejects its `dict[str, ...]` type
annotation in `analysis_mixin.py`; Verilator lint completed successfully for
both final GLOBAL OPT1 elaborations.
