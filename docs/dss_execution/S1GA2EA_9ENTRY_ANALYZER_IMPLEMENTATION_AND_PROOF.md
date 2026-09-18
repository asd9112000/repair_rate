# S1G-A2E-A — 9-Entry Analyzer Implementation and Proof

## 1. Executive status

**BLOCKED before RTL implementation.**  The A2E-A frozen projection rule keeps
an entry solely when `cfg_valid[Hi][ConfigID] == 1`.  That rule has a reachable
11-entry output, so it contradicts the same authorization's required
`PROJECTED_VISIBLE_COUNT <= 9` assertion.  It is not safe to truncate the
result or to alter the analyzer to hide the contradiction.

No production RTL, verification RTL, testbench, analyzer input packer, or
overlap-control source was changed.  The requested 9-entry contract document
is not frozen because it would encode an unsound membership-only boundary.

## 2. Frozen A2D decision

S1G-A2D correctly proved that the maximum **historical analyzer-consumed**
view is nine after both Config membership and final Must retirement:

```text
historical consumed entry = valid && cfg_valid[Config] && !Must_retired
max by ConfigID = [8,3,9,8,3,9,8].
```

The A2E-A text instead defines its projection as membership-only:

```text
keep Hi iff cfg_valid[Hi][ConfigID] == 1.
```

Those are different objects.  The A2D upper bound cannot be transferred to the
membership-only sequence.

## 3. Old 7-entry contract

The frozen V2 boundary contains seven logical Hybrid records in a 204-bit
input.  It is already known insufficient for the historical post-Must
Config-local view, whose legal maximum is nine.  Its analyzer also lacks the
historical prefilter used by `multi_config_analyzer_bank`.

## 4. Proposed 9-entry contract cannot be frozen

The intended HCV-9 contract would be viable only if its projection predicate
matched the historical logical analyzer view:

```text
valid
&& cfg_valid[Config]
&& legal_pointer
&& !(descriptor ? col_must[Config][pointer] : row_must[Config][pointer]).
```

The A2E-A frozen text permits only the second term.  This report does not add
the omitted Must predicate because that would revise the authorized frozen
contract.  Human architecture direction is required before a contract file can
be created.

## 5. Input packing

No 9-entry analyzer input was implemented.  The old input is 204 bits.  The
232-bit value from A2D is conditional on the corrected, post-Must HCV-9 view;
it is not a valid width for the membership-only rule under test.

## 6. 14-to-9 projection implementation

Not implemented.  Implementing the stated membership-only scanner would
produce an illegal 11-entry result on the directed legal trace below.  Silent
truncation would violate historical ordering/completeness and the A2E-A stop
condition.

## 7. Analyzer changes

Not implemented.  Increasing the V2 analyzer loop from seven to nine cannot
repair an 11-entry membership-only input, and the historical bank filters
Must-retired entries before passing them to `config_analyzer`.

## 8. ConfigID preservation

The source ConfigID contract is unchanged and not implicated in the blocker:
`cfg_k={4,3,5,4,3,5,4}` is defined in
`rtl/dss_2x2/analyzer/shared_fault_collector.sv:46-64`.  The failure occurs for
every Config because a relation to pivot pointer 0 is membership-valid for all
seven ConfigIDs.

## 9. PatternID preservation

PatternID source width, encoding, candidate priority, and scan order were not
changed.  No PatternID equivalence claim can be closed for the invalid input
contract: feeding a membership-only superset differs from the historical
post-Must analyzer view.

## 10. Capacity-bound directed counterexample

Use the legal default-parameter trace:

```text
F0  = (r0,c0)                         first pivot, pointer 0
F1..F11 = (r0,c1)..(r0,c11)           eleven distinct same-row faults
```

The collector accepts all twelve faults (`fault_ready_o` is
`fault_count_o < MAX_FAULTS` at `shared_fault_collector.sv:66-73`).  Each of
F1..F11 matches pivot 0 by row, obtains pointer 0 and descriptor 0
(`:49-58`), has `cfg_valid=1` for every Config because `0 < k(ConfigID)`
(`:59-64`), and writes one append-order Hybrid record (`:68-93`).

Thus the stated A2E-A projector emits H0 through H10 for every Config:

```text
membership-only projected visible count = 11 > 9.
```

The physical store capacity is 14, so this trace has neither physical Hybrid
full nor Hybrid overflow.  It is a legal reachable state, not a synthetic mask
or an overflow workaround.

## 11. Historical counterexample regression analysis

The historical multi-Config bank applies an additional Must predicate before
calling `config_analyzer`:

```text
hybrid_valid && cfg_valid && !row_or_column_must_for_pointer
```

See `rtl/dss_2x2/analyzer/multi_config_analyzer_bank.sv:35-56`.  In the
directed trace the pivot-row count is 12, greater than every Config's `Cs`, so
all eleven row-descriptor records are retired.  The historical analyzer view
therefore contains zero records, while the proposed membership-only projector
contains eleven.  The two representations are not equivalent.

## 12. Random historical equivalence

Not run.  The deterministic legal counterexample above already reaches the
mandatory `visible_count > 9` stop condition; starting a 5,000-state campaign
would not make the requested implementation safe.

## 13. <=7 backward-compatibility regression

Not run because no 9-entry analyzer or projector was written.  Existing
seven-entry RTL remains untouched.

## 14. S1G-A control impact

S1G-A ownership, generation invalidation, latest-generation-only commit, and
ledger/rollback semantics were not changed by this finding.  They remain
architecturally reusable once a valid analyzer-ready contract is authorized.
The current 204-bit payload remains restricted and cannot close full
historical semantics.

## 15. Width accounting

| Item | Status |
| --- | --- |
| Old Hybrid payload | 7 × 14 = 98 bits |
| Old analyzer input | 204 bits |
| Corrected post-Must HCV-9 payload | 9 × 14 = 126 bits, conditional only |
| Corrected post-Must HCV-9 input | 232 bits, conditional only |
| Membership-only projected capacity | 11 reachable; no valid nine-slot packing |

This is analyzer-side accounting only.  It does not derive complete retained
collector state.

## 16. Remaining blocker to retained-state work

The projection stage must be reconciled with historical Must retirement before
any 9-entry implementation or retained-state work resumes.  The two possible
directions are deliberately not selected here: amend the projection predicate
to match the historical view, or redefine the capacity/consumer contract for
the membership-only view.  Either requires new authority.

`MINIMUM_COMPLETE_RETAINED_STATE_BITS_PER_SA` remains `NOT_PROVEN`.

## 17. Closure decision

S1G-A2E-A cannot close under the exact frozen membership-only projection rule.
Stop at architecture-contract review; do not implement, truncate, rebaseline,
or resume S1G-B.

```text
S1GA2EA_STATUS:
BLOCKED

SELECTED_ARCHITECTURE:
OPTION_A

HISTORICAL_GLOBAL_HYBRID_CAPACITY:
14

CONFIG_VIEW_HYBRID_CAPACITY:
9

OLD_CONFIG_VIEW_HYBRID_CAPACITY:
7

CONFIG_MAX_REACHABLE_HYBRID:
[8,3,9,8,3,9,8]

14_TO_9_PROJECTION_IMPLEMENTED:
NO

PROJECTION_ORDER:
H0_TO_H13_STABLE

PROJECTION_VISIBLE_GT9_OBSERVED:
YES

NEW_ANALYZER_INPUT_BITS:
NOT_PROVEN

OLD_ANALYZER_INPUT_BITS:
204

CONFIG_ID_CONTRACT_PRESERVED:
YES

PATTERN_ID_CONTRACT_PRESERVED:
NO

PATTERN_ID_WIDTH_CHANGED:
NO

PATTERN_SCAN_ORDER_PRESERVED:
NO

HISTORICAL_COUNTEREXAMPLES_PASS:
NO

CONFIG2_9_ENTRY_CASE:
FAIL

CONFIG5_9_ENTRY_CASE:
FAIL

CONFIG0_8_ENTRY_CASE:
FAIL

CONFIG3_8_ENTRY_CASE:
FAIL

CONFIG6_8_ENTRY_CASE:
FAIL

CFG_VALID_FILTERING_TESTS:
FAIL

ORDERING_TESTS:
FAIL

REACHABLE_RANDOM_STATES:
0

REACHABLE_RANDOM_CONFIG_EVALUATIONS:
0

RANDOM_HISTORICAL_EQUIVALENCE_MISMATCHES:
0

OLD_LE7_SUBSPACE_EQUIVALENCE:
NOT_RUN

EXISTING_ANALYZER_REGRESSION:
NOT_RUN

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
NO

PRODUCTION_RTL_SCOPE:
NONE; STOPPED BEFORE IMPLEMENTATION

GIT_DIFF_CHECK:
PASS

CONTRACT_DOCUMENT:
NOT_CREATED; A2E-A MEMBERSHIP-ONLY PREDICATE CONFLICTS WITH 9-ENTRY BOUND

CLOSURE_DOCUMENT:
docs/dss_execution/S1GA2EA_9ENTRY_ANALYZER_IMPLEMENTATION_AND_PROOF.md

NEXT_RECOMMENDED_PHASE:
ARCHITECTURE BLOCKER REVIEW
```
