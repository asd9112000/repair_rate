# N3 CA-LIVE contract recovery and provenance archaeology

## 1. Evidence inventory

~~~
ARCHITECTURE_POINT: G2X2 directional, RS=3, CS=3, SHARE_M=1
BASELINE_COMMIT: a2a61b96280275019ab06f9e120c4951466b9777
TARGETS: G2X2_N3_R_GROUP_CA_LIVE; G2X2_N3_RC_GROUP_CA_LIVE
EVIDENCE_MODE: repository archaeology; no architectural inference
RTL_CREATED: NO
DC_RUN: NO
COMMIT: NOT_CREATED
~~~

| Evidence | Directly establishes | Disposition |
| --- | --- | --- |
| H1/H2 RS3-CS3-M1 documents | seven-config RC envelope, actions, K=7, PatternID=6, 17 Hybrid max and 112-bit history minimum | RC sizing/config evidence |
| rtl/dss_v2/rs3cs3m1 | actual historical analyzer, config table, GROUP core/top/store | implementation evidence, subject to limits |
| H3/H4/H5 | historical RS3 GROUP-NoScratch verification and PPA | non-CA background only |
| RTL_MODULE_REUSE_MATRIX.md | RS3 analyzer is REFERENCE_ONLY; runtime nth_pattern has no provenance-backed fixed-mask production replacement | production blocker |
| S1GA2D/A2F/A2G | historical 2x2 retained collector and update-wins generation behavior | distinct interface generation |
| tests/p3_synb_hyp02_exact_81path_proof_test.cpp | separate 2x2 static 81-path action model | not an N3 R RTL contract |

No evidence is transferred across interface generations merely because names or
total bit counts resemble each other.

## 2. N3 RC-GROUP recovered contract

### 2.1 Config and action contract: PROVEN

H1 and dss_v2_rs3cs3m1_config_table.sv prove a versioned, RS3-local table:

| Role | Slot | ID | R,C | Action |
| --- | ---: | --- | --- | --- |
| A/D | 0 | T0 | 3,3 | LOCAL |
| A/D | 1 | T1 | 2,3 | RELEASE_ONLY |
| A/D | 2 | T2 | 3,4 | BORROW_ONLY |
| A/D | 3 | T3 | 2,4 | RELEASE_AND_BORROW |
| B/C | 0 | T0 | 3,3 | LOCAL |
| B/C | 1 | T4 | 3,2 | RELEASE_ONLY |
| B/C | 2 | T5 | 4,3 | BORROW_ONLY |
| B/C | 3 | T6 | 4,2 | RELEASE_AND_BORROW |

Action encoding is {release_required, borrow_required}: 00 LOCAL, 10
RELEASE_ONLY, 01 BORROW_ONLY, 11 RELEASE_AND_BORROW. The m=1 topology proves
four resources and role-specific donor order. Numeric target IDs are local to
rs3cs3_m1 and must not be reused as a global ConfigID interpretation.

~~~
3R3C: C(6,3)=20, PatternID 1..20
2R3C/3R2C: C(5,2 or 3)=10, PatternID 1..10
3R4C/4R3C: C(7,3 or 4)=35, PatternID 1..35
2R4C/4R2C: C(6,2 or 4)=15, PatternID 1..15
MAX_K=7
PATTERN_ID_WIDTH=ceil(log2(35+1))=6
~~~

### 2.2 Analyzer contract: PARTIAL

recam_dss_v2_rs3cs3m1_group_top.sv declares the historical RS3 analyzer bundle:

~~~
pivot_valid_i                                      7
+ pivot_rows_flat_i                    7 x 9  =  63
+ pivot_cols_flat_i                    7 x 5  =  35
+ row_gt1_i..row_gt4_i                 4 x 7  =  28
+ col_gt1_i..col_gt4_i                 4 x 7  =  28
+ hybrid_valid_i                                  17
+ hybrid_pointer_flat_i                17 x 3 =  51
+ hybrid_descriptor_i                             17
+ hybrid_differing_flat_i             17 x 9 = 153
+ conventional_overflow_i                         1
=                                                   400
~~~

This proves: 7 pivot-valid bits; physical row/column widths 9/5; four row and
four column threshold vectors of width 7; 17 Hybrid valid entries; 3-bit pivot
pointer; 1-bit descriptor; 9-bit differing payload; overflow; and a 7x7,
35-candidate, four-class semantic envelope.

It is not a production-complete analyzer: the actual source uses runtime
nth_pattern and RTL_MODULE_REUSE_MATRIX.md marks the module REFERENCE_ONLY
because the fixed-mask production source has no proven repository provenance.

Important non-alias rule: S1GA2D separately derives a historical 400-bit
membership-aware interface as 106 + 14 x 21. That 2x2 historical interface
is not the RS3 400-bit bundle above. Equal total width does not prove an
interface mapping.

~~~
N3_RC_ANALYZER_PAYLOAD: PARTIAL
~~~

### 2.3 Candidate store and persistence: PARTIAL

dss_v2_rs3cs3m1_group_candidate_store.sv proves:

~~~
record = {valid[0], PatternID[5:0]} = 7 bits
store  = 4 SA x 4 semantic slots x 7 = 112 bits
~~~

The historical core writes all sixteen records in COLLECT, then reads them in
ALLOCATE using rank 1,0,3,2:
RELEASE_ONLY -> LOCAL -> RELEASE_AND_BORROW -> BORROW_ONLY. In this exact
non-CA greedy controller, role+slot statically determines action/config/donor
order, so valid plus PatternID suffices for its selection.

The store has reset clear only. Complete collection overwrites all sixteen
locations before historical allocation, but there is no generation tag,
state-update preemption, partial-collection restart, or CA-LIVE clear rule.

~~~
N3_RC_GROUP_CANDIDATE_STORE: PARTIAL
~~~

### 2.4 Selector and decision contract: PARTIAL

The semantic action space is four role slots for each of four SAs (4^4=256
nominal tuples), constrained by the four-resource ledger and directional donor
order. The historical RS3 GROUP representation is not a static selector or
DFS: it collects sixteen evaluations, then performs serial greedy A->B->C->D
allocation. It proves candidate enumeration plus greedy allocation only.

### 2.5 Reconstruction: PARTIAL

The RS3 group top exposes selection diagnostics but no physical repair-address
reconstruction output and no retained pivot/Hybrid state. H2 explicitly says
the 112-bit history is not a reconstruction-storage claim.

S1GA2F/A2G proves 821 retained bits per SA and update/generation coherence for
a different 2x2 historical collector: 10-bit row and column values, an HCV-9
232-bit analyzer view, 12-fault bound, and historical cfg-membership semantics.
That is evidence that ordered retention and generations matter; it does not
prove an RS3 9/5/9, 17-Hybrid retention interface or its exact width.

~~~
N3_RC_RECONSTRUCTION_RETENTION: PARTIAL
N3_RC_RECONSTRUCTION_WIDTH: NOT_DEFINED
~~~

## 3. N3 R-GROUP recovered contract

No synthesizable RS3 row-only GROUP top, analyzer boundary, candidate store,
selector, or reconstruction output was located. The available static-path C++
test is a separate 2x2 model: its kSlotSemantics begins at 2R2C and its
81-path table cannot define an N3 R hardware interface.

| Item | Classification | Source basis |
| --- | --- | --- |
| Analyzer payload / production representation | UNKNOWN | no RS3 R analyzer/top |
| Config universe and numeric IDs | PARTIAL | row-cycle capacity evidence only; no N3 versioned table |
| Action identity | PARTIAL | generic historic actions, no N3 R role table |
| Candidate store | UNKNOWN | no N3 R GROUP source |
| Reconstruction | UNKNOWN | no N3 R retained/reconstruction boundary |
| Selector | UNKNOWN | 2x2 81-path test is not N3 R hardware semantics |
| CA-LIVE schedule | UNKNOWN | no N3 R owner/update/test-done integration |

~~~
N3_R_ANALYZER_PAYLOAD: UNKNOWN
N3_R_GROUP_CANDIDATE_STORE: UNKNOWN
N3_R_RECONSTRUCTION_RETENTION: UNKNOWN
~~~

## 4. Shared CA-LIVE scheduling recovery

S1GA2G proves policy behavior for its own historical 2x2 path: an accepted
update changes retained state atomically, increments fault_generation,
invalidates old analysis, wins against same-edge completion, and permits a
commit only when test_done and analysis/fault generations match. Its controller
advances A->B->C->D without a provisional ledger commit.

It does not mechanically complete N3 CA-LIVE. The historical path consumes a
232-bit HCV-9 view from an 821-bit collector; the RS3 top consumes a different
pre-collected 400-bit bundle and has only start_i. No evidence defines, for
either requested N3 target:

- fault/state-update interface and priority against completion;
- live collector-to-analyzer projection and generation ownership;
- test-done source, group freeze condition, and active-SA schedule;
- restart rule for update during collection or allocation;
- candidate-store epoch/clear behavior and earlier-SA persistence;
- decision trigger or solution-ready registered-output timing.

~~~
N3_R_CA_LIVE_SCHEDULING: UNKNOWN
N3_RC_CA_LIVE_SCHEDULING: PARTIAL
~~~

RC is PARTIAL rather than DERIVABLE_FROM_FROZEN_CA_LIVE_POLICY: joining the
proven historical control rules to the RS3 analyzer/store requires an unproven
collector/projection/retention bridge and restart semantics. That is a new
architectural choice, not mechanical width scaling.

## 5. Exact unresolved inputs

| Field | Why required | Available evidence | Missing input | Derivable without a new architectural choice? |
| --- | --- | --- | --- | --- |
| Production RS3 fixed-mask analyzer | synthesizable CA-LIVE with stable PatternID ordering | RS3 capacity and field arithmetic; reference enumerator | provenance-backed fixed-mask source or frozen production representation plus equivalence contract | NO |
| RC collector -> 400-bit analyzer boundary | turns live faults into the RS3 analyzer input | pre-collected RS3 ports; unrelated HCV-9 collector | exact retained fields, address conversion, thresholds/overflow, generation ownership | NO |
| RC reconstruction retention | final selected PatternID must reconstruct repair addresses | diagnostics plus unrelated 821-bit proof | exact RS3 retained fields, widths, lifetimes and output contract | NO |
| RC restart/freeze/store rules | prevents stale/mixed candidates | historical update-wins control and non-CA full overwrite | update during COLLECT/ALLOCATE, earlier-SA validity, store epoch/clear, decision trigger | NO |
| R analyzer/config | defines legal candidates and stable ConfigID/PatternID meaning | non-target row-cycle capacity hints | RS3 R role table, IDs, payload arithmetic, PatternID order | NO |
| R store/selector | establishes deferred GROUP behavior | non-target 2x2 static model | N3 R record layout, ledger/selector semantics and representation | NO |
| R reconstruction/schedule | yields safe final repair outputs | no target-specific contract | retained state, update/test-done/preemption/output timing | NO |

## 6. Historical-representation caveats

- H3/H4/H5 are RS3 GROUP-NoScratch evidence, not CA-LIVE evidence. Historical
  PPA must not fill missing semantics.
- The RS3 analyzer is semantic/capacity reference only, not an approved
  production implementation.
- The 112-bit store is proven only for complete non-preempted collection and
  historical greedy allocation.
- S1GA2F/A2G is a different 2x2 collector generation, not a direct RS3
  reconstruction-source freeze.
- The 2x2 81-path C++ model cannot supply N3 R selector or ConfigID semantics.

## 7. RTL readiness decision

### N3 R-GROUP

~~~
ANALYZER: UNKNOWN
CONFIG: PARTIAL
ACTION: PARTIAL
CANDIDATE_STORE: UNKNOWN
RECONSTRUCTION: UNKNOWN
SELECTOR: UNKNOWN
CA_LIVE_SCHEDULING: UNKNOWN
READY_FOR_RTL: NO
~~~

### N3 RC-GROUP

~~~
ANALYZER: PARTIAL
CONFIG: PROVEN
ACTION: PROVEN
CANDIDATE_STORE: PARTIAL
RECONSTRUCTION: PARTIAL
SELECTOR: PARTIAL
CA_LIVE_SCHEDULING: PARTIAL
READY_FOR_RTL: NO
~~~

~~~
N3_CONTRACT_EXTRACTED: PARTIAL
NEW_ARCHITECTURAL_CHOICES_REQUIRED: YES
RTL_IMPLEMENTATION_STARTED: NO
VERIFICATION_STARTED: NO
SYNTHESIS_STARTED: NO
~~~

Recovery improved provenance but did not close production semantics. The next
safe action is review and an explicit authorization that freezes the unresolved
interfaces; do not implement N3 RTL from this document alone.

