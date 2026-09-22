# FINAL-EARLY-C1 — Priority Context and SA-Order Study

```text
FINAL_EARLY_C1_STATUS: BLOCKED
C1_START_HEAD: 78bf9c4367d3944a71f1454ba1abf67941ee1448
```

This C1 study is analysis only. It adds no production RTL, does not modify the
production simulator, does not modify `dss_final/`, and does not run synthesis
or a formal repair-rate sweep.

## Frozen first-claim interpretation

The tested resource state is `common_spare_available_q[3:0]`, reset to
`4'b1111`. A one means that its physical common line is unclaimed and available
to an allowed user; a zero means that a previously committed SA has claimed
the line. Token transitions are monotonic `1 -> 0`; an owner that does not use
its common line performs no release write.

The fixed token order is `[3:0] = {C_COL, B_COL, D_ROW, A_ROW}`. The
production-directional topology is unchanged: A_ROW->C, D_ROW->B, B_COL->A,
and C_COL->D.

For an actual selected candidate, the independent C1 oracle derives its claim
set from the production remap demand:

| SA | Own-token claim | Borrow-token claim |
| --- | --- | --- |
| A | `usedRows >= 2` claims A_ROW | `usedColumns > 2` claims B_COL |
| B | `usedColumns >= 2` claims B_COL | `usedRows > 2` claims D_ROW |
| C | `usedColumns >= 2` claims C_COL | `usedRows > 2` claims A_ROW |
| D | `usedRows >= 2` claims D_ROW | `usedColumns > 2` claims C_COL |

This is a simulator-oracle observation, not a proposed RTL decode. It is
equivalent to whether `PhysicalResourceLedger` assigns the relevant one of the
four directional shareable physical lines. The C1 oracle uses R,L,RB,B slot
order `{1,0,3,2}` and checks its ABCD selected slot and PatternID against the
production `DirectionalV2Early` result for every studied group.

## C1A priority-context claim-mask proof

The deterministic corpus uses seed 20260922, RS=CS=2, m=1, directional
topology, `maximumGroupBorrowedSpares=1`, ModerateImbalance/Mixed faults, and
F_GROUP 16/20/24/28 with 300 groups per point. For every accepted candidate,
the oracle recorded `{SA, accepted slot, ConfigID, PatternID, higher-priority
failure history, usedRows, usedColumns, claim mask}`. Higher-priority failures
are recorded as `I` (analyzer/candidate invalid) or `X` (resource illegal).

ABCD first-claim replay matched production selected slots and PatternIDs for
all 1,200 groups. It produced 13 observed detailed priority contexts, of which
two have more than one exact claim mask:

| Context | First witness | Claim | Second witness | Claim |
| --- | --- | --- | --- | --- |
| `B:L:R=I` | F20, group 108, Pattern 2, 2R1C | NONE | F24, group 4, Pattern 3, 2R2C | B_COL |
| `C:L:R=I` | F16, group 7, Pattern 3, 2R1C | NONE | F16, group 178, Pattern 6, 0R2C | C_COL |

Thus current SA + accepted role slot + all higher-priority slots having failed
does not uniquely determine the common-spare claim set. The ambiguity remains
even when the reason for each earlier failure is included. No fixed priority-
context claim-mask table can replace current remap demand.

`maximumGroupBorrowedSpares=1` did not add a separate observed ambiguity in
this corpus: no reachable resource-legal borrow probe had the same
SA/context/token/claim tuple with both zero and one prior borrow. This is
negative corpus evidence only; it does not repair the claim-mask ambiguity.

## Known FINAL-EARLY-C replay

The 19 previously recorded mismatch vector IDs were regenerated with the same
seed and the original 250-group-per-point corpus. The exact-demand first-claim
oracle matched production on all 19, confirming that the reconstructed corpus
and physical claim interpretation are consistent with the production source.

A static table formed from the observed detailed priority contexts resolved
14/19. It cannot resolve vectors 216, 751, 780, 810, or 954 because an
encountered context has multiple possible claim masks. Therefore:

```text
KNOWN_19_RESOLVED_BY_PRIORITY_CONTEXT: 14/19
COUNTEREXAMPLE_216_RESOLVED: NO
```

Vector 216 is the concrete `C:L:R=I` case. C selects PatternID 2 with actual
demand 2R1C, so it claims no C_COL; D then selects B with 2R3C and claims
D_ROW|C_COL. A fixed C:L claim of C_COL incorrectly blocks D, while a fixed
claim of NONE is wrong for the F16/group-178 witness.

## Analyzer interface verdict

ConfigID is a fixed reconstruction from current SA and role slot, so no
explicit analyzer ConfigID output is required. By contrast, the two ambiguity
witnesses prove that valid + PatternID + priority context do not determine the
claim. The final exact controller needs current-candidate demand or an
equivalent current claim-mask signal derived by a formally specified remap
decoder.

```text
EXPLICIT_ANALYZER_CONFIGID_OUTPUT_REQUIRED: NO
ACTUAL_DEMAND_OUTPUT_REQUIRED: YES
USED_ROWS_OUTPUT_REQUIRED: YES
USED_COLUMNS_OUTPUT_REQUIRED: YES
FIXED_NO_DEMAND_CLAIM_DECODE: NONE; DISPROVED BY B:L:R=I AND C:L:R=I
```

This conclusion does not require historical ConfigID, PatternID, borrower ID,
donor ID, or release-valid state. The missing information is current-candidate
resource use.

## C1B paired ABCD vs ABDC experiment

The isolated analysis runner evaluates ABCD and ABDC over the same 1,200
candidate/remap groups. It does not change production's SA order or add a
runtime production policy option.

| F_GROUP | Total | Both pass | Both fail | ABDC only | ABCD only | ABCD rate | ABDC rate | Delta pp |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 16 | 300 | 281 | 19 | 0 | 0 | 93.666667% | 93.666667% | 0.000000 |
| 20 | 300 | 230 | 70 | 0 | 0 | 76.666667% | 76.666667% | 0.000000 |
| 24 | 300 | 165 | 135 | 0 | 0 | 55.000000% | 55.000000% | 0.000000 |
| 28 | 300 | 84 | 216 | 0 | 0 | 28.000000% | 28.000000% | 0.000000 |
| Aggregate | 1200 | 760 | 440 | 0 | 0 | 63.333333% | 63.333333% | 0.000000 |

There are no ABDC-only or ABCD-only representatives in this corpus. Some
both-fail cases terminate at C for ABCD and at D for ABDC after a different
first claim, but neither order makes the group repairable. Consequently this
development run supplies no evidence for an SA-order change and makes no
formal dominance claim.

## Reproducibility and generated evidence

Reusable runner:
`tests/final_early_c1_priority_context_and_sa_order_study.cpp`

Command:

```text
make test_final_early_c1_priority_context_and_sa_order_study
```

Generated, ignored evidence is confined to:

```text
tmp/date2026/final_early_c1/claim_mask_proof/
tmp/date2026/final_early_c1/order_ab/
```

The runner writes manifests, full paired results, ambiguity witnesses, known-
mismatch replay, and divergence traces there. It also asserts ABCD slot and
PatternID equality with production on the 1,200-group C1 corpus and exact-
demand equality for all 19 replay vectors.

## Final C1 status

```text
FINAL_EARLY_C1_STATUS: BLOCKED
C1_START_HEAD: 78bf9c4367d3944a71f1454ba1abf67941ee1448
C1_FINAL_COMMIT: THIS_C1_CHECKPOINT_COMMIT
TOKEN_RESET: 4'b1111
TOKEN_SEMANTICS: UNCLAIMED_AVAILABLE
TOKEN_TRANSITIONS: 1_TO_0_ONLY
MINIMUM_PERSISTENT_SHARING_BITS: 4 AVAILABILITY BITS; NO-DEMAND CONTRACT NOT VIABLE
PRIORITY_CONTEXT_DETERMINES_EXACT_CLAIM_MASK: NO
RESOURCE_ACTION_AMBIGUITIES: 2
KNOWN_19_RESOLVED_BY_PRIORITY_CONTEXT: 14/19
COUNTEREXAMPLE_216_RESOLVED: NO
EXPLICIT_ANALYZER_CONFIGID_OUTPUT_REQUIRED: NO
ACTUAL_DEMAND_OUTPUT_REQUIRED: YES
USED_ROWS_OUTPUT_REQUIRED: YES
USED_COLUMNS_OUTPUT_REQUIRED: YES
F_GROUP_POINTS: 16,20,24,28
GROUPS_PER_POINT: 300
TOTAL_PAIRED_GROUPS: 1200
AGGREGATE_BOTH_PASS: 760
AGGREGATE_BOTH_FAIL: 440
AGGREGATE_ABDC_ONLY: 0
AGGREGATE_ABCD_ONLY: 0
ABCD_AGGREGATE_REPAIR_RATE: 63.333333%
ABDC_AGGREGATE_REPAIR_RATE: 63.333333%
ORDER_DELTA_PP: 0.000000
RECOMMENDED_FINAL_SA_ORDER: UNRESOLVED
FOUR_BIT_NO_DEMAND_PORT_ARCHITECTURE_VIABLE: NO
RTL_MODIFIED: NO
DSS_FINAL_MODIFIED: NO
SYNTHESIS_RUN: NO
FORMAL_SWEEP_RUN: NO
TMP_OUTPUT_ROOT: tmp/date2026/final_early_c1/
PERMANENT_REPORT: docs/dss_execution/FINAL_EARLY_C1_PRIORITY_CONTEXT_AND_SA_ORDER_STUDY.md
WORKTREE_CLEAN_AFTER_COMMIT: YES
NEXT_ACTION: STOP FOR HUMAN REVIEW BEFORE FINAL-EARLY-C2
```

```text
VERILOG_SKILL_RULES_USED:
- Read the readable-verilog-generator dispatcher, complete SKILL.md, and
  ASIC Verilog quality reference.
- Performed only read-only analyzer/RTL contract review; no RTL was created
  or modified in C1.
- Kept production policy, topology, and simulator source untouched; the SA
  order override exists only in the C1 C++ analysis runner.
```
