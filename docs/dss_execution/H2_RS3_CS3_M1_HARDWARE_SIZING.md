# H2 — RS=CS=3, SHARE_M=1 Analyzer / CAM / Control Sizing Architecture Freeze

> 文件狀態：Complete
> 適用範圍：2x2 Directional CAM DSS V2-derived RS=CS=3, SHARE_M=1 implementation-size contract
> 建立時間：2026-09-13T15:26:09+08:00
> 最後修改時間：2026-09-13T15:44:24+08:00

## Conclusion

H2 converts the frozen H1 envelope into logical implementation bounds. It does
not create RTL, change simulator behavior, run experiments, or synthesize.
H2R resolved the former H2-CL-005 blocker as a stale execution gate and made
the analyzer-logical versus physical mode-reused CAM accounting explicit.

~~~
ADDRESS_ENTRIES_2_2_1_EXISTING = 5
HYBRID_ENTRIES_2_2_1_EXISTING = 7
ADDRESS_ENTRIES_3_3_1_TARGET = 7
HYBRID_ENTRIES_3_3_1_TARGET = 17
MAX_K_REQUIRED = 7
MATRIX_BITS_REQUIRED = 49
MAX_CANDIDATE_COUNT = 35
PATTERN_ID_W = 6
CANDIDATE_BITMAP_W = 35
PIVOT_PTR_W_REQUIRED = 3
GROUP_HISTORY_BITS_REQUIRED = 112
LEDGER_STATE_CHANGED = NO
COMPATIBILITY_STRATEGY = C
~~~

The target counts are logical shared-analyzer interface capacities for the full
H1 configuration union. They are not a physical macro-depth or final
reconstruction-storage claim.

## 1. Reuse audit, updated for H1

| Block | H2 classification | Required target treatment |
|---|---|---|
| shared analyzer | NEW_MODULE_JUSTIFIED | Existing 5x5 arrays, 10 candidates, 4-bit PatternID and historical table cannot represent K=7 / 35 candidates. |
| Config adapter/table | NEW_MODULE_JUSTIFIED | Add a versioned target table without redefining frozen 2,2,1 IDs globally. |
| Candidate generator/table | NEW_MODULE_JUSTIFIED | Add four target canonical classes with a 35-candidate bound. |
| PatternID encoder / bitmap | PARAMETERIZE_EXISTING | Widen to 6 / 35 while preserving one-based ID. |
| Transpose wrapper and must decode | REDESIGN_REQUIRED | Existing hard-coded IDs and 5-entry logic cannot cover target classes. |
| Scheduler / four role slots | EXTEND_EXISTING | Preserve four semantic slots and A→B→C→D traversal. |
| EARLY control | EXTEND_EXISTING | Preserve selection/commit semantics; widen PatternID result fields. |
| GROUP candidate store | PARAMETERIZE_EXISTING | Change 16 records from valid+4-bit PatternID to valid+6-bit PatternID. |
| GROUP priority reader / slot decoder | EXTEND_EXISTING / REDESIGN_REQUIRED | Read target action-semantic priorities and decode target table. |
| Topology legality | EXTEND_EXISTING | Retain m=1 graph/donor priority; change only target descriptor envelope. |
| Action encoding | REUSE_AS_IS | Release/borrow booleans plus valid-qualified resource IDs encode all four actions. |
| Ledger / diagnostic adapter | REUSE_AS_IS | Four single-owner resources remain sufficient at m=1. |
| Verification bridge | EXTEND_EXISTING | Reuse golden/regression method with target tables and vectors. |
| Synthesis top | EXTEND_EXISTING | A later target-only manifest may be added after functional closure. |

## 2. Address CAM capacity

AddressEntries = R + C.

| Configuration | R | C | AddressEntries |
|---|---:|---:|---:|
| LOCAL_3R3C | 3 | 3 | 6 |
| AD_RELEASE_2R3C | 2 | 3 | 5 |
| AD_BORROW_3R4C | 3 | 4 | 7 |
| AD_RELEASE_BORROW_2R4C | 2 | 4 | 6 |
| BC_RELEASE_3R2C | 3 | 2 | 5 |
| BC_BORROW_4R3C | 4 | 3 | 7 |
| BC_RELEASE_BORROW_4R2C | 4 | 2 | 6 |

~~~
MAX_ADDRESS_ENTRIES_REQUIRED = 7
~~~

## 3. Hybrid CAM capacity

HybridEntries = R × (C - 1) + C × (R - 1).

| Configuration | R | C | HybridEntries |
|---|---:|---:|---:|
| LOCAL_3R3C | 3 | 3 | 12 |
| AD_RELEASE_2R3C | 2 | 3 | 7 |
| AD_BORROW_3R4C | 3 | 4 | 17 |
| AD_RELEASE_BORROW_2R4C | 2 | 4 | 10 |
| BC_RELEASE_3R2C | 3 | 2 | 7 |
| BC_BORROW_4R3C | 4 | 3 | 17 |
| BC_RELEASE_BORROW_4R2C | 4 | 2 | 10 |

~~~
MAX_HYBRID_ENTRIES_ANALYTICAL = 17
MAX_HYBRID_ENTRIES_REQUIRED = 17
~~~

The frozen V2 boundary takes one shared physical pivot/Hybrid bundle and does
not persist independent Hybrid state per role slot or configuration. H3 should
therefore use a shared target bundle with 7 pivots and 17 Hybrid entries, not
per-config replicated storage.

## 4. Existing versus target DSS geometry

Base 2R2C requires four Address entries and four analytical Hybrid entries.
The frozen 5/7 interface instead covers the complete old multi-config
envelope: legacy 3R2C/2R3C reaches R+C=5 and
3×(2-1)+2×(3-1)=7. Thus the existing 5x5 matrix and HYBRID_ENTRIES=7 are
envelope maxima, not merely base RECAM constants.

The target 3R4C/4R3C analogously sets the new maxima.

~~~
ADDRESS_ENTRIES_2_2_1_EXISTING = 5
HYBRID_ENTRIES_2_2_1_EXISTING = 7
ADDRESS_ENTRIES_3_3_1_TARGET = 7
HYBRID_ENTRIES_3_3_1_TARGET = 17
GEOMETRY_DERIVATION_METHOD = maximum over the architecture-point role-union,
  using R+C and R(C-1)+C(R-1), respectively
~~~

## 5. Entry widths and matrix sizing

The frozen address domains remain ROW_ADDR_W=9, COL_ADDR_W=5, and
DIFF_ADDR_W=9; H2 found no authority to change them.

| Logical entry | Fields | Bits |
|---|---|---:|
| Address/pivot | valid (1), row address (9), column address (5), row-must (1), column-must (1) | **17** |
| Hybrid | valid (1), pivot pointer (3), executable descriptor (1), differing address (9) | **14** |

~~~
ANALYZER_LOGICAL_ADDRESS_ENTRY_BITS = 17
ANALYZER_LOGICAL_HYBRID_ENTRY_BITS = 14
PIVOT_PTR_W_REQUIRED = ceil(log2(7)) = 3
~~~

Pointer indices 0 through 6 need three bits. The matching old width is
coincidental; it is derived anew here. The executable descriptor convention is
unchanged: zero means same pivot row/differing column; one means same pivot
column/differing row. Transpose must invert it.

| Matrix item | Target |
|---|---:|
| Maximum active K | 7 |
| Physical combinational matrix | 7x7 |
| MATRIX_BITS_REQUIRED | 49 |
| Active K in H1 table order | 6, 5, 7, 6, 5, 7, 6 |

Recommendation: one fixed 7x7 combinational matrix with active-dimension
masking. This retains shared-analyzer organization and makes inactive space
unambiguous. No implementation is made in H2.

### CAM entry accounting scope (H2R resolution)

The 17-bit Address and 14-bit Hybrid values above are specifically
ANALYZER_LOGICAL_ENTRY_BITS. They are the frozen Phase-3B logical state passed
to Matrix Builder; they must not be substituted for physical mode-reused CAM
entry widths.

Frozen Phase-3B reusable-CAM accounting includes offline and online modes:

| Structure | Offline entry derivation | Online entry derivation | Physical/mode-reused entry bits |
|---|---:|---:|---:|
| Address CAM | 1+9+5+1+1+2+2 = 21 | 1+9+5+13 = 28 | 28 |
| Hybrid CAM | 1+3+1+9 = 14 | 1+3+16 = 20 | 20 |

The 13-bit Address online suffix is the frozen Domain+Bank+Group+SA channel
tag; the 16-bit Hybrid online payload is the frozen replacement word. Those
fields are absent from the analyzer boundary but are required by the
mode-reused physical accounting. H2 freezes the target values below as an
H2_ARCHITECTURE_REQUIREMENT, not a post-synthesis fact or physical silicon-area
claim. The existing representation applies without width change because the
target still has the same 27-bit system address boundary, 16-bit replacement
word, and three-bit pivot pointer; only entry counts change.

| Structure / accounting item | Existing 2,2,1 | Target 3,3,1 | Scope | Width source / status |
|---|---:|---:|---|---|
| Address entries | 5 | 7 | analyzer and physical capacity | H1 envelope / H2 requirement |
| Address logical bits/entry | 17 | 17 | analyzer logical state | 1+9+5+1+1 |
| Address physical bits/entry | 28 | 28 | mode-reused CAM maximum | max(offline 21, online 28) |
| Address logical total bits | 85 | 119 | analyzer logical state | entries × 17 |
| Address physical total bits | 140 | 196 | raw mode-reused CAM storage | entries × 28; H2 requirement |
| Hybrid entries | 7 | 17 | analyzer and physical capacity | H1 envelope / H2 requirement |
| Hybrid logical bits/entry | 14 | 14 | analyzer logical state | 1+3+1+9 |
| Hybrid physical bits/entry | 20 | 20 | mode-reused CAM maximum | max(offline 14, online 20) |
| Hybrid logical total bits | 98 | 238 | analyzer logical state | entries × 14 |
| Hybrid physical total bits | 140 | 340 | raw mode-reused CAM storage | entries × 20; H2 requirement |
| Combined physical raw storage | 280 | 536 | mode-reused CAM storage | Address + Hybrid; no CAM circuitry/area implied |

The physical totals exclude comparators, update/clear logic, collector,
counters, decoder, temporary reuse buffer, analyzer logic, macro packing, and
silicon area. If H3 deliberately changes the frozen mode-reused representation,
its replacement accounting is DEFER_TO_H3 and must be separately justified;
H2 does not guess such a change.

## 6. Candidate sizing and canonical classes

H3 should expose a 35-candidate fixed combinational superset. Class-specific
internal candidate tables are allowed only if they preserve that uniform
one-based PatternID contract. A fixed superset is preferred now for semantic
clarity, transpose safety, and direct maximum-bound verification.

~~~
candidate_valid[34:0]
pattern_id[5:0]: 0 = invalid; 1..C(K,R) = valid
selection eligibility = solution_valid && repairable
~~~

The bitmap is combinational analyzer metadata, not GROUP retained state.

| Class | Canonical R,C | Physical members | K | Candidates | PatternID domain |
|---|---|---|---:|---:|---|
| CC_3R3C | 3,3 | 3R3C | 6 | 20 | 0, 1..20 |
| CC_3R2C | 3,2 | 3R2C / 2R3C transpose | 5 | 10 | 0, 1..10 |
| CC_4R3C | 4,3 | 4R3C / 3R4C transpose | 7 | 35 | 0, 1..35 |
| CC_4R2C | 4,2 | 4R2C / 2R4C transpose | 6 | 15 | 0, 1..15 |

## 7. Versioned configuration-table contract

A static architecture-point target table is required. Compact target IDs must
be interpreted only with config_table_version=RS3_CS3_M1_DIRECTIONAL; frozen
2,2,1 meanings and the legacy adapter remain untouched.

| Table | Fields actually needed | Function |
|---|---|---|
| Analyzer descriptor | target config index (3), R count (3), C count (3), canonical class (2), transpose (1) | Analysis normalization; K derives as R+C. |
| Policy/action descriptor | role applicability, slot semantic (2), release/borrow bits, release resource (valid + 2-bit ID), ordered donor pair (2+2 bits) | Legality and policy ranking without numeric-ID policy meaning. |

K, action class, and debug IDs are derivable table properties and need not be
separately retained. Table version is static interface context, not candidate
history.

| Target label | R,C | Class | Transpose | Role / semantic slot |
|---|---|---|---:|---|
| T0 | 3,3 | CC_3R3C | 0 | A/D LOCAL; B/C LOCAL |
| T1 | 2,3 | CC_3R2C | 1 | A/D RELEASE_ONLY |
| T2 | 3,4 | CC_4R3C | 1 | A/D BORROW_ONLY |
| T3 | 2,4 | CC_4R2C | 1 | A/D RELEASE_AND_BORROW |
| T4 | 3,2 | CC_3R2C | 0 | B/C RELEASE_ONLY |
| T5 | 4,3 | CC_4R3C | 0 | B/C BORROW_ONLY |
| T6 | 4,2 | CC_4R2C | 0 | B/C RELEASE_AND_BORROW |

T0..T6 are target-table labels only; they do not assign new global meanings to
historical CFG0..CFG6.

## 8. Role slots, actions, and scan

| Role class | Slot 0 | Slot 1 | Slot 2 | Slot 3 |
|---|---|---|---|---|
| A/D | LOCAL (3,3) | RELEASE_ONLY (2,3) | BORROW_ONLY (3,4) | RELEASE_AND_BORROW (2,4) |
| B/C | LOCAL (3,3) | RELEASE_ONLY (3,2) | BORROW_ONLY (4,3) | RELEASE_AND_BORROW (4,2) |

Policy priorities remain table lookups:

~~~
EARLY A/D = LOCAL → RELEASE_ONLY → BORROW_ONLY → RELEASE_AND_BORROW
EARLY B/C = LOCAL → BORROW_ONLY → RELEASE_ONLY → RELEASE_AND_BORROW
GROUP A/D,B/C = RELEASE_ONLY → LOCAL → RELEASE_AND_BORROW → BORROW_ONLY
~~~

Existing valid-qualified action representation is sufficient:

~~~
{release_required, borrow_required}
00 LOCAL; 10 RELEASE_ONLY; 01 BORROW_ONLY; 11 RELEASE_AND_BORROW
release and selected donor resource IDs: valid-qualified 2-bit fields
~~~

No donor-arbitration redesign is implied. Both policies still have four
semantic role slots and a maximum of four analyzer evaluations per SA. H3/H4
must verify actual EARLY short-circuit and GROUP collection cycle behavior.

~~~
ROLE_SLOT_COUNT = 4
EXPECTED_CONFIG_ANALYSIS_COUNT_PER_SA = 4 maximum
~~~

## 9. EARLY retained state

No bitmap or full candidate history is required in streaming EARLY. The target
retains the live ledger, SA/rank/lifecycle state, and selected diagnostics. Only
selected PatternID widens.

| Field class | 2,2,1 bits | 3,3,1 bits | Delta |
|---|---:|---:|---:|
| Ledger | 16 | 16 | 0 |
| Traversal/control | 9 | 9 | 0 |
| Selected ConfigID (4x3) | 12 | 12 | 0 |
| Selected PatternID | 16 | 24 | +8 |
| Commit/donor/borrow/release diagnostics | 20 | 20 | 0 |
| **Decision-boundary retained state** | **73** | **81** | **+8** |

The Phase-4E 75-bit report includes two transient ledger-status flops; on that
inclusive accounting scope the target projects to 83 bits. Neither count is a
synthesis-area prediction.

~~~
EARLY_STATE_FIELDS_2_2_1 = ledger, traversal/lifecycle, selected ConfigID/
  PatternID, commit/action diagnostics
EARLY_STATE_FIELDS_3_3_1 = same, with PatternID 4→6
EARLY_STATE_BITS_DELTA_ESTIMATE = +8
~~~

## 10. GROUP-NoScratch retained history

Config identity, descriptor, action, and donor order are static functions of
SA role and canonical slot. The store needs only local acceptance and the
first valid PatternID through collection.

| Layout | Per candidate record | Number | Bits |
|---|---|---:|---:|
| Frozen 2,2,1 | valid (1) + PatternID (4) | 4 SA x 4 slots | 80 |
| Minimum 3,3,1 | valid (1) + PatternID (6) | 4 SA x 4 slots | 112 |

~~~
GROUP_HISTORY_LAYOUT_2_2_1 = 16 x {local_candidate_valid, PatternID[3:0]}
GROUP_HISTORY_LAYOUT_3_3_1 = 16 x {local_candidate_valid, PatternID[5:0]}
GROUP_HISTORY_BITS_REQUIRED = 112
~~~

local_candidate_valid is solution_valid && repairable; it replaces separate
stored solution-valid/repairable state. Neither the 35-bit bitmap nor
pivot/Hybrid/reconstruction metadata is decision-history state. This does not
claim reconstruction metadata is unnecessary for a future reconstruction
boundary; that issue remains deferred.

## 11. Ledger compatibility

~~~
LEDGER_STATE_CHANGED = NO
~~~

At m=1 the only borrowable resources remain A_ROW, D_ROW, B_COL, and C_COL.
Every target action releases at most one own resource and borrows at most one
donor. Raising local RS/CS changes local configuration feasibility, not the
four-resource count, one-borrower ownership, atomic commit, or no-double-
allocation invariant.

## 12. Compatibility strategy

~~~
COMPATIBILITY_STRATEGY = C
separate 3,3,1 analyzer while retaining frozen 2,2,1 analyzer untouched
~~~

This is the lowest regression-risk choice. The target can reuse verified
scheduler/policy/topology/ledger structure at an explicit interface boundary,
but the current analyzer's historical tables and fixed dimensions cannot be
genericized without new risk. Common factoring is not authorized by H2.

## 13. H3 module delta plan

| Module/file | Current role | H3 recommendation | 2,2,1 risk | Required new test |
|---|---|---|---|---|
| New target analyzer under rtl/dss_v2 | fixed shared analyzer | Isolated 7x7 / 35-candidate analyzer, 7 pivot and 17 Hybrid inputs | None if old source untouched | all classes, K=7, pointer/Hybrid maxima, IDs 1..35 |
| New target table/adapter | legacy config decode | Versioned target descriptor/action tables | None | all seven configurations and transpose mapping |
| Target must/candidate table | fixed 5x5 / 10 candidates | Target active-K and candidate enumeration | None | thresholds, invalid candidate behavior, counts |
| EARLY target wrapper/core integration | traversal | Preserve frozen order, ledger commit and failure behavior | Medium | role slot and first-feasible vectors |
| Target GROUP candidate store | 80-bit store | Target 112-bit store with 7-bit records | Medium | packing, clears, PatternID width |
| GROUP reader / decoder | historical slot map | Target table priority and decode | Medium | both role priorities, static slot identity |
| Target legality boundary | RS/CS=2 assertion | Target descriptor envelope, same m=1 donor graph | Medium | all action classes and donor priority |
| Ledger / diagnostic adapter | resource ownership | Reuse unchanged | Low | existing plus target conservation vectors |
| Test/golden harness | 2,2,1 comparison | Extend only under H4 authorization | Low | target directed/random comparison |
| Source manifest | frozen point | Target-only list after functional closure | Low | source order and baseline isolation |

## 14. H3/H4 verification contract

H3 must compile/lint the target-only graph and rerun frozen 2,2,1 regression
unchanged. H4 must verify:

- all seven physical configs, four canonical classes, and all transpose pairs;
- K=7 / 49-bit matrix, 35-candidate boundary, PatternID 1..35 and invalid 0;
- pivot pointer indices 0..6 and Hybrid entries 0..16;
- target table, role slots, action mapping, EARLY and GROUP rank ordering;
- exact 112-bit GROUP storage and invalid clearing;
- resource conservation, donor priority, atomic commit, no double allocation,
  latest-ledger use, first failure, and no rollback;
- independent per-policy decision/resource equivalence; paired four-class
  outcome accounting; and
- frozen 2,2,1 full regression unchanged.

H2R supersedes the historical stale requirement “EARLY success => GROUP
success.” The required H4 policy contract is instead:

~~~
RTL EARLY == independent/frozen EARLY golden
RTL GROUP-NoScratch == independent/frozen GROUP-NoScratch golden
paired outcome accounting = BOTH_PASS, EARLY_ONLY, GROUP_ONLY, BOTH_FAIL
~~~

EARLY_ONLY is a legal paired outcome, not a functional failure, unless either
RTL result differs from its own policy golden. Universal H4 invariants are
legal ledger state; no double allocation; borrower exclusion; directional donor
legality; deterministic tie-break; no commit after first failure; no rollback;
no illegal externally committed partial state after failure; and unchanged
frozen 2,2,1 regression.

H4 golden source/dependency is the independent software allocator in
tb/recam/recam_dss_group_allocator_random_test.cpp, extended only under H4
authorization for the target table. It independently implements per-policy
ranking, donor feasibility, A→B→C→D commit, first failure, and no rollback.
The target RTL/V2 bridge must compare selected config/action, PatternID, donor,
release, borrow, per-commit and final ledger, failure position, and
group_repairable. Existing target-extension patterns are the frozen EARLY and
GROUP equivalence harnesses and runners, not DynamicSpareSharing
GroupCompressed; H0S0-CL-004 remains open.

## 15. Conflict and phase control

H0S0-CL-004 remains CONFLICT_OPEN. It still blocks S1, E0, and E1 because the
dynamic simulator's exhaustive GroupCompressed selector is not greedy V2
GROUP-NoScratch. It does not block H2/H3/H4/H5, and H2 found no target hardware
dependency on that simulator policy.

H2-CL-005 is resolved as STALE_EXECUTION_GATE. It was a stale verification
gate, not an RTL semantic or new architecture conflict. The valid
cross-policy contract is four-class paired outcome accounting with independent
per-policy golden equivalence. Frozen Phase-4J results
EARLY_ONLY=0 and GROUP_ONLY=4691 are EMPIRICAL_CORPUS_RESULT only; they are
not structural, mathematical, or universal dominance.

~~~
H2 = COMPLETE
H2R = COMPLETE
H2_CL_005_CLASSIFICATION = STALE_EXECUTION_GATE
READY_FOR_H3 = YES
NEXT_PHASE_AUTHORIZED = NONE
~~~

No RTL, simulator behavior, synthesis, or experiment was changed or run.
