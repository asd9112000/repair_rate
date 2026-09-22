# FINAL-EARLY-B1 — EARLY Priority Semantic-Delta and Simulator Fix Cost Audit

## Scope and archive check

```text
FINAL_EARLY_B1_STATUS: COMPLETE
MODE: READ-ONLY SIMULATOR / RTL / REGRESSION IMPACT AUDIT
AUTHORIZED_WORKTREE: /home/asd9112000/repair_rate_date2026_canonical
AUTHORIZED_BRANCH: integration/date2026-directional-canonical-v1
CURRENT_HEAD: 1158bea22ba2d45c062fec1856c4411895482324
MILESTONE_HEAD_MATCH: YES
WORKTREE_STATUS: DOCUMENTATION_ONLY_DIRTY
FUNCTIONAL_SOURCE_STATUS: CLEAN
FUNCTIONAL_RTL_MODIFIED: NO
SIMULATOR_MODIFIED: NO
SYNTHESIS_RUN: NO
```

The worktree is not literally clean: the pre-existing untracked FINAL-EARLY-A
and FINAL-EARLY-B0 audit files are documentation only. They were retained as
required; no historical dirty content was cleaned, reset, removed, or modified.

The readable-verilog-generator `SKILL.md` and ASIC quality rules were applied
for RTL inspection. The known Python 3.8 `dict[str, ...]` CLI failure remains;
the skill package was not changed. Manual rules used: establish elaborated top
and hierarchy, trace sequential commit state, distinguish inactive generate
branches, and flag dynamic indexing only where actually present.

## Priority mismatch: confirmed

| Subject | Authoritative location | Policy/call path | Order |
| --- | --- | --- | --- |
| DATE simulator | `src/DynamicRepairSimulator.cpp`: `run` → `findV2GroupNoScratchChoice` | `DirectionalV2Early` / `normalized_local_first` | `L,R,B,RB`; slots `0,1,2,3` |
| Historical SYN-A | `rtl/dss_canonical/policy/early/recam_dss_canonical_streaming_early_core.v` under `recam_dss_canonical_rs2_streaming_early_top` | streaming A→B→C→D first legal | `R,L,RB,B`; slots `1,0,3,2` |
| Documentation | `docs/EXPERIMENTS.md`, `CANONICAL_NAMING_MAP.md`, `P0B_RTL_SEMANTIC_AUDIT.md` | local-first and normalized streaming EARLY are distinct policy IDs | policy-ID dependent: `0,1,2,3` versus `1,0,3,2` |

```text
SIMULATOR_ORDER: L,R,B,RB
HISTORICAL_SYN_A_ORDER: R,L,RB,B
DOCUMENTED_ORDER: CONFLICTING_BY_POLICY_ID
MISMATCH_CONFIRMED: YES
```

`inc/V2GroupNoScratchPolicy.hpp` separates frozen role-slot/ConfigID encoding
from rank. `v2RtlGroupRoleSlotMappings()` alone transforms rank to `{1,0,3,2}`.
The current `DirectionalV2Early` dispatch supplies false for that selection;
`GroupGreedyRtlCanonical` supplies true.

## Exact candidate/resource meaning

Physical resource bit order is `[3:0]={C_COL,B_COL,D_ROW,A_ROW}` (IDs
`{A_ROW=0,D_ROW=1,B_COL=2,C_COL=3}`). R leaves the owned resource available;
L consumes it locally; RB leaves owned resource available and consumes a
prior-owner donor; B consumes the owned resource and donor.

| SA | Slot | ConfigID | Capacity | Borrows | Releases | Keeps/consumes |
| --- | --- | --- | --- | --- | --- | --- |
| A | L | 0 | 2R2C | none | none | consumes A_ROW |
| A | R | 4 | 1R2C | none | A_ROW | keeps A_ROW |
| A | RB | 6 | 1R3C | B_COL, C_COL | A_ROW | donor would be consumed; order-illegal |
| A | B | 5 | 2R3C | B_COL, C_COL | none | owner/donor would be consumed; order-illegal |
| B | L | 0 | 2R2C | none | none | consumes B_COL |
| B | R | 1 | 2R1C | none | B_COL | keeps B_COL |
| B | RB | 3 | 3R1C | A_ROW; D_ROW future | B_COL | keeps B_COL; consumes A_ROW |
| B | B | 2 | 3R2C | A_ROW; D_ROW future | none | consumes B_COL+A_ROW |
| C | L | 0 | 2R2C | none | none | consumes C_COL |
| C | R | 1 | 2R1C | none | C_COL | keeps C_COL |
| C | RB | 3 | 3R1C | A_ROW; D_ROW future | C_COL | keeps C_COL; consumes A_ROW |
| C | B | 2 | 3R2C | A_ROW; D_ROW future | none | consumes C_COL+A_ROW |
| D | L | 0 | 2R2C | none | none | consumes D_ROW |
| D | R | 4 | 1R2C | none | D_ROW | keeps D_ROW |
| D | RB | 6 | 1R3C | C_COL then B_COL | D_ROW | keeps D_ROW; consumes donor |
| D | B | 5 | 2R3C | C_COL then B_COL | none | consumes D_ROW+donor |

```text
RELEASE_FIRST_INTERPRETATION_OF_R_L_RB_B: VALID
```

This is a valid local resource-effect interpretation only, not a repair-rate
claim: R releases before local consumption and RB preserves its owner resource
relative to B.

## Four-token legality and exact transition table

With fixed A→B→C→D execution, availability is initially `4'b1111` and a bit
is cleared only when its line is consumed. Fixed order gates reject future
owners as donors. Thus ConfigID, PatternID, borrower ID, donor ID, and release
history are not required to decide later candidates; D's C-before-B selection
is fixed combinational priority, not stored history.

| SA | Slot | ConfigID | Borrow resource | Owner | Required mask | Consumed mask | Order gate |
| --- | --- | --- | --- | --- | --- | --- | --- |
| A | L | 0 | none | A_ROW | `0000` | `0001` | always |
| A | R | 4 | none | A_ROW | `0000` | `0000` | always |
| A | RB/B | 6/5 | B_COL/C_COL | A_ROW | n/a | n/a | reject: future owners |
| B | L | 0 | none | B_COL | `0000` | `0100` | always |
| B | R | 1 | none | B_COL | `0000` | `0000` | always |
| B | RB | 3 | A_ROW | B_COL | `0001` | `0001` | A prior; D future |
| B | B | 2 | A_ROW | B_COL | `0001` | `0101` | A prior; D future |
| C | L | 0 | none | C_COL | `0000` | `1000` | always |
| C | R | 1 | none | C_COL | `0000` | `0000` | always |
| C | RB | 3 | A_ROW | C_COL | `0001` | `0001` | A prior; D future |
| C | B | 2 | A_ROW | C_COL | `0001` | `1001` | A prior; D future |
| D | L | 0 | none | D_ROW | `0000` | `0010` | always |
| D | R | 4 | none | D_ROW | `0000` | `0000` | always |
| D | RB | 6 | C_COL else B_COL | D_ROW | `1000` else `0100` | same as required | prior donors; C first |
| D | B | 5 | C_COL else B_COL | D_ROW | `1000` else `0100` | `1010` else `0110` | prior donors; C first |

The transition is `legal = analyzer_valid && order_gate && ((available &
required_mask) == required_mask)` and `available_next = available &
~consumed_mask`. Owner availability at its own turn follows from the fixed gate.

```text
FOUR_BITS_SUFFICIENT_FOR_FUTURE_LEGALITY: PARTIAL
MISSING_PERSISTENT_INFORMATION: NONE IDENTIFIED
REMAINING_GAP: directed/exhaustive proof that this transition table matches the frozen ledger for every candidate-valid map
MINIMUM_PERSISTENT_RESOURCE_BITS_IF_PROVEN: 4
```

The partial verdict is an evidence boundary, not a request for additional
state: source topology supports the table, but B1 ran no exhaustive equivalence.

## Simulator cost and isolation

| Required B2 edit | Finding |
| --- | --- |
| File/function | `src/DynamicRepairSimulator.cpp`, `DynamicRepairSimulator::run` dispatch to `findV2GroupNoScratchChoice` |
| Current | canonical-rank Boolean is true only for `GroupGreedyRtlCanonical` |
| Proposed | make it true for `DirectionalV2Early` only, with a new policy/result version |
| Change type | one policy-scoped dispatch predicate; no table, sort, GLOBAL solver, or generic-helper rewrite |

| Policy | Same helper? | Changes under scoped edit? | Should change? |
| --- | --- | --- | --- |
| Directional EARLY RS2 | yes | yes | yes, after B2 oracle |
| Directional EARLY RS3 | yes | yes | yes, same implementation |
| `GroupNoScratchV2` | yes | no | no |
| `GroupGreedyRtlCanonical` | yes | no; already canonical rank | no |
| historical/canonical Directional GLOBAL | no | no | no |
| generic `LocalFirst`/`Early` | no | no | no |
| 1x4 Single-Hop and Two-Pairwise EARLY | no | no | no |
| pairwise-row / generic helpers | no | no | no |

```text
FILES_REQUIRING_SIMULATOR_CHANGE: 1
ESTIMATED_SIMULATOR_CHANGE_SIZE: SMALL
NUMBER_OF_POLICY_IMPLEMENTATIONS_AFFECTED: 1 (DirectionalV2Early; RS2 and RS3 parameter points)
CHANGE_ISOLATABLE_TO_DIRECTIONAL_EARLY: YES
GLOBAL_SEMANTICS_AFFECTED: NO
NON_DIRECTIONAL_POLICIES_AFFECTED: NO
1X4_SEMANTICS_AFFECTED: NO
```

## Oracle and test impact

`tests/directional_v2_group_global_test.cpp` currently asserts that
`DirectionalV2Early` starts at slot zero. `tests/solution_take_policy_test.cpp`
has a slot-zero golden for deferred V2 and a canonical `{1,0,3,2}` mapping
check. Neither independently specifies the proposed changed repair-rate EARLY:
changing production and a test that shares its priority helper is not proof.

```text
EARLY_PRIORITY_ORACLE_INDEPENDENT: PARTIAL
```

B2 needs a standalone directed C++ oracle with literal rank `{1,0,3,2}` and
literal donor/order tables, without either role-mapping ordering helper. It
must cover R/L tie, R-invalid fallback, RB/B tie after release, A future-donor
rejection, B/C prior-donor acceptance, D C-before-B selection, and retained
prefix after later failure. Canonical SYN-A RTL tests are an independent RTL
reference, but not a simulator repair-rate oracle.

| Test/artifact | Depends on current priority? | Expected to fail/change? | B2 action |
| --- | --- | --- | --- |
| `tests/directional_v2_group_global_test.cpp` | yes | yes | replace slot-zero expectation; retain GLOBAL containment |
| historical/deferred checks in `tests/solution_take_policy_test.cpp` | yes, another policy | no | preserve unchanged |
| canonical mapping checks in that test | yes | no | evidence only; add separate EARLY oracle |
| `tests/phase4j_policy_tradeoff_test.cpp` | independent literal old/new rank model | no direct failure | preserve old result; borrow its oracle pattern only |
| SYN-A RTL regressions | canonical order already | no | retain as semantic reference |
| DATE `directional_v2_early` sidecars/results | yes | semantically stale, not compile-fail | preserve and version new output |
| docs/naming maps | yes | no compile-fail | revise only with B2 contract change |

## Corpus preservation, A/B plan, and rerun cost

```text
RAW_FAULT_CORPUS_REUSABLE: YES
EXISTING_EARLY_RESULTS_INVALIDATED_IF_CHANGED: YES
GLOBAL_RESULTS_INVALIDATED_IF_CHANGED: NO
OTHER_POLICY_RESULTS_INVALIDATED: NO (scoped dispatch)
REQUIRES_NEW_FAULT_CORPUS: NO
PAIRED_PRIORITY_AB_TEST_FEASIBLE: YES
```

Do not run this in B1. Use the same seed, materialized corpus, candidate
universe, RS=CS=2, directional m=1, topology, and distribution for OLD
`L,R,B,RB` and NEW `R,L,RB,B`. Recommended minimum: 300 groups each at
fixed-total `F_group={16,20,24,28}`. Those points straddle the established
DATE moderate/mixed `8..32` sweep; 20 is medium and 24/28 are high. Record
`BOTH_PASS`, `BOTH_FAIL`, `NEW_ONLY`, `OLD_ONLY`, old/new repair rates, and
`DELTA_PP`. `NEW_ONLY > OLD_ONLY` favors the new order only for that sample;
both zero means the sample does not distinguish them.

```text
QUICK_RERUN_COST_300_GROUPS: 4 cases × 300 = 1,200 paired corpus groups; two EARLY evaluations/group
FORMAL_RERUN_COST: DATE RS2 recipe has F_group=8,12,16,20 × 100,000 groups for affected Directional EARLY; measured throughput unavailable, so no runtime estimate
REQUIRES_RERUN_OF_GLOBAL: NO
REQUIRES_RERUN_OF_OTHER_POLICIES: NO
```

Keep old history identifiable. Proposed new identity:
`directional_v2_early_r_l_rb_b_v2`; preserve current outputs as
`directional_v2_early_l_r_b_rb_v1` provenance rather than relabelling frozen
data.

## Hardware consequence and B2 gate

```text
SYN_A_PRIORITY_SEMANTICS_REUSABLE: YES
SYN_A_RESOURCE_LEDGER_REUSABLE: NO
```

SYN-A streaming first-legal commit and no-rollback semantics are reusable.
The historical generic ledger is not a minimum final realization. Subject to
B2 token equivalence, persistent policy state is exactly four availability
bits; ConfigID/PatternID are final destination data (zero policy bits), and
borrower ID, donor ID, release history, generic ledger, candidate store, and
rollback are absent. Donor selection is fixed combinational logic, not runtime
search or persistent state.

The accepted conditional dataflow is:

```text
current SA/slot -> shared analyzer -> valid + PatternID -> R,L,RB,B priority
-> fixed order gate -> common_spare_available_q[3:0] -> legal?
-> on yes: commit ConfigID/PatternID, clear consumed bits, advance A->B->C->D
```

All B2 entry conditions hold: mismatch confirmed, isolated, small edit. B2
must first freeze the independent oracle, then change only Directional EARLY,
run focused regression, run the 300-group paired A/B, report paired counts and
rates, and stop for human review. It must not begin a formal 100k sweep.
