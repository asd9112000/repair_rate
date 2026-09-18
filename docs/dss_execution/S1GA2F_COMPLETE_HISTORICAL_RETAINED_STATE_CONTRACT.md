# S1G-A2F — Complete Historical Retained-State Contract Re-Derivation

## 1. Executive conclusion

For the frozen historical collector defaults (`ROW_W=10`, `COL_W=10`,
`MAX_FAULTS=12`, `MAX_K=5`, 14 Hybrid slots, 12 reuse slots), the minimum
proven complete collector-semantic representation is **821 bits per SA** and
**3,284 bits for four SAs**. It excludes overlap control, ownership,
generation IDs, scheduler state, commit state, and debug state.

The result is based on the source transition model, three encoder/decoders,
post-Must projection, and an independent historical candidate evaluator. It
has 0 current-view, next-transition, and candidate-result mismatches. No
production RTL was modified.

The 821-bit number is parameter-frozen. If fault or storage capacities change,
sticky overflow must be retained until a new proof closes it.

## 2. Starting 232-bit analyzer contract

```text
POST_MUST_ANALYZER_INPUT_BITS = 232
POST_MUST_HYBRID_CAPACITY     = 9
HISTORICAL_PHYSICAL_HYBRID_CAPACITY = 14
```

The 232-bit boundary is a Config-specific post-Must view, not a collector
snapshot. The retained contract preserves the full 14-slot H order and all
state needed to reconstruct this view after a later fault. The proof compares
the 232-bit field serialization, stable projected H order, Must vectors, and
the source `config_analyzer.sv` candidate result after decode.

The serializer applies the frozen V2 consumer field widths. The retained
record itself keeps historical 10-bit rows and 10-bit columns; this phase does
not add or alter a source-to-V2 address adapter.

## 3. Historical collector state inventory

The audited source is `rtl/dss_2x2/analyzer/shared_fault_collector.sv` and its
instantiated stores. `CURRENT` is the post-Must view; `NEXT` is a future
accepted-fault transition.

| Field | Width × multiplicity | Update / order | Need | Result |
| --- | --- | --- | --- | --- |
| Fault count | 4 × 1 | increment every accepted fault | NEXT | derived from row-count sum |
| Pivot address pairs | 20 × 5 | append P0→P4 | CURRENT + NEXT | store |
| Pivot valid / occupancy | 5 / 3 | contiguous valid prefix | CURRENT + NEXT | store count; derive valid |
| Config Pivot valid | 35 wire bits | `valid && p < cfg_k` | CURRENT | derived |
| Hybrid H0..H13 valid/order | 14 / 4 source | contiguous append prefix | CURRENT + NEXT | H0..H10 store; H11..H13 are frozen-unreachable |
| Hybrid row/column | 20 × 14 | append on relation | CURRENT | compact to pointer+descriptor+differing |
| Hybrid pointer/descriptor | 3+1 × 14 | first pivot, row before col | CURRENT | compact form stores them; raw form derives them |
| Hybrid cfg mask | 7 × 14 | `pointer < cfg_k` | CURRENT | derived |
| Hybrid fault reference | 4 × 14 | written only | none | not required; output is unconnected |
| Row counter table | 15 × 12 | increment or first-free append | CURRENT + NEXT | store ordered address/count |
| Column counter table | 15 × 12 | increment or first-free append | CURRENT + NEXT | store ordered address/count |
| RowMust / ColMust | 35 + 35 wire bits | combinational decode | CURRENT | derived from pivots+counters |
| CAM-reuse payload | 27 × 12 direct form | append ordered | decode + NEXT capacity | store payload/order; derive valid/cfg |
| CAM-reuse occupancy/full | 4 / wire | contiguous append prefix | NEXT | store count; derive full |
| Counter/Hybrid/reuse overflow | 3 sticky bits | set on write-full | CURRENT/status | frozen-default zero, derived only in scope |
| Candidate dictionaries/Config dead | combinational scratch | rebuilt per evaluation | CURRENT only | implementation cache only |

Invalid payload RAM bits are not semantic: every source consumer gates them
with valid. Canonical decode assigns zero to invalid payload slots.

## 4. Current-view vs future-transition dependencies

Pivots, H ordering/count and metadata, plus the two counter tables, affect the
current view. Pivots, append counts, and counter tables also determine the
next relation, next append slot, full condition, and readiness. Reuse payload
does not enter the post-Must analyzer, but is historical decode state and its
count controls later allocation; it remains retained state.

Must is current-view state only. Its underlying counters are both current-view
and next-transition state. A field is never dropped merely because the 232-bit
view does not currently consume it.

## 5. Pivot-state requirements

Pivot order is mandatory. The collector scans P0 through P4, takes the first
match, and resolves a same-Pivot row match before a column match. An unordered
set of pivot addresses can therefore change a future pointer, descriptor, cfg
membership, Must index, H order, and PatternID. The selected representation
stores `pivot_count` and P0..P4 in source order; `pivot_valid[p]` derives as
`p < pivot_count`.

## 6. 14-entry Hybrid state requirements

The source is a 14-entry H0→H13 physical store. It is not compacted to nine;
nine is solely post-Must analyzer capacity. H index is
insertion order, and the projector consumes it in ascending order. F-CE5
reverses two otherwise equal logical entries and observes a different
post-Must sequence.

A legal H entry shares one coordinate with its selected Pivot. The compact
payload is `pointer[2:0] + descriptor + differing_address[9:0]`. For a row
descriptor, reconstruct `(pivot_row[pointer], differing_col)`; for a column
descriptor, reconstruct `(differing_row, pivot_col[pointer])`. The first
accepted fault necessarily creates a Pivot; therefore at most 11 later
accepted faults can append H records. Under frozen `MAX_FAULTS=12`, H11..H13
can never be valid and are omitted only from the selected canonical encoding.

## 7. cfg_valid proof

For every legal Hybrid write:

```text
hybrid_cfg_valid[H][C] = pointer < cfg_k(C)
cfg_k(C0..C6) = [4,3,5,4,3,5,4]
```

The proof regenerates this exact mask from pointer for every decoded state,
including pointer-3 and pointer-4 membership holes. No replayed raw faults
are required.

Reuse cfg has a separate proof. A related reuse entry has
`pointer >= cfg_k(C)`; an unrelated reuse entry is possible only after all
five Pivot slots are full and hence is valid for all Configs. A later Pivot
cannot displace the earlier source relation in the ascending scan. Thus reuse
cfg also derives from ordered Pivots and retained reuse address.

## 8. Pointer proof

Given a full raw H address pair, ordered Pivots, and the source row-before-
column scan, pointer is pure derivation. The compact representation stores the
pointer because it removes the redundant common address coordinate. This is a
packing trade-off, not an assumed semantic field.

## 9. Descriptor proof

Descriptor similarly derives from full raw addresses and ordered Pivots: row
for the first row match, otherwise column for the first column match. The
compact form must retain it because the differing coordinate otherwise has no
dimension. It is semantically derivable but required by this compact code.

## 10. RowMust / ColMust proof

`config_must_view.sv` is exactly:

```text
RowMust[C][p] = row_count(pivot[p].row) > cfg_cols(C)
ColMust[C][p] = col_count(pivot[p].col) > cfg_rows(C)
```

The proof recomputes it after every decode and next transition. F-CE3 shows
that identical Pivot addresses can have different Must values due to counter
state. Therefore Must is derived only because both counter tables are kept.

## 11. Counter derivability

| Field | Classification | Exact derivation |
| --- | --- | --- |
| Pivot valid | derived | `p < pivot_count` |
| Hybrid valid | derived | `h < hybrid_count` |
| Row/column dictionary occupancy | derived | number of positive-count prefix entries |
| Per-Config visible count | derived | post-Must projector scan |
| Per-Config dictionary occupancy | cache only | analyzer rebuilds it |
| CAM-reuse valid | derived | `r < reuse_count` |
| Fault count | derived | `sum(row_counter_count)` |
| Storage write pointer | derived | corresponding append count |

Counter count zero marks invalid in canonical packing. Legal source entries
are positive, stores never delete, and first-free allocation makes them a
prefix. Counter address/count mappings themselves must be retained.

## 12. Full / overflow semantics

Physical full derives from the relevant append count. Config dictionary full
and Config infeasibility are combinational analyzer results, not sticky
collector state.

At frozen defaults, the externally relevant three overflow flags cannot set:
at most 12 unique row/column entries exist before ready deasserts; the first
fault is a Pivot so at most 11 Hybrid writes fit in 14 slots; and at most 11
reuse writes fit in 12 slots. A generic 16-fault probe reaches Hybrid full,
sets sticky overflow, and verifies Candidate FULL retains it. Therefore
overflow is derived only under this exact frozen parameter set.

## 13. CAM-reuse state

CAM reuse is an independent 12-entry store, not a Hybrid alias. It is hidden
from the current post-Must analyzer but remains historical selected-decode and
future-capacity state. The old 264-bit payload is superseded.

Direct physical representation is 340 bits:
`valid12 + occupancy4 + 12×(row10+col10+cfg7)`. The proven compact form is
224 bits: `reuse_count4 + 11×(row10+col10)`. The twelfth source reuse slot is
unreachable for the same first-fault argument. Validity and cfg are derived by
the rules in section 7.

## 14. State-aliasing analysis

| Case | Outcome |
| --- | --- |
| F-CE1 hidden Hybrid cfg | no legal alias; pointer determines mask |
| F-CE2 pointer | derives from raw pair+ordered Pivots; compact form retains pointer |
| F-CE3 Must | counters are necessary; pivot addresses alone alias Must |
| F-CE4 overflow | frozen flags are zero; generic flag remains sticky |
| F-CE5 H ordering | mandatory; reverse order changes projection |
| F-CE6 reuse | same 232 view can hide different reuse payload; payload/order store, cfg derives |

No Candidate MINIMAL legal-state alias was found.

## 15. Current-view equivalence

For every checked Config/state pair:

```text
project(decode(encode(S))) == project(S)
```

The comparison includes stable H-order, Must and the frozen 232-bit boundary
serialization. Candidate-result checking adds the historical candidate bitmap,
lowest PatternID and matrix result. Current-view equivalence is `PASS` with 0
mismatches.

## 16. Next-fault transition equivalence

For each sampled legal or blocked next fault:

```text
transition(decode(encode(S)), F) == transition(S, F)
```

The comparison covers Pivots, Hybrid payload/metadata/cfg, reuse state,
counters, fault count, Must, overflow, the next 232-bit view, and candidate
result. It is `PASS` with 0 transition and candidate mismatches.

## 17. Candidate retained-state contracts

| Candidate | Bits | Contents / result |
| --- | ---: | --- |
| FULL | 1,337 | direct valid/address/metadata/cfg payload, physical counts, counters, fault count, Must snapshot and overflow; PASS |
| DERIVED-A | 1,253 | removes Must/full caches and frozen-zero overflow but retains raw Hybrid coordinates/cfg; PASS |
| MINIMAL | 821 | compact Pivots, 11 reachable compact H entries, 11 reachable reuse row/column payloads and both counter maps; PASS and selected |

FULL and DERIVED-A retain all allocated source positions. MINIMAL proves the
source-valid prefixes are bounded by eleven and omits only unreachable tails;
it does not reinterpret a nine-slot analyzer as collector storage.

## 18. Bit accounting

```text
pivot_count + 5×(row10+col10)                         3 + 100 = 103
hybrid_count + 11×(pointer3+descriptor1+differing10)   4 + 154 = 158
reuse_count + 11×(row10+col10)                         4 + 220 = 224
12×(row address10+count4)                                        = 168
12×(col address10+count4)                                        = 168
                                                               --------
MINIMUM_PROVEN_RETAINED_STATE_BITS_PER_SA                         821
4 SA                                                             3284
```

## 19. Generation atomicity

The atomic record is all selected collector state:

```text
pivot_count+P0..P4
+ hybrid_count+H0..H10 (H11..H13 unreachable)
+ reuse_count+R0..R10 (R11 unreachable)
+ row_counter[0..11]
+ col_counter[0..11]
```

An accepted fault updates both counters and conditionally an append block in
one coherent generation. Must, cfg tags, valid vectors, full flags, fault
count and the post-Must view derive only from that coherent record. This does
not include separately required overlap-control generation state.

## 20. Canonical packing

LSB-first proposal; no RTL implementation is authorized here.

| Bits | Width | Meaning |
| --- | ---: | --- |
| `[2:0]` | 3 | `pivot_count` |
| `[102:3]` | 100 | P0..P4, each row then column, source order |
| `[106:103]` | 4 | `hybrid_count` |
| `[260:107]` | 154 | H0..H10: pointer, descriptor, differing address, H order |
| `[264:261]` | 4 | `reuse_count` |
| `[484:265]` | 220 | R0..R10: row then column, insertion order |
| `[652:485]` | 168 | row counters: count then address; zero count is invalid |
| `[820:653]` | 168 | column counters: count then address; zero count is invalid |

## 21. Superseded contract audit

```text
204-bit: analyzer-ready only; lacks physical H order, counters and reuse.
468-bit: reuse extension but lacks complete membership/capacity/counter state.
9-entry: correct post-Must analyzer capacity, not future-fault-safe collector state.
```

None remains authoritative retained collector state.

## 22. Recommended retained-state architecture

Select `STATE_OPTION_3: proven compact historical representation` (821 bits).
It preserves semantic correctness first, retains 14-slot order, avoids raw
fault replay, and eliminates only fields with direct reconstruction proofs.
No timing or area conclusion is implied.

## 23. Remaining blockers

There is no frozen-default retained-state semantic blocker. A changed capacity
or fault bound reopens overflow derivation. This phase did not implement a
retained bank, scheduler, address adapter, synthesis, device work, or S1G-B.

## 24. Recommended next phase

After human review and explicit authorization only:

```text
S1G-A2G — Complete Historical Retained-State RTL Implementation and
Generation-Coherence Proof
```

## Final status

```text
S1GA2F_STATUS:
COMPLETE

POST_MUST_ANALYZER_INPUT_BITS:
232

POST_MUST_HYBRID_CAPACITY:
9

HISTORICAL_PHYSICAL_HYBRID_CAPACITY:
14

HISTORICAL_STATE_INVENTORY_COMPLETE:
YES

CURRENT_VIEW_DEPENDENCIES_COMPLETE:
YES

NEXT_FAULT_DEPENDENCIES_COMPLETE:
YES

PIVOT_ORDER_REQUIRED:
YES

HYBRID_CFG_VALID_REQUIRED:
DERIVABLE

HYBRID_POINTER_REQUIRED:
DERIVABLE

HYBRID_DESCRIPTOR_REQUIRED:
DERIVABLE

ROW_MUST_STATE:
DERIVABLE

COL_MUST_STATE:
DERIVABLE

CAM_REUSE_STATE:
MUST_STORE

COUNTER_STATE:
MUST_STORE ordered row/column address-count mappings; valid, occupancy, and fault count are derived

OVERFLOW_STATE:
DERIVABLE

ORDERING_STATE:
MUST_STORE

FULL_RAW_FAULT_HISTORY_REQUIRED:
NO

STATE_ALIASING_FOUND:
NO

CURRENT_VIEW_EQUIVALENCE:
PASS

NEXT_FAULT_TRANSITION_EQUIVALENCE:
PASS

REACHABLE_STATES_TESTED:
20013

NEXT_FAULT_TRANSITIONS_TESTED:
3072

CURRENT_VIEW_MISMATCHES:
0

NEXT_TRANSITION_MISMATCHES:
0

CANDIDATE_RESULT_MISMATCHES:
0

SELECTED_RETAINED_STATE_STRATEGY:
STATE_OPTION_3_PROVEN_COMPACT_HISTORICAL_REPRESENTATION

COMPLETE_RETAINED_COLLECTOR_STATE_BITS_PER_SA:
821

COMPLETE_RETAINED_COLLECTOR_STATE_BITS_4SA:
3284

GENERATION_ATOMIC_FIELD_SET:
pivot_count+P0..P4, hybrid_count+H0..H10, reuse_count+R0..R10, row_counter[0..11], col_counter[0..11]

OLD_204BIT_CONTRACT_STATUS:
SUPERSEDED_ANALYZER_ONLY

OLD_468BIT_CONTRACT_STATUS:
SUPERSEDED_INCOMPLETE

BASELINE_EARLY_CHANGED:
NO

GROUP_CHANGED:
NO

S1GA_CONTROL_CHANGED:
NO

S1GB_RESUMED:
NO

PRODUCTION_RTL_MODIFIED:
NO

GIT_DIFF_CHECK:
PASS

PROOF_ARTIFACT:
scripts/analysis/s1ga2f_retained_state_contract_proof.py

CONTRACT_DOCUMENT:
docs/dss_execution/S1GA2F_COMPLETE_HISTORICAL_RETAINED_STATE_CONTRACT.md

NEXT_RECOMMENDED_PHASE:
S1G-A2G_REQUIRES_EXPLICIT_AUTHORIZATION
```
