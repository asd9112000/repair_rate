# P3 SYN-B HF HYP02 — Canonical Static GLOBAL Preflight

## 1. Purpose and provenance

HYP02 tests a Phase4F-style implementation: 16 `{valid, PatternID[3:0]}`
records (80 source bits), followed by a static Directional GLOBAL path network
and deterministic path encoder.  Current SYN-B OPT1 is a secondary functional
reference only; its 160-position valid/release/borrow maps are not HYP02's
implementation contract.  This preflight is deliberately proof-first.  Worktree is
`/home/asd9112000/repair_rate_synb_hyp02`, branch
`opt/p3-synb-hf-hyp02-canonical-static-global`, base
`00fdb15fc6f7ac380397b48f159c734e5e483fa2`.

## 2. Skill and architecture evidence

The readable-Verilog ASIC rules were applied: fixed versus dynamic indexing,
complete decode, widths, state/hold behavior, high-fanout controls, and pruning
risk.  Current SYN-B uses one shared analyzer, seven ConfigIDs, four canonical
capacity classes, and the 160-position valid/release/borrow logical view.
The producer ordering is L,R,B,RB while DFS ordering is R,L,RB,B.

For A/D the action slots are: 0=2R2C/00, 1=1R2C/10, 2=2R3C/01,
3=1R3C/11.  For B/C they are: 0=2R2C/00, 1=2R1C/10,
2=3R2C/01, 3=3R1C/11.  The directional release edges are A_ROW->C,
C_COL->D, D_ROW->B, and B_COL->A; each slot therefore encodes outgoing
release and incoming borrow intent.

## 3. Phase4F context

The confirmed Phase4F 16x5-bit packed record is not a semantic baseline.  Its
one-result-per-slot and greedy GROUP decision cannot establish correctness for
current GLOBAL.

## 4. Theorem preflight finding

The current GLOBAL transition is not determined solely by the pair
`{actual_release, actual_borrow}`.  In
`recam_dss_canonical_global_noscratch_core`, an outstanding release obligation
requires both `explicit_release` (derived from action 1 or 3) and
`actual_release`.  The existing DFS audit contains an explicit
`effectOnlyCounterexample` and distinguishes effect-only collapse from the
accepted explicit-effect collapse.  Therefore any HYP02 path must retain the
full action slot identity, not merely an edge effect pair.

This does not itself disprove a 16-path table: a path may statically retain all
four action slots.  It does disprove the weaker unproven assumption that
release/borrow bits alone define legality.  More importantly, no exhaustive
current analyzer/C++ corpus proof has yet shown P1: a valid larger envelope can
always be replaced by the appropriate reduced envelope while preserving the
candidate universe and chosen local solution.

## 5. Gate decision

P1 config coverage, P2 Pattern resource equivalence, and P3 16-path
normalization are coupled proof obligations.  Existing directed DFS evidence
does not prove P1 over the analyzer's finite state space or a sufficiently
defined exhaustive corpus.  Implementing a static engine now would assume the
very dominance/normalization theorem HYP02 is intended to establish.

```text
CONFIG_COVERAGE_THEOREM: NOT_PROVEN
PATTERN_EQUIVALENCE_THEOREM: NOT_PROVEN
16_PATH_NORMALIZATION_THEOREM: NOT_PROVEN
STATIC_GLOBAL_SEARCH_EQUIVALENT_REPAIRABILITY: NOT_PROVEN
CURRENT_LOGICAL_TUPLES: 256
CANONICAL_STATIC_PATHS: 16 (hypothesis only)
OLD_CANDIDATE_MAP_BITS: 480
NEW_CANDIDATE_RECORD_BITS: 80 (proposal only)
P3_SYNB_HF_HYP02_STATUS: BLOCKED
HYPOTHESIS_VERDICT: NOT_CONFIRMED
REASON: required P1/P2/P3 proofs and static-search oracle are absent; action identity cannot be collapsed to effect bits
FUNCTIONAL_RTL_MODIFIED: NO
NEXT_ACTION: STOP FOR HUMAN REVIEW
```

## 6. Smallest valid next step

Before any RTL change, create a separate theorem/oracle phase that exhaustively
enumerates the defined analyzer candidate relation and compares the current
GLOBAL search with a slot-preserving 16-edge-path oracle.  It must report the
smallest P1/P2/P3 counterexample, or zero counterexamples, before HYP02 can
resume.

## 7. Correction and mandatory human-architecture review record

This section supersedes the older interpretation that treated the current
SYN-B effect-only-collapse counterexample as an HYP02 blocker.  That evidence
only proves that action identity cannot be discarded.  HYP02 retains all four
slot identities, so it is not a blocker.

The `readable-verilog-generator` ASIC reference was applied to the existing
RTL.  Applicable rules: keep packed-record storage and dynamic indexing
reviewable; use complete fixed decode for static paths; make widths and
slot/edge encodings explicit; preserve registered candidate lifetime and
reset/hold behavior; and review high-fanout path-valid and constant-pruning
risk.  No FPGA-specific guidance is used.  In particular,
`rtl/dss_v2/group/dss_v2_group_slot_decode.sv` derives ConfigID/descriptor
combinationally from `{SA role, canonical slot}`.  The proposed store therefore
requires only `{valid, PatternID}`, while a later implementation must retain
the collection/decision timing and reset contract.

`release` means release on the outgoing edge, `borrow` means demand on the
incoming edge, and table bits are `{release,borrow}`.

| SA | Slot | ConfigID | R/C envelope | release | borrow | outgoing edge | incoming edge |
| --- | ---: | ---: | --- | ---: | ---: | --- | --- |
| A | 0 | CFG0 | 2R2C | 0 | 0 | E0 A_ROW→C | E3 B_COL→A |
| A | 1 | CFG4 | 1R2C | 1 | 0 | E0 A_ROW→C | E3 B_COL→A |
| A | 2 | CFG5 | 2R3C | 0 | 1 | E0 A_ROW→C | E3 B_COL→A |
| A | 3 | CFG6 | 1R3C | 1 | 1 | E0 A_ROW→C | E3 B_COL→A |
| B | 0 | CFG0 | 2R2C | 0 | 0 | E3 B_COL→A | E2 D_ROW→B |
| B | 1 | CFG1 | 2R1C | 1 | 0 | E3 B_COL→A | E2 D_ROW→B |
| B | 2 | CFG2 | 3R2C | 0 | 1 | E3 B_COL→A | E2 D_ROW→B |
| B | 3 | CFG3 | 3R1C | 1 | 1 | E3 B_COL→A | E2 D_ROW→B |
| C | 0 | CFG0 | 2R2C | 0 | 0 | E1 C_COL→D | E0 A_ROW→C |
| C | 1 | CFG1 | 2R1C | 1 | 0 | E1 C_COL→D | E0 A_ROW→C |
| C | 2 | CFG2 | 3R2C | 0 | 1 | E1 C_COL→D | E0 A_ROW→C |
| C | 3 | CFG3 | 3R1C | 1 | 1 | E1 C_COL→D | E0 A_ROW→C |
| D | 0 | CFG0 | 2R2C | 0 | 0 | E2 D_ROW→B | E1 C_COL→D |
| D | 1 | CFG4 | 1R2C | 1 | 0 | E2 D_ROW→B | E1 C_COL→D |
| D | 2 | CFG5 | 2R3C | 0 | 1 | E2 D_ROW→B | E1 C_COL→D |
| D | 3 | CFG6 | 1R3C | 1 | 1 | E2 D_ROW→B | E1 C_COL→D |

The exact resource-conservation equations are:

```text
E0: A.release == C.borrow       (A_ROW -> C)
E1: C.release == D.borrow       (C_COL -> D)
E2: D.release == B.borrow       (D_ROW -> B)
E3: B.release == A.borrow       (B_COL -> A)
```

For edge mask `m = {E3,E2,E1,E0}`, the static slot equations are:

```text
A.slot = {E0,E3};  B.slot = {E3,E2};
C.slot = {E1,E0};  D.slot = {E2,E1};
```

Here `00/10/01/11` decode to slots `0/1/2/3`, respectively.  Each of the 16
masks has exactly one static path:

| Mask `{E3E2E1E0}` | A | B | C | D |
| --- | ---: | ---: | ---: | ---: |
| 0000 | 0 | 0 | 0 | 0 |
| 0001 | 1 | 0 | 2 | 0 |
| 0010 | 0 | 0 | 1 | 2 |
| 0011 | 1 | 0 | 3 | 2 |
| 0100 | 0 | 2 | 0 | 1 |
| 0101 | 1 | 2 | 2 | 1 |
| 0110 | 0 | 2 | 1 | 3 |
| 0111 | 1 | 2 | 3 | 3 |
| 1000 | 2 | 1 | 0 | 0 |
| 1001 | 3 | 1 | 2 | 0 |
| 1010 | 2 | 1 | 1 | 2 |
| 1011 | 3 | 1 | 3 | 2 |
| 1100 | 2 | 3 | 0 | 1 |
| 1101 | 3 | 3 | 2 | 1 |
| 1110 | 2 | 3 | 1 | 3 |
| 1111 | 3 | 3 | 3 | 3 |

Slot identity alone determines ConfigID, R/C envelope, donor/borrower
direction, and group-level resource behavior.  PatternID retains the
deterministic local repaired-line solution within an already-valid slot.  It
must not change release/borrow direction, resource envelope, or path legality;
P2 tests that contract.  Different local repaired row/column addresses remain
permitted.

The primary semantic golden is an independent Phase4F-slot-based 256-tuple
oracle, not current SYN-B internals.  Its proof obligations are:

1. P1, for every SA and borrow state: `valid(release=1,borrow=b)` implies
   `valid(release=0,borrow=b)`.
2. P2, valid PatternIDs within one SA/slot have equal group attributes.
3. P3, legal repairable tuples normalize through P1 to exactly one of the 16
   masks, then compare with `path_valid[k]` for repairability only.

```text
P1_GENERALIZED_RELEASE_MONOTONICITY: NOT_PROVEN
P2_PATTERN_WITHIN_SLOT_EQUIVALENCE: NOT_PROVEN
P3_256_TO_16_NORMALIZATION: NOT_PROVEN
RAW_TUPLES: 256
CANONICAL_EDGE_MASKS: 16
UNCOVERED_LEGAL_TUPLES: NOT_MEASURED
STATIC_SEARCH_REPAIRABILITY_MISMATCHES: NOT_MEASURED
FUNCTIONAL_RTL_MODIFIED: NO
NEXT_ACTION: HUMAN REVIEW RECORDED; PROOF/ORACLE TESTS ONLY
```

## 8. Proof/oracle execution record

Proof-only test: `tests/p3_synb_hyp02_phase4f_slot_static_oracle_test.cpp`.
It calls the independent `DirectionalMultiConfigAnalyzer` for an exhaustive
normalized 3x3 incidence corpus: 512 fault bitmaps, four SA roles, and both
required P1 mappings per role, for 4,096 P1 checks.  The corpus is finite
reference evidence, not a claim that coordinate extent is a complete formal
model of every possible fault matrix.

P1 **fails** in that reference evaluation.  There are 60 counterexamples.  The
first/smallest enumeration-order counterexample is:

```text
SA role: A (role 0)
fault bitmap: 115
faults: {(0,0), (0,1), (1,1), (1,2), (2,0)}
release=1 slot: CFG4 / 1R2C / PatternID 3 / valid
release=0 same-borrow slot: CFG0 / 2R2C / PatternID 0 / invalid
```

## 9. Fixed-four-edge exact 81-path GLOBAL proof

This section supersedes only the rejected 16-mask normalization as the HYP02
legality representation.  It preserves the P1 failure, P2 pass, and old
conditional 16-mask result above as historical evidence.  The proof baseline
is commit `8a7f3bdcc8ababffeae37ccb9087e71cba929c92` in the isolated HYP02
worktree.

### 9.1 Architecture contract

The authoritative Directional resource graph is exactly:

```text
A_ROW -> C
D_ROW -> B
B_COL -> A
C_COL -> D
```

The Phase4F candidate-record schema is reused, but its expanded-dual-donor
topology is explicitly not reused:

```text
PHASE4F_STORE_SCHEMA_REUSED: YES
PHASE4F_TOPOLOGY_SEMANTICS_REUSED: NO
STORE: 4 SA x 4 slots x {PatternID[3:0], valid} = 80 bits
```

The frozen role/slot decode remains the table in section 7: A/D use
`CFG0/CFG4/CFG5/CFG6 = 2R2C/1R2C/2R3C/1R3C`; B/C use
`CFG0/CFG1/CFG2/CFG3 = 2R2C/2R1C/3R2C/3R1C`.  A slot's identity supplies its
complete outgoing-release and incoming-borrow envelope.  PatternID is local
repair reconstruction metadata only; P2 remains the supporting evidence for
using one deterministic representative per valid slot.

For tuple `(A_slot, B_slot, C_slot, D_slot)`, physical legality is exactly:

```text
C_borrow_row <= A_release_row     (A_ROW -> C)
B_borrow_row <= D_release_row     (D_ROW -> B)
A_borrow_col <= B_release_col     (B_COL -> A)
D_borrow_col <= C_release_col     (C_COL -> D)
```

There is no alternative donor, donor priority, release map, borrow map, edge
mask, ledger traversal, or DFS in this proposed static legality network.

### 9.2 Exact counting and complete table

Each edge has three independent legal states: `00` (not released/not
borrowed), `10` (released/not borrowed), and `11` (released/borrowed).  The
fourth state, `01`, violates its corresponding inequality.  Consequently the
raw Cartesian space is `4^4 = 256`, and the legal static path space is
`3^4 = 81`.

The generated [complete static-path table](P3_SYNB_HF_HYP02_STATIC_PATH_TABLE.csv)
enumerates all and only those 81 tuples.  Its path IDs are ascending raw tuple
order, and the proof test independently re-enumerates its slot equations.

Ten representative Boolean terms are:

```text
P0  = A_valid[0] & B_valid[0] & C_valid[0] & D_valid[0]
P1  = A_valid[1] & B_valid[0] & C_valid[0] & D_valid[0]
P4  = A_valid[2] & B_valid[1] & C_valid[0] & D_valid[0]
P11 = A_valid[3] & B_valid[1] & C_valid[1] & D_valid[0]
P29 = A_valid[3] & B_valid[3] & C_valid[0] & D_valid[1]
P41 = A_valid[3] & B_valid[3] & C_valid[1] & D_valid[1]
P53 = A_valid[3] & B_valid[3] & C_valid[3] & D_valid[1]
P65 = A_valid[0] & B_valid[1] & C_valid[1] & D_valid[3]
P74 = A_valid[3] & B_valid[3] & C_valid[1] & D_valid[3]
P80 = A_valid[3] & B_valid[3] & C_valid[3] & D_valid[3]

group_repairable = P0 | P1 | ... | P80
```

### 9.3 Exhaustive proof and real-corpus cross-check

`tests/p3_synb_hyp02_exact_81path_proof_test.cpp` is a proof/test-only asset;
it does not alter functional RTL.  It first enumerates the 256 tuples with the
four fixed implications, then exhausts every 16-bit slot-validity map.  Its
independent reference tests all 256 tuples; its static model tests only the 81
precomputed terms.  No release-monotonicity assumption is used, so P1's known
failure is not a precondition.

The analyzer-generated corpus comprises empty/local operation, the previous
P1 counterexample, the established Directional GLOBAL witness, and 512
deterministic random fault groups.  It observed valid static paths covering
local, row-borrow, column-borrow, simultaneous-borrow, release-only, and
four-edge-sharing categories.  For each generated state, the static engine is
compared both with its 256-tuple reference and with
`DirectionalV2GroupGlobalCanonical`, the authoritative fixed-four-edge C++
GLOBAL implementation using `PhysicalResourceLedger`.

```text
TEST_TARGET: make test_p3_synb_hyp02_exact_81path_proof
RAW_CONFIG_TUPLES: 256
LEGAL_STATIC_PATHS: 81
PATH_ROWS: 81
DUPLICATE_PATHS: 0
MISSING_LEGAL_PATHS: 0
ILLEGAL_PATHS_INCLUDED: 0
VALIDITY_MAPS_CHECKED: 65536
REFERENCE_STATIC_REPAIRABILITY_MISMATCHES: 0
ANALYZER_CORPUS_CASES: 515
ANALYZER_REFERENCE_MISMATCHES: 0
CPP_GLOBAL_REPAIRABILITY_MISMATCHES: 0
CPP_GLOBAL_CROSSCHECK: PASS
P1_RELEASE_MONOTONICITY: FAIL__NOT_REQUIRED
P2_PATTERN_WITHIN_SLOT_EQUIVALENCE: PASS
```

### 9.4 Bit-level hardware walkthrough

The established Directional GLOBAL witness produces these slot-valid bits in
`A/B/C/D` order: `0100/1111/1111/1111`.  The first valid table term is path 4:

```text
path_valid[4] = A_valid[2] & B_valid[1] & C_valid[0] & D_valid[0]
```

The selected slots and reconstructed record contents are:

| SA | slot | ConfigID | record `{PatternID,valid}` |
| --- | ---: | ---: | --- |
| A | 2 | CFG5 | `{4'd5, 1'b1}` |
| B | 1 | CFG1 | `{4'd1, 1'b1}` |
| C | 0 | CFG0 | `{4'd1, 1'b1}` |
| D | 0 | CFG0 | `{4'd1, 1'b1}` |

For Phase4F-style packed storage, record `{SA,slot}` occupies
`store_q[5*(SA*4+slot) +: 5]`, with valid at bit 0 and PatternID at bits 4:1.
No donor identity is stored: each borrower has exactly one allowed donor in the
fixed graph, and the selected slot already determines whether that edge is
released or borrowed.

### 9.5 Structural hardware review; no RTL implementation

```text
registered candidate state: 16 x 5 = 80 bits
path terms:                 81 four-input valid AND terms
group reduction:            conceptual 81-input OR
naive path ID width:         ceil(log2(81)) = 7 bits
```

Each valid bit is referenced by `18/36/9/18` terms for slots `0/1/2/3`,
respectively, for every SA.  This is a visible fanout/timing consideration;
slot 1 has the highest direct fanout.  The network has considerable shared
Boolean structure because every term is a conjunction of edge-compatible
release/borrow states.  Synthesis may factor those cones, but no manual
semantic reduction is assumed here.

The preferred output representation for a future implementation is four
selected-slot fields `A_selected_slot`, `B_selected_slot`, `C_selected_slot`,
and `D_selected_slot`, each two bits.  It avoids carrying a generic seven-bit
path ID into four independent PatternID selectors.  A path-priority encoder
would still be required once a selection objective is approved; it must choose
among valid legal paths without changing repairability.  Possible objectives
remain current C++ GLOBAL ordering, minimum borrow count, minimum release
count, existing Config priority, or ascending path ID.  They can select
different successful tuples, so none is selected by this proof.

ASIC rule application: the review used fixed structural decode instead of a
runtime donor search; made slot/record widths explicit; treated the 80-bit
packed record's variable-base historical Phase4F write as an implementation
choice rather than free syntax; reviewed path-valid fanout, mux/encoder cost,
constant propagation, and the retained-store reset/hold boundary.  No FPGA
guidance or synthesis run is claimed.

```text
PATH_SELECTION_OBJECTIVE: UNFROZEN__HUMAN_REVIEW_REQUIRED
RUNTIME_RELEASE_MAP_REQUIRED: NO
RUNTIME_BORROW_MAP_REQUIRED: NO
RUNTIME_EDGE_MASK_REQUIRED: NO
RUNTIME_DONOR_SEARCH_REQUIRED: NO
RUNTIME_DFS_REQUIRED: NO
RUNTIME_RESOURCE_LEDGER_SEARCH_REQUIRED: NO
FUNCTIONAL_RTL_MODIFIED: NO
SYNTHESIS_RUN: NO
NEXT_ACTION: STOP FOR HUMAN ARCHITECTURE REVIEW
```

This is directly a failure of the requested generalized P1 mapping, not an
inference from current SYN-B `actual_release` or `actual_borrow` storage.
The cause inside the independent analyzer/solver candidate relation has not
been changed or assumed; it requires separate root-cause review before any
architecture relaxation is considered.

P2 passed its actual storage-boundary check over 21,220 valid PatternID
observations in the same corpus.  Every observed PatternID is a local candidate
inside one fixed SA/slot descriptor.  ConfigID, R/C envelope, release/borrow
identity, and directional donor/borrower edges therefore remain constant across
PatternIDs; only local repaired addresses may differ.

P3 was executed as the requested independent 256-tuple theorem over the full
abstract P1-monotonic validity domain (6,561 validity maps).  It does not read
current SYN-B candidate maps or DFS fields.  The tuple oracle derives edge
states solely from slot identity, rejects `release=0, borrow=1`, normalizes
`release=1, borrow=0` by clearing the donor release while retaining borrower
state, and compares its repairability with all 16 fixed paths.  It found zero
uncovered legal tuples, zero repairability mismatches, and zero illegal static
resource assignments.  This is a **conditional** P3 result because the real
analyzer violates P1.

```text
TEST_TARGET: make test_p3_synb_hyp02_phase4f_slot_static_oracle
TEST_BUILD: PASS (new proof target compiled)
TEST_EXIT: FAIL AS EXPECTED (P1 counterexamples cause nonzero exit)
P1_GENERALIZED_RELEASE_MONOTONICITY: FAIL
P1_ANALYZER_CORPUS_CASES: 4096
P1_COUNTEREXAMPLES: 60
P1_SMALLEST_COUNTEREXAMPLE_ROLE: A
P1_SMALLEST_COUNTEREXAMPLE_BITMAP: 115
P1_SMALLEST_COUNTEREXAMPLE_FAULTS: {(0,0),(0,1),(1,1),(1,2),(2,0)}
P1_SMALLEST_COUNTEREXAMPLE_RELEASED_CONFIG: CFG4 PatternID 3
P1_SMALLEST_COUNTEREXAMPLE_UNRELEASED_CONFIG: CFG0 PatternID 0
P2_PATTERN_WITHIN_SLOT_EQUIVALENCE: PASS
P2_VALID_PATTERN_OBSERVATIONS: 21220
P3_256_TO_16_NORMALIZATION: CONDITIONAL_PASS__P1_FAILED
RAW_TUPLES: 256
CANONICAL_EDGE_MASKS: 16
P3_MONOTONIC_VALIDITY_MAPS: 6561
UNCOVERED_LEGAL_TUPLES: 0
STATIC_SEARCH_REPAIRABILITY_MISMATCHES: 0
ILLEGAL_RESOURCE_ASSIGNMENTS: 0
FUNCTIONAL_RTL_MODIFIED: NO
SYNTHESIS_RUN: NO
P3_SYNB_HF_HYP02_STATUS: BLOCKED__P1_REFERENCE_COUNTEREXAMPLE
NEXT_ACTION: STOP FOR HUMAN REVIEW
```

## 10. HYP02-C fixed-priority RTL implementation and code review

This section supersedes the older `UNFROZEN`/`BLOCKED` implementation status
above.  The human architecture owner subsequently froze `P0 > P1 > ... > P80`
as the HYP02 selection objective.  The exact-81-path proof commit
`fbdb5c7f0b1af30891f94a3c50aa56be9dfe74a8` is the implementation base.

### 10.1 Implemented hierarchy and state

```text
recam_dss_hyp02_static_global_top
  recam_shared_config_analyzer                 existing, one shared instance
  recam_dss_hyp02_static_global_core           new control/commit boundary
    dss_v2_group_candidate_store               existing Phase4F 80-bit store
    dss_v2_group_slot_decode                   existing collection ConfigID decode
    recam_dss_hyp02_static_selector            new fixed 81-path selector
```

The only persistent candidate representation is existing
`store_q[79:0]`: four SA banks × four slots × `{PatternID[3:0], valid}`.
Record `{SA,slot}` starts at bit `5*(SA*4+slot)`; bit 0 is valid and bits
4:1 are PatternID.  ConfigID, release direction, and borrow direction are
encoded by physical `{SA,slot}` identity and are not stored per record.

The core owns only `state_q[1:0]`, `collect_sa_q[1:0]`, and
`collect_slot_q[1:0]` in addition to registered final-result outputs.  Its
cycle sequence is `IDLE -> COLLECT` (16 writes, A0 through D3) `-> DECIDE`
(one combinational selector result sampled and atomically published) `-> IDLE`.
There is no path-ID register, path ROM, DFS state, resource-ledger traversal,
or extra decision pipeline stage.  Reset is synchronous active-low, matching
the reused candidate-store reset convention; reset clears state and all result
registers, and `done_o` is a one-cycle registered pulse.

### 10.2 Functional RTL block review

| File / module / line range | Purpose and inferred hardware | Dynamic behavior |
| --- | --- | --- |
| `rtl/dss_hyp02/recam_dss_hyp02_static_selector.sv`, `recam_dss_hyp02_static_selector`, 21--63 | Fixed record-bit extraction: 16 valid bits and four 4×4-bit PatternID banks from the registered 80-bit image. | None; every bit select is constant. |
| Same file, 65--145 | Exactly 81 fixed four-valid-bit AND equations, in the committed ascending-path-table order. | None; no runtime tuple/index construction. |
| Same file, 147--723 | `priority case (1'b1)` scans `path_valid[0]` through `[80]`, directly emits four 2-bit selected slots, and defaults safely to no selection. | Fixed priority only; no `casex`, score, comparator, or path ID. |
| Same file, 725--754 | Four role-specific 2-bit-to-3-bit ConfigID decoders. | None; complete plain `case` decode with defaults. |
| Same file, 756--785 | Four independent 4-to-1, 4-bit PatternID muxes selected after slot selection. | Four 2-bit mux selects; no 81-to-1 PatternID mux. |
| `rtl/dss_hyp02/recam_dss_hyp02_static_global_core.sv`, `recam_dss_hyp02_static_global_core`, 31--101 | 2-bit collection state/counters, reused registered store, static selector instance, and collection-time slot ConfigID decode. | The reused store has a variable-base 5-bit write and read implementation; HYP02 adds no dynamic selector indexing or variable path access. |
| Same file, 103--119 | Compatibility diagnostic fields derived only from selected slots.  They are not input state and do not participate in legality, feasibility, or selection. | No donor search, map, or ledger traversal; donor identity is a fixed constant encoding of the four frozen edges. |
| Same file, 121--203 | Synchronous `IDLE/COLLECT/DECIDE` control and one atomic final-output commit. | Fixed 16-cycle collection plus one decision cycle; no iterative GLOBAL search. |
| `rtl/dss_hyp02/recam_dss_hyp02_static_global_top.sv`, `recam_dss_hyp02_static_global_top`, 40--98 | Existing shared analyzer feeds one candidate per `{SA,slot}` to the new core; public outputs preserve the selected-result boundary. | No topology helper, feasibility helper, or expanded-dual-donor implementation is instantiated. |

The four final compatibility fields `release_flat_o`, `borrow_flat_o`,
`selected_donor_flat_o`, and `ledger_released_borrower_o` are output metadata
derived from the already-selected static slots so the selected-result boundary
remains inspectable.  They are neither registered candidate maps nor a runtime
legality mechanism.  In particular, `selector_valid` depends solely on the 81
fixed valid-bit terms.

### 10.3 ASIC RTL-rule application

The `readable-verilog-generator` written guidance, ASIC reference, dispatcher,
and permanent project rulebook were applied manually.  Relevant rules and
their evidence are:

1. **Confirmed reset/latency/interface contract.**  The core keeps the
   registered Phase4F candidate lifetime and uses an explicit synchronous
   active-low reset; it separates 16 collection writes from the decision edge.
2. **Complete combinational assignment and case defaults.**  Selector outputs
   have safe defaults before `priority case`; every ConfigID and PatternID
   decode uses a plain complete `case` with a `default`.  There is no `casex`.
3. **Fixed structure instead of dynamic indexing where architecture is static.**
   Valid extraction, all 81 path terms, and all slot mappings use constant
   bit/index literals.  This expresses fixed AND terms, a fixed priority
   encoder, four 4-to-1 muxes, and small static decoders rather than a DFS or
   dynamic packed-vector candidate-map scan.
4. **Width and signedness review.**  Store image is 80 bits, records are five
   bits, slots/counters are two bits, ConfigIDs are three bits, PatternIDs are
   four bits, and no signed arithmetic or runtime integer datapath exists.
5. **Sequential safety.**  The only clocked process uses nonblocking
   assignments and no gated clock.  `done_o` is deasserted by default each
   clock and asserted only at the terminal decision edge.
6. **Fanout and pruning review.**  Valid-bit fanout is structurally
   slot0=18, slot1=36, slot2=9, slot3=18 for each SA.  The selector's likely
   critical cone is `store_q valid FF -> four-input path AND -> fixed-priority
   case -> selected slot -> PatternID mux/Config decode`.  No timing or area
   claim is made without synthesis.  Pattern payloads do not enter path terms,
   avoiding accidental PatternID-dependent legality.

### 10.4 Functional verification record

`make test_p3_synb_hyp02_c_static_global` compiles and executes selector,
core, and analyzer-integrated top simulations with Verilator.  The selector
test includes the requested P0/P4/P29/P74 pair-priority cases, only-P80, and
no-path, then exhausts every 16-bit slot-validity map against an independent
lowest-valid-static-path reference.  The core test checks P0, P1, P4, P11,
P29, P41, P53, P65, P74, P80, and no path through the registered collection
boundary.  The top test checks an all-local analyzer case and overflow
rejection.

The existing proof executable was rerun only as a post-implementation
regression, not as a replacement proof phase:

```text
RTL_VALIDITY_MAPS_CHECKED: 65536
PRIORITY_DIRECTED_CASES: 6
PRIORITY_SELECTION_MISMATCHES: 0
SELECTED_PATH_LEGALITY_ERRORS: 0
CONFIG_DECODE_ERRORS: 0
PATTERN_READ_ERRORS: 0
CORE_DIRECTED_PATHS_CHECKED: 10
CORE_EMPTY_MAP_CHECKED: 1
CORE_ATOMIC_COMMIT_ERRORS: 0
TOP_ALL_LOCAL_SMOKE: PASS
TOP_OVERFLOW_REJECTION: PASS

RAW_CONFIG_TUPLES: 256
LEGAL_STATIC_PATHS: 81
VALIDITY_MAPS_CHECKED: 65536
REFERENCE_STATIC_REPAIRABILITY_MISMATCHES: 0
ANALYZER_CORPUS_CASES: 515
ANALYZER_REFERENCE_MISMATCHES: 0
CPP_GLOBAL_REPAIRABILITY_MISMATCHES: 0
```

The skill's strict generated-deliverable CLI was also invoked on the selector
with reports directed to `/tmp`.  It could not initialize under the known
system Python 3.8 incompatibility (`dict[str, ...]` type annotation), so it is
recorded as toolchain-blocked rather than repaired.  Verilator compilation and
execution above are the available static/toolchain evidence; no synthesis was
run.

### 10.5 HYP02-C closure

```text
PLANNED_ARCHITECTURE: 80-bit registered store -> 81 fixed valid terms -> fixed priority -> 8 selected-slot bits -> static Config decode and four Pattern muxes
IMPLEMENTED_ARCHITECTURE: MATCHES_PLANNED_ARCHITECTURE
ARCHITECTURE_CONFORMANCE: PASS
DEVIATION: NONE

P3_SYNB_HF_HYP02_C_STATUS: COMPLETE
BASE_COMMIT: fbdb5c7f0b1af30891f94a3c50aa56be9dfe74a8
VERILOG_SKILL_USED: YES
AUTHORITATIVE_DIRECTIONAL: FIXED_FOUR_EDGE
FUNCTIONAL_RTL_MODIFIED: YES
SYNTHESIS_RUN: NO
CANDIDATE_STORE_BITS: 80
CANDIDATE_STORE_REGISTERED: YES
LEGAL_STATIC_PATHS: 81
PATH_EQUATIONS_COMBINATIONAL: YES
PATH_PRIORITY: P0_TO_P80_ASCENDING
PRIORITY_IMPLEMENTATION: priority case (1'b1)
DYNAMIC_SCORE_COMPARISON: NO
BORROW_COUNT_COMPARISON: NO
RELEASE_COUNT_COMPARISON: NO
PATH_ID_REQUIRED: NO
SELECTED_SLOT_BITS: 8
CONFIG_DECODE: COMBINATIONAL
CONFIGID_EXTRA_STORAGE: 0
PATTERN_MUX_COUNT: 4
PATTERN_MUX_INPUTS_PER_SA: 4
PATTERN_MUX_WIDTH: 4
RUNTIME_RELEASE_MAP: NO
RUNTIME_BORROW_MAP: NO
RUNTIME_EDGE_MASK: NO
RUNTIME_DONOR_SEARCH: NO
RUNTIME_RESOURCE_LEDGER_SEARCH: NO
RUNTIME_DFS: NO
VALID_BIT_FANOUT: slot0=18 slot1=36 slot2=9 slot3=18
EXPECTED_CRITICAL_CONE: store valid FF -> path ANDs -> fixed priority -> selected slots -> Pattern mux/Config decode
HUMAN_CODE_REVIEW_COMPLETE: YES
SYNTHESIS_AUTHORIZED: NO
NEXT_ACTION: STOP FOR HUMAN RTL REVIEW BEFORE SYNTHESIS
```
