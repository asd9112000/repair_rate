# P2-DOM: canonical GLOBAL DFS semantic audit

## Scope and status

```text
P2DOM_STATUS: COMPLETE
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH
LAYOUT: GRID_2X2
TOPOLOGY: DIRECTIONAL
RS: 2
CS: 2
SHARE_M: 1
SOLUTION_TAKE_POLICY: NORMALIZED_GLOBAL
SCRATCH_MODE: WITHOUT_SCRATCH
```

This is a read-only audit of
`rtl/dss_canonical/policy/global/recam_dss_canonical_global_noscratch_core.v`.
The core intentionally ends at the selected speculative A/B/C/D tuple.  Its
boundary is `completed candidate maps -> GLOBAL DFS -> selected tuple`; it is
not an integrated collector/analyzer/ledger implementation and is not area- or
timing-comparable with the RS2 2x2 Directional Normalized Streaming EARLY
integrated top.

No production RTL, frozen data, formal 100k corpus, device simulation, RS3
production path, or GLOBAL-WithScratch path was modified or started.

## Current implementation facts

The core captures three independent 160-bit maps: `candidate_valid_i`,
`candidate_release_i`, and `candidate_borrow_i`.  A slot is
`SA*40 + action*10 + (PatternID-1)`, with stored action regions L, R, B, RB.
The cursor decodes the fixed canonical order R, L, RB, B and scans PatternID
in ascending order.  It has four DFS depths A, B, C, D.

```text
CURRENT_SEARCH_ORDER: R,L,RB,B
CURRENT_CANDIDATE_SLOTS: 160
CURRENT_HISTORY_BITS: 480
SEARCH_ORDER_OPTIMIZED: YES
EQUIVALENT_EFFECT_COLLAPSE: NO
DOMINANCE_PRUNING: NO
DUPLICATE_STATE_PRUNING: NO
```

The remaining 150 state bits are 48 speculative released/used/obligation stack
bits, 8 borrow-count stack bits, 24 cursor bits, and 70 control/result bits.
Together with the 480 input-history bits this is the documented 630-bit core.

The resource mapping is A: release A_ROW / borrow B_COL, B: release B_COL /
borrow D_ROW, C: release C_COL / borrow A_ROW, and D: release D_ROW / borrow
C_COL.  A borrow from a later owner makes an obligation for that owner; the
owner may clear it only with an explicit-release nominal action whose
`actual_release` bit is set.

## Candidate-effect consumer trace

The DFS uses PatternID only to locate a frozen candidate-map slot and to record
the selected output tuple.  ConfigID is a deterministic projection of depth and
nominal action; donor resource is a deterministic projection of depth.  There
is no pivot, row, column, or source identity input at this boundary.  After a
candidate is read, legality and the next speculative state consume the slot's
valid/release/borrow bits plus whether its nominal action is explicit-release.

```text
PATTERN_ID_HAS_DOWNSTREAM_RESOURCE_EFFECT: NO
CONFIG_ID_HAS_DOWNSTREAM_RESOURCE_EFFECT: NO
PIVOT_IDENTITY_HAS_DOWNSTREAM_RESOURCE_EFFECT: NO
NOMINAL_ACTION_HAS_EFFECT_BEYOND_ACTUAL_RELEASE_BORROW: YES
```

The last answer is essential: nominal R/RB is `explicit_release`, while L/B is
not.  Therefore `(actual_release, actual_borrow)` alone is not an equivalent
resource effect.  The safe fixed-depth equivalence key is
`(explicit_release, actual_release, actual_borrow)`.

## Verification evidence

`tests/p2dom_grid2x2_directional_rs2_cs2_m1_normalized_global_noscratch_dfs_audit_test.cpp`
is a non-production C++ transition model; it neither instantiates nor changes
RTL.  It compares repairability, action, PatternID, ConfigID, donor, release,
borrow, final state, and obligation after every selected depth.

```text
COMMAND: make test_p2dom_grid2x2_directional_rs2_cs2_m1_normalized_global_noscratch_dfs_audit
DIRECTED N2/F16/group172: PASS
DIRECTED effect-only-collapse counterexample: PASS
FIXED RANDOM SEED: 20260918
RANDOM CANDIDATE MAPS: 1000
EQUIVALENCE MISMATCHES: 0
DOMINANCE MISMATCHES: 0
COMBINED MISMATCHES: 0
FUTURE-DONOR OBLIGATION MISMATCHES: 0
```

The existing 14,000-group frozen C++ corpus remains unmodified and has its
separate closure record in `GROUP_CPP_CANONICAL_FINAL_CLOSURE.md`.  It is not
rerun here: its foreground replay exceeds the local 30-second command limit,
and its group-fault interface does not expose the raw 160x3 candidate-map
boundary needed to compare this independent audit model.  It is prior
canonical-semantic evidence, not a substitute for this candidate-map test.

## Decision

Both a safe equivalent-effect representative rule and a safe failed-subtree
dominance rule were proved.  The dominance rule is only canonical-safe when a
dominating state is an earlier, fully explored, failed state at the same depth;
a later state must never prune an earlier canonical prefix.

```text
P2DOM_DECISION: DOMINANCE_PRUNING_SUPPORTED
NEXT_PROPOSED_PHASE: explicit authorization of an optimized RTL specification/review
```

This is not authorization to change production GLOBAL RTL.

## Required closure summary

```text
P2DOM_STATUS: COMPLETE
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH
LAYOUT: GRID_2X2
TOPOLOGY: DIRECTIONAL
RS: 2
CS: 2
SHARE_M: 1
SOLUTION_TAKE_POLICY: NORMALIZED_GLOBAL
SCRATCH_MODE: WITHOUT_SCRATCH
CURRENT_SEARCH_ORDER: R,L,RB,B
CURRENT_CANDIDATE_SLOTS: 160
CURRENT_HISTORY_BITS: 480
CURRENT_EQUIVALENT_EFFECT_COLLAPSE: NO
CURRENT_DOMINANCE_PRUNING: NO
PATTERN_ID_HAS_DOWNSTREAM_RESOURCE_EFFECT: NO
CONFIG_ID_HAS_DOWNSTREAM_RESOURCE_EFFECT: NO
NOMINAL_ACTION_HAS_EFFECT_BEYOND_ACTUAL_EFFECT: YES
EQUIVALENT_EFFECT_COLLAPSE_SAFE_FOR_REPAIRABILITY: YES
EQUIVALENT_EFFECT_COLLAPSE_SAFE_FOR_CANONICAL_TUPLE: YES
MORE_RELEASED_MONOTONIC: YES
LESS_USED_MONOTONIC: YES
FEWER_OBLIGATIONS_MONOTONIC: YES
LOWER_BORROW_COUNT_MONOTONIC: YES
LEVEL1_FAIL_PRUNES_LEVEL2: YES (conditional earlier failed subtree)
LEVEL1_FAIL_PRUNES_LEVEL3: YES (conditional earlier failed subtree)
LEVEL2_FAIL_PRUNES_LEVEL3: YES (all applicable earlier L-like/RB-like subtrees fail)
CURRENT_WORST_CASE_VISITS: 2625640
OPTIMIZED_THEORETICAL_WORST_CASE_VISITS: 4680 (safe class collapse; dominance is additional data-dependent pruning)
480_BITS_INFORMATION_THEORETICALLY_REQUIRED: NO
NAMING_AUDIT_COMPLETE: YES
ALL_NEW_ARCH_SPECIFIC_NAMES_ENCODE_TOPOLOGY_RS_CS_M_POLICY: YES
COMMON_UNIT_EXCEPTIONS_JUSTIFIED: YES
REPAIRABILITY_MISMATCHES: 0
SELECTED_TUPLE_MISMATCHES: 0
FUTURE_DONOR_MISMATCHES: 0
PRODUCTION_GLOBAL_RTL_MODIFIED: NO
P2_SCRATCH_REOPENED: NO
GLOBAL_WITHSCRATCH_RTL_STARTED: NO
FORMAL_100K_TOUCHED: NO
DEVICE_SWEEP_STARTED: NO
RS3_PRODUCTION_STARTED: NO
P2DOM_DECISION: DOMINANCE_PRUNING_SUPPORTED
NEXT_PROPOSED_PHASE: explicit authorization of optimized RTL specification/review
```
