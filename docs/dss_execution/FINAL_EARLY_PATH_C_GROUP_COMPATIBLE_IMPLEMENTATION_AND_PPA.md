# FINAL-EARLY-PATH-C — GROUP-Compatible EARLY Implementation and PPA Gate

```text
FINAL_EARLY_PATH_C_STATUS: BLOCKED
SEMANTIC_BOUNDARY_REQUESTED: FINAL_GROUP_COMPATIBLE
RTL_MODIFIED: NO
SIMULATOR_SEMANTICS_MODIFIED: NO
SYNTHESIS_RUN: NO
LATENCY_RUN: NO
DSS_FINAL_MODIFIED: NO
```

## Stop decision

The requested Path-C implementation cannot truthfully begin yet.  The frozen
archive, the C++ policy source, and the prior minimal-EARLY evidence give two
incompatible definitions of “GROUP-compatible resource legality.”  They agree
on the common 204-bit/7-entry analyzer and static ConfigID slot encoding, but
they do not agree on whether a candidate's Pattern-dependent physical demand
is an input to immediate commit.

The task expressly requires a stop before coding when historical evidence
conflicts.  This report records that stop.  It does not reinterpret, modify,
or delete either the immutable GROUP archive or the Model-B2 exploratory
archive.

## Completed precondition cleanup

| Required action | Result |
| --- | --- |
| Commit previous RCA | PASS — commit `1e64201` (`docs: close early area inversion root cause audit`) |
| Preserve raw Model-B2 DC workspace | PASS — moved, without deletion, from `results/final_early/` to `tmp/date2026/model_b2_early_superseded_dc_run/` |
| Worktree before any Path-C RTL action | CLEAN |
| Path-C RTL action | NOT STARTED — blocked by the semantic conflict below |

The new report itself is the only uncommitted Path-C output at the end of this
phase; that does not change the fact that the required pre-RTL cleanup gate
passed.

## Required planned architecture comparison (before coding)

### Frozen GROUP mother — what is unambiguous

```text
recam_dss_hyp02_static_global_top
  ├─ recam_shared_config_analyzer, H=7, one instance
  └─ recam_dss_hyp02_static_global_core
       ├─ 80-bit candidate store: 4 SA × 4 slots × {valid, PatternID[3:0]}
       ├─ static slot-to-ConfigID decoder
       ├─ 81 fixed legal static tuple terms and P0..P80 selector
       └─ deferred atomic result publication
```

| GROUP mother block | Exact source behavior |
| --- | --- |
| Shared front end / analyzer | one `recam_shared_config_analyzer`, `HYBRID_ENTRIES=7`, one 204-bit summary boundary |
| Matrix builder, Config analysis, Pattern evaluation | folded into the shared analyzer |
| Candidate producer | analyzer emits local feasibility / lowest PatternID; GROUP records only `{valid, PatternID}` for each SA/slot |
| Candidate store | 80-bit registered image |
| Selector | 81 constant fixed-edge validity terms, then fixed priority |
| Control | `IDLE → COLLECT(16) → DECIDE`; no per-SA commit during collection |
| Result state | atomic selected ConfigID/PatternID/diagnostic metadata |

### Intended Path-C EARLY — only after the missing contract is chosen

```text
new GROUP-compatible EARLY top
  ├─ exactly the GROUP H=7 shared analyzer and 204-bit summary boundary
  └─ new streaming EARLY controller
       ├─ A -> B -> C -> D
       ├─ frozen rank slots 1,0,3,2 = R,L,RB,B
       ├─ immediate selected-SA commit, preserved prefix, no rollback
       └─ no candidate store, no 81-path selector, no deferred tuple search
```

| Intended EARLY block | Required disposition | Current status |
| --- | --- | --- |
| Shared front end / analyzer | byte-identical to GROUP | feasible and required |
| Matrix / Config / Pattern logic | byte-identical to GROUP | feasible and required |
| Candidate store | remove | feasible |
| 81-path selector | remove | feasible |
| Controller | new small sequential first-legal controller | **contract blocked** |
| Result state | only committed prefix plus terminal status | width depends on resource-legality contract |

The abstract fixed-slot route would have a small controller: SA rank/control,
four directional resource tokens, result registers, and an `m=1` borrow
condition.  The physical-ledger route needs the selected candidate's
`usedRows`/`usedColumns` (and, if not derivable at the same boundary, more
repair-demand information).  An exact persistent-state-bit claim is therefore
not yet defined.  Reporting a fixed number now would be an unsupported
architecture decision.

## Exact shared-source evidence

| Source / module | GROUP mother SHA-256 | Canonical source SHA-256 | Planned Path-C classification |
| --- | --- | --- | --- |
| `recam_shared_config_analyzer.sv` | `b4aa07117af410e84c809ae833f7e926ef440a880523c78e11cd1a7df3dd5b6c` | same | `IDENTICAL` required; H must remain 7 |
| `dss_v2_group_slot_decode.sv` | `69b713b8bf36bcc715ffefedea7b215a04fc47a613012239d83e232fe22d4fd7` | same | `IDENTICAL` required if instantiated; its fixed mapping is authoritative |
| `dss_v2_group_candidate_store.sv` | `7ca8e4b2219b0d16b0b05f56d32f1c783a8b6480e0a0a78124a08b2cf007e3db` | same archive provenance | `GROUP_ONLY`, remove |
| `recam_dss_hyp02_static_selector.sv` | `5edc23a40e97da04c5c9e105699ec60821a507bb467cf597890c104df077b740` | same archive provenance | `GROUP_ONLY`, remove |
| `recam_dss_hyp02_static_global_core.sv` | `29d18300d87ef107df64d34702dad288ba76c829f4f8bbbdf8d270dae5ccb64c` | same archive provenance | replace with EARLY controller only after contract resolution |
| `recam_dss_hyp02_static_global_top.sv` | `c78d63e7d612b56d78d00e2fcf4d03b463813d3cb81b549f550f29795ae2578c` | same archive provenance | replace with H=7 EARLY wrapper only after contract resolution |
| `dss_v2_params_pkg.sv`, `dss_v2_types_pkg.sv` | archived package hashes | same archive provenance | retain only if an instantiated reusable GROUP module requires them |

```text
SHARED_SOURCE_FILES_TOTAL: 2 required semantic modules (analyzer, slot decode)
SHARED_SOURCE_FILES_IDENTICAL_TO_GROUP: not applicable; no new EARLY source created
SHARED_SOURCE_FILES_MODIFIED: 0
GROUP_ONLY_MODULES_PLANNED_FOR_REMOVAL: candidate store; static selector; deferred GROUP core/control
EARLY_ONLY_MODULES_ADDED: 0 (blocked before creation)
ANALYZER_HASH_IDENTICAL_TO_GROUP: YES for canonical source; no Path-C copy yet exists
MODEL_B2_H11_ANALYZER_PRESENT: NO
MODEL_B2_SUMMARY_EXPANSION_PRESENT: NO
```

## The blocking semantic conflict

### 1. Frozen HYP02 GROUP RTL defines static slot legality only

`recam_dss_hyp02_static_global_top` passes only `solution_valid && repairable`
and one PatternID from the common analyzer to its core.  The core records five
bits per `{SA, slot}`.  `recam_dss_hyp02_static_selector` extracts only those
valid bits and PatternIDs, evaluates 81 constant four-valid-bit terms, and
explicitly has no ledger, donor search, score, or path state.

Therefore this exact frozen hardware boundary has no selected-candidate
`usedRows`, `usedColumns`, Pattern-dependent demand, or runtime physical
ledger input.  Under this interpretation, resource legality is a fixed
slot-tuple property.

### 2. The GROUP-compatible C++ policy uses Pattern-dependent physical demand

The C++ function `findV2GroupNoScratchChoice`—the sequential, RTL-rank
`1,0,3,2` candidate policy—does not decide legality from a slot valid bit:

1. it locates the slot's candidate plan;
2. records `plan.usedRows` and `plan.usedColumns`;
3. inserts those values into the committed demand prefix; and
4. calls `PhysicalResourceLedger::allocateSequential` before committing.

`findDirectionalV2GroupGlobalCanonicalChoice` uses the same data to determine
actual release/borrow behavior from capacity-relative demand before checking
future-owner release obligations and the physical ledger.

Thus this C++ definition has a different input contract from the 80-bit HYP02
store.  Its legality can depend on PatternID/repair demand even where the
slot's analyzer-valid bit is unchanged.

### 3. The previous fixed-mask prototype demonstrates the consequence

`FINAL_EARLY_C_MINIMAL_FOUR_BIT_RTL_IMPLEMENTATION.md` documented a prior
attempt to retain the analyzer interface and make immediate commits with a
fixed four-token slot mask.  It matched its own abstract model, but disagreed
with the production demand/ledger result in 19 of 1,000 candidate vectors.
Its recorded Vector216 example has C:L / ConfigID 0 with an actual `2R,1C`
demand; the fixed mask falsely consumes `C_COL`, rejects later D:B, and
changes a successful physical result.

That earlier vector source must not be relabeled as a Path-C oracle—the task
correctly prohibits Model-B2 EARLY validation—but it is still direct evidence
that the fixed-mask controller is not automatically equivalent to a
Pattern-demand ledger.  No evidence in the final archive proves the two
definitions equivalent.

| Property | HYP02 final GROUP RTL | GROUP-compatible C++ sequential policy |
| --- | --- | --- |
| Candidate data preserved | valid + lowest PatternID per slot | candidate plan, including used rows/columns |
| Resource legality input | fixed `{SA,slot}` relationship in one of 81 tuples | physical demand prefix via `allocateSequential` |
| Pattern-dependent resource use | unavailable | explicit |
| Immediate prefix commit | no; atomic decision | yes |
| Exact immediate-EARLY derivation with no new demand data | possible only for an abstract static-slot policy | not possible from current analyzer output |

## Why this is a hard stop, not an implementation delay

The requested final EARLY must simultaneously be:

1. source/semantic-boundary identical to final GROUP;
2. a streaming, first resource-legal commit policy; and
3. verified against a GROUP-compatible independent oracle with zero mismatches.

Those requirements do not select one of the two resource-legality contracts
above.  Implementing the static contract would yield an EARLY that is exact
only to an abstract HYP02 slot-valid model.  Implementing the ledger contract
requires demand information not exposed by the final analyzer/top boundary,
or a separately proven reconstruction path.  Choosing either silently would
violate the task's conflict rule and make the requested 1,000-vector claim
ambiguous.

No RTL, C++, testbench, archive, thesis-area table, or synthesis constraint
was changed after this finding.  In particular, `THESIS_AREA_FASTTRACK_TABLE`
is intentionally unchanged because there is no matched Path-C QoR result.

## Required human semantic decision

One of the following must be explicitly frozen before implementation resumes:

1. **Static-HYP02 policy boundary.** Define `FINAL_GROUP_COMPATIBLE` as the
   archived 16-slot `{valid, PatternID}` / 81-fixed-tuple abstraction.  Then
   authorize an independent static-slot streaming-EARLY oracle and state that
   it is not the `PhysicalResourceLedger` candidate-demand policy.
2. **Physical-ledger policy boundary.** Define `FINAL_GROUP_COMPATIBLE` as the
   C++ V2 candidate-plan and `allocateSequential` contract.  Then authorize a
   precise demand/repair-decode interface or a proved equivalent decoder,
   including its ownership and timing boundary.
3. **Equivalence bridge.** Provide or authorize a proof that the exact HYP02
   slot-valid/Pattern interface can reconstruct all physical demand facts used
   by the C++ ledger, with a fixed mapping valid for the frozen 2×2 point.

Only after one option is selected can the requested source-provenance gate,
directed tests, independent 1,000-vector lockstep, synthesis, new immutable
archive, and thesis-table update be carried out honestly.

## Final return

```text
FINAL_EARLY_PATH_C_STATUS: BLOCKED
SEMANTIC_BOUNDARY: FINAL_GROUP_COMPATIBLE (ambiguous: static-HYP02 vs physical-ledger)
RTL_COMMIT: NONE
ANALYZER_HASH_IDENTICAL_TO_GROUP: YES (canonical analyzer hash); no new EARLY RTL exists
CANDIDATE_SEMANTICS_IDENTICAL_TO_GROUP: UNRESOLVED — two conflicting GROUP-compatible contracts exist
GROUP_ONLY_STRUCTURES_REMOVED: NONE (implementation blocked before source creation)
EARLY_ONLY_STRUCTURES_ADDED: NONE
1000_VECTOR_MISMATCHES: NOT_RUN; no unambiguous oracle
MODEL_B2_H11_ANALYZER_PRESENT: NO
MODEL_B2_SUMMARY_EXPANSION_PRESENT: NO
AREA_UM2: NOT_RUN
GE: NOT_RUN
GROUP_AREA_UM2: 106341.682630
EARLY_LT_GROUP: NOT_RUN
AREA_DELTA_VS_GROUP_PERCENT: NOT_RUN
WNS_NS: NOT_RUN
TNS_NS: NOT_RUN
TIMING_MET: NOT_RUN
EARLY_CORE_AREA: NOT_RUN
GROUP_CORE_AREA: 33180.8404
THESIS_AREA_TABLE_UPDATED: NO
FINAL_ARCHIVE: NOT_CREATED
WORKTREE_CLEAN: NO (this required blocker report is uncommitted; no RTL changes)
NEXT_PHASE: HUMAN_SEMANTIC_DECISION_REQUIRED_BEFORE_DATE2026-LAT0-FINAL-RTL-CONTRACT-AUDIT
```
