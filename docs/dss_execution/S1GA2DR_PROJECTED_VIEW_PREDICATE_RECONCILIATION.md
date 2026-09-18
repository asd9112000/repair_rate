# S1G-A2D-R — Projected-View Predicate Reconciliation

## 1. Executive conclusion

**COMPLETE — Option A is valid only as a post-Must projected analyzer.**

The historical 14-entry Hybrid store does not have a nine-entry bound after
Config membership alone.  Its exact analyzer-consumed view applies a second,
final, Config-specific Must retirement predicate.  The corrected consumer
boundary is therefore:

```text
PHYSICAL_HYBRID_SET (H0..H13)
  -> MEMBERSHIP_VISIBLE_SET
  -> POST_MUST_ANALYZER_VIEW
  -> stable H0..H13 compaction to HCV-9
  -> Config/Pattern analyzer
```

The independently executable proof reproduces the post-Must maxima
`[8,3,9,8,3,9,8]` and finds no post-Must view greater than nine in its 145,834
bounded allocation states.  The symbolic bound below applies to arbitrary
legal traces, not just these witnesses.

No production or verification RTL was modified.  This reconciliation does not
authorize S1G-A2E-A2 implementation.

## 2. A2E-A blocker recap

The former membership-only rule was:

```text
keep Hi iff cfg_valid[Hi][ConfigID]
```

It is disproven as a nine-entry boundary.  In the legal trace
`(r0,c0), (r0,c1) ... (r0,c11)`, the first fault is Pivot 0 and the following
eleven faults are row-related Hybrid records.  Pointer zero is inside every
Config's `k`, so all eleven records are membership-visible in every Config.
The physical store has 14 allocated entries; no physical-full condition is
involved.

The historical bank nevertheless suppresses all eleven at the final view,
because their pivot row count is 12 and `12 > Cs` for every Config.  The
previous blocker report remains correct:
[S1GA2EA_9ENTRY_ANALYZER_IMPLEMENTATION_AND_PROOF.md](S1GA2EA_9ENTRY_ANALYZER_IMPLEMENTATION_AND_PROOF.md).

## 3. Physical, membership, and post-Must views

| Term | Exact contents | Capacity statement |
| --- | --- | --- |
| `PHYSICAL_HYBRID_SET` | Append-order, physically valid `H0..H13` records in `tagged_hybrid_store`. | Allocated capacity 14; default 12-fault collection can create at most 11 related records. |
| `MEMBERSHIP_VISIBLE_SET(C)` | Physical records whose stored `cfg_valid[Hi][C]` is one. | Exact default maximum is 11 for every ConfigID; it is not a nine-entry boundary. |
| `POST_MUST_ANALYZER_VIEW(C)` | Membership-visible records whose descriptor-selected final Must bit is zero. | Proven maxima are `[8,3,9,8,3,9,8]`; common capacity is nine. |

`multi_config_analyzer_bank` does not physically compact or delete records.  It
creates a per-Config valid mask at
`rtl/dss_2x2/analyzer/multi_config_analyzer_bank.sv:35-46`.  A future HCV-9
projector must compact that same surviving subsequence, not reinterpret the
physical store.

## 4. Historical Must semantics

`shared_fault_counter` updates physical row and column counters once for every
accepted fault (`shared_fault_counter.sv:38-66`).  Counts only increase until
clear/reset, so the threshold predicates below are monotonic over a collection
transaction.

`config_must_view` looks up each stored Pivot's row and column in those counter
tables and recomputes the output combinationally (`config_must_view.sv:30-48`):

```text
RowMust(C,p) = pivot_valid[p] && row_count[pivot_row[p]] > Cs(C)
ColMust(C,p) = pivot_valid[p] && col_count[pivot_col[p]] > Rs(C)
```

| Property | RowMust | ColMust |
| --- | --- | --- |
| Inputs | Pivot row address and physical row-counter count | Pivot column address and physical column-counter count |
| Config dependence | `Cs(C)` | `Rs(C)` |
| Pivot dependence | One bit per Pivot pointer | One bit per Pivot pointer |
| Row/column dependence | Row count only | Column count only |
| Evaluation time | Combinationally from current registered counter state | Same |
| Recomputed after an accepted fault | Yes, after the edge updates counter state | Yes |
| Monotonic within transaction | Yes | Yes |
| Used before candidate enumeration | Yes: it masks Hybrid validity and initializes matrix lines | Yes: same |

Must retirement is logical, not a physical-store mutation.  A row-descriptor
Hybrid is retired on RowMust for its pointer; a column-descriptor Hybrid is
retired on ColMust for its pointer.  Must bits still enter `config_analyzer`
matrix initialization (`config_analyzer.sv:25-28`).

## 5. Historical processing order

The following is a dependency order, not a claim that all arrows are separate
clock stages.  After a stable collector state is available, the Must view,
valid masks, matrix, and candidate evaluation are combinational.

```text
accepted fault
  -> greedy row-before-column Pivot relation and stored cfg_valid membership
  -> append PHYSICAL_HYBRID_SET record in H order
  -> physical row/column counter update
  -> final RowMust / ColMust derivation for each Config and Pivot
  -> Config membership filter
  -> descriptor-selected Must retirement
  -> POST_MUST_ANALYZER_VIEW valid mask
  -> Pivot/Must matrix construction and surviving-Hybrid traversal
  -> fixed candidate enumeration
  -> feasibility bitmap and first feasible PatternID
```

Source evidence:

- Collector relation scan, membership mask, and append request:
  `shared_fault_collector.sv:49-70,86-93`.
- Counter-derived Must view: `config_must_view.sv:30-48`.
- Membership and Must mask before `config_analyzer`:
  `multi_config_analyzer_bank.sv:35-57`.
- Matrix construction, H-index traversal, and fixed PatternID scan:
  `config_analyzer.sv:25-49`.

## 6. Exact retirement predicate and pointer semantics

For physical entry `Hi`, selected Config `C`, and stored `ptr = Hi.pivot_ptr`,
the exact historical survival predicate is:

```text
SURVIVES_TO_ANALYZER(Hi,C) =
    physical_valid(Hi)
 && cfg_valid(Hi,C)
 && ptr < MAX_K
 && !(Hi.descriptor == COLUMN
        ? ColMust(C,ptr)
        : RowMust(C,ptr))
```

The collector finds `ptr` by scanning valid Pivots in ascending pointer order;
for each Pivot it tests equal row before equal column
(`shared_fault_collector.sv:49-58`).  Thus the pointer identifies the first
related physical Pivot in append order.  It is Config-independent as stored
metadata, but it controls Config membership (`ptr < k(C)`) and chooses the
Config/Pivot Must bit.  It is required for retirement.

## 7. Directed counterexample and required tests

The verification-only model is
[s1ga2dr_projected_view_proof.py](../../scripts/analysis/s1ga2dr_projected_view_proof.py).
It models the actual row-before-column relation, physical counters, membership
by pointer, and the exact final retirement predicate.  Run it with:

```bash
/home/asd9112000/repair_rate/venv/bin/python \
  scripts/analysis/s1ga2dr_projected_view_proof.py
```

The command completed successfully.  Its directed results are:

| Test | Historical legal trace/property | Result |
| --- | --- | --- |
| DR-1 | One Pivot + eleven same-row faults | Every Config: membership 11, RowMust true, post-Must 0. |
| DR-2 | One Pivot + eleven same-column faults | Every Config: membership 11, ColMust true, post-Must 0. |
| DR-3 | One row and one column Hybrid for Pivot 0 | Membership 2 each; descriptor-selected retirement differs by Config threshold. |
| DR-4 | Pointer-3 membership holes plus two row records | C1/C4 see none; C0/C2/C3 retire both; C5/C6 retain both. |
| DR-5 | Three Pivot stars with row extras `[1,1,1]`, column extras `[2,2,2]` | Config 2 has post-Must count 9. |
| DR-6 | Three Pivot stars with row extras `[2,2,2]`, column extras `[1,1,1]` | Config 5 has post-Must count 9. |
| DR-7 | DR-1 restated as capacity distinction | Membership 11 while post-Must is 0. |
| DR-8 | One Pivot with one row and one column extra | Config 0: membership and post-Must counts are both 2. |

For DR-1 specifically, the physical records are H0 through H10, all with
`pointer=0`, `descriptor=ROW`, and `cfg_valid={0,1,2,3,4,5,6}`.  The final
pivot row count is 12.  The row thresholds `Cs={2,1,2,1,2,3,3}` make RowMust
true for every Config; all H0..H10 are retired.  DR-2 is the column dual with
`descriptor=COLUMN`, column count 12, and `Rs={2,2,3,3,1,2,1}`.

## 8. Corrected per-Config capacity proof

For a selected Config with `(R,C)`, let `p` be the number of membership-valid
Pivots.  A surviving row-descriptor record for one Pivot requires its final row
count to be at most `C`; after the Pivot fault, at most `C-1` such records can
survive.  Similarly, at most `R-1` column-descriptor records per Pivot can
survive.  The remaining accepted-fault budget is `12-p`.  Therefore every
legal trace satisfies:

```text
POST_MUST_VIEW(p) <= min(12-p, p * ((C-1) + (R-1)))
                     min(12-p, p * (R+C-2)), 1 <= p <= R+C.
```

Pivots not membership-valid for `C`, unrelated faults, and Must-retired
records consume the 12-fault budget but cannot increase this view.  Independent
Pivot-star witnesses attain each Config maximum.  The model exhaustively
enumerated 145,834 count-allocation states under that budget; this is an
exhaustive allocation-envelope check, not a claim to enumerate all concrete
address permutations.  The symbolic argument covers those permutations.

| ConfigID | Geometry | Max membership-visible | Max post-Must analyzer-visible | Witness `(p; row extras; col extras)` | Symbolic upper bound |
| ---: | --- | ---: | ---: | --- | ---: |
| 0 | 2R2C | 11 | 8 | `(4; [1,1,1,1]; [1,1,1,1])` | 8 |
| 1 | 2R1C | 11 | 3 | `(3; [0,0,0]; [1,1,1])` | 3 |
| 2 | 3R2C | 11 | 9 | `(3; [1,1,1]; [2,2,2])` | 9 |
| 3 | 3R1C | 11 | 8 | `(4; [0,0,0,0]; [2,2,2,2])` | 8 |
| 4 | 1R2C | 11 | 3 | `(3; [1,1,1]; [0,0,0])` | 3 |
| 5 | 2R3C | 11 | 9 | `(3; [2,2,2]; [1,1,1])` | 9 |
| 6 | 1R3C | 11 | 8 | `(4; [2,2,2,2]; [0,0,0,0])` | 8 |

The exact membership maximum is 11, not merely a lower bound: each Hybrid
record needs an accepted related fault, and at least one of 12 accepted faults
must be a Pivot before any related record exists.  DR-1 reaches 11 for every
Config because pointer zero is valid for all seven Config geometries.

## 9. Stable ordering, PatternID, and dictionary/full semantics

The historical bank preserves a physical H-index position: it presents each
payload at its original H slot and only changes its valid bit.  `config_analyzer`
then traverses `h=0..HYBRID_ENTRY_NUM-1` and skips invalid entries
(`config_analyzer.sv:28-43`).  Therefore removing H1 and H3 semantically leaves
the ordered traversal `H0, H2, H4`.  Stable compaction into a prefix followed
by invalid tail slots performs exactly the same valid payload traversal.  It
must never sort by address, pointer, descriptor, or membership.

Retired payload records have no other input path to `config_analyzer`.  The
required Pivot fields and final Must vectors remain inputs; matrix initialization
uses the latter before surviving Hybrid traversal (`config_analyzer.sv:25-28`).
Candidate enumeration is fixed after matrix construction and selects the first
feasible pattern in scan order (`config_analyzer.sv:44-49`).  Consequently an
exact stable post-Must projection preserves Config feasibility, candidate
feasibility, PatternID encoding, and PatternID priority.

Physical store occupancy/full is evaluated before Must retirement:
`tagged_hybrid_store.sv:45,60-77`.  Retirement never frees or repacks a
physical slot.  Counter or Hybrid-store overflow is separately ORed and passed
to the analyzer (`dss_analyzer_top.sv:91-99`), where it invalidates candidates
(`config_analyzer.sv:45-49`).  The analyzer has no membership or post-Must
occupancy input; it consumes individual valid bits and overflow.  Thus:

```text
physical occupancy: append-store state, pre-retirement
membership occupancy: logical count only, not an analyzer port
post-Must occupancy: logical surviving count / valid-mask population
```

## 10. Corrected projector contract and inputs

The only valid HCV-9 contract is:

```text
For selected Config C, scan physical H0..H13 in ascending order.

Emit Hi iff:
  physical_valid(Hi)
  && cfg_valid(Hi,C)
  && legal_pointer(Hi.pointer)
  && !(Hi.descriptor == COLUMN
       ? ColMust(C,Hi.pointer)
       : RowMust(C,Hi.pointer)).

Append emitted records in that scan order; fill the remaining HCV-9 slots
invalid.  Preserve the final Must vectors, Pivot inputs, and overflow sideband
needed by the analyzer.
```

| Projector/analyzer datum | Classification | Reason |
| --- | --- | --- |
| Physical Hybrid valid | Required | First survival term. |
| Stored `cfg_valid[6:0]` | Required | Selects membership for C. |
| Hybrid pointer and descriptor | Required | Select Must bit and validate pointer. |
| Hybrid row/column payload | Required for emitted analyzer record | The historical analyzer consumes payload for matrix construction. |
| ConfigID | Required | Selects membership and Config-specific Must view. |
| Selected Config RowMust/ColMust vectors | Required, or derivable | Exact retirement input. |
| Pivot addresses, validity, and order | Derivable for selection; required by analyzer | They derive Must and form analyzer dictionaries. |
| Counter tables / RowMust and ColMust source | Derivable | Required only if projector derives Must rather than receiving it. |
| Row/column dictionary state | Not required by projector | `config_analyzer` reconstructs it from Pivots and surviving records. |
| Physical Hybrid occupancy/full | Not required for selection | Preserve the overflow sideband separately; do not reinterpret occupancy. |
| Hybrid fault reference | Not required | It is not consumed by this analyzer path. |

The nine-entry capacity begins **only** after membership filtering, final Must
retirement, and stable compaction.  It is not physical collector capacity, raw
Hybrid capacity, or membership-visible capacity.

## 11. Documentation reconciliation and Option A decision

The prior A2D document already states the correct complete predicate at
`S1GA2D_HISTORICAL_SEMANTICS_COMPATIBILITY_ARCH_DECISION.md:128-136`; no
cfg-valid-only statement in that decision needs correction.  The rejected
A2E-A authorization, rather than A2D, omitted Must retirement.  This document
freezes the terminology needed to prevent that ambiguity.

```text
OPTION_A_STATUS:
OPTION_A_VALID_AS_POST_MUST_PROJECTED_ANALYZER
```

Option A is not a membership-only projector.  It is viable because the exact
predicate is deterministic, retains H-order, has defined inputs, preserves the
historical analyzer's effective input stream, and has a proven common capacity
of nine.

## 12. Required final status

```text
S1GA2DR_STATUS:
COMPLETE

PHYSICAL_HYBRID_CAPACITY:
14

MEMBERSHIP_ONLY_9_ENTRY_BOUND:
DISPROVEN

KNOWN_MEMBERSHIP_VISIBLE_GT9:
YES

POST_MUST_PROJECTOR_DEFINED:
YES

PROCESSING_ORDER_PROVEN:
YES

ROW_MUST_SEMANTICS_PROVEN:
YES

COL_MUST_SEMANTICS_PROVEN:
YES

POINTER_SEMANTICS_PROVEN:
YES

POST_MUST_MAX_PER_CONFIG:
[8,3,9,8,3,9,8]

EXPECTED_POST_MUST_MAX:
[8,3,9,8,3,9,8]

ALL_POST_MUST_MAXIMA_PROVEN:
YES

ANY_REACHABLE_POST_MUST_VISIBLE_GT9:
NO

POST_MUST_PROJECTION_PRESERVES_ORDER:
YES

POST_MUST_PROJECTION_PRESERVES_CONFIG_FEASIBILITY:
YES

POST_MUST_PROJECTION_PRESERVES_PATTERN_FEASIBILITY:
YES

POST_MUST_PROJECTION_PRESERVES_PATTERN_ID:
YES

FINAL_ANALYZER_VIEW_CAPACITY:
9

OPTION_A_STATUS:
VALID_AS_POST_MUST_PROJECTED_ANALYZER

PROJECTOR_REQUIRED_INPUTS:
physical valid; cfg_valid; pointer; descriptor; Hybrid payload; ConfigID;
selected RowMust/ColMust or their counter/Pivot derivation; Pivot inputs;
overflow sideband

PRODUCTION_RTL_MODIFIED:
NO

V2_ANALYZER_CHANGED:
NO

BASELINE_EARLY_CHANGED:
NO

GROUP_CHANGED:
NO

S1GB_RESUMED:
NO

GIT_DIFF_CHECK:
PASS

AUDIT_DOCUMENT:
docs/dss_execution/S1GA2DR_PROJECTED_VIEW_PREDICATE_RECONCILIATION.md

NEXT_RECOMMENDED_PHASE:
S1G-A2E-A2 — 9-Entry Post-Must Per-Config Analyzer Implementation and Proof
```
