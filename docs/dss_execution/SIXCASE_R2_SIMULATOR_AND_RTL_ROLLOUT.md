# DATE2026 Six-Case R2 Simulator and RTL Rollout

> Phase: `SIXCASE-R2-SIMULATOR-CLOSURE-AND-RTL-ROLLOUT`
> Status: simulator closure complete; full N=2/3/4 common-corpus preflight partial; RTL rollout pending
> Scope reached: archive provenance, final HYP02 static-path contract, and
> capacity/action mapping audit

## Result

The human decision resolves the mapping conflict: final archived G2X2_RC is
the authoritative contract.  The C++ bridge now exposes separate static EARLY
and GROUP policies without modifying either final archive or using the
physical-ledger selector as a surrogate for HYP02.

Focused regression confirms the existing 81-path proof, the static GROUP
bridge across the 515-case analyzer corpus, archive EARLY priority, and
N=2/N=3 role-slot capacities. Independent test-side action/config/pattern
oracles also pass 1,000 deterministic random fault vectors for both EARLY and
GLOBAL policies in each of HYP02 RC, G2X2_R, and L1X4_R. G2X2_R and L1X4_R
CLI smoke paths pass. The existing runner now has an N=2-only `sixcase_static`
preset; its 1,000-group common-corpus preflight completed. New RTL and
synthesis remain pending.

## R2 mapping correction

R2 §12 handwritten resource mapping: **SUPERSEDED**.

Authoritative mapping: **FINAL_ARCHIVE_DERIVED**.  The continuation decision
selects exact final-archive reproduction and prohibits redefining or changing
either final G2X2_RC RTL archive.  This preserves the bridge provenance.

The static C++ bridge now has separate `hyp02_static_early` and
`hyp02_static_global` policy identities.  They use the archive capacity/action
slots and static path legality only; they do not report a PhysicalResourceLedger
allocation as an HYP02 decision.

## Final archive graph

| SA | Outgoing resource | Incoming resource | Release action | Borrow action |
|---|---|---|---|---|
| A | A --ROW--> C | B --COLUMN--> A | ROW | COLUMN |
| B | B --COLUMN--> A | D --ROW--> B | COLUMN | ROW |
| C | C --COLUMN--> D | A --ROW--> C | COLUMN | ROW |
| D | D --ROW--> B | C --COLUMN--> D | ROW | COLUMN |

```text
DIRECTED_EDGE_0 = B --COLUMN--> A
DIRECTED_EDGE_1 = D --ROW--> B
DIRECTED_EDGE_2 = A --ROW--> C
DIRECTED_EDGE_3 = C --COLUMN--> D
```

## Sparse action/config adapter

`ACTION_SLOT_ID` is fixed as `LOCAL=0`, `RELEASE=1`, `BORROW=2`,
`RELEASE_BORROW=3`; it is now distinct from the dense analyzer-result index.
L1X4_R maps A to `{0->CFG_LOCAL,1->CFG_RELEASE}`, B/C to
`{0->CFG_LOCAL,1->CFG_RELEASE,2->CFG_BORROW,3->CFG_LOCAL}`, and D to
`{0->CFG_LOCAL,2->CFG_BORROW}`.  D action slot 2 is not compacted.

`L1X4_R_SPARSE_SLOT_BLOCKER = RESOLVED`; analyzer core algorithm changes: none.

## Independent static-policy oracle closure

The focused test owns a separate literal tuple/priority evaluator. It derives
availability and minimum PatternID only from retained analyzer attempt results;
it does not call the production static selector. For each successful choice it
compares repairability, fixed action slot, external ConfigID, dense analyzer
config index, and PatternID. The deterministic random corpus contains 1,000
vectors per architecture, with both EARLY and GLOBAL evaluated on each vector.

```text
HYP02_RC_STATIC_POLICY_VECTOR_ORACLE_CASES: 1000
G2X2_R_STATIC_POLICY_VECTOR_ORACLE_CASES: 1000
L1X4_R_STATIC_POLICY_VECTOR_ORACLE_CASES: 1000
STATIC_POLICY_ORACLE_MISMATCHES: 0

N2_COMMON_CORPUS_PREFLIGHT: 1000 groups at F_GROUP=8, seed=20260923
SIX_STATIC_CASE_REPAIR_RESULTS: 1000/1000 each
```

## Archive evidence

The frozen GROUP archive's `static_path_table.csv` names the active action
semantics directly:

| Final HYP02 role | Slot 1 | Slot 2 | Slot 3 |
|---|---|---|---|
| A/D | release **row**: `(N-1,N)` | borrow **column**: `(N,N+1)` | release-row + borrow-column: `(N-1,N+1)` |
| B/C | release **column**: `(N,N-1)` | borrow **row**: `(N+1,N)` | release-column + borrow-row: `(N+1,N-1)` |

This is independently encoded by the final source:

- `dss_v2_group_slot_decode.sv` maps A/D `ConfigID 4/5/6` to
  `1R2C/2R3C/1R3C` at N=2 and B/C `ConfigID 1/2/3` to
  `2R1C/3R2C/3R1C`.
- `tests/p3_synb_hyp02_exact_81path_proof_test.cpp` declares the same role
  slot semantics before comparing C++ and the 81 static paths.
- `recam_dss_hyp02_static_global_core.sv` documents the directed resource
  interpretation as `A<-B_COL, B<-D_ROW, C<-A_ROW, D<-C_COL`.

The R2 instruction section 12 instead requires:

| Requested role | Release | Borrow |
|---|---|---|
| A/D | column: `(N,N-1)` | row: `(N+1,N)` |
| B/C | row: `(N-1,N)` | column: `(N,N+1)` |

Thus the requested mapping is not an N-parameter extension of the final
archive mapping.  At N=2, for example, final HYP02 `A slot 1` is `1R2C`, while
the new rule requires `2R1C`; final HYP02 `A slot 2` is `2R3C`, while the new
rule requires `3R2C`.  The analyzer ConfigID, candidate validity, PatternID,
and external H=7 analyzer input must therefore change.  Such a bridge cannot
both match final G2X2_RC RTL and use the requested new capacity table.

## Static edge-state proof

The final HYP02 legality relation is:

```text
C.borrow -> A.release
B.borrow -> D.release
A.borrow -> B.release
D.borrow -> C.release
```

For each source-owned shareable edge, the two endpoint action bits reduce to
exactly three legal states:

```text
OWNER    = source does not release; destination does not borrow
BORROWER = source releases; destination borrows
UNUSED   = source releases; destination does not borrow
```

The fourth combination (source retains while destination borrows) is excluded
by the relation.  The four independent relations therefore produce `3^4 = 81`
legal slot tuples.  This proves the abstract three-state edge representation.
It does not erase the archive's role-specific row/column capacity semantics.

The existing focused proof was run:

```bash
make test_p3_synb_hyp02_exact_81path_proof
```

Result:

```text
LEGAL_STATIC_PATHS: 81
DUPLICATE_PATHS: 0
MISSING_LEGAL_PATHS: 0
ILLEGAL_PATHS_INCLUDED: 0
VALIDITY_MAPS_CHECKED: 65536
REFERENCE_STATIC_REPAIRABILITY_MISMATCHES: 0
ANALYZER_CORPUS_CASES: 515
ANALYZER_REFERENCE_MISMATCHES: 0
CPP_GLOBAL_REPAIRABILITY_MISMATCHES: 0
HYP02_RC_STATIC_POLICY_VECTOR_ORACLE_CASES: 1000
G2X2_R_STATIC_POLICY_VECTOR_ORACLE_CASES: 1000
L1X4_R_STATIC_POLICY_VECTOR_ORACLE_CASES: 1000
```

The path order is also archive-defined as `P0_TO_P80_ASCENDING`.  It may be
reused only after the capacity/action identity is resolved; order alone cannot
make the two contradictory Config tables equivalent.

## Resolved authority

The human decision selects final archive compatibility.  The previous conflict
is therefore resolved; the following historical decision text is retained only
for audit provenance.

## Historical conflict record

The earlier fork is retained solely as provenance.  It is closed by the human
decision above: final archive compatibility is the only active authority.

## Required status

```text
SIXCASE_R2_STATUS = PARTIAL: simulator closure and full N=2 preflight passed; N=3/N=4 preflight pending completion
R2_MAPPING_CONFLICT = RESOLVED
AUTHORITATIVE_MAPPING = FINAL_ARCHIVE
R2_SECTION_12_HANDWRITTEN_MAPPING = SUPERSEDED

HYP02_EDGE_STATE_EQUIVALENCE = PASS (abstract four directed edges, three legal states each)
G2X2_RC_PATH_COUNT = 81
G2X2_R_PATH_COUNT = 81: same four archive-directed edges, all ROW
L1X4_R_PATH_COUNT = 27: sparse static simulator and independent enumeration pass
G2X2_RC_PATH_ORDER_REPRODUCTION = PASS for existing P0_TO_P80 archive order; not generalized

G2X2_RC_EARLY_CPP_MAPPING = PASS: hyp02_static_early, archive slot capacity and R,L,RB,B prefix contract
G2X2_RC_GROUP_CPP_MAPPING = PASS: hyp02_static_global, archive P0_TO_P80 static path order
G2X2_R_EARLY_CPP_MAPPING = PASS: static archive graph, all-ROW capacity/action contract
G2X2_R_GROUP_CPP_MAPPING = PASS: static archive graph, all-ROW capacity/action contract, N=2/3/4 smoke
L1X4_R_EARLY_CPP_MAPPING = PASS: sparse action-to-dense-config adapter
L1X4_R_GROUP_CPP_MAPPING = PASS: sparse action-to-dense-config adapter

NEW_CPP_POLICY_IMPLEMENTATIONS = 6: hyp02_static_early, hyp02_static_global, g2x2_r_static_early, g2x2_r_static_global, l1x4_r_static_early, l1x4_r_static_global
ALIASES_REUSED = 0
SMALL_ADAPTERS_ADDED = 3: final-archive bridge; G2X2_R all-row map; L1X4_R sparse action-to-config map

N2_CONFIG_REPRODUCTION = PASS: final archive-derived A/D and B/C slot capacities
N3_CONFIG_REPRODUCTION = PASS: generic N extension of final archive-derived role slots
N4_SUPPORT = G2X2_R capacity smoke PASS; HYP02 RC static hardware contract remains N=2 archive only
PRIOR_RS4_MAX_K = 8
PRIOR_RS4_MAX_PATTERN_COUNT = 70
PRIOR_RS4_MAX_HYBRID_ENTRIES = 24
RS4_SIZING_REAUDITED = YES
RS4_MAX_K = 9
RS4_MAX_PATTERN_COUNT = 126
RS4_PATTERNID_WIDTH = 7 bits for 126 candidate IDs
RS4_MAX_ADDRESS_ENTRIES = 9
RS4_MAX_HYBRID_ENTRIES = 31

RS4_SIZING_BASIS = local-only `4R4C` gives `K=8`, `C(8,4)=70`,
and hybrid entries `4*(4-1)+4*(4-1)=24`; the all-action upper bound includes
borrow `5R4C`, giving `K=9`, `C(9,5)=126`, address entries `9`, and hybrid
entries `5*(4-1)+4*(5-1)=31`.
PATH_COUNT_N_INDEPENDENT = PASS for G2X2_R: 81; L1X4_R: 27

G2X2_RC_EARLY_CPP_VS_HYP02_MISMATCHES = 0 across 1000 independent static-policy oracle vectors; all-valid priority witness PASS
G2X2_RC_GROUP_CPP_VS_HYP02_MISMATCHES = 0 across 515 analyzer-corpus cases and 1000 independent oracle vectors
G2X2_R_EARLY_ORACLE_MISMATCHES = 0 across 1000 independent oracle vectors
G2X2_R_GROUP_ORACLE_MISMATCHES = 0 across 1000 independent oracle vectors
L1X4_R_EARLY_ORACLE_MISMATCHES = 0 across 1000 independent oracle vectors
L1X4_R_GROUP_ORACLE_MISMATCHES = 0 across 1000 independent oracle vectors
SAME_CORPUS = PASS: N=2, F_GROUP=8, 1000 groups, seed=20260923; one corpus ID/hash across local source and six static cases

PREFLIGHT_STATUS = PARTIAL: N=2 all F_GROUP=8..48 points complete; N=3/N=4 sharded runs not yet complete
G2X2_RC_RTL_MODIFIED = NO
G2X2_R_EARLY_RTL = NOT_STARTED
G2X2_R_GROUP_RTL = NOT_STARTED
L1X4_R_EARLY_RTL = NOT_STARTED
L1X4_R_GROUP_RTL = NOT_STARTED
ARCHITECTURAL_CHANGE_FROM_TEMPLATE = NO

G2X2_R_EARLY_AREA = NOT RUN
G2X2_R_GROUP_AREA = NOT RUN
L1X4_R_EARLY_AREA = NOT RUN
L1X4_R_GROUP_AREA = NOT RUN
TIMING_STATUS = NOT RUN

MAIN_RUNNER_REUSED = YES: existing `r3_formal_group_repair_rate.py` via additive `sixcase_static` preset
R3_ANALYSIS_REUSED = YES: existing aggregate output written for preflight only
R3_PLOT_REUSED = YES: existing plot stage completed for preflight only
OLD_CASES_PRESERVED = YES
NEW_PARALLEL_FRAMEWORK_CREATED = NO

FILES_CREATED = docs/dss_execution/SIXCASE_R2_SIMULATOR_AND_RTL_ROLLOUT.md
RTL_COMMITS = NONE
SIMULATOR_COMMITS = 1: Add six-case static simulator closure
NEXT_BLOCKER = complete N=3/N=4 preflight matrix and RTL rollout for four new static policies; final G2X2_RC RTL remains immutable
NEXT_RECOMMENDED_PHASE = resume sharded N=3/N=4 preflight; prepare RTL semantic specs in parallel
```


## 2026-09-23 Fasttrack 1K RCA and G2X2_R_GROUP G0

This section supersedes the earlier partial N=3/N=4 preflight status above. The
completed primary evidence is `tmp/date2026/6case_1k`: N=2/3/4, every
`F_GROUP=8,12,...,48`, 1,000 groups per point, seven policies, and 77 complete
policy sidecars per N. It was checked by group-id alignment; no corpus was
regenerated and no policy was re-executed during paired RCA.

```text
N2_1K_COMPLETE = YES
N3_1K_COMPLETE = YES
N4_1K_COMPLETE = YES
RC_R_SAME_CORPUS = PASS
RC_R_EQUAL_TOTAL_PHYSICAL_SPARES = PASS
RC_STATIC_PATH_COUNT = 81
R_STATIC_PATH_COUNT = 81
IMPLEMENTATION_MISMATCH_FOUND = NO
SIMULATOR_CLEANUP_COMMIT = 816da42
```

Aggregate SHA-256 values are N2 `92e927cbb6822f8fc20f01547320443c61cf213a8f22adc30105c876a7d3821d`,
N3 `7ca1cf50ef23ca48016afe39a3bae4e0a65d9ee02701353bc64666b4642a3987`,
and N4 `9ef00b8718ff5c56a03d155e5446589d89fa80540528075e41baf82bb473679f`.
The re-aggregated paired-outcome SHA-256 values are N2
`8194cdd9cc5873d3c26624570ad05d7dd8fee9cb19b26d1ed5bbe3478a43f1cd`,
N3 `e39326b5aa153e68fcbde195f149e3173d74f67af6779e4c2c19527811e1f3ce`,
and N4 `7ff927008e778ed13a6ef4c9b1c8b5bbb8a3b80741f4f4495ea57f42d717be34`.

### Paired RC-vs-R result

For GROUP at N=2, the `(RC_ONLY,R_ONLY)` counts are F24 `(11,24)`, F28
`(9,22)`, F32 `(18,36)`, and F36 `(11,26)`; their exact two-sided paired
binomial diagnostics are respectively `0.04096`, `0.02945`, `0.01983`, and
`0.02007`. This is an N=2 transition-region R advantage in this corpus, not a
universal row-only ordering: N=3 maximum R-minus-RC is `+1.0 pp` (F44), while
N=4 maximum absolute difference is `0.2 pp` (F40).

The generator defaults are `ROW_ADDRESS_BITS=10`, `COLUMN_ADDRESS_BITS=10`,
`ROW_DOMAIN_SIZE=1024`, and `COLUMN_DOMAIN_SIZE=1024`.
`multinomial_uniform` assigns each group fault with probability 1/4 to every
SA. In mixed spatial mode, a new fault is clustered for 20 percent, shares an
existing row or column for the next 30 percent with exact 1/2 orientation, and
is otherwise independently uniform; duplicate cells are resolved by a
deterministic flat-address scan. No address-space normalization is performed.
Thus `ROW_COLUMN_GENERATION_SYMMETRIC = YES` at this 1024-by-1024 run.
The discordant strata do not show a stable excess of row collisions in R_ONLY:
at N2/F24 R_ONLY has 0.625 row-collision probability versus 0.542 column,
whereas RC_ONLY has 0.477 versus 0.659; the sign also changes at other F
points. The supported conclusion is `RESOURCE_TYPE_PLACEMENT` plus static
capacity/path interaction, not a demonstrated row-address-space bias.

Persisted policy sidecars contain selected ConfigID and PatternID for successful
groups, but do not retain every failed action-valid map, candidate bitmap, or
selected static path. Exact first-failure claims for a single losing instance
therefore cannot be reconstructed from frozen sidecars alone without rerunning
the analyzer, which is prohibited for this RCA.

### G2X2_R_GROUP semantic-spec gate

The immutable mother archive remains
`recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch`.
The derived N=2 target preserves edges `B->A`, `D->B`, `A->C`, `C->D`, all
ROW; its static selector uses the same ordered 81 tuples (`P0_TO_P80`). The
fixed action identities are LOCAL=0, RELEASE=1, BORROW=2,
RELEASE_BORROW=3, mapping for every SA to dense analyzer results
`{0->0, 1->1, 2->2, 3->0}` and external ConfigIDs `{0,4,2,0}`. The three
dense configurations are `(2R,2C)`, `(1R,2C)`, `(3R,2C)`.

| Target module class | G2X2_R_GROUP treatment |
|---|---|
| `dss_v2_params_pkg`, `dss_v2_types_pkg` | IDENTICAL_TO_TEMPLATE at N=2 widths |
| shared analyzer | IDENTICAL_TO_TEMPLATE; no analyzer math change |
| candidate store | WIDTH_ONLY: 12 dense entries x (valid + 4-bit PatternID) = 60 bits; selector aliases RB to local |
| dense-config decoder | CONFIG_TABLE_ONLY: 0/1/2 -> ConfigID 0/4/2 |
| static selector | ACTION_MAPPING_ONLY: 81 tuple/order retained; action 3 reads dense 0 |
| core/top wrapper | WRAPPER_ONLY: collect 12 dense results, then one static decision |

`ARCHITECTURAL_CHANGE = NONE`. There are 4 action slots but 3 analyzer
config classes; LOCAL and RELEASE_BORROW remain distinct selected actions while
sharing a single analyzer result. The N=2 PatternID width is 4 bits (at most 10
candidates for 3R2C); no N=4 K=9 or 126-pattern state is present. The FSM is
IDLE, 12 COLLECT cycles, and one DECIDE cycle, yielding `done` on the 14th
clock edge after an accepted start under the template convention.
