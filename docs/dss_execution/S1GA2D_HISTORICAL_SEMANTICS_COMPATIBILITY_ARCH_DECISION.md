# S1G-A2D — Historical-Semantics Compatibility Architecture Decision

## 1. Executive conclusion

**COMPLETE — Option A is selected: a 9-entry, per-Config projected analyzer.**

The default historical collector's proven maximum Config-local Hybrid demand is
`[8, 3, 9, 8, 3, 9, 8]` for ConfigIDs 0 through 6.  The smallest common
analyzer-view capacity preserving full historical semantics is therefore **9**.
It is neither the obsolete V2 value 7 nor the global physical-store capacity
14.

Option A preserves the exact historical ordered Config view using stable
projection from complete historical collector state.  It requires no global
14-entry membership-aware analyzer at the analyzer boundary.  This document
freezes that contract only; no RTL, retained-state sizing, synthesis, or
S1G-B implementation is authorized or performed.

## 2. Starting Model-C blocker

S1G-A2C proved that the historical collector has 14 allocated global Hybrid
slots, whereas frozen V2 takes seven untagged logical slots.  It also found
reachable Config-local views of eight and nine entries.  This phase proves
whether those were maxima and chooses the smallest semantics-preserving
architecture.

The source facts used are:

- `MAX_FAULTS=12`, `MAX_K=5`, and `HYBRID_SHARED_ENTRIES=14`:
  `rtl/dss_2x2/analyzer/shared_fault_collector.sv:5-10`.
- An accepted related fault creates at most one Hybrid record; membership is
  `relation_ptr < k(ConfigID)`: `shared_fault_collector.sv:49-70`.
- The historical view filters valid records by Config membership and final
  Must state: `rtl/dss_2x2/analyzer/multi_config_analyzer_bank.sv:35-56`.
- RowMust is `row_count > Cs`; ColumnMust is `col_count > Rs`:
  `rtl/dss_2x2/analyzer/config_must_view.sv:24-46`.

## 3. Per-Config Hybrid capacity derivation

For Config geometry `(R,C)`, `k=R+C`, and `p` useful pivots in the Config's
visible prefix, every historical trace has this upper bound:

```text
H_visible(p) <= min(12 - p, p * ((C - 1) + (R - 1)))
                 min(12 - p, p * (k - 2)), for 1 <= p <= k.
```

Reason: a surviving row-descriptor record requires its pivot row count to be
at most `C`, leaving at most `C-1` such records after the pivot itself.
Likewise a column-descriptor record is bounded by `R-1`.  The `12-p` term is
the accepted-fault budget.  Cross-pivot faults only consume these counts and
produce at most one record, so cannot exceed the bound.  Pivots outside the
Config prefix use budget but have no membership-valid records.

| ConfigID | Role/config | R | C | k | Row + column cases per pivot | Symbolic maximum |
| ---: | --- | ---: | ---: | ---: | --- | ---: |
| 0 | 2R2C | 2 | 2 | 4 | 1 + 1 | 8 |
| 1 | 2R1C | 2 | 1 | 3 | 0 + 1 | 3 |
| 2 | 3R2C | 3 | 2 | 5 | 1 + 2 | 9 |
| 3 | 3R1C | 3 | 1 | 4 | 0 + 2 | 8 |
| 4 | 1R2C | 1 | 2 | 3 | 1 + 0 | 3 |
| 5 | 2R3C | 2 | 3 | 5 | 2 + 1 | 9 |
| 6 | 1R3C | 1 | 3 | 4 | 2 + 0 | 8 |

CAM-reuse is a separate `cam_reuse_temp_buffer` path for an unrelatable fault
after the relevant pivot prefix; it does not increase Hybrid-view demand
(`shared_fault_collector.sv:62-70,94-98`).

## 4. MAX_REACHABLE_HYBRID_PER_CONFIG proof

The isolated helper
[s1ga2d_hybrid_capacity_proof.py](../../scripts/analysis/s1ga2d_hybrid_capacity_proof.py)
enumerates every permitted no-Must row/column allocation vector for each
`p=1..k`, then constructs and evaluates a concrete fault trace using frozen
greedy relation, membership, and final Must rules.  It imports and changes no
production RTL.

```bash
/home/asd9112000/repair_rate/venv/bin/python \
  scripts/analysis/s1ga2d_hybrid_capacity_proof.py
```

The executable maximum equals the symbolic upper bound for every Config.
This is not random search: the derivation bounds arbitrary traces; the bounded
enumeration checks every allocation capable of attaining it and emits a
witness.

| ConfigID | Existing lower bound | Upper bound | Witness pivots | Row extras/pivot | Column extras/pivot | Proven maximum |
| ---: | ---: | ---: | ---: | --- | --- | ---: |
| 0 | 8 | 8 | 4 | `[1,1,1,1]` | `[1,1,1,1]` | **8** |
| 1 | 3 | 3 | 3 | `[0,0,0]` | `[1,1,1]` | **3** |
| 2 | 9 | 9 | 3 | `[1,1,1]` | `[2,2,2]` | **9** |
| 3 | 8 | 8 | 4 | `[0,0,0,0]` | `[2,2,2,2]` | **8** |
| 4 | 3 | 3 | 3 | `[1,1,1]` | `[0,0,0]` | **3** |
| 5 | 9 | 9 | 3 | `[2,2,2]` | `[1,1,1]` | **9** |
| 6 | 8 | 8 | 4 | `[2,2,2,2]` | `[0,0,0,0]` | **8** |

Each extra uses a fresh opposite coordinate, so witness records cannot
accidentally create another pivot relation.  Configs 0, 2, 3, 5, and 6 consume
all 12 accepted faults; Configs 1 and 4 need six.  For Config 2, three pivots
plus one row and two column extras per pivot produce nine members while final
row counts equal 2 and column counts equal 3: neither is Must.

## 5. Global required analyzer capacity

```text
GLOBAL_REQUIRED_CONFIG_VIEW_CAPACITY
  = max(8,3,9,8,3,9,8)
  = 9.
```

This is a per-selected-Config analyzer-view capacity.  It is not the global
physical-store capacity 14.  The complete global historical state remains
required upstream for projection and future fault transitions.  At default
parameters, only 11 global Hybrid slots are reachable because the first of 12
accepted faults must become a pivot; that does not alter the allocated
14-entry contract.

## 6. Option A architecture

```text
complete global historical H0..H13 state + cfg_valid + Must view
  -> stable Config-specific filter in increasing H index
  -> compact HCV-9 view
  -> one shared Config/Pattern analyzer
```

For selected Config `c`, an HCV slot is emitted only for:

```text
physical_valid && cfg_valid[c] && legal_pointer &&
!(descriptor ? col_must[c][pointer] : row_must[c][pointer])
```

The resulting sequence is an increasing-H-index subsequence, padded with
invalid tail slots.  It is the historical `multi_config_analyzer_bank` view
expressed as a compact nine-entry boundary.  Ordering, Config feasibility,
Pattern feasibility, and PatternID semantics are preserved when this exact
projection is used.

At frozen DATE geometry, the HCV-9 analyzer input is 232 bits:

```text
pivot/Must/overflow base: 106 bits
9 x (valid + pointer[2:0] + descriptor + differing_address[8:0]): 126 bits
total: 232 bits
```

This is an analyzer-interface width only, not a retained-state width.  It is
28 bits larger than the 204-bit seven-slot interface.  The V2 analyzer's
Hybrid traversal loop is parameterized by `HYBRID_ENTRIES`
(`rtl/recam/recam_shared_config_analyzer.sv:5-10,185-245`), but its packer,
projector boundary, and integration need future work.  Reuse is therefore
partial.

## 7. Option B architecture

```text
14 global logical Hybrid records + cfg_valid[6:0] per record
  -> membership-aware analyzer for selected Config
```

Option B is semantically feasible: the existing historical
`multi_config_analyzer_bank` already filters this representation in physical
order.  A shared analyzer could consume one selected Config sequentially, so
it need not instantiate seven analyzers simultaneously.

At frozen geometry, each global analyzer-facing logical entry is
`valid + pointer + descriptor + differing_address + cfg_valid[6:0] = 21` bits.
Fourteen entries plus the 106-bit pivot/Must/overflow base are a 400-bit
analyzer structural input.  It retains global ordering naturally but doubles
Hybrid traversal relative to current V2 and transports 98 membership bits.

## 8. Option C consequences

Keeping the 7-entry/204-bit boundary defines a reduced-capacity V2 derivative,
not full historical RECAM semantics.  It would require a matched reduced
baseline for repair-rate comparisons.  Existing 7-slot synthesis remains valid
only as QoR for that restricted design; latency data remains valid only for its
event path, not a full-historical analyzer.  Option C is not selected.

## 9. Semantic-equivalence matrix

| Property | Historical RECAM | Option A | Option B | Option C |
| --- | --- | --- | --- | --- |
| 14 physical Hybrid entries | source state | PRESERVED upstream | PRESERVED at boundary | CHANGED |
| `cfg_valid` | source state | PRESERVED upstream; projector consumes it | PRESERVED | CHANGED / absent |
| per-Config visible capacity | 8/3/9/8/3/9/8 | PRESERVED | PRESERVED | CHANGED |
| entry order | append H order | PRESERVED by stable compaction | PRESERVED directly | cannot preserve 8/9 views |
| dictionary/full semantics | filtered ordered view | PRESERVED | PRESERVED | CHANGED |
| overflow semantics | historical collector condition | REQUIRES exact propagation | REQUIRES exact propagation | REQUIRES reinterpretation |
| Config feasibility | reference | PRESERVED | PRESERVED | CHANGED |
| Pattern feasibility / PatternID | reference | PRESERVED | PRESERVED | CHANGED |
| future-fault transition | complete source state | PRESERVED upstream | PRESERVED | CHANGED |
| full historical equivalence | reference | PRESERVED | PRESERVED | CHANGED |

## 10. Pattern/order implications

Hybrid capacity is fault-entry capacity, not spare-pattern capacity.  Candidate
patterns depend only on `R`, `C`, and `k` in both the historical analyzer
(`config_analyzer.sv:25,45-49`) and V2 canonical tables
(`recam_shared_config_analyzer.sv:54-115,248-265`).

| Geometry | Patterns | PatternID width | Candidate bitmap | Change from 7 to 9 Hybrid entries? |
| --- | ---: | ---: | ---: | --- |
| 2R1C / 1R2C | 3 | 4 | 10 | no |
| 2R2C | 6 | 4 | 10 | no |
| 3R1C / 1R3C | 4 | 4 | 10 | no |
| 3R2C / 2R3C | 10 | 4 | 10 | no |

Additional Hybrid records can change the matrix and thus which fixed Pattern
is first feasible.  They do not create candidates or widen PatternID.  The
required invariant is stable increasing H-index projection: never sort by
pointer, descriptor, address, or Config membership.  Existing transpose
normalization for Configs 4–6 must remain unchanged.

## 11. Complexity scaling

Structural estimates only; no area or timing measurement was made.

| Aspect | Option A | Option B |
| --- | --- | --- |
| Analyzer Hybrid slots | 9 | 14 |
| Growth from current 7 slots | `9/7 = 1.286x` | `14/7 = 2.0x` |
| Logical Hybrid input | 9 x 14 = 126 bits | 14 x 21 = 294 bits |
| Membership logic | 14-entry stable projection for active Config | 14 membership tests per active Config traversal |
| Compaction | 14-to-9 stable selection | none; skip invalid records in H order |
| Traversal/dictionary comparisons | 9 records | 14 records |
| Candidate patterns / PatternID | unchanged | unchanged |
| Main risk | projector mux/priority structure | input width, membership fan-in, 14-slot path |

## 12. Overlap-control impact

| S1G-A concept | Option A | Option B | Option C |
| --- | --- | --- | --- |
| latest-generation-only analysis | UNCHANGED | UNCHANGED | UNCHANGED, restricted |
| update invalidates old analysis | UNCHANGED | UNCHANGED | UNCHANGED |
| no provisional ledger / no rollback | UNCHANGED | UNCHANGED | UNCHANGED |
| A->B->C->D ownership | UNCHANGED | UNCHANGED | UNCHANGED |
| commit after test_done + latest analysis | UNCHANGED | UNCHANGED | UNCHANGED |

Only the analyzer-ready state contract changes.  The existing S1G-A control's
generation and commit semantics are reusable, but its fixed 204-bit payload is
restricted.  The current role scheduler already evaluates four permitted
Configs sequentially (`recam_role_aware_config_scheduler.sv:45-105`); Option A
projects once per active Config, while Option B masks global records directly.
Neither option yields an exact cycle claim in this audit.

## 13. Prior-result impact

| Evidence | Classification | Reason |
| --- | --- | --- |
| S0R simulator alignment | UNAFFECTED | policy alignment is independent of 204-bit analyzer input |
| 2x2 EARLY synthesis | NEEDS REINTERPRETATION | valid QoR for seven-slot V2 only |
| 2x2 GROUP synthesis | NEEDS REINTERPRETATION | same seven-slot limitation |
| S1A event contract | UNAFFECTED | endpoints remain valid |
| S1B latency instrumentation | NEEDS REINTERPRETATION | measured the seven-slot path |
| E0-L latency results | MUST BE RE-RUN | full-historical analyzer path changes |
| S1E concurrency audit | UNAFFECTED | control feasibility is capacity-independent |
| S1F overlap contract | NEEDS REINTERPRETATION | analyzer-ready summary is incomplete |
| S1G-A overlap control | NEEDS REINTERPRETATION | control is useful, fixed payload is restricted |

## 14. Paper-impact audit

If Option C were selected later, the following claims would need revision:

| Claim | Status under Option C |
| --- | --- |
| “RECAM baseline” | must be weakened to a seven-slot V2 derivative |
| “preserves RECAM semantics” | invalid |
| “extends RECAM” | must be weakened |
| “optimal repairability” | invalid; historical RECAM is not a physical-optimum oracle either |
| “same repair capability” | invalid |
| “fair baseline” | invalid without a matched reduced-capacity baseline |

The thesis outline labels RECAM as a baseline but has no wording that rescues
reduced-capacity equivalence.  No thesis/paper file was modified.

## 15. Architecture decision

| Criterion | Option A | Option B | Option C |
| --- | --- | --- | --- |
| Historical semantic fidelity | HIGH | HIGH | LOW |
| Compact V2 compatibility | HIGH after projector update | MEDIUM | HIGH but restricted |
| Expected hardware complexity | MEDIUM | HIGH | LOW |
| Expected timing risk | MEDIUM | HIGH | LOW / not comparable |
| Verification complexity | MEDIUM | HIGH | MEDIUM plus rebaseline |
| Prior-result impact | MEDIUM | HIGH | HIGH |
| Paper/baseline consistency | HIGH | HIGH | LOW |
| Implementation risk | MEDIUM | HIGH | low technically, unacceptable semantically |

**Select Option A.**  It is the smallest common capacity (`N=9 < 14`) proved
to preserve the historical ordered view.  Option B is feasible but carries
global-14 membership complexity without extra semantic benefit after exact
projection.  Option C is a semantic redefinition, not a compatibility choice.

## 16. Frozen next analyzer contract

This is a contract for a later S1G-A2E-A implementation/proof, not RTL.

```text
Name: Historical Config View (HCV-9)
CONFIG_VIEW_HYBRID_ENTRIES: 9

Input abstraction:
  One selected ConfigID's exact historical logical analyzer view.

Producer prerequisite:
  Complete historical global collector state remains upstream: H0..H13 order,
  valid state, cfg_valid, pointer, descriptor, payload/differing-address
  source, and Must/overflow source.

Projection:
  Scan H0..H13 in increasing index.  Emit a compact prefix only for an entry
  valid for selected ConfigID and not retired by its final Must condition.
  Do not sort, deduplicate, or otherwise reorder.  Invalid-fill tail slots.

Analyzer record:
  valid; pivot_pointer[2:0]; descriptor; differing_address[8:0].

Pivots/Must:
  Frozen DATE five-pivot row/ColumnWord fields and selected-Config derived
  Must predicates.

Membership:
  cfg_valid is consumed by the upstream projector and removed only after exact
  projection.  It remains source state for future collector transitions.

Overflow/full:
  Propagate the historical candidate-gating condition exactly.  Physical
  collector-full remains an upstream source fact, not an HCV-9 capacity flag.

Sharing:
  One common nine-entry interface serves all Configs; smaller views have an
  invalid tail.  SA sharing A->B->C->D and Config sequential iteration remain
  separate control concerns.

PatternID invariant:
  Stable H-index order preserves ordered dictionary insertion and fixed
  spare-index Pattern scan order.
```

## 17. Remaining unknowns

1. Complete retained collector-state representation and its minimum width are
   still not proven.
2. HCV-9 projection equivalence, overflow propagation, and role-schedule
   integration need an explicitly authorized RTL/equivalence phase.
3. Projector timing/area and resulting latency are unknown; no synthesis or
   latency experiment was run.

## 18. Recommended next phase

```text
NEXT:
S1G-A2E-A
Expanded Per-Config Analyzer Contract Implementation/Proof
```

This next phase is not started here.

```text
S1GA2D_STATUS:
COMPLETE

CONFIG0_MAX_REACHABLE_HYBRID:
8

CONFIG1_MAX_REACHABLE_HYBRID:
3

CONFIG2_MAX_REACHABLE_HYBRID:
9

CONFIG3_MAX_REACHABLE_HYBRID:
8

CONFIG4_MAX_REACHABLE_HYBRID:
3

CONFIG5_MAX_REACHABLE_HYBRID:
9

CONFIG6_MAX_REACHABLE_HYBRID:
8

ALL_CONFIG_MAXIMA_PROVEN:
YES

GLOBAL_REQUIRED_CONFIG_VIEW_CAPACITY:
9

HISTORICAL_GLOBAL_HYBRID_CAPACITY:
14

OPTION_A_FULL_HISTORICAL_EQUIVALENCE:
YES

OPTION_B_FULL_HISTORICAL_EQUIVALENCE:
YES

OPTION_C_FULL_HISTORICAL_EQUIVALENCE:
NO

OPTION_A_EXISTING_ANALYZER_STRUCTURE_REUSABLE:
PARTIAL

OPTION_B_MEMBERSHIP_AWARE_ANALYZER_FEASIBLE:
YES

SELECTED_ARCHITECTURE:
OPTION_A

SELECTED_ANALYZER_HYBRID_CAPACITY:
9

SELECTED_INTERFACE_PRESERVES_CFG_VALID_SEMANTICS:
YES

SELECTED_INTERFACE_PRESERVES_ORDERING:
YES

SELECTED_INTERFACE_PRESERVES_CONFIG_FEASIBILITY:
YES

SELECTED_INTERFACE_PRESERVES_PATTERN_FEASIBILITY:
YES

SELECTED_INTERFACE_PRESERVES_PATTERN_ID:
YES

OVERLAP_CONTROL_ARCHITECTURE_REUSABLE:
PARTIAL

PRIOR_E0L_RESULT_STATUS:
RERUN

PRIOR_S1GA_CONTROL_STATUS:
REINTERPRET

MINIMUM_COMPLETE_RETAINED_STATE_BITS_PER_SA:
NOT_PROVEN

V2_ANALYZER_RTL_CHANGED:
NO

BASELINE_EARLY_CHANGED:
NO

GROUP_CHANGED:
NO

S1GB_RESUMED:
NO

PRODUCTION_RTL_MODIFIED:
NO

GIT_DIFF_CHECK:
PASS

NEXT_RECOMMENDED_PHASE:
S1G-A2E-A — Expanded Per-Config Analyzer Contract Implementation/Proof

DECISION_DOCUMENT:
docs/dss_execution/S1GA2D_HISTORICAL_SEMANTICS_COMPATIBILITY_ARCH_DECISION.md
```
