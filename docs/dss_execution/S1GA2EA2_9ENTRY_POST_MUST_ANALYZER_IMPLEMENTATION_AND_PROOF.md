# S1G-A2E-A2 — 9-Entry Post-Must Analyzer Implementation and Proof

## 1. Executive status

**COMPLETE.** The isolated Option-A path now applies exact post-Must filtering
to a historical 14-entry physical Hybrid view, stably compacts survivors, and
passes nine slots to the canonical Config/Pattern analyzer. No legal projected
view greater than nine was observed.

## 2. A2D-R frozen predicate

```text
physical_valid && cfg_valid && legal_pointer && !final_must_retired
```

`legal_pointer` is historical `pointer < MAX_K`. Descriptor zero selects
`row_must_by_cfg`; descriptor one selects `col_must_by_cfg`. Config membership
remains a separate stored field.

## 3. RTL scope

| Production RTL file | Authorized function |
| --- | --- |
| `rtl/recam/recam_post_must_hybrid_projector.sv` | Exact predicate, H0..H13 compaction, count, and explicit no-truncation checker. |
| `rtl/recam/recam_post_must_projected_config_analyzer.sv` | Isolated wrapper and nine-slot specialization of the canonical analyzer. |

No baseline analyzer, EARLY, GROUP, collector, ledger, or control module was
modified.

## 4. Projector implementation

The projector scans the fourteen physical slots in ascending H index and copies
each survivor to the next compact slot. `projected_count_o` is a four-bit full
survivor count. If an impossible tenth survivor occurs, the module sets
`projection_overflow_o` and a simulation-only assertion, rather than silently
accepting a nine-entry truncation.

## 5. 7-to-9 analyzer expansion

The wrapper instantiates `recam_shared_config_analyzer` with
`HYBRID_ENTRIES=9`. Its ConfigID decode, candidate tables, transpose behavior,
candidate priority, scan order, and PatternID selection are unchanged.

## 6. Input-width derivation

```text
old Hybrid payload = 7 × 14 = 98 bits
new Hybrid payload = 9 × 14 = 126 bits
unchanged analyzer base = 106 bits

OLD_ANALYZER_INPUT_BITS = 204
NEW_ANALYZER_INPUT_BITS = 232
```

`NUM_CONFIGS=7` remains distinct from `POST_MUST_VIEW_ENTRIES=9`; the
four-bit projected count is not retained-state sizing.

## 7. ConfigID preservation

The three-bit ConfigID and seven membership bits per physical record are
unchanged. Role-slot meaning and ConfigID encoding are not modified.

## 8. PatternID preservation

PatternID remains a four-bit first-feasible candidate index. Additional compact
slots carry fault evidence only; they do not create candidates or alter
candidate priority, scan order, or spare-index allocation semantics.

## 9. Blocker counterexample regression

One Pivot plus eleven same-row faults produces membership-visible count 11 for
every Config. Final RowMust retires all eleven, so new RTL projects zero entries
without overflow and matches the historical oracle. The same-column dual does
the same for ColMust.

## 10. RowMust tests

DR-1 and the capacity witnesses verify descriptor-zero selection of
`row_must_by_cfg`. Config 2 reaches nine survivors with row extras
`[1,1,1]` and column extras `[2,2,2]`.

## 11. ColMust tests

DR-2 verifies descriptor-one selection of `col_must_by_cfg`. Config 5 reaches
nine survivors with row extras `[2,2,2]` and column extras `[1,1,1]`.

## 12. Mixed retirement tests

Directed tests cover simultaneous row and column retirement, pointer-3
membership holes, and a directly injected H-order vector. The injected vector
contains survivors at H0, H3, and H6; the projector emits exactly that order
while excluding cfg-invalid, physical-invalid, RowMust-retired, and
ColMust-retired records.

## 13. Capacity-bound witnesses

All A2D-R maxima `[8,3,9,8,3,9,8]` were driven through RTL/reference
comparison. Configs 2 and 5 occupy all nine compact slots without projection
overflow, truncation, candidate mismatch, feasibility mismatch, PatternID
mismatch, or dictionary-overflow mismatch.

## 14. Historical-equivalence results

The C++ oracle constructs historical physical Pivot/Hybrid state, membership,
physical counts, final Must retirement, stable order, and canonical candidate
evaluation. It compares candidate bitmap, PatternID, solution-valid,
repairable, and dictionary-overflow against RTL.

```text
RTL/reference states:       10,007
Config evaluations:         70,049
Membership-visible >9:      1,232 evaluations
Post-Must exactly 9:         2 evaluations
Post-Must >9:                0
Historical mismatches:       0
```

The campaign contains 10,000 deterministic-seed random reachable collector
traces and seven directed states. The full A2D-R allocation-envelope proof was
rerun separately:

```text
Reachable proof allocation states: 145,834
Post-Must maxima: [8,3,9,8,3,9,8]
Post-Must >9: 0
```

This is the documented split of complete proof-model traversal plus a large
representative RTL corpus.

## 15. Old <=7 backward-equivalence

For 70,040 Config evaluations with at most seven projected entries, the test
top drives those compact slots into an actual frozen seven-slot analyzer. Its
candidate bitmap, PatternID, solution-valid, repairable, and dictionary-overflow
fields exactly match the nine-slot result.

## 16. Existing analyzer regression

The existing Phase 3B functional suite passed its CFG0 golden comparison,
directed CFG0..CFG6 tests, threshold decode, Pivot 4, address-width handling,
entry 6, transpose tests, and overflow-negative test.

```bash
bash scripts/simulation/run_recam_phase3b_functional.sh
```

## 17. S1G-A control impact

The new modules are combinational analyzer-boundary logic only. They contain no
generation, ownership, commit, ledger, or rollback state and no EARLY/GROUP
instance changed. Existing S1G-A overlap-control semantics remain reusable.

## 18. Production file diff

Verification-only additions are:

```text
tb/recam/tb_recam_post_must_projected_config_analyzer.sv
tb/recam/recam_post_must_projected_config_analyzer_test.cpp
```

The pre-existing A2D-R proof script was reused without modification. Same-name
specification companions were added for each new RTL and verification module.

## 19. Remaining retained-state blocker

This phase proves an analyzer boundary only. Complete retained physical Hybrid
state, membership, Must source, counter source, and overflow source are not
implemented or sized. `MINIMUM_COMPLETE_RETAINED_STATE_BITS_PER_SA` remains
`NOT_PROVEN`.

## 20. Closure decision

The corrected representation is semantics-preserving within its proven scope.
S1G-B remains blocked; no retained-state RTL is authorized. The next phase
remains review-gated S1G-A2F.

```text
S1GA2EA2_STATUS:
COMPLETE

SELECTED_ARCHITECTURE:
OPTION_A_POST_MUST

PHYSICAL_HYBRID_CAPACITY:
14

MAX_MEMBERSHIP_VISIBLE:
[11,11,11,11,11,11,11]

POST_MUST_MAX_PER_CONFIG:
[8,3,9,8,3,9,8]

POST_MUST_VIEW_CAPACITY:
9

PROJECTOR_PREDICATE_IMPLEMENTED:
YES

PROJECTOR_PREDICATE:
physical_valid &&
cfg_valid &&
legal_pointer &&
!final_must_retired

STABLE_H0_TO_H13_ORDER:
YES

PROJECTED_VISIBLE_GT9_OBSERVED:
NO

CONFIG2_9_ENTRY_WITNESS:
PASS

CONFIG5_9_ENTRY_WITNESS:
PASS

MEMBERSHIP_11_POST_MUST_LE9_BLOCKER_CASE:
PASS

ROW_MUST_TESTS:
PASS

COL_MUST_TESTS:
PASS

MIXED_MUST_TESTS:
PASS

CFG_VALID_FILTERING_TESTS:
PASS

ORDERING_TESTS:
PASS

CONFIG_ID_CONTRACT_PRESERVED:
YES

PATTERN_ID_CONTRACT_PRESERVED:
YES

PATTERN_ID_WIDTH_CHANGED:
NO

PATTERN_SCAN_ORDER_PRESERVED:
YES

OLD_ANALYZER_INPUT_BITS:
204

NEW_ANALYZER_INPUT_BITS:
232

REACHABLE_PROOF_STATES:
145834

RTL_EQUIVALENCE_STATES:
10007

RANDOM_REACHABLE_STATES:
10000

TOTAL_CONFIG_EVALUATIONS:
70049

HISTORICAL_EQUIVALENCE_MISMATCHES:
0

OLD_LE7_SUBSPACE_EQUIVALENCE:
PASS

EXISTING_ANALYZER_REGRESSION:
PASS

S1GA_CONTROL_SEMANTICS_REUSABLE:
YES

MINIMUM_COMPLETE_RETAINED_STATE_BITS_PER_SA:
NOT_PROVEN

BASELINE_EARLY_CHANGED:
NO

GROUP_CHANGED:
NO

S1GB_RESUMED:
NO

PRODUCTION_RTL_MODIFIED:
YES

PRODUCTION_RTL_SCOPE_VALID:
YES

GIT_DIFF_CHECK:
PASS

CONTRACT_DOCUMENT:
docs/dss_execution/S1GA2EA2_9ENTRY_POST_MUST_ANALYZER_CONTRACT.md

CLOSURE_DOCUMENT:
docs/dss_execution/S1GA2EA2_9ENTRY_POST_MUST_ANALYZER_IMPLEMENTATION_AND_PROOF.md

NEXT_RECOMMENDED_PHASE:
S1G-A2F — Complete Historical Retained-State Contract Re-Derivation
```
