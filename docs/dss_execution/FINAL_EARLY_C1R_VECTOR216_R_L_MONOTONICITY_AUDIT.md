# FINAL-EARLY-C1R — Vector-216 R-vs-L Monotonicity Root-Cause Audit

```text
FINAL_EARLY_C1R_STATUS: COMPLETE
```

This is a read-only semantic and analyzer audit on
`integration/date2026-directional-canonical-v1`.  It does not change the
production priority, RTL, simulator, synthesis collateral, or `dss_final/`.
The future exploratory order remains a human-proposed `R -> RB -> L -> B`;
this audit evaluates only the apparent C:R/C:L contradiction.

## Verified C role-slot contract

For middle subarrays B and C, the canonical slot decoder maps slots 0..3 to
ConfigIDs 0..3, with capacities `2R2C`, `2R1C`, `3R2C`, and `3R1C`
respectively ([`dss_v2_group_slot_decode.sv`](../../rtl/dss_v2/group/dss_v2_group_slot_decode.sv)).
The simulator's role mapping uses those same capacity pairs.  Its C actions
are interpreted against the directional physical topology: C owns the shared
`C_COL` line and may borrow the `A_ROW` line.

| C role | Slot / ConfigID | Nominal capacity | Resource meaning |
| --- | ---: | ---: | --- |
| R | 1 / 1 | 2R1C | release C's column-share opportunity; no row borrow |
| RB | 3 / 3 | 3R1C | release C's column-share opportunity and borrow A's row-share opportunity |
| L | 0 / 0 | 2R2C | retain C-local capacity; neither release nor borrow |
| B | 2 / 2 | 3R2C | retain C-local capacity and borrow A's row-share opportunity |

Thus `C_R_IS_2R1C: YES` and `C_L_IS_2R2C: YES`.

## Exact Vector 216 replay

The deterministic replay uses the C1 corpus contract: seed 20260922,
directional 2x2, RS=CS=2, m=1, `maximumGroupBorrowedSpares=1`, Moderate
Imbalance/Mixed faults, `F_GROUP=16`, group 216.  The four C faults are:

| Fault | Row | Column |
| --- | ---: | ---: |
| C0 | 493 | 355 |
| C1 | 860 | 355 |
| C2 | 491 | 353 |
| C3 | 677 | 353 |

Before C, the production trace has selected A:R ConfigID 4 / Pattern 1 with
actual `1R1C`, then B:R ConfigID 1 / Pattern 2 with actual `2R1C`.  Neither
selection claims a directional common line under the C1 physical-demand
oracle.  The current common-spare availability is therefore still
`[C_COL,B_COL,D_ROW,A_ROW]=4'b1111`.

At C:

| Role | Config | Result | Collection / candidate evidence |
| --- | ---: | --- | --- |
| R | 1, 2R1C | invalid | 2 pivots, 2 hybrids, `camStorageOverflow=1`; raw patterns 2 and 3 exist but `repairSuccess=0` |
| RB | 3, 3R1C | valid | 2 pivots, 2 hybrids, no overflow; valid patterns 3 and 4 |
| L | 0, 2R2C | selected | 2 pivots, 2 hybrids, no overflow; valid patterns 2, 5, and 6 |
| B | 2, 3R2C | not reached | not relevant after L commits |

C:L Pattern 2 is `RCRC` over the four matrix slots.  Its decoded line mapping
is exactly:

| Line | Source | Replacement |
| --- | --- | ---: |
| row | `(493,355)` / row 493 | 999000 |
| column | `(491,353)` / column 353 | 999000 |
| row | `(860,355)` / row 860 | 999001 |

Those three lines cover all four C faults: row 493 covers C0, row 860 covers
C1, and column 353 covers C2 and C3.  The decoded demand is `2R1C`.  It claims
neither C's own `C_COL` (only one column is used) nor A's `A_ROW` (only two
rows are used), so C's common-spare claim is `NONE`.  The later D:B selection
uses `2R3C` and consequently claims `D_ROW|C_COL`; that is why a fixed C:L
claim mask was disproved in C1.

The complete replay is retained only as ignored temporary evidence in
`tmp/date2026/final_early_c1r/vector216_simulator_trace.txt`.

## Why C:R is invalid despite the 2R1C output map

The C:R rejection is not caused by the final row/column count.  It is caused
by a configuration-dependent **Hybrid-CAM collection constraint** before a
repair is admitted.

The Hybrid-CAM size is
`R*(C-1) + C*(R-1)` ([`RECAM_CAM.cpp`](../../src/RECAM_CAM.cpp)).  Therefore:

| Config | Hybrid-CAM capacity | Vector 216 descriptors needed |
| --- | ---: | ---: |
| C:R = 2R1C | 1 | 2 |
| C:L = 2R2C | 4 | 2 |

The two non-pivots are same-column/different-row descriptors: `(860,355)`
points to pivot `(493,355)`, and `(677,353)` points to pivot `(491,353)`.
C:R can store only the first.  The second descriptor causes
`hybridCAM_overflow`; `loadFaultsToCAMs` converts that to
`camStorageOverflow` and `isRepairable=false`
([`RECAM_PE.cpp`](../../src/RECAM_PE.cpp)).  Although the partial matrix still
has raw valid patterns 2 and 3, `genValidSolList` defines
`RepairSuccess = isRepairable && !validSolList.empty()`.  The group selection
only considers successful attempts, so it excludes C:R before ledger/resource
comparison ([`DynamicRepairSimulator.cpp`](../../src/DynamicRepairSimulator.cpp)).

The exact L Pattern-2 *external* row/column mapping fits `2R1C` and covers all
four faults.  It is nevertheless **not legal under full C:R constraints**:
C:R lacks storage for the required second Hybrid-CAM descriptor.  The violated
constraint is the 2R1C Hybrid-CAM capacity of one descriptor, not row or
column spare capacity.

This also explains a potentially confusing trace detail.  C:R's partial
matrix reports raw Pattern 2 with the same three physical line identities, but
it contains no admitted descriptor for `(677,353)`.  It cannot establish that
all four faults are collected and is correctly gated from committing that
map.

```text
VECTOR216_C_R_VALID: NO
VECTOR216_C_L_VALID: YES
VECTOR216_L_ACTUAL_DEMAND: 2R1C
L_SELECTED_SOLUTION_LEGAL_UNDER_R: NO
L_SELECTED_SOLUTION_PHYSICALLY_FITS_R_ROWS_AND_COLUMNS: YES
L_SELECTED_SOLUTION_R_VIOLATION: 2R1C Hybrid-CAM capacity 1 < 2 required descriptors
```

## Candidate-universe and monotonicity audit

The configurations do not differ only in external capacity:

| Property | C:R / 2R1C | C:L / 2R2C |
| --- | ---: | ---: |
| Matrix dimension | 3 | 4 |
| Nominal patterns | 3 | 6 |
| Vector-216 raw valid PatternIDs | 2, 3 | 2, 5, 6 |
| Hybrid-CAM capacity | 1 | 4 |
| Admission precondition | no collector overflow | no collector overflow |

The pattern generators enumerate the fixed `C(R+C,R)` solution space; the
shared analyzer also defines 3 and 6 candidate cones for these two classes
([`recam_shared_config_analyzer.sv`](../../rtl/recam/recam_shared_config_analyzer.sv)).
For Vector 216 specifically, raw C:R Pattern 2 represents the same external
`2R1C` assignment as C:L Pattern 2.  Hence there is no demonstrated
PatternID-enumeration miss or representation asymmetry.  A universal proof
over all possible physical `2R1C` realizations was outside this finite audit,
so it is not claimed.

The requested monotonicity premise holds only if configurations differ solely
by available repair lines.  Here it does not: ConfigID changes matrix size,
candidate count, Hybrid-CAM provisioning, which descriptors can be retained,
and the overflow admission gate.  An output map's `usedRows/usedColumns` is a
post-decode count of line mappings; it does not encode collector state.
([`RECAMSolverAdapter.cpp`](../../src/RECAMSolverAdapter.cpp) counts only
decoded physical mappings.)

```text
R_AND_L_DIFFER_ONLY_BY_CAPACITY: NO
CAPACITY_MONOTONICITY_EXPECTED: NO
R_PATTERN_UNIVERSE_COMPLETE_FOR_ALL_2R1C_SOLUTIONS: NOT_PROVEN
L_HAS_2R1C_REALIZATION_NOT_REPRESENTABLE_BY_R: NO
```

## C1 corpus extension

The existing C1 deterministic corpus was scanned at `F_GROUP=16,20,24,28`,
300 groups per point (1,200 groups), for middle SAs B and C.  A case was
counted when R was invalid, L was valid, and L's first selected candidate had
actual demand at most `2R1C`.  This found eight C cases and eight analogous B
cases, 16 total:

| SA | Cases | A: L illegal under full R | B: R enumeration miss | C: decode/trace issue | D: other |
| --- | ---: | ---: | ---: | ---: | ---: |
| B | 8 | 8 | 0 | 0 | 0 |
| C | 8 | 8 | 0 | 0 | 0 |
| Total | 16 | 16 | 0 | 0 | 0 |

Every counted case has `r.camStorageOverflow=1`; all are therefore the same
full-configuration collector-capacity class as Vector 216.  The scan is
evidence for this C1 corpus only, not a global frequency claim.  Its detailed
CSV is temporary-only at
`tmp/date2026/final_early_c1r/low_demand_r_invalid_l_valid.csv`.

## Shared RTL cross-check

The canonical shared analyzer was read without modification.  A temporary
Verilator harness drove Vector 216's two pivots and descriptor topology using
10-bit address parameters to preserve its 1024-by-1024 address values.  It
produced:

```text
C_L_2R2C                         bitmap=0x32 pattern=2 solution_valid=1 repairable=1
C_R_2R1C_WITH_COLLECTOR_OVERFLOW bitmap=0x00 pattern=0 solution_valid=0 repairable=0
C_R_2R1C_RAW_ANALYZER            bitmap=0x06 pattern=2 solution_valid=1 repairable=1
```

The RTL's candidate validity explicitly includes
`!conventional_overflow_i`; thus when supplied the C:R collection-overflow
condition it makes the same invalid/valid distinction as the simulator.  The
raw C:R RTL result additionally confirms that Pattern 2 itself is present
without that gate.  Pattern 2's shared configuration/pattern semantics are
the same decoded external `2R1C` mapping listed above.

```text
RTL_R_RESULT: INVALID
RTL_L_RESULT: VALID
RTL_L_ACTUAL_DEMAND_EQUIVALENT: Pattern 2 decodes to the same 2 row + 1 column external mapping
SIMULATOR_AND_RTL_AGREE_ON_VECTOR216: YES
```

The harness and its output are temporary-only under `/tmp` and
`tmp/date2026/final_early_c1r/`; no reusable RTL test was added.

## Consequences

Vector 216 is **Case C**: `actual 2R1C` is insufficient as a complete
characterization of the *configuration-level* repair requirement.  It remains
an accurate and necessary characterization of the **external physical spare
line claim** after a candidate has been accepted.  It is not sufficient to
infer whether a lower-capacity configuration can collect and admit that
candidate.

Therefore the C1 claim-mask ambiguity is not caused by analyzer enumeration.
The exact current-candidate external demand (or an equivalently specified
claim-mask output) is still required to implement physical common-spare
claims.  It must not be repurposed as a claim that collection feasibility can
be reconstructed from the output demand alone.

The proposed R->RB->L->B priority is semantically well-defined if each role
means its complete ConfigID contract—including collector capacity and
admission—not merely its final line count.  Vector 216 provides no basis to
declare a candidate-search defect or to change that frozen production order.

## Final status

```text
FINAL_EARLY_C1R_STATUS: COMPLETE
C_R_IS_2R1C: YES
C_L_IS_2R2C: YES
VECTOR216_C_R_VALID: NO
VECTOR216_C_L_VALID: YES
VECTOR216_L_ACTUAL_DEMAND: 2R1C
L_SELECTED_SOLUTION_LEGAL_UNDER_R: NO
R_AND_L_DIFFER_ONLY_BY_CAPACITY: NO
R_PATTERN_UNIVERSE_COMPLETE_FOR_ALL_2R1C_SOLUTIONS: NOT_PROVEN
L_HAS_2R1C_REALIZATION_NOT_REPRESENTABLE_BY_R: NO
TOTAL_R_INVALID_L_VALID_LOW_DEMAND_CASES: 16 (C=8, B=8)
ROOT_CAUSE_CLASS: CONFIG_SEMANTICS_DIFFERENCE / Hybrid-CAM collection-capacity overflow
VECTOR216_SEMANTIC_CLASS: C
SIMULATOR_AND_RTL_AGREE_ON_VECTOR216: YES
AMBIGUITY_CAUSED_BY_ANALYZER_ENUMERATION: NO
ACTUAL_DEMAND_SIGNAL_STILL_REQUIRED_AFTER_ROOT_CAUSE: YES
R_RB_L_B_PRIORITY_SEMANTICALLY_WELL_DEFINED: YES
FUNCTIONAL_SOURCE_MODIFIED: NO
SYNTHESIS_RUN: NO
WORKTREE_CLEAN_AFTER_COMMIT: YES
NEXT_ACTION: STOP FOR HUMAN ROOT-CAUSE REVIEW
```

```text
VERILOG_SKILL_RULES_USED:
- Read the complete readable-verilog-generator skill, dispatcher workflow,
  and ASIC-quality reference before the read-only RTL audit.
- Performed contract inspection plus a temporary, parameter-preserving
  shared-analyzer simulation only; no production RTL was created or modified.
```
